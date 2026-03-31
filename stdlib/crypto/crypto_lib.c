/*
 * X# Standard Library - Crypto Module Implementation
 * =====================================================
 * Pure C implementations of SHA-256, SHA-512, MD5, HMAC, AES-128-ECB,
 * base64 encoding/decoding, and cryptographic random bytes.
 */

#include "crypto_lib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

/* =========================================================================
 * SHA-256 implementation
 * ========================================================================= */

static const uint32_t sha256_k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

#define ROTR32(x,n) (((x)>>(n))|((x)<<(32-(n))))
#define CH(x,y,z) (((x)&(y))^((~(x))&(z)))
#define MAJ(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))
#define EP0(x) (ROTR32(x,2)^ROTR32(x,13)^ROTR32(x,22))
#define EP1(x) (ROTR32(x,6)^ROTR32(x,11)^ROTR32(x,25))
#define SIG0(x) (ROTR32(x,7)^ROTR32(x,18)^((x)>>3))
#define SIG1(x) (ROTR32(x,17)^ROTR32(x,19)^((x)>>10))

static void sha256_compute(const uint8_t* data, size_t len, uint8_t hash[32]) {
    uint32_t h[8] = {
        0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
        0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19
    };

    /* Padding */
    size_t padded_len = ((len + 8) / 64 + 1) * 64;
    uint8_t* msg = (uint8_t*)calloc(padded_len, 1);
    memcpy(msg, data, len);
    msg[len] = 0x80;
    uint64_t bits = len * 8;
    for (int i = 0; i < 8; i++) msg[padded_len - 1 - i] = (uint8_t)(bits >> (i * 8));

    /* Process blocks */
    for (size_t offset = 0; offset < padded_len; offset += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) {
            w[i] = ((uint32_t)msg[offset + i*4] << 24) |
                   ((uint32_t)msg[offset + i*4+1] << 16) |
                   ((uint32_t)msg[offset + i*4+2] << 8) |
                   ((uint32_t)msg[offset + i*4+3]);
        }
        for (int i = 16; i < 64; i++) {
            w[i] = SIG1(w[i-2]) + w[i-7] + SIG0(w[i-15]) + w[i-16];
        }

        uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
        for (int i = 0; i < 64; i++) {
            uint32_t t1 = hh + EP1(e) + CH(e,f,g) + sha256_k[i] + w[i];
            uint32_t t2 = EP0(a) + MAJ(a,b,c);
            hh=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
        }
        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
    }
    free(msg);

    for (int i = 0; i < 8; i++) {
        hash[i*4]   = (h[i] >> 24) & 0xFF;
        hash[i*4+1] = (h[i] >> 16) & 0xFF;
        hash[i*4+2] = (h[i] >> 8)  & 0xFF;
        hash[i*4+3] = h[i] & 0xFF;
    }
}

static char* bytes_to_hex(const uint8_t* data, int len) {
    char* hex = (char*)malloc(len * 2 + 1);
    for (int i = 0; i < len; i++) sprintf(hex + i * 2, "%02x", data[i]);
    hex[len * 2] = '\0';
    return hex;
}

XsValue xs_crypto_sha256(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();
    uint8_t hash[32];
    sha256_compute((const uint8_t*)args[0].scroll, strlen(args[0].scroll), hash);
    return xs_scroll(bytes_to_hex(hash, 32));
}

/* =========================================================================
 * SHA-512 implementation
 * ========================================================================= */

