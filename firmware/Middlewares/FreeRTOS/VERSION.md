# FreeRTOS-Kernel — зафіксована версія

| Поле | Значення |
|---|---|
| Репозиторій | https://github.com/FreeRTOS/FreeRTOS-Kernel |
| Тег | **V11.1.0** |
| Коміт | `dbf70559b27d39c1fdb68dfb9a32140b6a6777a0` (2024-04-22) |
| Ліцензія | MIT (`LICENSE.md`) |
| Порт | `portable/GCC/ARM_CM0` (Cortex-M0/M0+, ARMv6-M; без FPU і без BASEPRI) |
| Heap | `portable/MemMang/heap_4.c` (first-fit з об'єднанням вільних блоків) |

Скопійовано лише файли, що компілюються: `tasks.c`, `queue.c`, `list.c`, `include/*.h`,
порт (`port.c`, `portasm.c`, `portasm.h`, `portmacro.h`) і `heap_4.c`. Таймери (`timers.c`),
групи подій, stream/message buffers і корутини не використовуються — не скопійовано, щоб
не займати 32 КБ Flash.

Адаптації (журнал ЛР4, П-29 — Wokwi не маскує SysTick/PendSV через PRIMASK):

1. Під час компіляції: `port.c` — `-DSysTick_Handler=xPortSysTickHandler`, `portasm.c` —
   `-DPendSV_Handler=xPortPendSVHandler` (`firmware/Makefile`). Вектори — обгортки в
   `Core/Src/os_simfix.c`: до старту планувальника SysTick веде час HAL; після — тік і
   перемикання відкладаються, якщо ядро в критичній секції.
2. **Зміни в `portable/GCC/ARM_CM0/port.c`** (позначені `LR4 (П-29)`): `ulCriticalNesting`
   без `static`; у `vPortExitCritical` після `portENABLE_INTERRUPTS()` — виклик
   `vPortSimCriticalExit()` (знову виставляє відкладені тік/PendSV). На справжньому МК ці
   гілки не виконуються.
3. Ядро (`tasks.c`, `queue.c`, `list.c`) компілюється з `-fno-strict-aliasing`,
   `configUSE_MINI_LIST_ITEM 0` — перестраховка, перевірена під час пошуку П-27.
4. Збірка `DIAG=1` додає `Core/Inc/freertos_tasks_c_additions.h` (перевірка цілісності списків
   ядра в idle hook) через штатний механізм `configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H`.

Оновлення: `git clone --branch <тег> https://github.com/FreeRTOS/FreeRTOS-Kernel` і
скопіювати ті самі файли; записати новий тег і коміт сюди.
