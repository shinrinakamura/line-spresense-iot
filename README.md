# LINE × Spresense IoT

LINEから「写真」と送信すると、Spresenseがカメラで撮影し、画像をLINEに返すIoTシステムの検証用プロジェクトです。

GitHub Codespaces上のNode-REDを中心に、LINE Messaging API、MQTT（CloudAMQP）、SpresenseのLTE通信を組み合わせています。

## 動作の流れ

```text
LINEで「写真」を送信
    ↓ HTTPS / Webhook
Webhookプロキシ（Codespaces・8080番）
    ↓
Node-RED（1880番）
    ↓ MQTT/TLS
CloudAMQP（8883番）
    ↓ MQTT/TLS
Spresense（LTE）でJPEG撮影
    ↓ HTTPS POST
画像プロキシ（Codespaces・8081番）
    ↓
Node-REDでJPEG保存
    ↓ LINE Messaging API（Push）
LINEに撮影画像を送信
```

## 動作確認済みの構成

- GitHub Codespaces
- Node.js 24.22.1
- Node-RED 5.0.9
- MQTTブローカー：CloudAMQP（MQTT/TLS、8883番）
- Spresense（LTE通信・カメラ）
- LINE Messaging API

※ 2026年10月25日時点で、LINEへの撮影指示から画像返信まで一連の動作を確認しています。検証用構成であり、本番運用向けの耐久性・セキュリティは未整備です。

## 1. Codespacesを起動

このリポジトリでCodespacesを作成します。`.devcontainer/devcontainer.json`により、Node.js環境とNode-REDの依存パッケージを準備します。

自動インストールが完了していない場合は、リポジトリのルートで次を実行します。

```bash
npm ci --prefix node-red
```

## 2. Node-REDの初期設定

初回のみ、次を実行します。

```bash
bash scripts/setup.sh
```

管理者ユーザー名と12文字以上のパスワードを設定すると、`.env`が作成されます。`.env`がすでに存在する場合、このスクリプトは上書きしません。

続いて、生成された `.env` を開き、LINE連携などの設定項目に自分の環境の値を入力します。項目名は `.env.example` も参照してください。

```dotenv
LINE_CHANNEL_SECRET=
LINE_ALLOWED_USER_ID=
LINE_CHANNEL_ACCESS_TOKEN=
UPLOAD_API_KEY=
LINE_IMAGE_BASE_URL=
```

- `LINE_CHANNEL_SECRET`：LINE Webhookの署名検証に使用
- `LINE_ALLOWED_USER_ID`：撮影指示を受け付けるLINEユーザーID／画像Push先
- `LINE_CHANNEL_ACCESS_TOKEN`：LINE Messaging APIのアクセストークン
- `UPLOAD_API_KEY`：Spresenseからの画像アップロードを認証する共有キー
- `LINE_IMAGE_BASE_URL`：8081番ポートの公開HTTPS URL（例：`https://<codespace-name>-8081.app.github.dev`。末尾の`/`は不要）

実際の認証情報は**`.env`にのみ設定**し、`.env.example`やGitHubには記載しないでください。Spresense側の画像送信キーも`UPLOAD_API_KEY`と一致させます。

## 3. Node-REDを起動

ターミナルで次を実行します。

```bash
bash scripts/start.sh
```

Codespacesの「PORTS」から1880番ポートを開き、設定した管理者アカウントでログインします。**1880番ポートはPrivateのまま**使用してください。

`.env`を編集した場合、変更内容を反映するにはNode-REDの再起動が必要です。

## 4. Node-REDフローを読み込む

Node-RED編集画面のメニューから「読み込み」を開き、次のファイルをインポートします。

```text
node-red/examples/line-spresense-e2e.json
```

MQTTブローカーの接続設定を開き、自分のCloudAMQP環境に合わせて以下を設定します。

- ホスト名：CloudAMQPの実際のホスト名（サンプルの`your-broker`を置き換える）
- ポート：`8883`
- TLS：有効、サーバー証明書の検証を有効
- ユーザー名とパスワード：CloudAMQPから取得した値
- 撮影指示トピック：`test/line`（Spresense側と一致させる）

設定後にDeployし、MQTTノードが接続状態になったことを確認してください。サンプルJSONにはCloudAMQPのパスワードは含まれていません。

## 5. 公開プロキシを起動

**Node-REDとは別のターミナル**で、それぞれ起動します。

Webhook受信用（8080番）：

```bash
node scripts/webhook-proxy.js
```

画像アップロード・配信用（8081番）：

```bash
node scripts/upload-proxy.js
```

Codespacesの「PORTS」で、次の公開範囲を設定します。

| ポート | 用途 | 公開範囲 |
| --- | --- | --- |
| 1880 | Node-RED編集画面・内部HTTP API | **Private** |
| 8080 | LINE Webhook受付 | **Public** |
| 8081 | SpresenseのJPEGアップロードとLINE用画像配信 | **Public** |

LINE DevelopersでMessaging APIのWebhook URLを次の形式に設定します。

