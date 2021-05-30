/*
 * Copyright (c) 2019, Advanced Radar Technology Co., Ltd
 * All rights reserved.
 *
 */
/*
 *  Created on: Apr 17, 2019
 *      Author: joelai
 */
/** @defgroup ARADTEK_WDT_API Watch dog timer(WDT)
 * @brief Public WDT API for application.
 *
 * AWR1642, AWR1843 max WDT duration about 160ms. and the driver do not
 * implement close WDT in runtime.
 *
 * This module also provide software WDT which feed by ARadTek_wdt_client_feed().
 * The duration depends on tick width.
 *
 * Usage abbreviate:
 * @code{.c}
 * if (ARadTek_wdt_start(socHandle, 150) != 0) {
 *   // failed start wdt
 * }
 * while (1) {
 *   ARadTek_wdt_feed();
 *   Task_sleep(ARadTek_wdt_ms2tick(100));
 * }
 * @endcode
 */

#ifndef _H_ARADTEK_WDT
#define _H_ARADTEK_WDT

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/drivers/watchdog/Watchdog.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup ARADTEK_WDT_API
 * @{
 */

/** Convert milliseconds to ticks. */
#define ARadTek_wdt_ms2tick(_ms) ((_ms) * 1000 / Clock_tickPeriod)


/** Convert ticks to milliseconds. */
#define ARadTek_wdt_tick2ms(_ts) ((_ts) * Clock_tickPeriod / 1000)

/** Start WDT.
 *
 * AWR1642, AWR1843 max WDT duration about 160ms. and the driver do not
 * implement close WDT in runtime.
 *
 * @param socHandle
 * @param starve_ms Set WDT timeout in milliseconds.
 * @param on_nmi Callback instead of reset.
 * @return 0 for successful, otherwise failure occurred.
 */
int ARadTek_wdt_start(SOC_Handle socHandle, int starve_ms,
		void (*on_nmi)(void*), void *on_nmi_arg);

/** Reset WDT. */
void ARadTek_wdt_feed(void);

typedef struct ARadTek_wdt_task_env {
	int quit, delay, starve_ms;
	void (*on_quit)(struct ARadTek_wdt_task_env*);
	void (*on_nmi)(struct ARadTek_wdt_task_env*);
} ARadTek_wdt_task_env_t;

/** Check if any client out of time to reset.
 *
 * @return 0 when all client reset in time.
 */
int ARadTek_wdt_validate_client(void);

/** Reset software WDT.
 *
 * @param id Software WDT identity, starting from 0.
 * @param starve_tick Ticks to timeout.
 */
void ARadTek_wdt_client_feed(int id, uint32_t starve_tick);

/** Find when timeout.
 *
 * @code{.c}
 * tick = 20;
 * tick_max = tick_prev = 0;
 * ARadTek_wdt_test_dur(tick, &tick_max, &tick_prev);
 * aloe_log_debug("tick_max: %d, tick_prev: %d\n", tick_max, tick_prev);
 *
 * tick += 2;
 * ARadTek_wdt_test_dur(tick, &tick_max, &tick_prev);
 * aloe_log_debug("tick_max: %d, tick_prev: %d\n", tick_max, tick_prev);
 *
 * tick += 3;
 * ARadTek_wdt_test_dur(tick, &tick_max, &tick_prev);
 * aloe_log_debug("tick_max: %d, tick_prev: %d\n", tick_max, tick_prev);
 *
 * tick += 2;
 * ARadTek_wdt_test_dur(tick, &tick_max, &tick_prev);
 * aloe_log_debug("tick_max: %d, tick_prev: %d\n", tick_max, tick_prev);
 * @endcode
 *
 * output:
 * @code
 * Debug test_tout #69 tick: 20, tick_prev: 20 => 0
 * Debug test_tout #73 tick: 24, tick_prev: 20 => 0
 * Debug test_tout #77 tick: 25, tick_prev: 25 => 1
 * Debug test_tout #81 tick: 26, tick_prev: 25 => 0
 * @endcode
 *
 * @param[in] tick Current ticks.
 * @param[in] tick_tout Timeout duration in ticks.
 * @param[in,out] tick_prev Initial fill 0.
 * @return 1 when timeout, otherwise 0.
 */
int ARadTek_wdt_test_tout(uint32_t tick, uint32_t tick_tout,
		uint32_t *tick_prev);
/** Find max ticks between each call.
 *
 * @code{.c}
 * tick = 20;
 * tick_max = tick_prev = 0;
 * r = ARadTek_wdt_test_dur(tick, &tick_max, &tick_prev);
 * aloe_log_debug("tick_max: %d, tick_prev: %d => %d\n", tick_max, tick_prev, r);
 *
 * tick += 2;
 * r = ARadTek_wdt_test_dur(tick, &tick_max, &tick_prev);
 * aloe_log_debug("tick_max: %d, tick_prev: %d => %d\n", tick_max, tick_prev, r);
 *
 * tick += 3;
 * r = ARadTek_wdt_test_dur(tick, &tick_max, &tick_prev);
 * aloe_log_debug("tick_max: %d, tick_prev: %d => %d\n", tick_max, tick_prev, r);
 *
 * tick += 2;
 * r = ARadTek_wdt_test_dur(tick, &tick_max, &tick_prev);
 * aloe_log_debug("tick_max: %d, tick_prev: %d => %d\n", tick_max, tick_prev, r);
 * @endcode
 *
 * output:
 * @code
 * Debug test_dur #47 tick_max: 0, tick_prev: 20 => 0
 * Debug test_dur #51 tick_max: 2, tick_prev: 22 => 1
 * Debug test_dur #55 tick_max: 3, tick_prev: 25 => 1
 * Debug test_dur #59 tick_max: 3, tick_prev: 27 => 0
 * @endcode
 *
 * @param[in] tick Current ticks.
 * @param[in,out] tick_max Max duration in ticks.
 * @param[in,out] tick_prev Initial fill 0.
 * @return
 *   - bit[0] for tick_max updated.
 *   - bit[1] for tick_max updated and tick_prev contain 0
 */
int ARadTek_wdt_test_dur(uint32_t tick, uint32_t *tick_max,
		uint32_t *tick_prev);

/** Sample WDT task.
 *
 * Start WDT and check all client.
 */
void ARadTek_wdt_task(UArg arg0, UArg arg1);

#define ARADTEK_WDT_TASK_INIT()


/** @} ARADTEK_WDT_API */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _H_ARADTEK_WDT */