static const uint64_t sha512_k[80] = {
    0x428a2f98d728ae22ULL,0x7137449123ef65cdULL,0xb5c0fbcfec4d3b2fULL,0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL,0x59f111f1b605d019ULL,0x923f82a4af194f9bULL,0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL,0x12835b0145706fbeULL,0x243185be4ee4b28cULL,0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL,0x80deb1fe3b1696b1ULL,0x9bdc06a725c71235ULL,0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL,0xefbe4786384f25e3ULL,0x0fc19dc68b8cd5b5ULL,0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL,0x4a7484aa6ea6e483ULL,0x5cb0a9dcbd41fbd4ULL,0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL,0xa831c66d2db43210ULL,0xb00327c898fb213fULL,0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL,0xd5a79147930aa725ULL,0x06ca6351e003826fULL,0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL,0x2e1b21385c26c926ULL,0x4d2c6dfc5ac42aedULL,0x53380d139d95b3dfULL,
    0x650a73548baf63deULL,0x766a0abb3c77b2a8ULL,0x81c2c92e47edaee6ULL,0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL,0xa81a664bbc423001ULL,0xc24b8b70d0f89791ULL,0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL,0xd69906245565a910ULL,0xf40e35855771202aULL,0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL,0x1e376c085141ab53ULL,0x2748774cdf8eeb99ULL,0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL,0x4ed8aa4ae3418acbULL,0x5b9cca4f7763e373ULL,0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL,0x78a5636f43172f60ULL,0x84c87814a1f0ab72ULL,0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL,0xa4506cebde82bde9ULL,0xbef9a3f7b2c67915ULL,0xc67178f2e372532bULL,
    0xca273eceea26619cULL,0xd186b8c721c0c207ULL,0xeada7dd6cde0eb1eULL,0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL,0x0a637dc5a2c898a6ULL,0x113f9804bef90daeULL,0x1b710b35131c471bULL,
    0x28db77f523047d84ULL,0x32caab7b40c72493ULL,0x3c9ebe0a15c9bebcULL,0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL,0x597f299cfc657e2aULL,0x5fcb6fab3ad6faecULL,0x6c44198c4a475817ULL
};

#define ROTR64(x,n) (((x)>>(n))|((x)<<(64-(n))))
#define S512_CH(x,y,z) (((x)&(y))^((~(x))&(z)))
#define S512_MAJ(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))
#define S512_EP0(x) (ROTR64(x,28)^ROTR64(x,34)^ROTR64(x,39))
#define S512_EP1(x) (ROTR64(x,14)^ROTR64(x,18)^ROTR64(x,41))
#define S512_SIG0(x) (ROTR64(x,1)^ROTR64(x,8)^((x)>>7))
#define S512_SIG1(x) (ROTR64(x,19)^ROTR64(x,61)^((x)>>6))

static void sha512_compute(const uint8_t* data, size_t len, uint8_t hash[64]) {
    uint64_t h[8] = {
        0x6a09e667f3bcc908ULL,0xbb67ae8584caa73bULL,
        0x3c6ef372fe94f82bULL,0xa54ff53a5f1d36f1ULL,
        0x510e527fade682d1ULL,0x9b05688c2b3e6c1fULL,
        0x1f83d9abfb41bd6bULL,0x5be0cd19137e2179ULL
    };

    size_t padded_len = ((len + 16) / 128 + 1) * 128;
    uint8_t* msg = (uint8_t*)calloc(padded_len, 1);
    memcpy(msg, data, len);
    msg[len] = 0x80;
    uint64_t bits = len * 8;
    for (int i = 0; i < 8; i++) msg[padded_len - 1 - i] = (uint8_t)(bits >> (i * 8));

    for (size_t offset = 0; offset < padded_len; offset += 128) {
        uint64_t w[80];
        for (int i = 0; i < 16; i++) {
            w[i] = 0;
            for (int j = 0; j < 8; j++)
                w[i] |= ((uint64_t)msg[offset + i*8 + j]) << (56 - j*8);
        }
        for (int i = 16; i < 80; i++)
            w[i] = S512_SIG1(w[i-2]) + w[i-7] + S512_SIG0(w[i-15]) + w[i-16];

        uint64_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hv=h[7];
        for (int i = 0; i < 80; i++) {
            uint64_t t1 = hv + S512_EP1(e) + S512_CH(e,f,g) + sha512_k[i] + w[i];
            uint64_t t2 = S512_EP0(a) + S512_MAJ(a,b,c);
            hv=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
        }
        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hv;
    }
    free(msg);

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++)
            hash[i*8+j] = (uint8_t)(h[i] >> (56 - j*8));
    }
}

XsValue xs_crypto_sha512(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();
    uint8_t hash[64];
    sha512_compute((const uint8_t*)args[0].scroll, strlen(args[0].scroll), hash);
    return xs_scroll(bytes_to_hex(hash, 64));
}

