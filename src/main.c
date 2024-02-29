/*
 * Copyright (c) 2024 SAKURA internet Inc.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>
#include <modem/at_monitor.h>
#include <nrf_modem_at.h>

#define WAIT_CEREG_REGISTERED_SEC	30	// 接続完了待ち時間(秒)

LOG_MODULE_REGISTER(SMC, LOG_LEVEL_INF);
K_SEM_DEFINE(wait_network_registered, 0, 1);
AT_MONITOR_ISR(at_notify_cereg, "+CEREG", at_notify_handler_cereg);

static void at_notify_handler_cereg(const char *notify)
{
	LOG_INF("%s", notify);

	if (strncmp(notify, "+CEREG: 5", strlen("+CEREG: 5")) == 0) {	// "5 - Registered, roaming"の受信で、接続完了待ちのセマフォを解除
		k_sem_give(&wait_network_registered);
	}
}

static int modem_at_cmd(char *response, size_t len, char *format, ...)
{
	va_list vl;
	char command[64];
	int err;

	va_start(vl, format);
	vsnprintf(command, sizeof(command), format, vl);
	va_end(vl);

	LOG_INF("%s", command);

	err = nrf_modem_at_cmd(response, len, command);
	if (err) {
		LOG_ERR("nrf_modem_at_cmd error: %d", err);
		return err;
	}

	LOG_INF("%s", response);
	return 0;
}

static int modem_connect(uint32_t plmn)
{
	char response[64];
	int err;

	k_sem_reset(&wait_network_registered);

	err = modem_at_cmd(response, sizeof(response), "AT+CFUN=0");
	if (err) {
		return err;
	}
	err = modem_at_cmd(response, sizeof(response), "AT+CEREG=5");
	if (err) {
		return err;
	}
	err = modem_at_cmd(response, sizeof(response), "AT+CGDCONT=1,\"IP\",\"sakura\"");
	if (err) {
		return err;
	}
	err = modem_at_cmd(response, sizeof(response), "AT+COPS=1,2,\"%u\"", plmn);
	if (err) {
		return err;
	}
	err = modem_at_cmd(response, sizeof(response), "AT+CFUN=1");
	if (err) {
		return err;
	}

	err = k_sem_take(&wait_network_registered, K_SECONDS(WAIT_CEREG_REGISTERED_SEC));	// セマフォで接続完了を待つ。タイムアウト(-EAGAIN)の場合は、エラーを返す。
	if (err == -EAGAIN) {
		return -1;
	}
	return 0;
}

static int check_connect(void)
{
	char response[64];
	int err;

	err = modem_at_cmd(response, sizeof(response), "AT+COPS?");
	if (err) {
		return err;
	}
	if (strncmp(response, "+COPS: 1\r", strlen("+COPS: 1\r")) == 0) {
		return -1;
	}
	return 0;
}

static int try_connect(void)
{
	static const uint32_t plmn_list[] = {
		44020,	// ソフトバンク
		44051,	// KDDI
		44010,	// NTTドコモ
		0,		// 終端
	};

	const uint32_t *plmn = &plmn_list[0];
	int try_count = 1;
	int err;

	while (true) {
		err = modem_connect(*plmn);
		if (err == 0) {
			LOG_INF("%s: LTE network connected.", __func__);
			break;
		}

		try_count++;
		if (try_count > 3) {	// 接続の試行回数が3回を超えた場合は、
			plmn++;				// 次のPLMNへ。
			if (*plmn == 0) {	// 次のPLMNがない場合(終端だった場合)は、接続失敗でエラー。
				LOG_ERR("%s: retry failed.", __func__);
				return -1;
			}
			try_count = 1;
		}

		LOG_WRN("%s: PLMN %u, count %d, retry wait %d sec", __func__, *plmn, try_count, (try_count * 60));
		k_sleep(K_SECONDS(try_count * 60));
	}
	return 0;
}

int main(void)
{
	int err;

	err = nrf_modem_lib_init();
	if (err) {
		return -1;
	}

	while (true) {
		err = check_connect();
		if (err) {
			err = try_connect();
			if (err) {
				NVIC_SystemReset();
				return -1;
			}
		}
		k_sleep(K_SECONDS(10));
	}
	return 0;
}
