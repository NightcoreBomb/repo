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

Вихідні файли ядра не змінено. Єдина адаптація — під час компіляції `port.c`
`-DSysTick_Handler=xPortSysTickHandler` (`firmware/Makefile`): обробник SysTick порту
викликається з нашого `SysTick_Handler` (`Core/Src/stm32c0xx_it.c`), який до старту
планувальника веде час HAL і вимірює вартість тіку.

Оновлення: `git clone --branch <тег> https://github.com/FreeRTOS/FreeRTOS-Kernel` і
скопіювати ті самі файли; записати новий тег і коміт сюди.
