# Spresense デバイス側サンプルプログラム

このディレクトリには、技術書「LINEからIoTデバイスを操作しよう ― Spresenseで実践する双方向通信 ―」で使用するSpresense向けのArduinoサンプルプログラムを収録しています。

シリアル通信やカメラの動作確認から始め、LTE通信、MQTT/TLSによるメッセージ受信、HTTPSによる画像アップロードを段階的に確認し、最後にLINEと連携したシステムを動作させます。

各サンプルは学習・動作確認用です。長期間の無人運用や本番環境での利用を目的としたものではありません。

## 1. 必要な機材と開発環境

### ハードウェア

- Spresenseメインボード
- Spresense LTE拡張ボード
- Spresenseカメラボード
- LTE通信に対応したSIMカード
- USBケーブル、開発用PC

使用するSIMカードに応じて、APNなどの通信設定を変更してください。

### ソフトウェア

- Arduino IDE
- Spresense Arduinoボードパッケージ
- ArduinoMqttClientライブラリ（MQTTを使用するサンプル）
- Arduino IDEのシリアルモニタ

Spresenseの開発環境については、Sony公式ドキュメントも参照してください。

https://developer.sony.com/spresense/

## 2. サンプルプログラム一覧

| ディレクトリ | 内容 |
|---|---|
| `Serial_countup_simple` | シリアルモニタで1秒ごとにカウントを表示 |
| `Single_LED_Blynk` | Spresense基板上のLEDを点滅 |
| `Spresense_Camera_OperationCheck` | カメラの初期化と設定を確認 |
| `Spresense_MQTT_TLS_Pub` | LTE経由でMQTT/TLSメッセージを送信 |
| `Spresense_MQTT_TLS_Sub` | MQTT/TLSメッセージを受信してシリアルに表示 |
| `Spresense_HTTPS_Upload_Test` | カメラ画像をHTTPSでアップロード |
| `Spresense_MQTT_HTTPS_Camera` | MQTT撮影指示の受信から画像アップロードまでを統合 |

各ディレクトリの `.ino` ファイルをArduino IDEで開いて使用します。

## 3. サンプルの実行方法

1. Arduino IDEで使用するサンプルの `.ino` ファイルを開きます。
2. 使用するSpresenseボードとポートを選択します。
3. 通信を使用するサンプルでは、必要な接続情報を設定します。
4. プログラムをSpresenseに書き込みます。
5. シリアルモニタを115200bpsで開いて動作を確認します。

各サンプルは基本的に独立して動作するため、確認したい機能に応じてプログラムを書き換えてください。

## 4. 通信設定

LTE、MQTT、HTTPSを使用するサンプルでは、書き込み前に接続先や認証情報を設定する必要があります。

### LTE接続

各通信サンプルの `.ino` にある以下の設定を、使用するSIMに合わせて変更してください。

```cpp
#define APP_LTE_APN       "YOUR-APN"
#define APP_LTE_USER_NAME "USER-NAME"
#define APP_LTE_PASSWORD  "PASS-WORD"
```

必要に応じて認証方式、IPタイプ、通信方式も変更してください。

### MQTT/TLS

次のサンプルでは `config.h` を使用します。

- `Spresense_MQTT_TLS_Pub`
- `Spresense_MQTT_TLS_Sub`
- `Spresense_MQTT_HTTPS_Camera`

CloudAMQPなど、使用するMQTTブローカーの接続情報を設定してください。

主な設定項目は以下のとおりです。

| 設定項目 | 内容 |
|---|---|
| `MQTT_BROKER` | MQTTブローカーのホスト名 |
| `MQTT_PORT` | TLS通信ポート（8883） |
| `MQTT_USERNAME` | MQTTユーザー名 |
| `MQTT_PASSWORD` | MQTTパスワード |
| `MQTT_TOPIC` | 送受信するトピック |
| `MQTT_ROOT_CA` | TLSサーバー証明書検証用CA |

Node-RED側と同じトピックを設定してください。サンプルでは `test/line` を使用しています。

