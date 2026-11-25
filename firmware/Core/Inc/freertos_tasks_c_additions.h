/*
 * freertos_tasks_c_additions.h — діагностика цілісності ядра (лише збірка DIAG=1, журнал П-27).
 * Включається в кінець tasks.c (configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H), тож бачить
 * статичні списки й TCB. Викликається з idle hook у критичній секції: idle виконується лише
 * тоді, коли жодна задача не посеред операції над списками, тож знімок узгоджений.
 */
#ifndef FREERTOS_TASKS_C_ADDITIONS_H
#define FREERTOS_TASKS_C_ADDITIONS_H

/* ucHeap — static у heap_4.c: перевіряємо «вказівник у RAM і вирівняний на 4» */
static int diag_in_heap( const void * p )
{
    uint32_t a = ( uint32_t ) p;
    return a >= 0x20000000U && a < 0x20003000U && ( a & 3U ) == 0U;
}
size_t plat_fmt( char * buf, size_t size, const char * fmt, ... );
void plat_log_raw( const char * fmt, ... );

/* Тік (ISR, переривання замасковані): «канарка» дна стека перерваної задачі */
void os_diag_tick( void );
void os_diag_tick( void )
{
    const TCB_t * t = pxCurrentTCB;
    if( t != NULL && diag_in_heap( t ) && diag_in_heap( t->pxStack ) &&
        ( t->pxStack[ 0 ] != 0xA5A5A5A5U || t->pxStack[ 1 ] != 0xA5A5A5A5U ) )
    {
        portDISABLE_INTERRUPTS();
        plat_log_raw( "FATAL diag stack_bottom task=%s\r\n", t->pcTaskName );
        for( ; ; ) { }
    }
}


static const char * diag_tcb_bad( const TCB_t * t )
{
    if( !diag_in_heap( t ) ) return "tcb_ptr";
    if( t->uxPriority >= configMAX_PRIORITIES ) return "prio";
    if( t->uxBasePriority >= configMAX_PRIORITIES ) return "base_prio";
    if( t->uxMutexesHeld > 4U ) return "mutexes_held";
    if( !diag_in_heap( t->pxStack ) ) return "pxStack";
    if( ( const void * ) t->pxTopOfStack < ( const void * ) t->pxStack ) return "top<stack";
    if( t->pxStack[ 0 ] != 0xA5A5A5A5U || t->pxStack[ 1 ] != 0xA5A5A5A5U ) return "stack_bottom";
    if( t->xStateListItem.pvOwner != t ) return "state_owner";
    if( t->xEventListItem.pvOwner != t ) return "event_owner";
    return NULL;
}

/* 0 — усе гаразд; інакше друкує першу знайдену невідповідність */
static int diag_list( List_t * l, const char * nm, char * out, size_t n )
{
    UBaseType_t k = 0;
    const ListItem_t * it = l->xListEnd.pxNext;
    const ListItem_t * prev = ( const ListItem_t * ) &l->xListEnd;
    while( it != ( const ListItem_t * ) &l->xListEnd )
    {
        const char * why = NULL;
        if( !diag_in_heap( it ) ) why = "item_ptr";
        else if( it->pxContainer != l ) why = "container";
        else if( it->pxPrevious != prev ) why = "prev_link";
        else why = diag_tcb_bad( ( const TCB_t * ) it->pvOwner );
        if( why != NULL || ++k > 16U )
        {
            const TCB_t * t = diag_in_heap( it ) ? ( const TCB_t * ) it->pvOwner : NULL;
            plat_fmt( out, n, "list=%s idx=%u why=%s task=%s items=%u", nm, ( unsigned ) k,
                      why ? why : "loop", ( t && diag_in_heap( t ) ) ? t->pcTaskName : "?",
                      ( unsigned ) l->uxNumberOfItems );
            return 1;
        }
        prev = it;
        it = it->pxNext;
    }
    if( k != l->uxNumberOfItems )
    {
        plat_fmt( out, n, "list=%s count walk=%u stored=%u", nm, ( unsigned ) k, ( unsigned ) l->uxNumberOfItems );
        return 1;
    }
    return 0;
}

int os_diag_check( char * out, size_t n );
int os_diag_check( char * out, size_t n )
{
    static const char * const rn[ configMAX_PRIORITIES ] = { "ready0", "ready1", "ready2", "ready3", "ready4", "ready5", "ready6" };
    int bad = 0;
    taskENTER_CRITICAL();
    {
        for( UBaseType_t p = 0; p < configMAX_PRIORITIES && !bad; p++ )
        {
            bad = diag_list( &pxReadyTasksLists[ p ], rn[ p ], out, n );
            if( !bad && p > uxTopReadyPriority && pxReadyTasksLists[ p ].uxNumberOfItems )
            {
                plat_fmt( out, n, "top_ready=%u but %s has %u", ( unsigned ) uxTopReadyPriority, rn[ p ],
                          ( unsigned ) pxReadyTasksLists[ p ].uxNumberOfItems );
                bad = 1;
            }
        }
        if( !bad ) bad = diag_list( &xDelayedTaskList1, "delayed1", out, n );
        if( !bad ) bad = diag_list( &xDelayedTaskList2, "delayed2", out, n );
        if( !bad ) bad = diag_list( &xPendingReadyList, "pending", out, n );
        if( !bad ) bad = diag_list( &xSuspendedTaskList, "suspended", out, n );
        if( !bad )
        {
            const char * why = diag_tcb_bad( pxCurrentTCB );
            if( why ) { plat_fmt( out, n, "current why=%s", why ); bad = 1; }
        }
        if( !bad && uxSchedulerSuspended != 0U )
        {
            plat_fmt( out, n, "idle with scheduler suspended=%u", ( unsigned ) uxSchedulerSuspended );
            bad = 1;
        }
    }
    taskEXIT_CRITICAL();
    return bad;
}

#endif
