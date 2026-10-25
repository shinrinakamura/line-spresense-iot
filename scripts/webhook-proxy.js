
"use strict";

const http = require("node:http");

const PUBLIC_PORT = 8080;
const NODE_RED_PORT = 1880;
const WEBHOOK_PATH = "/api/line/webhook";
const MAX_BODY_SIZE = 1024 * 1024; // 1MB

const server = http.createServer((req, res) => {
    // Webhook以外のURL・HTTPメソッドを拒否
    if (req.method !== "POST" || req.url !== WEBHOOK_PATH) {
        res.writeHead(404);
        res.end("Not Found");
        return;
    }

    const chunks = [];
    let size = 0;
    let rejected = false;

    req.on("data", (chunk) => {
        if (rejected) return;

        size += chunk.length;

        if (size > MAX_BODY_SIZE) {
            rejected = true;
            res.writeHead(413);
            res.end("Payload Too Large");
            return;
        }

        chunks.push(chunk);
    });

    req.on("end", () => {
        if (rejected) return;

        // 署名検証に必要な本文を変更せずに転送
        const body = Buffer.concat(chunks);

        const upstream = http.request({
            hostname: "127.0.0.1",
            port: NODE_RED_PORT,
            path: WEBHOOK_PATH,
            method: "POST",
            headers: {
                "content-type": req.headers["content-type"] || "application/json",
                "x-line-signature": req.headers["x-line-signature"] || "",
                "content-length": body.length
            },
            timeout: 10000
        }, (response) => {
            res.writeHead(response.statusCode || 502);
            response.pipe(res);
        });

        upstream.on("timeout", () => {
            upstream.destroy(new Error("Upstream timeout"));
        });

        upstream.on("error", () => {
            if (!res.headersSent) {
                res.writeHead(502);
                res.end("Bad Gateway");
            }
        });

        upstream.end(body);
    });
});

server.listen(PUBLIC_PORT, "0.0.0.0", () => {
    console.log(`Webhook proxy listening on port ${PUBLIC_PORT}`);
});
