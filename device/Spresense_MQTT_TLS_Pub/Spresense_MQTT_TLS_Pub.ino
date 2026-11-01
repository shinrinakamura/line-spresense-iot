// SPDX-License-Identifier: GPL-2.0-only
// 本改変版はGNU General Public License version 2で配布します。
// 元サンプルのLGPL 2.1第3条に基づくライセンスの切り替えです。
// 元サンプルからの変更: MQTT/TLS、ユーザー認証、MQTT再接続処理、日本語説明の追加。
// 2026-11-01: ライセンス表記と配布説明を追加。
// 本プログラムは無保証です。商品性・特定目的適合性の保証もありません。
// 許諾条件と免責の全文は同梱のLICENSE、出典と適用範囲はREADME.mdを参照してください。
// MQTTの送信のシンプルな確認
// https://github.com/TomonobuHayakawa/Spresense-Playground/tree/master/sketches/MQTT/viaLTE/simple_pub
// 早川さんが用意しているものを使用
// 2026-11-01: subに合わせた構成整理、送信結果確認、送信間隔管理を追加。


/*
 * Spresense MQTT/TLS 送信確認
 *
 * Spresense -> CloudAMQP -> Node-RED / MQTT Subscriber
 *
 * 早川さんのLTE/MQTT送信サンプルをベースに、
 * CloudAMQPへのMQTT/TLS接続と認証を追加。
 *
 * test0! から test29! まで、1秒間隔で30回送信します。
 */

#include <LTE.h>
#include <ArduinoMqttClient.h>
#include "config.h"

// ==================================================
// LTE SIM 設定（使用するSIMに合わせて変更）
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
// MQTT送信設定と状態
// ==================================================
#define PUBLISH_INTERVAL_SEC   1
#define MAX_NUMBER_OF_PUBLISH  30

int numOfPubs = 0;
unsigned long lastPublishMs = 0;

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
// MQTT/TLS接続
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
  Serial.println("Spresense MQTT/TLS Publisher");
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
  mqttClient.setId("spresense-test-pub");

  // MQTT接続
  // 接続失敗時はloop()で再試行する
  connectMQTT();
  lastPublishMs = millis();
}

// ==================================================
// メインループ：MQTT送信確認
// ==================================================
void loop()
{
  // 指定回数の送信処理が完了したら、再起動するまで終了状態を維持する
  if (numOfPubs >= MAX_NUMBER_OF_PUBLISH) {
    delay(1000);
    return;
  }

  // MQTT切断を検出したら再接続
  if (!mqttClient.connected()) {
    Serial.println("MQTT disconnected. Reconnecting...");

    if (!connectMQTT()) {
      delay(5000);
    }

    return;
  }

  // MQTTの接続維持処理
  mqttClient.poll();
  if (!mqttClient.connected()) {
    return;
  }

  // 経過時間で判定し、LTE時刻の補正やmillis()の周回に影響されにくくする
  unsigned long now = millis();
  if (now - lastPublishMs < PUBLISH_INTERVAL_SEC * 1000UL) {
    delay(10);
    return;
  }
  lastPublishMs = now;

  String testString = "test" + String(numOfPubs) + "!";

  Serial.print("Sending message to topic: ");
  Serial.println(MQTT_TOPIC);
  Serial.print("Publish: ");
  Serial.println(testString);

  // QoS 0、retainなしで送信する
  if (!mqttClient.beginMessage(MQTT_TOPIC)) {
    Serial.println("MQTT publish begin failed.");
    mqttClient.stop();
    delay(1000);
    return;
  }

  if (mqttClient.print(testString) != testString.length()) {
    Serial.println("MQTT payload write failed.");
    mqttClient.stop();
    delay(1000);
    return;
  }

  if (!mqttClient.endMessage()) {
    Serial.println("MQTT publish failed.");
    mqttClient.stop();
    delay(1000);
    return;
  }

  // 送信処理の成功時だけ回数を増やす（QoS 0なので到達確認ではない）
  numOfPubs++;
  Serial.println("MQTT publish completed.");
  Serial.println();

  if (numOfPubs >= MAX_NUMBER_OF_PUBLISH) {
    Serial.println("Publish end");
    mqttClient.stop();
  }

  delay(10);
}