/*
 * X# (Xsharp) LZMA2 Compression
 * ================================
 * Simplified but working LZMA2 implementation using LZ77 + range coding.
 *
 * Supports compression levels 1-9 and full round-trip compress/decompress.
 */

#ifndef XSHARP_LZMA2_H
#define XSHARP_LZMA2_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ===== Compression levels ===== */
#define LZMA2_MIN_LEVEL 1
#define LZMA2_MAX_LEVEL 9
#define LZMA2_DEFAULT_LEVEL 5

/* ===== Dictionary sizes by level ===== */
/* Level 1: 64KB, 2: 256KB, ..., 9: 32MB */
uint32_t lzma2_dict_size_for_level(int level);

/* ===== LZMA2 chunk types ===== */
#define LZMA2_CHUNK_UNCOMPRESSED 0x01
#define LZMA2_CHUNK_LZMA 0x02
#define LZMA2_CHUNK_END 0x00

/* ===== Range coder constants ===== */
#define RC_TOP_VALUE (1u << 24)
#define RC_BIT_MODEL_TOTAL (1u << 11)
#define RC_MOVE_BITS 5

/* ===== LZ77 match ===== */
typedef struct {
    uint32_t offset; /* distance (1-based) */
    uint32_t length; /* match length */
} Lz77Match;

/* ===== Range encoder state ===== */
typedef struct {
    uint64_t low;
    uint32_t range;
    uint8_t cache;
    uint32_t cache_size;
    uint8_t* out_buf;
    size_t out_size;
    size_t out_cap;
} RangeEncoder;

/* ===== Range decoder state ===== */
typedef struct {
    uint32_t code;
    uint32_t range;
    const uint8_t* in_buf;
    size_t in_size;
    size_t in_pos;
} RangeDecoder;

/* ===== LZMA state machine ===== */
#define LZMA_NUM_STATES 12
#define LZMA_NUM_POS_BITS 4
#define LZMA_NUM_POS_STATES (1 << LZMA_NUM_POS_BITS)
#define LZMA_NUM_LEN_BITS 3
#define LZMA_LEN_LOW_SIZE (1 << LZMA_NUM_LEN_BITS)
#define LZMA_LEN_MID_SIZE (1 << LZMA_NUM_LEN_BITS)
#define LZMA_LEN_HIGH_BITS 8
#define LZMA_LEN_HIGH_SIZE (1 << LZMA_LEN_HIGH_BITS)
#define LZMA_MIN_MATCH_LEN 2
#define LZMA_NUM_DIST_SLOTS 64

/* Probability model */
typedef uint16_t Prob;

/* Length coder probabilities */
typedef struct {
    Prob choice;
    Prob choice2;
    Prob low[LZMA_NUM_POS_STATES][LZMA_LEN_LOW_SIZE];
    Prob mid[LZMA_NUM_POS_STATES][LZMA_LEN_MID_SIZE];
    Prob high[LZMA_LEN_HIGH_SIZE];
} LzmaLenCoder;

/* ===== LZMA encoder context ===== */
typedef struct {
    /* Probability models */
    Prob is_match[LZMA_NUM_STATES][LZMA_NUM_POS_STATES];
    Prob is_rep[LZMA_NUM_STATES];
    Prob is_rep_g0[LZMA_NUM_STATES];
    Prob is_rep_g1[LZMA_NUM_STATES];
    Prob is_rep_g2[LZMA_NUM_STATES];
    Prob is_rep0_long[LZMA_NUM_STATES][LZMA_NUM_POS_STATES];

    LzmaLenCoder match_len_coder;
    LzmaLenCoder rep_len_coder;

    Prob dist_slot[4][LZMA_NUM_DIST_SLOTS];
    Prob dist_special[128 - 4]; /* direct-coded distance bits */
    Prob dist_align[16];        /* alignment bits */

    /* State */
    int state;
    uint32_t reps[4]; /* rep distances */

    /* Range encoder */
    RangeEncoder rc;

    /* LZ77 dictionary */
    uint8_t* window;
    uint32_t window_size;
    uint32_t window_pos;

    /* Hash chain for match finding */
    uint32_t* hash_table;
    uint32_t* hash_chain;
    uint32_t hash_mask;

    /* Compression level */
    int level;
} LzmaEncoder;

/* ===== LZMA decoder context ===== */
typedef struct {
    /* Probability models (same layout as encoder) */
    Prob is_match[LZMA_NUM_STATES][LZMA_NUM_POS_STATES];
    Prob is_rep[LZMA_NUM_STATES];
    Prob is_rep_g0[LZMA_NUM_STATES];
    Prob is_rep_g1[LZMA_NUM_STATES];
    Prob is_rep_g2[LZMA_NUM_STATES];
    Prob is_rep0_long[LZMA_NUM_STATES][LZMA_NUM_POS_STATES];

    LzmaLenCoder match_len_coder;
    LzmaLenCoder rep_len_coder;

    Prob dist_slot[4][LZMA_NUM_DIST_SLOTS];
    Prob dist_special[128 - 4];
    Prob dist_align[16];

    /* State */
    int state;
    uint32_t reps[4];

    /* Range decoder */
    RangeDecoder rd;

    /* Dictionary */
    uint8_t* window;
    uint32_t window_size;
    uint32_t window_pos;
    uint64_t total_decoded;
} LzmaDecoder;

/* ===== Range coder API ===== */
void rc_encoder_init(RangeEncoder* rc);
void rc_encoder_flush(RangeEncoder* rc);
void rc_encode_bit(RangeEncoder* rc, Prob* prob, int bit);
void rc_encode_direct(RangeEncoder* rc, uint32_t value, int bits);

void rc_decoder_init(RangeDecoder* rd, const uint8_t* data, size_t size);
int rc_decode_bit(RangeDecoder* rd, Prob* prob);
uint32_t rc_decode_direct(RangeDecoder* rd, int bits);

/* ===== LZMA encoder/decoder API ===== */
bool lzma_encoder_init(LzmaEncoder* enc, uint32_t dict_size, int level);
void lzma_encoder_free(LzmaEncoder* enc);
bool lzma_encode_block(LzmaEncoder* enc, const uint8_t* data, size_t size, uint8_t** out,
                       size_t* out_size);

bool lzma_decoder_init(LzmaDecoder* dec, uint32_t dict_size);
void lzma_decoder_free(LzmaDecoder* dec);
bool lzma_decode_block(LzmaDecoder* dec, const uint8_t* comp, size_t comp_size, uint8_t* out,
                       size_t out_size);

/* ===== High-level LZMA2 API ===== */

/*
 * Compress data using LZMA2 format.
 * LZMA2 stream = sequence of chunks:
 *   [chunk_type(1)][uncompressed_size(2)][compressed_size(2)][data...]
 *   ...
 *   [0x00] (end marker)
 *
 * Returns true on success. Caller must free *out.
 */
bool lzma2_compress(const uint8_t* data, size_t data_size, uint8_t** out, size_t* out_size,
                    int level, uint32_t dict_size);

/*
 * Decompress LZMA2 data.
 * uncompressed_size must be known (stored in the .Xscsc header).
 * Returns true on success.
 */
bool lzma2_decompress(const uint8_t* comp, size_t comp_size, uint8_t* out, size_t out_size,
                      uint32_t dict_size);

#endif /* XSHARP_LZMA2_H */