/* =========================================================================
 * MD5 implementation
 * ========================================================================= */

#define MD5_F(x,y,z) (((x)&(y))|((~(x))&(z)))
#define MD5_G(x,y,z) (((x)&(z))|((y)&(~(z))))
#define MD5_H(x,y,z) ((x)^(y)^(z))
#define MD5_I(x,y,z) ((y)^((x)|(~(z))))
#define ROTL32(x,n) (((x)<<(n))|((x)>>(32-(n))))

static void md5_compute(const uint8_t* data, size_t len, uint8_t hash[16]) {
    static const uint32_t s[64] = {
        7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
        5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
        4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
        6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21
    };
    static const uint32_t K[64] = {
        0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
        0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
        0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
        0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
        0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
        0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
        0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
        0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
    };

    uint32_t a0=0x67452301, b0=0xefcdab89, c0=0x98badcfe, d0=0x10325476;

    size_t padded_len = ((len + 8) / 64 + 1) * 64;
    uint8_t* msg = (uint8_t*)calloc(padded_len, 1);
    memcpy(msg, data, len);
    msg[len] = 0x80;
    uint64_t bits = len * 8;
    memcpy(msg + padded_len - 8, &bits, 8); /* little-endian */

    for (size_t offset = 0; offset < padded_len; offset += 64) {
        uint32_t M[16];
        for (int i = 0; i < 16; i++)
            memcpy(&M[i], msg + offset + i * 4, 4); /* little-endian */

        uint32_t A = a0, B = b0, C = c0, D = d0;
        for (int i = 0; i < 64; i++) {
            uint32_t F_val, g;
            if (i < 16)      { F_val = MD5_F(B,C,D); g = i; }
            else if (i < 32) { F_val = MD5_G(B,C,D); g = (5*i+1)%16; }
            else if (i < 48) { F_val = MD5_H(B,C,D); g = (3*i+5)%16; }
            else              { F_val = MD5_I(B,C,D); g = (7*i)%16; }
            F_val += A + K[i] + M[g];
            A = D; D = C; C = B; B = B + ROTL32(F_val, s[i]);
        }
        a0 += A; b0 += B; c0 += C; d0 += D;
    }
    free(msg);

    memcpy(hash,      &a0, 4);
    memcpy(hash + 4,  &b0, 4);
    memcpy(hash + 8,  &c0, 4);
    memcpy(hash + 12, &d0, 4);
}

XsValue xs_crypto_md5(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();
    uint8_t hash[16];
    md5_compute((const uint8_t*)args[0].scroll, strlen(args[0].scroll), hash);
    return xs_scroll(bytes_to_hex(hash, 16));
}

/* =========================================================================
 * HMAC-SHA256
 * ========================================================================= */

XsValue xs_crypto_hmac(int argc, XsValue* args) {
    if (argc < 2 || args[0].type != VAL_SCROLL || args[1].type != VAL_SCROLL) return xs_abyss();
    const char* key_str = args[0].scroll;
    const char* msg_str = args[1].scroll;
    size_t key_len = strlen(key_str);
    size_t msg_len = strlen(msg_str);

    uint8_t key[64];
    memset(key, 0, 64);
    if (key_len > 64) {
        sha256_compute((const uint8_t*)key_str, key_len, key);
        /* key is now 32 bytes, rest 0-padded */
    } else {
        memcpy(key, key_str, key_len);
    }

    uint8_t o_key_pad[64], i_key_pad[64];
    for (int i = 0; i < 64; i++) {
        o_key_pad[i] = key[i] ^ 0x5c;
        i_key_pad[i] = key[i] ^ 0x36;
    }

    /* inner hash = SHA256(i_key_pad || message) */
    size_t inner_len = 64 + msg_len;
    uint8_t* inner_data = (uint8_t*)malloc(inner_len);
    memcpy(inner_data, i_key_pad, 64);
    memcpy(inner_data + 64, msg_str, msg_len);
    uint8_t inner_hash[32];
    sha256_compute(inner_data, inner_len, inner_hash);
    free(inner_data);

    /* outer hash = SHA256(o_key_pad || inner_hash) */
    uint8_t outer_data[96]; /* 64 + 32 */
    memcpy(outer_data, o_key_pad, 64);
    memcpy(outer_data + 64, inner_hash, 32);
    uint8_t hmac_hash[32];
    sha256_compute(outer_data, 96, hmac_hash);

    return xs_scroll(bytes_to_hex(hmac_hash, 32));
}

