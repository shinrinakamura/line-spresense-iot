
#!/usr/bin/env bash
set -euo pipefail

# ========================================
# Node-RED 起動スクリプト
# ========================================

# リポジトリのルートディレクトリへ移動
repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_dir"

# 初期設定が完了しているか確認する
if [[ ! -f .env ]]; then
  echo "初期設定が必要です。" >&2
  echo "先に bash scripts/setup.sh を実行してください。" >&2
  exit 1
fi

# Node-REDがインストールされているか確認する
if [[ ! -f node-red/node_modules/node-red/red.js ]]; then
  echo "Node-REDが見つかりません。" >&2
  echo "npm ci --prefix node-red を実行してください。" >&2
  exit 1
fi

# ========================================
# JPEG画像の保存先ディレクトリを準備
# ========================================

# Codespaces上でJPEGを保存するディレクトリ
# 既に存在する場合はそのまま使用する
mkdir -p /tmp/spresense-node-red-uploads

# ========================================
# Node-REDを起動
# ========================================

# 環境変数を読み込んでNode-REDを起動する
exec node --env-file=.env \
  node-red/node_modules/node-red/red.js \
  --settings node-red/settings.js \
  --userDir node-red
