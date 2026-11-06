/**
 * capture.h — відтворювані сценарії для логічного аналізатора.
 * Кожен сценарій: «CAP <name> begin» → MARK↑ → транзакція → MARK↓ → «CAP <name> end».
 * MARK (PB3, канал D7 у профілі bus) — природний тригер: у diagram.json
 * можна ввімкнути triggerMode=level/triggerPin=D7, і запис міститиме лише
 * вікна з цікавими транзакціями (див. docs/captures/README.md).
 */
#ifndef APP_CAPTURE_H
#define APP_CAPTURE_H

#include "app.h"

/* Повертає 0, якщо сценарій невідомий */
int  capture_run(app_t *a, const char *name);
void capture_list(void);

#endif
