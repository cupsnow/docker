/*
 * Copyright (c) 2019, Advanced Radar Technology Co., Ltd
 * All rights reserved.
 *
 */
/*
 *  Created on: Apr 17, 2019
 *      Author: joelai
 */
#include <wdt.h>
#include <ti/utils/cli/cli.h>
#include <external.h>

#define aloe_lmt(_v, _a, _b) ((_v) <= (_a) ? (_a) : (_v) >= (_b) ? (_b) : (_v))
#define aloe_arraysize(_a) (sizeof(_a) / sizeof((_a)[0]))

#define aloe_log_fmsg(_lvl, _fmt, args...) do { \
	int ts = ARadTek_wdt_tick2ms(Clock_getTicks()); \
	CLI_write(_lvl "%03d.%03d %s #%d " _fmt, (ts / 1000) % 1000, ts % 1000, \
			__func__, __LINE__, ##args); \
} while(0)
#define aloe_log_d(args...) aloe_log_fmsg("Debug ", ##args)
#define aloe_log_e(args...) aloe_log_fmsg("ERROR ", ##args)
#define aloe_endl "\r\n"

extern ESM_Handle esmHandle;

typedef void (*wdt_on_nmi_t)(void*);

static struct {
	Watchdog_Handle handle;
	int starve_ms;
	struct {
		uint32_t footprint; /**< 0 when not used, otherwise for latest timestamp in ticks. */
		uint32_t hp_max; /**< Ticks to timeout. */
	} client[2]; /**< Software WDT. */
	void (*on_nmi)(void*);
	void *on_nmi_arg;
} impl;
static Semaphore_Handle impl_lock;

static void wdt_on_nmi(Watchdog_Handle _h) {
	if (_h != impl.handle) {
		aloe_log_e("wdt nmi for unknown handle" aloe_endl);
	}
	if (impl.on_nmi) (*impl.on_nmi)(impl.on_nmi_arg);
}

int ARadTek_wdt_start(SOC_Handle socHandle, int starve_ms,
		void (*on_nmi)(void*), void *on_nmi_arg) {
	Watchdog_Params wdtParams;
	int r;

	Watchdog_init();
	if (!impl_lock) {
		Semaphore_Params semParams;

		Semaphore_Params_init(&semParams);
		semParams.mode = Semaphore_Mode_BINARY;
		if ((impl_lock = Semaphore_create(1, &semParams, NULL)) == NULL) {
			return -1;
		}
	}

	Semaphore_pend(impl_lock, BIOS_WAIT_FOREVER);
	if (impl.handle) {
		Watchdog_close(impl.handle);
		impl.handle = NULL;
	}
	if (starve_ms < 0) {
		r = 0;
		goto finally;
	}

	memset(&impl, 0, sizeof(impl));

	// dur: ((1..4096) << 13) / 2e8
	impl.starve_ms = aloe_lmt(starve_ms, 0, (0xfff << 13) / 200000);
	starve_ms = aloe_lmt((impl.starve_ms * 200000) >> 13, 0, 0xfff);

	aloe_log_d("wdt timeout: %dms(%d preload value)" aloe_endl, impl.starve_ms,
			starve_ms);

	if ((impl.on_nmi = on_nmi)) impl.on_nmi_arg = on_nmi_arg;
	Watchdog_Params_init(&wdtParams);
	if (on_nmi) {
		wdtParams.resetMode = Watchdog_RESET_OFF;
		wdtParams.callbackFxn = &wdt_on_nmi;
	} else {
		wdtParams.resetMode = Watchdog_RESET_ON;
	}
	wdtParams.socHandle = socHandle;
	wdtParams.esmHandle = esmHandle;
	wdtParams.preloadValue = starve_ms;

#if 1
	if ((impl.handle = Watchdog_open(0, &wdtParams)) == NULL) {
		r = -1;
		goto finally;
	}
#else
	aloe_log_d("wdt pseudo open" aloe_endl);
#endif
	r = 0;
finally:
	if (impl_lock) Semaphore_post(impl_lock);
	return r;
}

