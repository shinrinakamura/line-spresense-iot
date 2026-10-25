
# LINE × Spresense IoT

LINE、Node-RED、MQTT、Spresenseを連携させるIoTシステムのサンプルプロジェクトです。

## 動作環境

- GitHub Codespaces
- Node.js 24.22.1
- Node-RED 5.0.9

Node-REDのインストールはCodespacesの作成時に自動実行されます。

## 1. Codespacesを作成する

1. このGitHubリポジトリを開きます。
2. 「Code」→「Codespaces」を選択します。
3. 新しいCodespaceを作成します。
4. 環境構築が完了するまで待ちます。

## 2. 初期設定を行う

Codespacesのターミナルで、次のコマンドを実行します。

```bash
bash scripts/setup.sh
```

画面の指示に従い、Node-REDの管理者名とパスワードを設定してください。

パスワードは12文字以上とし、他のサービスとの使い回しは避けてください。

認証情報は`.env`ファイルに保存されます。このファイルはGitの管理対象から除外されています。

## 3. Node-REDを起動する

次のコマンドを実行します。

```bash
bash scripts/start.sh
```

Codespacesの「PORTS」タブから1880番ポートをブラウザで開いてください。

Node-REDのログイン画面が表示されます。初期設定で登録した管理者名とパスワードでログインしてください。

## セキュリティ上の注意

- Node-REDの編集画面にはパスワード認証を設定しています。
- Codespacesの1880番ポートは、検証中はPrivateのまま使用してください。
- `.env`には認証情報が含まれるため、GitHubに登録しないでください。
- LINE WebhookなどのHTTPエンドポイントは、管理画面とは別にセキュリティ対策が必要です。
- 依存パッケージに未解決の脆弱性があるため、外部公開前に再評価してください。

## 今後追加する内容

- SpresenseのArduinoプログラム
- Node-REDのLINE連携フロー
- MQTT通信の設定
- カメラ撮影・画像送信機能
```
