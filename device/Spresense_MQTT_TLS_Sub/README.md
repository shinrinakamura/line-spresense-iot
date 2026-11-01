# Spresense MQTT/TLS受信確認サンプル

LTE経由でMQTTブローカーへTLS接続し、受信したメッセージをシリアルモニタに表示する、技術書の学習・動作確認用サンプルです。撮影処理は含みません。

## ライセンスと適用範囲

このフォルダーの `Spresense_MQTT_TLS_Sub.ino` および `config.h` の設定コードは、GNU General Public License version 2 only（`GPL-2.0-only`）で配布します。正式な許諾条件と免責事項は同梱の [LICENSE](LICENSE) を参照してください。ソースコードを再配布する際は、ライセンス本文および権利・ライセンス・変更に関する表示を保持してください。

この指定は、リポジトリ内の他の独立したプログラムや技術書本文に一括して適用するものではありません。依存するライブラリは各ライブラリのライセンスに従います。

本プログラムは無保証で提供します。商品性または特定目的への適合性についても保証しません。詳細はLICENSEを参照してください。

## 元サンプルと変更内容

本スケッチは、早川氏（GitHub: TomonobuHayakawa）が公開している次のサンプルを基にしています。

- 元リポジトリ: https://github.com/TomonobuHayakawa/Spresense-Playground
- 元ファイル: https://github.com/TomonobuHayakawa/Spresense-Playground/blob/5e5403efce89e06ebef4ef1680de6d2059b7054f/sketches/MQTT/viaLTE/simple_sub/simple_sub.ino
- 元ライセンス: https://github.com/TomonobuHayakawa/Spresense-Playground/blob/5e5403efce89e06ebef4ef1680de6d2059b7054f/LICENSE

2026-11-01に確認した元リポジトリのライセンスはLGPL 2.1です。その第3条に基づき、本改変版にはGPL v2を適用します。元リポジトリ自体のライセンスを変更するものではありません。

元サンプルから、MQTT/TLS接続、CloudAMQPのユーザー認証、MQTT再接続と再購読、設定ファイルの分離、日本語説明を追加・変更しています。既存の日本語コメントのクレジットはスケッチ内に保持しています。

2026-11-01の変更: CA証明書の説明を修正し、ライセンス表記、LICENSEおよび本READMEを追加しました。上記の機能変更はこの日以前から存在するもので、個別の変更日は未記録です。

## CA証明書

`config.h` に含まれるISRG Root X1は認証局が公開する証明書データです。このコードへのGPL指定によって証明書データを再ライセンスするものではありません。

- 公式情報: https://letsencrypt.org/certificates/
- PEM取得元: https://letsencrypt.org/certs/isrgrootx1.pem

2026-11-01に公式PEMとの一致を確認しました。接続先の証明書チェーンに応じて、適切なルートCAを設定してください。

## 設定と制約

`.ino` のAPN設定と `config.h` のMQTT接続情報を使用環境に合わせて変更してください。認証情報はダミーで配布しています。実際の認証情報を設定したファイルを公開しないでください。

シリアルモニタは115200bpsを使用します。LTE切断後の自動復旧や受信途中の通信断への対策は十分ではなく、長期連続運転を想定していません。通信障害で処理が進まなくなった場合は、接続環境を確認して再起動してください。
