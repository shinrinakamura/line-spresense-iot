
#!/usr/bin/env bash
set -euo pipefail

# ========================================
# Node-RED 初期設定スクリプト
# ========================================

# リポジトリのルートディレクトリへ移動
repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_dir"

# 既存の設定ファイルを保護する
if [[ -e .env ]]; then
  echo "既に .env が存在するため、上書きしません。"
  exit 1
fi

# Node-REDがインストールされているか確認する
if [[ ! -f node-red/node_modules/node-red/package.json ]]; then
  echo "先にNode-REDをインストールしてください。" >&2
  exit 1
fi

# ========================================
# 管理者名・パスワードの入力
# ========================================

read -r -p "管理者名 [admin]: " admin_user
admin_user="${admin_user:-admin}"

if [[ ! "$admin_user" =~ ^[A-Za-z0-9_-]+$ ]]; then
  echo "管理者名には英数字、_、- のみ使用してください。" >&2
  exit 1
fi

# -s オプションにより入力内容を表示しない
read -r -s -p "パスワード（12文字以上）: " password
printf '\n'

read -r -s -p "確認のため再入力: " confirmation
printf '\n'

if [[ ${#password} -lt 12 || "$password" != "$confirmation" ]]; then
  echo "12文字以上かつ一致するパスワードを入力してください。" >&2
  exit 1
fi

# ========================================
# パスワードのハッシュ化
# ========================================

# パスワードをコマンド引数に含めず、標準入力から渡す
password_hash="$(printf '%s' "$password" | node -e '
const { readFileSync } = require("node:fs");
const { resolve } = require("node:path");
const { createRequire } = require("node:module");

const bcrypt = createRequire(
  resolve("node-red/node_modules/node-red/package.json")
)("bcryptjs");

process.stdout.write(
  bcrypt.hashSync(readFileSync(0, "utf8"), 12)
);
')"

unset password confirmation

# ========================================
# 認証情報暗号化用の秘密鍵を生成
# ========================================

credential_secret="$(node -e '
process.stdout.write(
  require("node:crypto").randomBytes(32).toString("hex")
);
')"

# ========================================
# .env ファイルを作成
# ========================================

# 作成するファイルを所有者のみ読み書き可能にする
umask 077

# 一時ファイルを使ってから正式な名前に変更する
tmp_file="$(mktemp .env.tmp.XXXXXX)"
trap 'rm -f -- "$tmp_file"' EXIT

cat > "$tmp_file" <<EOF
# ========================================
# Node-RED 管理者認証
# ========================================
# Do not commit this file to Git.

NODE_RED_ADMIN_USER=$admin_user
NODE_RED_ADMIN_PASSWORD_HASH=$password_hash
NODE_RED_CREDENTIAL_SECRET=$credential_secret

# ========================================
# LINE Messaging API
# ========================================

# LINEチャネルシークレット
LINE_CHANNEL_SECRET=

# 許可するLINEユーザーID
LINE_ALLOWED_USER_ID=

# LINEチャネルアクセストークン
LINE_CHANNEL_ACCESS_TOKEN=

# ========================================
# JPEGアップロード認証
# ========================================

# Spresenseと共通の認証キー
UPLOAD_API_KEY=

# ========================================
# JPEG画像の配信用HTTPS URL
# ========================================

# Codespacesの8081番ポートの公開URL
LINE_IMAGE_BASE_URL=
EOF

mv -- "$tmp_file" .env
trap - EXIT

# ========================================
# 初期設定完了後の案内
# ========================================

echo "初期設定が完了しました。"
echo ".env ファイルを作成しました。"
echo ""
echo "次に .env を開き、以下の項目を設定してください。"
echo "  - LINE_CHANNEL_SECRET"
echo "  - LINE_ALLOWED_USER_ID"
echo "  - LINE_CHANNEL_ACCESS_TOKEN"
echo "  - UPLOAD_API_KEY"
echo "  - LINE_IMAGE_BASE_URL"
echo ""
echo "設定後、bash scripts/start.sh でNode-REDを起動してください。"