/* =========================================================================
 * Random bytes
 * ========================================================================= */

XsValue xs_crypto_randomBytes(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    int n = (int)xs_as_spark(args[0]);
    if (n <= 0 || n > 1048576) return xs_abyss();

    uint8_t* buf = (uint8_t*)malloc(n);
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) { free(buf); return xs_abyss(); }
    ssize_t total = 0;
    while (total < n) {
        ssize_t r = read(fd, buf + total, n - total);
        if (r <= 0) break;
        total += r;
    }
    close(fd);

    if (total < n) { free(buf); return xs_abyss(); }
    char* hex = bytes_to_hex(buf, n);
    free(buf);
    return xs_scroll(hex);
}

/* =========================================================================
 * Base64
 * ========================================================================= */

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

XsValue xs_crypto_base64Encode(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();
    const uint8_t* data = (const uint8_t*)args[0].scroll;
    size_t len = strlen((const char*)data);
    size_t out_len = 4 * ((len + 2) / 3);
    char* result = (char*)malloc(out_len + 1);

    size_t j = 0;
    for (size_t i = 0; i < len; i += 3) {
        uint32_t n = ((uint32_t)data[i]) << 16;
        if (i + 1 < len) n |= ((uint32_t)data[i+1]) << 8;
        if (i + 2 < len) n |= ((uint32_t)data[i+2]);

        result[j++] = b64_table[(n >> 18) & 0x3F];
        result[j++] = b64_table[(n >> 12) & 0x3F];
        result[j++] = (i + 1 < len) ? b64_table[(n >> 6) & 0x3F] : '=';
        result[j++] = (i + 2 < len) ? b64_table[n & 0x3F] : '=';
    }
    result[j] = '\0';
    return xs_scroll(result);
}