CA証明書は接続先サーバーの証明書チェーンに対応したものを設定する必要があります。

### HTTPS画像アップロード

次のサンプルでは `https_config.h` と `https_ca.h` を使用します。

- `Spresense_HTTPS_Upload_Test`
- `Spresense_MQTT_HTTPS_Camera`

`https_config.h` で設定する主な項目は以下のとおりです。

| 設定項目 | 内容 |
|---|---|
| `HTTPS_HOST` | Codespacesの8081番ポートに対応する公開ホスト名 |
| `HTTPS_PORT` | HTTPS通信ポート（443） |
| `HTTPS_PATH` | 画像アップロード先（`/api/upload`） |
| `HTTPS_DEVICE_KEY` | Node-RED側の `UPLOAD_API_KEY` と同じ値 |

`HTTPS_HOST` には `https://` を付けず、ホスト名のみを設定してください。

`https_ca.h` には、HTTPS接続先の証明書チェーンに対応するCA証明書を設定します。

**注意：** 各サンプルの認証情報は仮の値です。利用者自身の環境に合わせて設定した後、Spresenseへ書き込んでください。実際の認証情報を設定したファイルをGitHubなどへ公開しないでください。

## 5. 動作確認の順序

以下の順序で確認することをおすすめします。

### Step 1：基本動作

`Serial_countup_simple` でシリアル通信を確認します。

続いて `Single_LED_Blynk` で基板上のLEDが点滅することを確認します。

### Step 2：カメラ

`Spresense_Camera_OperationCheck` を実行し、カメラの初期化と設定を確認します。

撮影した画像のSDカードへの保存は、このサンプルでは無効化されています。

### Step 3：MQTT/TLS通信

`Spresense_MQTT_TLS_Pub` でメッセージを送信します。

Node-REDのMQTT受信ノードなどで、メッセージが届くことを確認します。

次に `Spresense_MQTT_TLS_Sub` を使用し、Node-REDから送信したメッセージがSpresenseのシリアルモニタに表示されることを確認します。

### Step 4：HTTPSアップロード

`Spresense_HTTPS_Upload_Test` を使用します。

起動後にJPEGを1枚撮影してHTTPSで送信します。シリアルモニタで `Upload result: SUCCESS` と表示されれば、HTTP 201を受信できています。

### Step 5：統合動作

`Spresense_MQTT_HTTPS_Camera` を使用します。

LINEから「写真」と送信すると、Node-REDを経由してSpresenseへMQTTの撮影指示が届きます。

Spresenseは指示を受信して撮影を行い、JPEG画像をHTTPSでアップロードします。

成功すると、Node-REDからLINEへ画像が送信されます。

このプログラムでは、画像アップロード時にMQTTを一度切断し、その後再接続して次の撮影指示を待ちます。

## 6. 注意事項

- 各通信サンプルは使用するSIM、MQTTブローカー、Codespacesの環境に合わせて設定してください。
- MQTT/TLSとHTTPSでは、それぞれ接続先に対応したCA証明書が必要です。
- Codespacesが停止すると、画像アップロードやLINEとの連携は利用できません。
- 通信状況によっては、LTE接続やMQTT再接続に時間がかかる場合があります。
- 統合版は撮影指示を1件ずつ処理する検証用プログラムです。
- 本サンプルには、長期間の無人運用に必要な障害復旧や監視の仕組みは十分に実装されていません。

## 7. ライセンス

MQTT/TLS送信・受信サンプルは、早川氏が公開したSpresense用サンプルを基に変更を加えています。

該当ディレクトリには、ライセンス情報と元コードの出典を記載したREADMEおよびLICENSEを収録しています。

これらのファイルを再利用・再配布する場合は、各ディレクトリのライセンス表記を確認してください。

他のサンプルについても、元コードや利用するライブラリのライセンス条件に従ってください。

## 8. 関連情報

- [プロジェクト全体のREADME](../README.md)
- [Node-REDサンプル](../node-red/examples/)
- [Spresense公式サイト](https://developer.sony.com/spresense/)
