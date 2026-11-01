// SPDX-License-Identifier: GPL-2.0-only
// 本改変版はGNU General Public License version 2で配布します。
// 元サンプルのLGPL 2.1第3条に基づくライセンスの切り替えです。
// 元サンプルからの変更: MQTT/TLS、ユーザー認証、MQTT再接続処理、日本語説明の追加。
// 2026-11-01: ライセンス表記と配布説明を追加。
// 本プログラムは無保証です。商品性・特定目的適合性の保証もありません。
// 許諾条件と免責の全文は同梱のLICENSE、出典と適用範囲はREADME.mdを参照してください。
// MQTTの受信のシンプルな確認
// https://github.com/TomonobuHayakawa/Spresense-Playground/tree/master/sketches/MQTT/viaLTE/simple_sub
// 早川さんが用意しているものを使用
// 日本語コメントは中村


/*
 * Spresense LINE -> MQTT 受信確認
 *
 * LINE -> Node-RED -> CloudAMQP -> Spresense
 *
 * 早川さんのLTE/MQTT受信サンプルをベースに、
 * CloudAMQPへのMQTT/TLS接続と認証を追加。
 *
 * 今回は受信メッセージをシリアルに表示するだけ。
 */

#include <LTE.h>
#include <ArduinoMqttClient.h>
#include "config.h"

// ==================================================
// MEEQ SIM 設定（元のプログラムから継承）
// ==================================================
#define APP_LTE_APN       "YOUR-APN"
#define APP_LTE_USER_NAME "USER-NAME"
#define APP_LTE_PASSWORD  "PASS-WORD"

#define APP_LTE_IP_TYPE   LTE_NET_IPTYPE_V4V6
#define APP_LTE_AUTH_TYPE LTE_NET_AUTHTYPE_CHAP
#define APP_LTE_RAT       LTE_NET_RAT_CATM

// ==================================================
// 通信オブジェクト
// ==================================================
LTE lteAccess;

// 通常のLTEClientからTLS対応版に変更
LTETLSClient tlsClient;
MqttClient mqttClient(tlsClient);

// ==================================================
// LTE接続開始（元のプログラムを踏襲）
// ==================================================
void doAttach()
{
  while (true) {

    if (lteAccess.begin() != LTE_SEARCHING) {
      Serial.println("LTE modem initialization failed.");

      // 初期化失敗時は停止
      while (true) {
        delay(1000);
      }
    }

    if (lteAccess.attach(
          APP_LTE_RAT,
          APP_LTE_APN,
          APP_LTE_USER_NAME,
          APP_LTE_PASSWORD,
          APP_LTE_AUTH_TYPE,
          APP_LTE_IP_TYPE,
          false) == LTE_CONNECTING) {

      Serial.println("Attempting LTE connection...");
      break;
    }

    Serial.println("LTE attach failed. Retrying...");
    lteAccess.shutdown();
    delay(1000);
  }
}

// ==================================================
// LTE接続完了待ち
// ==================================================
void waitForLTE()
{
  Serial.println("Waiting for LTE network...");

  LTEModemStatus modemStatus = lteAccess.getStatus();

  while (modemStatus != LTE_READY) {

    if (modemStatus == LTE_ERROR) {
      Serial.println("LTE network error. Retrying...");

      lteAccess.shutdown();
      delay(1000);
      doAttach();
    }

    delay(1000);
    modemStatus = lteAccess.getStatus();
  }

  Serial.println("LTE connected.");
}

// ==================================================
// MQTT/TLS接続とトピック購読
// ==================================================
bool connectMQTT()
{
  Serial.print("Connecting to MQTT broker: ");
  Serial.println(MQTT_BROKER);

  // CloudAMQPへTLS接続
  if (!mqttClient.connect(MQTT_BROKER, MQTT_PORT)) {

    Serial.print("MQTT connection failed. Error = ");
    Serial.println(mqttClient.connectError());

    mqttClient.stop();
    return false;
  }

  Serial.println("MQTT connected.");

  // Node-REDが撮影指示を送信するトピック
  Serial.print("Subscribing to: ");
  Serial.println(MQTT_TOPIC);

  if (!mqttClient.subscribe(MQTT_TOPIC, 0)) {

    Serial.println("MQTT subscription failed.");
    mqttClient.stop();
    return false;
  }

  Serial.println("MQTT subscribed.");
  Serial.println("Waiting for LINE commands...");
  Serial.println();

  return true;
}

// ==================================================
// 初期設定
// ==================================================
void setup()
{
  Serial.begin(115200);

  delay(1000);
  Serial.println();
  Serial.println("Spresense MQTT/TLS Receiver");
  Serial.println("--------------------------");

  // LTE接続
  doAttach();
  waitForLTE();

  // TLSサーバー認証用CA証明書を設定
  tlsClient.setCACert(MQTT_ROOT_CA);

  // MQTT接続時のユーザー名とパスワード
  mqttClient.setUsernamePassword(
    MQTT_USERNAME,
    MQTT_PASSWORD
  );

  // MQTT Client ID
  // 同じブローカー内で重複しないIDにする
  mqttClient.setId("spresense-line-recv");

  // MQTT接続
  // 接続失敗時はloop()で再試行する
  connectMQTT();
}

// ==================================================
// メインループ：MQTT受信確認
// ==================================================
void loop()
{
  // MQTT切断を検出したら再接続
  if (!mqttClient.connected()) {

    Serial.println("MQTT disconnected. Reconnecting...");

    if (!connectMQTT()) {
      delay(5000);
    }

    return;
  }

  // 受信メッセージを確認
  int messageSize = mqttClient.parseMessage();

  if (messageSize > 0) {

    Serial.println("----- MQTT Received -----");

    Serial.print("Topic: ");
    Serial.println(mqttClient.messageTopic());

    Serial.print("Length: ");
    Serial.print(messageSize);
    Serial.println(" bytes");

    Serial.print("Payload: ");

    // 受信データをそのまま表示
    while (mqttClient.available()) {
      Serial.print((char)mqttClient.read());
    }

    Serial.println();
    Serial.println("-------------------------");
    Serial.println();
  }

  delay(10);
}