void ARadTek_wdt_feed(void) {
	if (!impl_lock) return;
	Semaphore_pend(impl_lock, BIOS_WAIT_FOREVER);
	if (impl.handle) Watchdog_clear(impl.handle);
	Semaphore_post(impl_lock);
}

#define client_starve(_cli, _ts) \
    ((_cli)->footprint > 0 && (_ts) > (_cli)->footprint && \
            ((_ts) - (_cli)->footprint > (_cli)->hp_max))

int ARadTek_wdt_validate_client(void) {
	int id, r;
	uint32_t ts = Clock_getTicks();

	if (!impl_lock) return 0;
	Semaphore_pend(impl_lock, BIOS_WAIT_FOREVER);
	for (id = 0; id < aloe_arraysize(impl.client); id++) {
		if (client_starve(&impl.client[id], ts)) {
			aloe_log_e("invalid wdt client[%d]" aloe_endl, id);
			r = -1;
			goto finally;
		}
	}
	r = 0;
finally:
	Semaphore_post(impl_lock);
	return r;
}

void ARadTek_wdt_client_feed(int id, uint32_t hp_max) {
	if (!impl_lock) return;
	if (id >= aloe_arraysize(impl.client)) return;
	Semaphore_pend(impl_lock, BIOS_WAIT_FOREVER);
	if (hp_max <= 0) {
		impl.client[id].footprint = 0;
		goto finally;
	}
	impl.client[id].footprint = Clock_getTicks();
	impl.client[id].hp_max = hp_max;
finally:
	Semaphore_post(impl_lock);
}

void ARadTek_wdt_client_feed_from_isr(int id, uint32_t hp_max) {
	if (hp_max <= 0) {
		impl.client[id].footprint = 0;
		return;
	}
	impl.client[id].hp_max = hp_max;
	impl.client[id].footprint = Clock_getTicks();
}

int ARadTek_wdt_test_tout(uint32_t tick, uint32_t tick_tout,
		uint32_t *tick_prev) {
	if (*tick_prev == 0) {
		*tick_prev = tick;
		return 0;
	}
	if ((tick - *tick_prev) >= tick_tout) {
		*tick_prev = tick;
		return 1;
	}
	return 0;
}

int ARadTek_wdt_test_dur(uint32_t tick, uint32_t *tick_max, uint32_t *tick_prev) {
	int r;

	if ((r = (*tick_prev != 0) && (tick - *tick_prev > *tick_max)))
		*tick_max = tick - *tick_prev;
	*tick_prev = tick;
	return r;
}

void ARadTek_wdt_task(UArg arg0, UArg arg1) {
	ARadTek_wdt_task_env_t *env =
			(ARadTek_wdt_task_env_t*)Task_getEnv(Task_self());
	uint32_t dur_ctx[4], dur_max, ts;
	int delay = (env && env->delay > 0) ? env->delay : 0;
	int starve_ms = (env && env->starve_ms > 0) ? env->starve_ms : 150;

	if (env) {
		aloe_log_d("wdt task env delay: %d, starve_ms: %d, on_quit: %s" aloe_endl,
				env->delay, env->starve_ms, env->on_quit ? "yes" : "no");
	} else {
		aloe_log_d("wdt task started" aloe_endl);
	}
	while (!env || !env->quit) {
		aloe_log_d("wdt start in: %d seconds" aloe_endl, delay);
		if (delay-- <= 0) break;
		Task_sleep(ARadTek_wdt_ms2tick(1000));
	}
	if (ARadTek_wdt_start(gMmwMssMCB.socHandle, starve_ms,
			(env ? (void (*)(void*))env->on_nmi : NULL), env) != 0) {
		aloe_log_e("wdt open" aloe_endl);
		goto finally;
	}
	delay = aloe_lmt(50, impl.starve_ms * 2 / 3, impl.starve_ms);
	aloe_log_d("wdt feed period: %dms" aloe_endl, delay);

	delay = ARadTek_wdt_ms2tick(delay);

	memset(dur_ctx, 0, sizeof(dur_ctx));
	dur_max = 0;

	while (!env || !env->quit) {
		if (ARadTek_wdt_validate_client() == 0) ARadTek_wdt_feed();
#if 1
		ts = Clock_getTicks();
		ARadTek_wdt_test_dur(ts, &dur_max, &dur_ctx[2]);
		if (ARadTek_wdt_test_tout(ts, ARadTek_wdt_ms2tick(1000), &dur_ctx[3])) {
			aloe_log_d("max duration: %d" aloe_endl, dur_max);
		}
#endif
		Task_sleep(delay);
	}
	ARadTek_wdt_feed();
finally:
	aloe_log_d("wdt %s" aloe_endl, (env && env->quit) ? "quit" : "close");
	ARadTek_wdt_start(gMmwMssMCB.socHandle, -1, NULL, NULL);
	if (env && env->on_quit) (*env->on_quit)(env);
}

