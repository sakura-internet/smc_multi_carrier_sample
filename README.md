# smc_multi_carrier_sample

## 概要

このプログラムは、[さくらのセキュアモバイルコネクト](https://iot.sakura.ad.jp/sim/)のマルチキャリア接続のサンプルプログラムです。

開発および動作確認は、以下の環境にて実施しています。

* [nRF9160-DK](https://www.nordicsemi.com/Products/Development-hardware/nrf9160-dk)
* [Modem Firmware v1.3.5](https://www.nordicsemi.com/Products/Development-hardware/nRF9160-DK/Download?lang=en#infotabs)
* [nRF Connect SDK 2.5.1](https://www.nordicsemi.com/Products/Development-software/nrf-connect-sdk)
* [nRF Connect for VS Code](https://www.nordicsemi.com/Products/Development-tools/nrf-connect-for-vs-code)

## プログラムの内容

周期的に接続状況を確認し、接続が確認できなかった場合にキャリアを切り替えて接続を試行します。

接続の処理は、`try_connect`関数で実行されます。
`try_connect`関数内に定義された[PLMN](https://iot.sakura.ad.jp/column/plmn/)のリスト(`plmn_list`)が接続先キャリアの一覧になります。サンプルプログラムでは、ソフトバンク、KDDI、NTTドコモの順番にて接続を試行します。

1つのPLMNにつき接続処理を3回試行し、接続できなかった場合は、次のPLMNにて接続を試行します。すべてのPLMNにおいて接続が失敗した場合、`try_connect`関数は、エラー(-1)を返します。(サンプルプログラムでは、`NVIC_SystemReset`関数を実行しシステムリセットします)

具体的な接続の処理は、`modem_connect`関数にて、モデムに一連のATコマンドを送信することで実行されます。(OSのセマフォの機能を利用し、接続完了もしくは接続タイムアウトを待ちます)

なお、接続処理では、接続の試行回数に応じてウェイト時間を設けることで、nRF9160の[Modem Reset Loop Restriction](https://docs.nordicsemi.com/bundle/nwp_042/page/WP/nwp_042/intro.html)(モデムまたはアプリケーションのリセットループが連続した場合、LTE接続が一定時間ブロックされる機能)の回避や、ネットワークへの過度な負荷を回避しています。

接続完了は、モデムから`"+CEREG: 5"`(5 - Registered, roaming)の通知を受信することで接続完了と判定しています。
サンプルプログラムでは、nRF Connect SDKの[AT monitor library](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/libraries/modem/at_monitor.html)を利用し、`at_notify_handler_cereg`関数にて通知を受け取っています。

接続状況の確認は、`check_connect`関数で実行されます。
`AT+COPS?`コマンドを実行し、接続先のPLMNが取得できなかった場合に接続エラーと判定します。

実行時のログは、ターミナルに出力されます。ATコマンドの送受信の内容も出力されます。

## ビルド方法

1. [Installing the nRF Connect SDK](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/installation.html)を参考に、nRF Connect for Visual Studio Code の環境をインストールしてください。
1. GitHubからリポジトリをクローンしてください。

	```
	git clone https://github.com/sakura-internet/smc_multi_carrier_sample
	```

1. クローンしたフォルダをVisual Studio Codeで開いてください。
1. [How to build an application](https://nrfconnect.github.io/vscode-nrf-connect/get_started/build_app_ncs.html#how-to-build-an-application)を参考にBuild Configurationを作成してください。Boardの箇所には、`nrf9160dk_nrf9160_ns`を指定してください。
1. [Actions View](https://nrfconnect.github.io/vscode-nrf-connect/reference/ui_sidebar_actions.html)のBuildをクリックすることでビルドが開始されます。
1. nRF9160-DKとPCを接続し、[Actions View](https://nrfconnect.github.io/vscode-nrf-connect/reference/ui_sidebar_actions.html)のFlashをクリックすることでサンプルプログラムがnRF9160-DKに書き込まれます。

## 動作確認の方法

### 事前準備

1. [SIM設定の変更](https://manual.sakura.ad.jp/cloud/mobile-connect/sim/sim-others.html#sim-others-configuration)にて、全てのキャリアを有効な状態にしてください。
1. nRF9160-DKにサンプルプログラムを書き込み、起動してください。
1. ソフトバンク(PLMN 44020)に接続された状態を確認してください。

### 接続先キャリア変更動作の確認手順

1. [SIM設定の変更](https://manual.sakura.ad.jp/cloud/mobile-connect/sim/sim-others.html#sim-others-configuration)にて、ソフトバンクのチェックボックスを外し、無効な状態にします。
1. [SIMの操作](https://manual.sakura.ad.jp/cloud/mobile-connect/sim/sim-others.html#sim-others-action)にて「再接続」を実行します。
1. サンプルプログラムが接続エラーを検知し、再接続の処理が実行されます。
1. ソフトバンク(PLMN 44020)への再接続が3回失敗した後、KDDI(PLMN 44051)への接続処理が実行されます。

## 実行時のログ出力例

全キャリアを有効にした状態でサンプルプログラムを起動した後、途中でソフトバンクを無効にした状態でSIMの操作にて再接続を実施、ソフトバンクへの再接続がエラーとなり、KDDIへ接続された場合のログの出力例となります。(#で始まる行は、補足のコメントです)

	*** Booting nRF Connect SDK v2.5.1 ***
	[00:00:00.587,463] <inf> SMC: AT+COPS?
	[00:00:00.587,860] <inf> SMC: +COPS: 1
	OK
	
    # 起動時の初回のtry_connect関数実行時のログ。modem_connect関数にて実行されるATコマンドが出力される。初回の接続先は、ソフトバンク(PLMN 44020)。
	[00:00:00.587,890] <inf> SMC: AT+CFUN=0
	[00:00:00.756,317] <inf> SMC: OK
	
	[00:00:00.756,378] <inf> SMC: AT+CEREG=5
	[00:00:00.756,835] <inf> SMC: OK
	
	[00:00:00.756,896] <inf> SMC: AT+CGDCONT=1,"IP","sakura"
	[00:00:00.757,446] <inf> SMC: OK
	
	[00:00:00.757,507] <inf> SMC: AT+COPS=1,2,"44020"
	[00:00:00.758,148] <inf> SMC: OK
	
	[00:00:00.758,178] <inf> SMC: AT+CFUN=1
	[00:00:00.799,041] <inf> SMC: OK
	
    # +CEREGで始まる行は、at_notify_handler_cereg関数にてモデムから受信した通知。
	[00:00:05.433,288] <inf> SMC: +CEREG: 2,"2447","04C037D4",7
	
    # "+CEREG: 5"(5 - Registered, roaming)の通知を受信、接続完了。
	[00:00:06.967,895] <inf> SMC: +CEREG: 5,"2447","04C037D4",7,0,18,"11100000","11100000"
	
	[00:00:06.967,987] <inf> SMC: try_connect: LTE network connected.

    # check_connect関数にて実行されるATコマンドのログ。接続が完了しており、AT+COPS?の応答で接続先のPLMNが取得できている。
	[00:00:16.968,078] <inf> SMC: AT+COPS?
	[00:00:16.968,475] <inf> SMC: +COPS: 1,2,"44020",7
	OK
	
	[00:00:26.968,597] <inf> SMC: AT+COPS?
	[00:00:26.977,416] <inf> SMC: +COPS: 1,2,"44020",7
	OK
	
    # この時点で、SIM設定の変更にてソフトバンクを無効化し、SIMの操作にて「再接続」を実行。
	[00:00:30.007,690] <inf> SMC: +CEREG: 2,"2447","04C037D4",7
	
	[00:00:32.498,352] <inf> SMC: +CEREG: 2,"2447","04C037D4",7,0,17
	
    # check_connect関数でのAT+COPS?の応答で接続先のPLMNが取得できなくなる。(接続エラーと判定)
	[00:00:36.977,539] <inf> SMC: AT+COPS?
	[00:00:36.986,328] <inf> SMC: +COPS: 1
	OK
	
    # try_connect関数を実行し、ソフトバンク(PLMN 44020)への再接続を試行。
	[00:00:36.986,389] <inf> SMC: AT+CFUN=0
	[00:00:37.011,871] <inf> SMC: +CEREG: 0
	
	[00:00:38.471,160] <inf> SMC: OK
	
	[00:00:38.471,221] <inf> SMC: AT+CEREG=5
	[00:00:38.471,649] <inf> SMC: OK
	
	[00:00:38.471,710] <inf> SMC: AT+CGDCONT=1,"IP","sakura"
	[00:00:38.472,106] <inf> SMC: OK
	
	[00:00:38.472,167] <inf> SMC: AT+COPS=1,2,"44020"
	[00:00:38.472,778] <inf> SMC: OK
	
	[00:00:38.472,808] <inf> SMC: AT+CFUN=1
	[00:00:38.512,359] <inf> SMC: OK
	
	[00:00:41.275,054] <inf> SMC: +CEREG: 2,"2447","04C037D4",7
	
	[00:00:42.546,905] <inf> SMC: +CEREG: 2,"2447","04C037D4",7,0,17
	
    # ソフトバンクが無効となっていて接続が完了せずエラーとなる。120秒間の待ち時間の後、2回目の再接続を試行する。
	[00:01:08.512,481] <wrn> SMC: try_connect: PLMN 44020, count 2, retry wait 120 sec
	[00:03:08.512,603] <inf> SMC: AT+CFUN=0
	[00:03:08.546,539] <inf> SMC: +CEREG: 0
	
	[00:03:08.619,934] <inf> SMC: OK
	
	[00:03:08.619,964] <inf> SMC: AT+CEREG=5
	[00:03:08.620,422] <inf> SMC: OK
	
	[00:03:08.620,483] <inf> SMC: AT+CGDCONT=1,"IP","sakura"
	[00:03:08.620,880] <inf> SMC: OK
	
	[00:03:08.620,941] <inf> SMC: AT+COPS=1,2,"44020"
	[00:03:08.621,551] <inf> SMC: OK
	
	[00:03:08.621,582] <inf> SMC: AT+CFUN=1
	[00:03:08.661,132] <inf> SMC: OK
	
	[00:03:09.758,972] <inf> SMC: +CEREG: 2,"2447","04C037D4",7
	
	[00:03:10.847,473] <inf> SMC: +CEREG: 2,"2447","04C037D4",7,0,17
	
    # 引き続き、接続が完了せずエラーとなる。180秒間の待ち時間の後、3回目の再接続を試行する。
	[00:03:38.661,254] <wrn> SMC: try_connect: PLMN 44020, count 3, retry wait 180 sec
	[00:06:38.661,376] <inf> SMC: AT+CFUN=0
	[00:06:38.695,312] <inf> SMC: +CEREG: 0
	
	[00:06:38.771,972] <inf> SMC: OK
	
	[00:06:38.772,003] <inf> SMC: AT+CEREG=5
	[00:06:38.772,460] <inf> SMC: OK
	
	[00:06:38.772,521] <inf> SMC: AT+CGDCONT=1,"IP","sakura"
	[00:06:38.772,918] <inf> SMC: OK
	
	[00:06:38.772,979] <inf> SMC: AT+COPS=1,2,"44020"
	[00:06:38.773,559] <inf> SMC: OK
	
	[00:06:38.773,590] <inf> SMC: AT+CFUN=1
	[00:06:38.813,140] <inf> SMC: OK
	
	[00:06:39.685,485] <inf> SMC: +CEREG: 2,"2447","04C037D4",7
	
	[00:06:40.711,944] <inf> SMC: +CEREG: 2,"2447","04C037D4",7,0,17
	
    # ソフトバンク(PLMN 44020)への接続が3回連続でエラーとなったため、次のKDDI(PLMN 44051)で接続を試行する。
	[00:07:08.813,262] <wrn> SMC: try_connect: PLMN 44051, count 1, retry wait 60 sec ...
	[00:08:08.813,385] <inf> SMC: AT+CFUN=0
	[00:08:08.847,320] <inf> SMC: +CEREG: 0
	
	[00:08:08.920,715] <inf> SMC: OK
	
	[00:08:08.920,776] <inf> SMC: AT+CEREG=5
	[00:08:08.921,203] <inf> SMC: OK
	
	[00:08:08.921,264] <inf> SMC: AT+CGDCONT=1,"IP","sakura"
	[00:08:08.921,661] <inf> SMC: OK
	
	[00:08:08.921,722] <inf> SMC: AT+COPS=1,2,"44051"
	[00:08:08.922,332] <inf> SMC: OK
	
	[00:08:08.922,363] <inf> SMC: AT+CFUN=1
	[00:08:08.961,914] <inf> SMC: OK
	
	[00:08:10.406,494] <inf> SMC: +CEREG: 2,"091D","0116E505",7

    # "+CEREG: 5"(5 - Registered, roaming)の通知を受信、接続完了。
	[00:08:11.956,359] <inf> SMC: +CEREG: 5,"091D","0116E505",7,0,18,"11100000","11100000"
	
	[00:08:11.956,451] <inf> SMC: try_connect: LTE network connected.