static int b64_decode_char(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

XsValue xs_crypto_base64Decode(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();
    const char* data = args[0].scroll;
    size_t len = strlen(data);
    if (len % 4 != 0) return xs_abyss();

    size_t out_len = (len / 4) * 3;
    if (len > 0 && data[len-1] == '=') out_len--;
    if (len > 1 && data[len-2] == '=') out_len--;

    char* result = (char*)malloc(out_len + 1);
    size_t j = 0;
    for (size_t i = 0; i < len; i += 4) {
        int a = b64_decode_char(data[i]);
        int b = b64_decode_char(data[i+1]);
        int c = (data[i+2] != '=') ? b64_decode_char(data[i+2]) : 0;
        int d = (data[i+3] != '=') ? b64_decode_char(data[i+3]) : 0;
        if (a < 0 || b < 0) break;
        uint32_t n = (a << 18) | (b << 12) | (c << 6) | d;
        if (j < out_len) result[j++] = (n >> 16) & 0xFF;
        if (j < out_len) result[j++] = (n >> 8) & 0xFF;
        if (j < out_len) result[j++] = n & 0xFF;
    }
    result[j] = '\0';
    return xs_scroll(result);
}

/* =========================================================================
 * AES-128 ECB (simplified, for educational purposes)
 * ========================================================================= */

/* AES S-Box */
static const uint8_t aes_sbox[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static const uint8_t aes_inv_sbox[256] = {
    0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
    0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
    0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
    0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
    0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
    0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
    0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
    0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
    0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
    0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
    0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
    0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
    0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
    0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
    0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
    0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
};

static const uint8_t aes_rcon[11] = {0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36};

static void aes128_key_expansion(const uint8_t key[16], uint8_t round_keys[176]) {
    memcpy(round_keys, key, 16);
    for (int i = 4; i < 44; i++) {
        uint8_t temp[4];
        memcpy(temp, round_keys + (i-1)*4, 4);
        if (i % 4 == 0) {
            uint8_t t = temp[0];
            temp[0] = aes_sbox[temp[1]] ^ aes_rcon[i/4];
            temp[1] = aes_sbox[temp[2]];
            temp[2] = aes_sbox[temp[3]];
            temp[3] = aes_sbox[t];
        }
        for (int j = 0; j < 4; j++)
            round_keys[i*4+j] = round_keys[(i-4)*4+j] ^ temp[j];
    }
}

static uint8_t gmul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    for (int i = 0; i < 8; i++) {
        if (b & 1) p ^= a;
        uint8_t hi = a & 0x80;
        a <<= 1;
        if (hi) a ^= 0x1b;
        b >>= 1;
    }
    return p;
}

static void aes128_encrypt_block(const uint8_t in[16], uint8_t out[16], const uint8_t rk[176]) {
    uint8_t state[16];
    memcpy(state, in, 16);

    /* AddRoundKey */
    for (int i = 0; i < 16; i++) state[i] ^= rk[i];

    for (int round = 1; round <= 10; round++) {
        /* SubBytes */
        for (int i = 0; i < 16; i++) state[i] = aes_sbox[state[i]];
        /* ShiftRows */
        uint8_t t;
        t=state[1]; state[1]=state[5]; state[5]=state[9]; state[9]=state[13]; state[13]=t;
        t=state[2]; state[2]=state[10]; state[10]=t; t=state[6]; state[6]=state[14]; state[14]=t;
        t=state[15]; state[15]=state[11]; state[11]=state[7]; state[7]=state[3]; state[3]=t;
        /* MixColumns (skip in last round) */
        if (round < 10) {
            for (int c = 0; c < 4; c++) {
                int i = c * 4;
                uint8_t a0=state[i],a1=state[i+1],a2=state[i+2],a3=state[i+3];
                state[i]   = gmul(a0,2)^gmul(a1,3)^a2^a3;
                state[i+1] = a0^gmul(a1,2)^gmul(a2,3)^a3;
                state[i+2] = a0^a1^gmul(a2,2)^gmul(a3,3);
                state[i+3] = gmul(a0,3)^a1^a2^gmul(a3,2);
            }
        }
        /* AddRoundKey */
        for (int i = 0; i < 16; i++) state[i] ^= rk[round*16+i];
    }
    memcpy(out, state, 16);
}

static void aes128_decrypt_block(const uint8_t in[16], uint8_t out[16], const uint8_t rk[176]) {
    uint8_t state[16];
    memcpy(state, in, 16);

    /* AddRoundKey (last round key) */
    for (int i = 0; i < 16; i++) state[i] ^= rk[160+i];

    for (int round = 9; round >= 0; round--) {
        /* InvShiftRows */
        uint8_t t;
        t=state[13]; state[13]=state[9]; state[9]=state[5]; state[5]=state[1]; state[1]=t;
        t=state[2]; state[2]=state[10]; state[10]=t; t=state[6]; state[6]=state[14]; state[14]=t;
        t=state[3]; state[3]=state[7]; state[7]=state[11]; state[11]=state[15]; state[15]=t;
        /* InvSubBytes */
        for (int i = 0; i < 16; i++) state[i] = aes_inv_sbox[state[i]];
        /* AddRoundKey */
        for (int i = 0; i < 16; i++) state[i] ^= rk[round*16+i];
        /* InvMixColumns (skip in round 0) */
        if (round > 0) {
            for (int c = 0; c < 4; c++) {
                int i = c * 4;
                uint8_t a0=state[i],a1=state[i+1],a2=state[i+2],a3=state[i+3];
                state[i]   = gmul(a0,14)^gmul(a1,11)^gmul(a2,13)^gmul(a3,9);
                state[i+1] = gmul(a0,9)^gmul(a1,14)^gmul(a2,11)^gmul(a3,13);
                state[i+2] = gmul(a0,13)^gmul(a1,9)^gmul(a2,14)^gmul(a3,11);
                state[i+3] = gmul(a0,11)^gmul(a1,13)^gmul(a2,9)^gmul(a3,14);
            }
        }
    }
    memcpy(out, state, 16);
}

/* aesEncrypt(plaintext, key16bytes) -> hex string */
XsValue xs_crypto_aesEncrypt(int argc, XsValue* args) {
    if (argc < 2 || args[0].type != VAL_SCROLL || args[1].type != VAL_SCROLL) return xs_abyss();
    const uint8_t* plaintext = (const uint8_t*)args[0].scroll;
    const uint8_t* key = (const uint8_t*)args[1].scroll;
    size_t pt_len = strlen((const char*)plaintext);
    size_t key_len = strlen((const char*)key);
    if (key_len < 16) return xs_abyss();

    uint8_t round_keys[176];
    aes128_key_expansion(key, round_keys);

    /* PKCS7 padding */
    size_t padded_len = ((pt_len / 16) + 1) * 16;
    uint8_t* padded = (uint8_t*)calloc(padded_len, 1);
    memcpy(padded, plaintext, pt_len);
    uint8_t pad_val = (uint8_t)(padded_len - pt_len);
    for (size_t i = pt_len; i < padded_len; i++) padded[i] = pad_val;

    uint8_t* cipher = (uint8_t*)malloc(padded_len);
    for (size_t i = 0; i < padded_len; i += 16) {
        aes128_encrypt_block(padded + i, cipher + i, round_keys);
    }
    free(padded);

    char* hex = bytes_to_hex(cipher, (int)padded_len);
    free(cipher);
    return xs_scroll(hex);
}

/* aesDecrypt(hexCiphertext, key16bytes) -> plaintext string */
XsValue xs_crypto_aesDecrypt(int argc, XsValue* args) {
    if (argc < 2 || args[0].type != VAL_SCROLL || args[1].type != VAL_SCROLL) return xs_abyss();
    const char* hex = args[0].scroll;
    const uint8_t* key = (const uint8_t*)args[1].scroll;
    size_t hex_len = strlen(hex);
    if (hex_len % 32 != 0 || strlen((const char*)key) < 16) return xs_abyss();

    size_t ct_len = hex_len / 2;
    uint8_t* cipher = (uint8_t*)malloc(ct_len);
    for (size_t i = 0; i < ct_len; i++) {
        unsigned int byte;
        sscanf(hex + i * 2, "%2x", &byte);
        cipher[i] = (uint8_t)byte;
    }

    uint8_t round_keys[176];
    aes128_key_expansion(key, round_keys);

    uint8_t* plain = (uint8_t*)malloc(ct_len + 1);
    for (size_t i = 0; i < ct_len; i += 16) {
        aes128_decrypt_block(cipher + i, plain + i, round_keys);
    }
    free(cipher);

    /* Remove PKCS7 padding */
    uint8_t pad_val = plain[ct_len - 1];
    size_t real_len = ct_len;
    if (pad_val > 0 && pad_val <= 16) {
        int valid = 1;
        for (size_t i = ct_len - pad_val; i < ct_len; i++) {
            if (plain[i] != pad_val) { valid = 0; break; }
        }
        if (valid) real_len = ct_len - pad_val;
    }
    plain[real_len] = '\0';
    return xs_scroll((char*)plain);
}

/* ===== Registration ===== */

void xs_crypto_register(VM* vm) {
    vm_register_native(vm, "Crypto.sha256",       xs_crypto_sha256);
    vm_register_native(vm, "Crypto.sha512",       xs_crypto_sha512);
    vm_register_native(vm, "Crypto.md5",          xs_crypto_md5);
    vm_register_native(vm, "Crypto.hmac",         xs_crypto_hmac);
    vm_register_native(vm, "Crypto.randomBytes",  xs_crypto_randomBytes);
    vm_register_native(vm, "Crypto.base64Encode", xs_crypto_base64Encode);
    vm_register_native(vm, "Crypto.base64Decode", xs_crypto_base64Decode);
    vm_register_native(vm, "Crypto.aesEncrypt",   xs_crypto_aesEncrypt);
    vm_register_native(vm, "Crypto.aesDecrypt",   xs_crypto_aesDecrypt);
}