#if 0
/**
 *
 * @code
 * static ARadTek_wdt_task_env_t wdt_env = {
 *   .quit = 0, .delay = 5, .on_quit = &ARadTek_wdt_on_quit,
 * };
 * int i;
 *
 * Task_Params_init(&taskParams);
 * taskParams.priority = 2;//4;//3
 * taskParams.stackSize = 3*1024;//3*1024;
 * taskParams.env = &wdt_env;
 * Task_create(ARadTek_wdt_task, &taskParams, NULL);
 *
 * for (i = 0; i < 5; i++) {
 * 	 Task_Params_init(&taskParams);
 * 	 taskParams.priority = 2;//4;//3
 * 	 taskParams.stackSize = 3*1024;//3*1024;
 * 	 taskParams.arg0 = i;
 * 	 taskParams.env = &wdt_env;
 * 	 Task_create(ARadTek_wdt_test_task, &taskParams, NULL);
 * }
 * * @endcode
 *
 */
void ARadTek_wdt_test_task(UArg arg0, UArg arg1) {
	ARadTek_wdt_task_env_t *env =
			(ARadTek_wdt_task_env_t*)Task_getEnv(Task_self());
	int id = (int)arg0;

	aloe_log_d("test task[%d]" aloe_endl, id);
#if 0 // driver not support close wdt
	if (id == 1 && env && !env->quit) {
		int cnt = env->delay + 5;

		while (1) {
			aloe_log_d("test task[%d] wdt quit in %d sec." aloe_endl, id, cnt);
			if (cnt-- <= 0) break;
			Task_sleep(ARadTek_wdt_ms2tick(1000));
		}
		env->quit = 1;
	}
#endif
#if 0 // test reset system with ARadTek_wdt_client_feed()
	if (id == 2 && env && !env->quit) {
		int cnt = env->delay + 5, wdt = 1;

		while (1) {
			aloe_log_d("test task[%d] wdt[%d] touch in %d sec." aloe_endl, id, wdt, cnt);
			if (cnt-- <= 0) break;
			Task_sleep(ARadTek_wdt_ms2tick(1000));
		}
		ARadTek_wdt_client_feed(wdt, 1);
	}
#endif
#if 0 // test reset system when no more feed wdt
	if (id == 3 && env && !env->quit) {
		uint32_t ts, dur_ctx = 0;
		int hp = 500 /* ms */, cnt = env->delay + 5, wdt = 1;

		while (1) {
			ARadTek_wdt_client_feed(wdt, ARadTek_wdt_ms2tick(hp));

			ts = Clock_getTicks();
			if (ARadTek_wdt_test_tout(ts, ARadTek_wdt_ms2tick(1000), &dur_ctx)) {
				aloe_log_d("test task[%d] wdt[%d] feed %d sec. more" aloe_endl,
						id, wdt, cnt);
				if (cnt-- <= 0) break;
			}

			Task_sleep(ARadTek_wdt_ms2tick(hp * 2 / 3));
		}
		aloe_log_d("test task[%d] wdt[%d] no more feed" aloe_endl, id, wdt);
	}
#endif
}
#endif
