
"use strict";

// ========================================
// 必須の環境変数を確認する
// ========================================

// 認証情報が未設定の場合、Node-REDを起動させない
const requiredVariables = [
    "NODE_RED_ADMIN_USER",
    "NODE_RED_ADMIN_PASSWORD_HASH",
    "NODE_RED_CREDENTIAL_SECRET"
];

for (const name of requiredVariables) {
    if (!process.env[name] || !process.env[name].trim()) {
        throw new Error(`環境変数 ${name} が設定されていません`);
    }
}

// bcrypt形式のパスワードハッシュであることを確認する
if (!/^\$2[aby]\$\d{2}\$[./A-Za-z0-9]{53}$/.test(
    process.env.NODE_RED_ADMIN_PASSWORD_HASH
)) {
    throw new Error("パスワードハッシュの形式が正しくありません");
}

// ========================================
// Node-REDの設定
// ========================================

module.exports = {
    // Node-REDが使用するポート
    uiPort: 1880,

    // 編集画面とHTTP APIのURLを分離する
    httpAdminRoot: "/",
    httpNodeRoot: "/api",

    // フローファイル名を固定する
    flowFile: "flows.json",

    // フローの認証情報を暗号化するための秘密鍵
    credentialSecret: process.env.NODE_RED_CREDENTIAL_SECRET,

    // LINE Webhookの署名検証に使用する暗号処理
    // Functionノードからglobal.get("crypto")で参照する
    functionGlobalContext: {
        crypto: require("node:crypto")
    },

    // 編集画面と管理APIのログイン認証
    adminAuth: {
        type: "credentials",
        users: [
            {
                username: process.env.NODE_RED_ADMIN_USER,
                password: process.env.NODE_RED_ADMIN_PASSWORD_HASH,
                permissions: "*"
            }
        ]
    }
};