```text
https://<codespace-name>-8080.app.github.dev/api/line/webhook
```

`.env`の`LINE_IMAGE_BASE_URL`には8080番ではなく、**8081番の公開URL**を設定します。

Codespacesの公開URLは環境により変わります。変更した場合はLINE DevelopersのWebhook URLと`.env`を更新してください。

## 6. Spresense側の設定

Spresense用のArduinoサンプルは [device/](device/) に収録しています。シリアル通信・LED・カメラの基本動作、MQTT/TLSの送受信、JPEGのHTTPSアップロードを個別に確認した後、統合プログラムで一連の動作を試せます。

**サンプル一覧、必要機材、書き込み手順、設定項目は [device/README.md](device/README.md) を参照してください。**

LINE連携まで確認する場合は、次のスケッチを使用します。

```text
device/Spresense_MQTT_HTTPS_Camera/Spresense_MQTT_HTTPS_Camera.ino
```

書き込み前に、自分の環境に合わせて次の値を設定してください。

- スケッチ（`.ino`）内：使用するSIMのAPN・認証情報・必要に応じた通信設定
- `config.h`：CloudAMQPのホスト名・認証情報・MQTTトピック（Node-REDと同じ `test/line`）
- `https_config.h`：8081番ポートに対応する公開ホスト名・アップロード先 `/api/upload`・共有キー（`.env` の `UPLOAD_API_KEY` と一致）
- `config.h` と `https_ca.h`：各接続先の証明書チェーンに対応するCA証明書

統合プログラムは、MQTTで受け取った `capture` 指示と `requestToken` に基づきJPEGを撮影し、HTTPSでアップロードします。アップロード時はMQTTを一度切断し、その後再接続します。通信状況によって再接続に時間がかかる場合があります。

実際の認証情報を設定したファイルはGitHubにコミットしないでください。

## 7. 動作確認

1. Node-RED、8080番プロキシ、8081番プロキシが起動していることを確認します。
2. Node-REDのMQTT接続状態を確認し、Spresenseに上記の統合スケッチを書き込んでシリアルモニタ（115200bps）でMQTT購読を確認します。
3. 許可したLINEユーザーから「写真」と送信します。
4. Spresenseが撮影・HTTPSアップロードを行うことを確認します。
5. Node-REDでJPEG保存が成功し、アップロードにHTTP `201`が返ることを確認します。
6. LINE Messaging APIへのPush送信がHTTP `200`となり、LINEに写真が届くことを確認します。

撮影画像はCodespaces内の次のディレクトリに保存されます。

```text
/tmp/spresense-node-red-uploads/
```

画像は8081番ポートの`/api/images/<imageId>.jpg`から配信されます。

## 現時点の制約・注意事項

- **検証環境です。** Codespacesが停止すると、Webhook・画像アップロード・画像配信は利用できなくなります。
- JPEG画像は`/tmp`に保存され、永続保存は保証されません。画像の自動削除・保存容量管理も未実装です。
- 画像取得URLは現在、認証なしでアクセスできます。URLを知っている人は画像を取得できるため、機密画像は使用しないでください。
- 8080番と8081番はPublicですが、Node-RED編集画面の1880番はPrivateを維持してください。
- シークレットやCloudAMQPの認証情報をGitHubにコミットしないでください。
- Node-REDの実行中フロー`node-red/flows.json`はGit管理対象外です。共有用として`node-red/examples/line-spresense-e2e.json`を保存しています。**フロー更新時は共有用JSONも更新してください。**
- 外部公開・本番利用に進む前に、`npm audit --prefix node-red` などで依存関係の脆弱性を確認し、対応を判断してください。
- SpresenseのMQTT再接続速度、プロセスの自動起動、公開URLの固定化などは未対応です。

## 今後追加・改善する内容

- デバイス側設定ファイルのテンプレート化と、認証情報の誤コミット防止
- MQTT再接続時間の短縮と通信の安定性確認
- プロセス自動起動とCodespaces停止・再開時の復旧手順
- JPEG画像の保存期限と削除処理
- 公開画像URLのアクセス制御など、本番運用に向けたセキュリティ対策

## 主なファイル

| ファイル | 内容 |
| --- | --- |
| `.env.example` | 必要な環境変数の見本 |
| `device/README.md` | Spresense側サンプルの一覧・設定・使用方法 |
| `device/Spresense_MQTT_HTTPS_Camera/` | MQTT撮影指示の受信からJPEGアップロードまでの統合スケッチ |
| `scripts/setup.sh` | Node-RED初期設定、`.env`の作成 |
| `scripts/start.sh` | Node-RED起動 |
| `scripts/webhook-proxy.js` | LINE Webhookの中継（8080番） |
| `scripts/upload-proxy.js` | JPEGアップロードの中継・画像配信（8081番） |
| `node-red/examples/line-spresense-e2e.json` | LINE～MQTT～画像保存・返信のNode-REDフロー |
| `node-red/settings.js` | Node-RED設定 |

