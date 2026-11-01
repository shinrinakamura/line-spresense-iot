// 写真送信後の再接続に多少時間がかかっている


/*
 * Spresense LINE -> MQTT -> Camera -> HTTPS
 *
 * 1. LINEから撮影指示を受信
 * 2. MQTTのcommandとrequestTokenを取得
 * 3. カメラでJPEG撮影
 * 4. JPEG末尾のFFパディングを除去
 * 5. Node-REDへHTTPSアップロード
 * 6. MQTTを再接続して次の指示を待つ
 *
 * 今回は撮影指示を1件ずつ処理する検証版。
 */

#include <LTE.h>
#include <Camera.h>
#include <ArduinoMqttClient.h>

#include "config.h"
#include "https_config.h"
#include "https_ca.h"

// ========================================
// MEEQ SIM
// ========================================
#define APP_LTE_APN       "YOUR-APN"
#define APP_LTE_USER_NAME "USER-NAME"
#define APP_LTE_PASSWORD  "PASS-WORD"

#define APP_LTE_IP_TYPE   LTE_NET_IPTYPE_V4V6
#define APP_LTE_AUTH_TYPE LTE_NET_AUTHTYPE_CHAP
#define APP_LTE_RAT       LTE_NET_RAT_CATM

// ========================================
// 通信オブジェクト
// ========================================
LTE lteAccess;

LTETLSClient mqttTlsClient;
MqttClient mqttClient(mqttTlsClient);

LTETLSClient httpsClient;

// ========================================
// 撮影指示管理
// ========================================
bool captureRequested = false;
String pendingRequestToken = "";

const int MAX_MQTT_PAYLOAD = 512;
const unsigned long MQTT_RECEIVE_TIMEOUT_MS = 30000UL;

// ========================================
// LTE接続
// ========================================
void doAttach()
{
  while (true) {

    if (lteAccess.begin() != LTE_SEARCHING) {
      Serial.println("LTE initialization failed.");
      while (true) delay(1000);
    }

    if (lteAccess.attach(
          APP_LTE_RAT,
          APP_LTE_APN,
          APP_LTE_USER_NAME,
          APP_LTE_PASSWORD,
          APP_LTE_AUTH_TYPE,
          APP_LTE_IP_TYPE,
          false) == LTE_CONNECTING) {

      Serial.println("Connecting LTE...");
      break;
    }

    Serial.println("LTE attach failed.");
    lteAccess.shutdown();
    delay(1000);
  }
}

void waitForLTE()
{
  LTEModemStatus status = lteAccess.getStatus();

  while (status != LTE_READY) {

    if (status == LTE_ERROR) {
      Serial.println("LTE network error.");
      lteAccess.shutdown();
      delay(1000);
      doAttach();
    }

    delay(1000);
    status = lteAccess.getStatus();
  }

  Serial.println("LTE connected.");
}

// ========================================
// カメラ初期化
// ========================================
bool setupCamera()
{
  CamErr err = theCamera.begin();

  if (err != CAM_ERR_SUCCESS) {
    Serial.print("Camera begin error: ");
    Serial.println((int)err);
    return false;
  }

  err = theCamera.setStillPictureImageFormat(
    CAM_IMGSIZE_QVGA_H,
    CAM_IMGSIZE_QVGA_V,
    CAM_IMAGE_PIX_FMT_JPG
  );

  if (err != CAM_ERR_SUCCESS) {
    Serial.print("Camera format error: ");
    Serial.println((int)err);
    return false;
  }

  Serial.println("Camera ready.");
  return true;
}

// ========================================
// MQTT接続・購読
// ========================================
bool connectMQTT()
{
  Serial.print("Connecting MQTT: ");
  Serial.println(MQTT_BROKER);

  if (!mqttClient.connect(MQTT_BROKER, MQTT_PORT)) {
    Serial.print("MQTT connection failed: ");
    Serial.println(mqttClient.connectError());

    mqttClient.stop();
    return false;
  }

  if (!mqttClient.subscribe(MQTT_TOPIC, 0)) {
    Serial.println("MQTT subscribe failed.");
    mqttClient.stop();
    return false;
  }

  Serial.println("MQTT connected.");
  Serial.print("Subscribed: ");
  Serial.println(MQTT_TOPIC);

  return true;
}

