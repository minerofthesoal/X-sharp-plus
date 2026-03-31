/*
 * X# Standard Library - Crypto Module
 * =====================================
 * Cryptographic hashing, HMAC, AES, random bytes, base64.
 * 9 functions.
 */

#ifndef XS_CRYPTO_LIB_H
#define XS_CRYPTO_LIB_H

#include "../../src/runtime/runtime.h"

XsValue xs_crypto_sha256(int argc, XsValue* args);
XsValue xs_crypto_sha512(int argc, XsValue* args);
XsValue xs_crypto_md5(int argc, XsValue* args);
XsValue xs_crypto_hmac(int argc, XsValue* args);
XsValue xs_crypto_randomBytes(int argc, XsValue* args);
XsValue xs_crypto_base64Encode(int argc, XsValue* args);
XsValue xs_crypto_base64Decode(int argc, XsValue* args);
XsValue xs_crypto_aesEncrypt(int argc, XsValue* args);
XsValue xs_crypto_aesDecrypt(int argc, XsValue* args);

void xs_crypto_register(VM* vm);

#endif /* XS_CRYPTO_LIB_H */
