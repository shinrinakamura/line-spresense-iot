
/*
 * Spresense HTTPS JPEG Upload Test
 *
 * LTE -> Camera -> HTTPS -> Node-RED
 *
 * MQTTやLINEとの連携は行わない。
 * HTTPSでのJPEGアップロードのみ検証する。
 */

#include <LTE.h>
#include <Camera.h>

#include "https_config.h"
#include "https_ca.h"

// MEEQ SIM
#define APP_LTE_APN       "YOUR-APN"
#define APP_LTE_USER_NAME "USER-NAME"
#define APP_LTE_PASSWORD  "PASS-WORD"

#define APP_LTE_IP_TYPE   LTE_NET_IPTYPE_V4V6
#define APP_LTE_AUTH_TYPE LTE_NET_AUTHTYPE_CHAP
#define APP_LTE_RAT       LTE_NET_RAT_CATM

LTE lteAccess;
LTETLSClient httpsClient;

// LTE接続開始
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

    Serial.println("LTE attach failed. Retrying...");
    lteAccess.shutdown();
    delay(1000);
  }
}

// LTE READY待ち
void waitForLTE()
{
  LTEModemStatus status = lteAccess.getStatus();

  while (status != LTE_READY) {
    if (status == LTE_ERROR) {
      Serial.println("LTE error. Retrying...");
      lteAccess.shutdown();
      delay(1000);
      doAttach();
    }

    delay(1000);
    status = lteAccess.getStatus();
  }

  Serial.println("LTE connected.");
}

// カメラ初期化
bool setupCamera()
{
  CamErr err = theCamera.begin();

  if (err != CAM_ERR_SUCCESS) {
    Serial.print("Camera begin failed: ");
    Serial.println((int)err);
    return false;
  }

  // 以前の検証で成功したQVGA/JPEG
  err = theCamera.setStillPictureImageFormat(
    CAM_IMGSIZE_QVGA_H,
    CAM_IMGSIZE_QVGA_V,
    CAM_IMAGE_PIX_FMT_JPG
  );

  if (err != CAM_ERR_SUCCESS) {
    Serial.print("Camera format failed: ");
    Serial.println((int)err);
    return false;
  }

  Serial.println("Camera ready.");
  return true;
}

// JPEGをHTTPSで送信
bool uploadJpeg(
  const uint8_t* jpeg,
  size_t jpegSize,
  const String& requestToken)
{
  Serial.print("Connecting HTTPS: ");
  Serial.println(HTTPS_HOST);

  if (!httpsClient.connect(HTTPS_HOST, HTTPS_PORT)) {
    Serial.println("HTTPS connection failed.");
    return false;
  }

  Serial.println("HTTPS connected.");

  // HTTP/1.1ヘッダー
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

  // JPEGを1024バイト単位で送信
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

  // HTTPレスポンス待ち
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

  // ステータス行を確認
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

  // Node-REDがJPEGを保存すると201を返す
  return statusCode == 201;
}

// カメラ撮影 + HTTPSアップロード


bool captureAndUpload()
{
  Serial.println("Taking picture...");

  CamImage img = theCamera.takePicture();

  if (!img.isAvailable()) {
    Serial.println("Capture failed.");
    return false;
  }

  uint8_t* jpeg = img.getImgBuff();
  size_t jpegSize = img.getImgSize();

  if (jpeg == NULL || jpegSize < 4) {
    Serial.println("JPEG data unavailable.");
    return false;
  }

  // ========================================
  // 撮影直後のJPEGデータを確認
  // ========================================
  Serial.println("----- JPEG Source Check -----");

  Serial.print("Image size: ");
  Serial.println(jpegSize);

  Serial.print("Buffer size: ");
  Serial.println(img.getImgBuffSize());

  // JPEG開始マーカー FF D8 を確認
  if (jpeg[0] != 0xFF || jpeg[1] != 0xD8) {
    Serial.println("Invalid JPEG start marker.");
    return false;
  }

  // ========================================
  // JPEG終了マーカー FF D9 を探す
  // ========================================
  // 最後に現れる終了マーカーを採用する
  int eoiPosition = -1;

  for (size_t i = 0; i + 1 < jpegSize; i++) {
    if (jpeg[i] == 0xFF && jpeg[i + 1] == 0xD9) {
      eoiPosition = (int)i;
    }
  }

  Serial.print("EOI position: ");
  Serial.println(eoiPosition);

  // JPEG終了マーカーが存在しなければ送信しない
  if (eoiPosition < 0) {
    Serial.println("JPEG end marker not found.");
    return false;
  }

  // FF D9 の2バイトを含めた実際のJPEGサイズ
  size_t actualJpegSize = (size_t)eoiPosition + 2;

  // ========================================
  // JPEG末尾の内容を確認
  // ========================================
  Serial.print("Last 8 bytes: ");

  size_t start = jpegSize > 8 ? jpegSize - 8 : 0;

  for (size_t i = start; i < jpegSize; i++) {
    if (jpeg[i] < 0x10) {
      Serial.print("0");
    }
    Serial.print(jpeg[i], HEX);
    Serial.print(" ");
  }

  Serial.println();

  // ========================================
  // 終了マーカー以降がFFパディングか確認
  // ========================================
  for (size_t i = actualJpegSize; i < jpegSize; i++) {
    if (jpeg[i] != 0xFF) {
      Serial.println("Unexpected data after JPEG.");
      return false;
    }
  }

  // サイズ情報を表示
  Serial.print("Original JPEG size: ");
  Serial.println(jpegSize);

  Serial.print("Actual JPEG size: ");
  Serial.println(actualJpegSize);

  Serial.print("Padding bytes: ");
  Serial.println(jpegSize - actualJpegSize);

  Serial.println("-----------------------------");

  // ========================================
  // リクエストトークン生成
  // ========================================
  // 単独試験用の識別子
  // LINE由来のrequestTokenは統合時に使用
  String requestToken =
    "spresense-test-" + String((unsigned long)millis());

  Serial.print("RequestToken: ");
  Serial.println(requestToken);

  // ========================================
  // HTTPSでJPEGをアップロード
  // ========================================
  // JPEG終了マーカーまでのデータだけを送信
  return uploadJpeg(
    jpeg,
    actualJpegSize,
    requestToken
  );
}


void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Spresense HTTPS Upload Test");

  if (!setupCamera()) {
    while (true) delay(1000);
  }

  doAttach();
  waitForLTE();

  // TLS接続前にルートCAを設定
  httpsClient.setCACert(HTTPS_ROOT_CA);

  // LTE通信のタイムアウト
  httpsClient.setTimeout(30000);
  httpsClient.setSendTimeout(30000);

  bool result = captureAndUpload();

  Serial.println();
  Serial.print("Upload result: ");
  Serial.println(result ? "SUCCESS" : "FAILED");
}

void loop()
{
  // 今回の検証では起動後に1回だけ撮影・送信
  delay(1000);
}
