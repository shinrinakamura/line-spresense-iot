# Spresense MQTT/TLS送信確認サンプル

LTE経由でMQTTブローカーへTLS接続し、テスト文字列を1秒間隔で30回送信する、技術書の学習・動作確認用サンプルです。撮影処理は含みません。

## ライセンスと適用範囲

このフォルダーの `Spresense_Auth_TLS_pub.ino` および `config.h` の設定コードは、GNU General Public License version 2 only（`GPL-2.0-only`）で配布します。正式な許諾条件と免責事項は同梱の [LICENSE](LICENSE) を参照してください。ソースコードを再配布する際は、ライセンス本文および権利・ライセンス・変更に関する表示を保持してください。

この指定は、リポジトリ内の他の独立したプログラムや技術書本文に一括して適用するものではありません。依存するライブラリは各ライブラリのライセンスに従います。

本プログラムは無保証で提供します。商品性または特定目的への適合性についても保証しません。詳細はLICENSEを参照してください。

## 元サンプルと変更内容

本スケッチは、早川氏（GitHub: TomonobuHayakawa）が公開している次のサンプルを基にしています。

- 元リポジトリ: https://github.com/TomonobuHayakawa/Spresense-Playground
- 元ファイル: https://github.com/TomonobuHayakawa/Spresense-Playground/blob/5e5403efce89e06ebef4ef1680de6d2059b7054f/sketches/MQTT/viaLTE/simple_pub/simple_pub.ino
- 元ライセンス: https://github.com/TomonobuHayakawa/Spresense-Playground/blob/5e5403efce89e06ebef4ef1680de6d2059b7054f/LICENSE

2026-11-01に確認した元リポジトリのライセンスはLGPL 2.1です。その第3条に基づき、本改変版にはGPL v2を適用します。元リポジトリ自体のライセンスを変更するものではありません。

元サンプルから、MQTT/TLS接続、CloudAMQPのユーザー認証、MQTT再接続、設定ファイルの分離、日本語説明を追加・変更しています。

2026-11-01の変更: subと同じ関数構成・コメント・設定ファイルへ整理しました。MQTT再接続、送信結果の確認、millis()による送信間隔管理、固定Client IDの指定、終了時の切断を追加しました。CA証明書末尾の余分な文字を除去し、ライセンス表記、LICENSEおよび本READMEを追加しました。TLS・ユーザー認証は既存の実装を引き継いでいます。

## CA証明書

`config.h` に含まれるISRG Root X1は認証局が公開する証明書データです。このコードへのGPL指定によって証明書データを再ライセンスするものではありません。

- 公式情報: https://letsencrypt.org/certificates/
- PEM取得元: https://letsencrypt.org/certs/isrgrootx1.pem

2026-11-01に公式PEMとの一致を確認しました。接続先の証明書チェーンに応じて、適切なルートCAを設定してください。

## 設定と制約

`.ino` のAPN設定と `config.h` のMQTT接続情報を使用環境に合わせて変更してください。認証情報はダミーで配布しています。実際の認証情報を設定したファイルを公開しないでください。

シリアルモニタは115200bpsを使用します。LTE切断後の自動復旧や送信中の通信断への対策は十分ではなく、長期連続運転を想定していません。通信障害で処理が進まなくなった場合は、接続環境を確認して再起動してください。


## 送信内容と確認方法

Arduino IDEでこのフォルダーのスケッチを開き、SpresenseボードパッケージとArduinoMqttClientライブラリを使用します。APNとMQTT接続情報を設定し、シリアルモニタを115200bpsで開いてください。

- トピック: test/line
- 内容: test0! から test29!
- 間隔: 1秒（通信処理や再接続により長くなる場合があります）
- 回数: 送信APIが成功したものを30回カウント
- Client ID: spresense-test-pub（複数のpubを同時使用する場合は変更）
- 終了: Publish endを表示してMQTTを切断。再送する場合は再起動

Node-REDのMQTT InとDebug、またはsubスケッチで受信内容を確認できます。送信するのは撮影指示JSONではなくテスト文字列です。

QoS 0を使用するため、送信APIの成功は受信側への到達を保証しません。通信障害時の再試行では、同じ番号のメッセージが重複する可能性もあります。

今回の整理後のコードは静的確認のみで、ビルドと実機通信は未確認です。