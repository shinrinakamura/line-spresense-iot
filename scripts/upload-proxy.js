
"use strict";

const http = require("node:http");
const fs = require("node:fs/promises");
const path = require("node:path");

const PUBLIC_PORT = 8081;
const NODE_RED_PORT = 1880;
const UPLOAD_PATH = "/api/upload";
const MAX_BYTES = 1024 * 1024; // 1MB

// Node-REDで保存したJPEGのディレクトリ
const IMAGE_DIR = "/tmp/spresense-node-red-uploads";

// UUID形式の画像ファイルだけを取得対象にする
const IMAGE_URL_PATTERN =
  /^\/api\/images\/([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})\.jpg$/i;


// ============================================
// エラー応答
// ============================================
function respond(res, status, error) {
  if (res.headersSent || res.writableEnded) return;

  res.writeHead(status, {
    "content-type": "application/json",
    "cache-control": "no-store"
  });

  res.end(JSON.stringify({ ok: false, error }));
}


// ============================================
// HTTPサーバー
// ============================================
const server = http.createServer((req, res) => {

  // ==========================================
  // GET /api/images/{imageId}.jpg
  //
  // Node-REDが保存したJPEGを返す
  // LINEの画像メッセージから利用する
  // ==========================================
  const imageMatch = req.url.match(IMAGE_URL_PATTERN);

  if (
    (req.method === "GET" || req.method === "HEAD") &&
    imageMatch
  ) {
    const imageId = imageMatch[1];
    const filename = path.join(
      IMAGE_DIR,
      imageId + ".jpg"
    );

    fs.readFile(filename)
      .then((jpeg) => {

        // サイズ上限
        if (jpeg.length > MAX_BYTES) {
          respond(res, 413, "Image too large");
          return;
        }

        // JPEGを返す
        res.writeHead(200, {
          "content-type": "image/jpeg",
          "content-length": jpeg.length,
          "cache-control": "public, max-age=3600",
          "x-content-type-options": "nosniff"
        });

        if (req.method === "HEAD") {
          // HEADでは本文を返さない
          res.end();
        } else {
          // GETではJPEGのバイナリを返す
          res.end(jpeg);
        }

        console.log(
          `Image served: ${imageId} (${jpeg.length} bytes)`
        );

      })
      .catch((err) => {

        if (err.code === "ENOENT") {
          respond(res, 404, "Image not found");
        } else {
          console.error(
            "Image read error:",
            err.message
          );
          respond(res, 500, "Image read error");
        }
      });

    return;
  }


  // ==========================================
  // POST /api/upload
  //
  // JPEGをNode-REDへ転送する
  // ==========================================

  // アップロード以外のアクセスを拒否
  if (req.method !== "POST" || req.url !== UPLOAD_PATH) {
    respond(res, 404, "Not Found");
    return;
  }

  // Content-Lengthによるサイズ制限
  const declaredLength = req.headers["content-length"];

  if (declaredLength !== undefined) {
    const length = Number(declaredLength);

    if (
      !Number.isSafeInteger(length) ||
      length < 0 ||
      length > MAX_BYTES
    ) {
      respond(res, 413, "Payload Too Large");
      req.resume();
      return;
    }
  }

  const chunks = [];
  let size = 0;
  let rejected = false;

  // JPEGをバイナリのまま受信
  req.on("data", (chunk) => {
    if (rejected) return;

    size += chunk.length;

    if (size > MAX_BYTES) {
      rejected = true;
      chunks.length = 0;
      respond(res, 413, "Payload Too Large");
      return;
    }

    chunks.push(chunk);
  });

  req.on("end", () => {
    if (rejected) return;

    const body = Buffer.concat(chunks, size);

    // Node-REDに必要なヘッダーだけを転送
    const headers = {
      "content-type":
        req.headers["content-type"] ||
        "application/octet-stream",

      "content-length": body.length,

      "x-device-key":
        req.headers["x-device-key"] || "",

      "x-requesttoken":
        req.headers["x-requesttoken"] || ""
    };

    const upstream = http.request({
      hostname: "127.0.0.1",
      port: NODE_RED_PORT,
      path: UPLOAD_PATH,
      method: "POST",
      headers,
      timeout: 15000

    }, (response) => {

      console.log(
        `Upload forwarded: ${body.length} bytes, ` +
        `Node-RED status: ${response.statusCode}`
      );

      res.writeHead(response.statusCode || 502, {
        "content-type":
          response.headers["content-type"] ||
          "application/json",
        "cache-control": "no-store"
      });

      // Node-REDの応答をそのまま返す
      response.pipe(res);
    });

    upstream.on("timeout", () => {
      upstream.destroy(
        new Error("Node-RED timeout")
      );
    });

    upstream.on("error", (err) => {
      console.error("Proxy error:", err.message);
      respond(res, 502, "Bad Gateway");
    });

    // JPEGの内容を変更せずNode-REDへ転送
    upstream.end(body);
  });

  req.on("error", (err) => {
    console.error("Request error:", err.message);
    respond(res, 400, "Bad Request");
  });
});


// ============================================
// サーバー起動
// ============================================
server.requestTimeout = 30000;

server.listen(PUBLIC_PORT, "0.0.0.0", () => {
  console.log(
    `Upload proxy listening on port ${PUBLIC_PORT}`
  );

  console.log(
    `Forwarding uploads to http://127.0.0.1:${NODE_RED_PORT}${UPLOAD_PATH}`
  );

  console.log(
    `Serving images from ${IMAGE_DIR}`
  );
});
