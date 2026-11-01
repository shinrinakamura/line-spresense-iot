
#pragma once

// Codespacesの公開URLからホスト名だけ指定
// https:// や末尾の / は付けない
#define HTTPS_HOST \
  "your-url"

#define HTTPS_PORT 443
#define HTTPS_PATH "/api/upload"

// .envのUPLOAD_API_KEYと同じ値（自分で任意に設定します）
#define HTTPS_DEVICE_KEY "your-upload-api-key"