// ========================================
// JSON文字列から値を取得
// 今回の単純な撮影指示用
// ========================================
String extractJsonStringValue(
  const String& json,
  const String& key)
{
  String pattern = "\"" + key + "\"";

  int keyPos = json.indexOf(pattern);
  if (keyPos < 0) return "";

  int colonPos = json.indexOf(
    ':', keyPos + pattern.length()
  );

  if (colonPos < 0) return "";

  unsigned int pos = static_cast<unsigned int>(colonPos) + 1U;

  while (pos < json.length() &&
         (json[pos] == ' ' ||
          json[pos] == '\t' ||
          json[pos] == '\n')) {
    pos++;
  }

  if (pos >= json.length() || json[pos] != '"') {
    return "";
  }

  int endPos = json.indexOf('"', pos + 1);

  if (endPos < 0) return "";

  return json.substring(pos + 1, endPos);
}

// HTTPヘッダーに使用可能なrequestTokenか確認
bool validRequestToken(const String& token)
{
  if (token.length() == 0 ||
      token.length() > 128) {
    return false;
  }

  for (size_t i = 0; i < token.length(); i++) {
    char c = token[i];

    bool valid =
      (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') ||
      c == '-' || c == '_' ||
      c == '.' || c == ':';

    if (!valid) return false;
  }

  return true;
}

// ========================================
// MQTTメッセージ受信
// ========================================
void processMqttMessage(int messageSize)
{
  String topic = mqttClient.messageTopic();

  if (messageSize > MAX_MQTT_PAYLOAD) {
    Serial.println("MQTT message too large.");

    // 大きなメッセージを読み捨て続けず、切断して次のloop()で再接続する
    mqttClient.stop();
    return;
  }

  if (messageSize <= 0) return;

  String payload = "";
  if (!payload.reserve(messageSize + 1)) {
    Serial.println("MQTT payload allocation failed.");
    mqttClient.stop();
    return;
  }

  unsigned long receiveStart = millis();

  // available()は未読の残量を返すため、read()の結果を必ず確認する
  while (payload.length() < (size_t)messageSize) {
    if (millis() - receiveStart >= MQTT_RECEIVE_TIMEOUT_MS) {
      Serial.println("MQTT receive timeout.");
      mqttClient.stop();
      return;
    }

    int value = mqttClient.read();
    if (value < 0) {
      if (!mqttClient.connected()) {
        Serial.println("MQTT disconnected during receive.");
        mqttClient.stop();
        return;
      }

      // 分割されたデータの到着待ち。失敗値を文字列へ追加しない
      delay(10);
      continue;
    }

    if (!payload.concat((char)value)) {
      Serial.println("MQTT payload append failed.");
      mqttClient.stop();
      return;
    }
  }

  if (topic != MQTT_TOPIC) return;

  Serial.println();
  Serial.println("----- MQTT Received -----");
  Serial.print("Payload: ");
  Serial.println(payload);

  String command =
    extractJsonStringValue(payload, "command");

  String requestToken =
    extractJsonStringValue(payload, "requestToken");

  Serial.print("Command: ");
  Serial.println(command);

  Serial.print("RequestToken: ");
  Serial.println(requestToken);

  if (command != "capture") {
    Serial.println("Command ignored.");
    return;
  }

  if (!validRequestToken(requestToken)) {
    Serial.println("Invalid requestToken.");
    return;
  }

  pendingRequestToken = requestToken;
  captureRequested = true;

  Serial.println("Capture command accepted.");
}

// ========================================
// HTTPS JPEGアップロード
// ========================================
bool uploadJpeg(
  const uint8_t* jpeg,
  size_t jpegSize,
  const String& requestToken)
{
  Serial.print("Connecting HTTPS: ");
  Serial.println(HTTPS_HOST);

  httpsClient.stop();

  if (!httpsClient.connect(
        HTTPS_HOST, HTTPS_PORT)) {
    Serial.println("HTTPS connection failed.");
    return false;
  }

  Serial.println("HTTPS connected.");

  // HTTPリクエスト
  httpsClient.print("POST ");
  httpsClient.print(HTTPS_PATH);
  httpsClient.println(" HTTP/1.1");

  httpsClient.print("Host: ");
  httpsClient.println(HTTPS_HOST);

  httpsClient.println("Content-Type: image/jpeg");

  httpsClient.print("x-device-key: ");
  httpsClient.println(HTTPS_DEVICE_KEY);

  httpsClient.print("x-requesttoken: ");
  httpsClient.println(requestToken);

  httpsClient.print("Content-Length: ");
  httpsClient.println(jpegSize);

  httpsClient.println("Connection: close");
  httpsClient.println();

  // JPEGを分割して送信
  size_t totalSent = 0;

  while (totalSent < jpegSize) {
    size_t remaining = jpegSize - totalSent;

    size_t chunkSize =
      remaining > 1024 ? 1024 : remaining;

    size_t sent = httpsClient.write(
      jpeg + totalSent,
      chunkSize
    );

    if (sent == 0 || sent > chunkSize) {
      Serial.println("JPEG transfer failed.");
      httpsClient.stop();
      return false;
    }

    totalSent += sent;
  }

  Serial.print("JPEG sent: ");
  Serial.print(totalSent);
  Serial.println(" bytes");

  // HTTP応答を待つ
  unsigned long start = millis();

  while (!httpsClient.available() &&
         httpsClient.connected() &&
         millis() - start < 30000) {
    delay(20);
  }

  if (!httpsClient.available()) {
    Serial.println("HTTP response timeout.");
    httpsClient.stop();
    return false;
  }

  String statusLine =
    httpsClient.readStringUntil('\n');

  statusLine.trim();

  Serial.print("HTTP response: ");
  Serial.println(statusLine);

  int spacePos = statusLine.indexOf(' ');

  int statusCode = 0;

  if (spacePos >= 0) {
    statusCode =
      statusLine.substring(spacePos + 1).toInt();
  }

  httpsClient.stop();

  return statusCode == 201;
}

// ========================================
// JPEG撮影・アップロード
// ========================================
bool captureAndUpload(const String& requestToken)
{
  Serial.println();
  Serial.println("----- Camera Capture -----");

  Serial.print("RequestToken: ");
  Serial.println(requestToken);

  CamImage img = theCamera.takePicture();

  if (!img.isAvailable()) {
    Serial.println("Capture failed.");
    return false;
  }

  uint8_t* jpeg = img.getImgBuff();
  size_t jpegSize = img.getImgSize();

  if (jpeg == NULL || jpegSize < 4) {
    Serial.println("JPEG unavailable.");
    return false;
  }

  // JPEG開始マーカー
  if (jpeg[0] != 0xFF || jpeg[1] != 0xD8) {
    Serial.println("Invalid JPEG start.");
    return false;
  }

  // 最後のJPEG終了マーカー FF D9を探す
  size_t actualJpegSize = 0;

  for (size_t i = jpegSize; i >= 2; i--) {
    if (jpeg[i - 2] == 0xFF &&
        jpeg[i - 1] == 0xD9) {
      actualJpegSize = i;
      break;
    }
  }

  if (actualJpegSize == 0) {
    Serial.println("JPEG end marker not found.");
    return false;
  }

  // 後ろがFFパディングのみであることを確認
  for (size_t i = actualJpegSize; i < jpegSize; i++) {
    if (jpeg[i] != 0xFF) {
      Serial.println("Unexpected JPEG trailing data.");
      return false;
    }
  }

  Serial.print("Original JPEG size: ");
  Serial.println(jpegSize);

  Serial.print("Actual JPEG size: ");
  Serial.println(actualJpegSize);

  Serial.print("Padding bytes: ");
  Serial.println(jpegSize - actualJpegSize);

  // カメラ画像のバッファが有効な間に送信
  return uploadJpeg(
    jpeg,
    actualJpegSize,
    requestToken
  );
}

// ========================================
// 初期化
// ========================================
void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Spresense LINE Camera Integration");

  if (!setupCamera()) {
    while (true) delay(1000);
  }

  doAttach();
  waitForLTE();

  // MQTT/TLS：CloudAMQP用のCA
  mqttTlsClient.setCACert(MQTT_ROOT_CA);

  mqttClient.setUsernamePassword(
    MQTT_USERNAME,
    MQTT_PASSWORD
  );

  mqttClient.setId("spresense-line-camera");

  // HTTPS：Codespaces用のCA
  httpsClient.setCACert(HTTPS_ROOT_CA);
  httpsClient.setTimeout(30000);
  httpsClient.setSendTimeout(30000);

  if (!connectMQTT()) {
    Serial.println("Initial MQTT connection failed.");
  }

  Serial.println("Setup completed.");
}

// ========================================
// メインループ
// ========================================
void loop()
{
  // 撮影指示を受け取っていたら処理する
  if (captureRequested) {

    captureRequested = false;

    String requestToken = pendingRequestToken;
    pendingRequestToken = "";

    // HTTPS送信中はMQTTをいったん切断する
    mqttClient.stop();

    Serial.println("MQTT disconnected for HTTPS upload.");

    bool result = captureAndUpload(requestToken);

    Serial.print("Upload result: ");
    Serial.println(result ? "SUCCESS" : "FAILED");

    Serial.println("Returning to MQTT standby.");
    // 次のloop()でMQTTを再接続する
    return;
  }

  // MQTT切断時は再接続する
  if (!mqttClient.connected()) {

    Serial.println("MQTT reconnecting...");

    if (!connectMQTT()) {
      delay(5000);
    }

    return;
  }

  // MQTT撮影指示を受信する
  int messageSize = mqttClient.parseMessage();

  if (messageSize > 0) {
    processMqttMessage(messageSize);
  }

  delay(10);
}



