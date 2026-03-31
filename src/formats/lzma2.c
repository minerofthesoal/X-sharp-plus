/*
 * X# (Xsharp) LZMA2 Compression - Implementation
 * ==================================================
 * LZ77 sliding window + range coder based compression.
 */

#include "lzma2.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ===== Dictionary sizes by level ===== */

uint32_t lzma2_dict_size_for_level(int level) {
    if (level < 1) level = 1;
    if (level > 9) level = 9;
    /* 1:64K, 2:256K, 3:1M, 4:2M, 5:4M, 6:8M, 7:16M, 8:24M, 9:32M */
    static const uint32_t sizes[] = {
        0,
        1 << 16,   /* 64K */
        1 << 18,   /* 256K */
        1 << 20,   /* 1M */
        1 << 21,   /* 2M */
        1 << 22,   /* 4M */
        1 << 23,   /* 8M */
        1 << 24,   /* 16M */
        24 << 20,  /* 24M */
        1 << 25    /* 32M */
    };
    return sizes[level];
}

/* ===== Probability initialization ===== */

static void prob_init(Prob *probs, size_t count) {
    for (size_t i = 0; i < count; i++)
        probs[i] = RC_BIT_MODEL_TOTAL / 2;
}

/* ===== Range encoder ===== */

static void rc_out_byte(RangeEncoder *rc, uint8_t b) {
    if (rc->out_size >= rc->out_cap) {
        rc->out_cap = rc->out_cap < 256 ? 256 : rc->out_cap * 2;
        rc->out_buf = (uint8_t *)realloc(rc->out_buf, rc->out_cap);
    }
    rc->out_buf[rc->out_size++] = b;
}

static void rc_shift_low(RangeEncoder *rc) {
    if ((uint32_t)(rc->low) < 0xFF000000u || (rc->low >> 32) != 0) {
        uint8_t temp = rc->cache;
        do {
            rc_out_byte(rc, (uint8_t)(temp + (uint8_t)(rc->low >> 32)));
            temp = 0xFF;
        } while (--rc->cache_size != 0);
        rc->cache = (uint8_t)((uint32_t)(rc->low) >> 24);
    }
    rc->cache_size++;
    rc->low = (uint32_t)(rc->low) << 8;
}

void rc_encoder_init(RangeEncoder *rc) {
    rc->low = 0;
    rc->range = 0xFFFFFFFFu;
    rc->cache = 0;
    rc->cache_size = 1;
    rc->out_buf = NULL;
    rc->out_size = 0;
    rc->out_cap = 0;
}

void rc_encoder_flush(RangeEncoder *rc) {
    for (int i = 0; i < 5; i++)
        rc_shift_low(rc);
}

void rc_encode_bit(RangeEncoder *rc, Prob *prob, int bit) {
    uint32_t bound = (rc->range >> 11) * (*prob);
    if (bit == 0) {
        rc->range = bound;
        *prob += (RC_BIT_MODEL_TOTAL - *prob) >> RC_MOVE_BITS;
    } else {
        rc->low += bound;
        rc->range -= bound;
        *prob -= (*prob) >> RC_MOVE_BITS;
    }
    if (rc->range < RC_TOP_VALUE) {
        rc->range <<= 8;
        rc_shift_low(rc);
    }
}

void rc_encode_direct(RangeEncoder *rc, uint32_t value, int bits) {
    for (int i = bits - 1; i >= 0; i--) {
        rc->range >>= 1;
        if ((value >> i) & 1)
            rc->low += rc->range;
        if (rc->range < RC_TOP_VALUE) {
            rc->range <<= 8;
            rc_shift_low(rc);
        }
    }
}

/* ===== Range decoder ===== */

void rc_decoder_init(RangeDecoder *rd, const uint8_t *data, size_t size) {
    rd->in_buf = data;
    rd->in_size = size;
    rd->in_pos = 0;
    rd->code = 0;
    rd->range = 0xFFFFFFFFu;

    /* Read initial 5 bytes (first byte is ignored per LZMA convention) */
    if (rd->in_pos < rd->in_size) rd->in_pos++; /* skip first byte */
    for (int i = 0; i < 4; i++) {
        rd->code = (rd->code << 8);
        if (rd->in_pos < rd->in_size)
            rd->code |= rd->in_buf[rd->in_pos++];
    }
}

static uint8_t rc_read_byte(RangeDecoder *rd) {
    if (rd->in_pos < rd->in_size)
        return rd->in_buf[rd->in_pos++];
    return 0;
}

int rc_decode_bit(RangeDecoder *rd, Prob *prob) {
    uint32_t bound = (rd->range >> 11) * (*prob);
    int bit;
    if (rd->code < bound) {
        rd->range = bound;
        *prob += (RC_BIT_MODEL_TOTAL - *prob) >> RC_MOVE_BITS;
        bit = 0;
    } else {
        rd->code -= bound;
        rd->range -= bound;
        *prob -= (*prob) >> RC_MOVE_BITS;
        bit = 1;
    }
    if (rd->range < RC_TOP_VALUE) {
        rd->range <<= 8;
        rd->code = (rd->code << 8) | rc_read_byte(rd);
    }
    return bit;
}

uint32_t rc_decode_direct(RangeDecoder *rd, int bits) {
    uint32_t value = 0;
    for (int i = 0; i < bits; i++) {
        rd->range >>= 1;
        rd->code -= rd->range;
        uint32_t t = 0 - ((uint32_t)rd->code >> 31);
        rd->code += rd->range & t;
        value = (value << 1) | (t + 1);
        if (rd->range < RC_TOP_VALUE) {
            rd->range <<= 8;
            rd->code = (rd->code << 8) | rc_read_byte(rd);
        }
    }
    return value;
}

/* ===== Bit tree coder helpers ===== */

static void rc_encode_tree(RangeEncoder *rc, Prob *probs, int bits, uint32_t symbol) {
    uint32_t m = 1;
    for (int i = bits - 1; i >= 0; i--) {
        int bit = (symbol >> i) & 1;
        rc_encode_bit(rc, &probs[m], bit);
        m = (m << 1) | bit;
    }
}

static uint32_t rc_decode_tree(RangeDecoder *rd, Prob *probs, int bits) {
    uint32_t m = 1;
    for (int i = 0; i < bits; i++) {
        m = (m << 1) | rc_decode_bit(rd, &probs[m]);
    }
    return m - (1u << bits);
}

static void rc_encode_tree_reverse(RangeEncoder *rc, Prob *probs, int bits, uint32_t symbol) {
    uint32_t m = 1;
    for (int i = 0; i < bits; i++) {
        int bit = symbol & 1;
        rc_encode_bit(rc, &probs[m], bit);
        m = (m << 1) | bit;
        symbol >>= 1;
    }
}

static uint32_t rc_decode_tree_reverse(RangeDecoder *rd, Prob *probs, int bits) {
    uint32_t m = 1;
    uint32_t symbol = 0;
    for (int i = 0; i < bits; i++) {
        int bit = rc_decode_bit(rd, &probs[m]);
        m = (m << 1) | bit;
        symbol |= ((uint32_t)bit << i);
    }
    return symbol;
}

/* ===== Length coder ===== */

static void len_coder_init(LzmaLenCoder *lc) {
    prob_init(&lc->choice, 1);
    prob_init(&lc->choice2, 1);
    prob_init(&lc->low[0][0], LZMA_NUM_POS_STATES * LZMA_LEN_LOW_SIZE);
    prob_init(&lc->mid[0][0], LZMA_NUM_POS_STATES * LZMA_LEN_MID_SIZE);
    prob_init(lc->high, LZMA_LEN_HIGH_SIZE);
}

static void len_encode(RangeEncoder *rc, LzmaLenCoder *lc, uint32_t len, uint32_t pos_state) {
    len -= LZMA_MIN_MATCH_LEN;
    if (len < LZMA_LEN_LOW_SIZE) {
        rc_encode_bit(rc, &lc->choice, 0);
        rc_encode_tree(rc, lc->low[pos_state], LZMA_NUM_LEN_BITS, len);
    } else {
        rc_encode_bit(rc, &lc->choice, 1);
        len -= LZMA_LEN_LOW_SIZE;
        if (len < LZMA_LEN_MID_SIZE) {
            rc_encode_bit(rc, &lc->choice2, 0);
            rc_encode_tree(rc, lc->mid[pos_state], LZMA_NUM_LEN_BITS, len);
        } else {
            rc_encode_bit(rc, &lc->choice2, 1);
            len -= LZMA_LEN_MID_SIZE;
            if (len >= LZMA_LEN_HIGH_SIZE)
                len = LZMA_LEN_HIGH_SIZE - 1;
            rc_encode_tree(rc, lc->high, LZMA_LEN_HIGH_BITS, len);
        }
    }
}

static uint32_t len_decode(RangeDecoder *rd, LzmaLenCoder *lc, uint32_t pos_state) {
    if (rc_decode_bit(rd, &lc->choice) == 0) {
        return LZMA_MIN_MATCH_LEN + rc_decode_tree(rd, lc->low[pos_state], LZMA_NUM_LEN_BITS);
    }
    if (rc_decode_bit(rd, &lc->choice2) == 0) {
        return LZMA_MIN_MATCH_LEN + LZMA_LEN_LOW_SIZE +
               rc_decode_tree(rd, lc->mid[pos_state], LZMA_NUM_LEN_BITS);
    }
    return LZMA_MIN_MATCH_LEN + LZMA_LEN_LOW_SIZE + LZMA_LEN_MID_SIZE +
           rc_decode_tree(rd, lc->high, LZMA_LEN_HIGH_BITS);
}

/* ===== Distance slot helpers ===== */

static uint32_t get_dist_slot(uint32_t dist) {
    /* Binary logarithm based slot */
    if (dist < 4) return dist;
    uint32_t n = dist;
    int bits = 0;
    while (n >= 2) { n >>= 1; bits++; }
    return (uint32_t)(bits * 2) + ((dist >> (bits - 1)) & 1);
}

static int get_len_to_pos_state(uint32_t len) {
    len -= LZMA_MIN_MATCH_LEN;
    if (len >= 4) return 3;
    return (int)len;
}

/* ===== LZMA State machine transitions ===== */

static int state_after_literal(int state) {
    if (state < 4) return 0;
    if (state < 10) return state - 3;
    return state - 6;
}

static int state_after_match(int state) {
    if (state < 7) return 7;
    return 10;
}

static int state_after_rep(int state) {
    if (state < 7) return 8;
    return 11;
}

static int state_after_short_rep(int state) {
    if (state < 7) return 9;
    return 11;
}

/* ===== Hash function for LZ77 match finding ===== */

static uint32_t hash3(const uint8_t *p) {
    return ((uint32_t)p[0] ^ ((uint32_t)p[1] << 8) ^ ((uint32_t)p[2] << 5)) & 0xFFFF;
}

/* ===== LZ77 match finder ===== */

static Lz77Match find_match(LzmaEncoder *enc, const uint8_t *data,
                            size_t data_size, size_t pos) {
    Lz77Match best = {0, 0};
    if (pos + 2 >= data_size) return best;

    uint32_t h = hash3(data + pos);
    uint32_t chain = enc->hash_table[h];
    enc->hash_table[h] = (uint32_t)pos;

    /* Link into chain */
    if (pos < enc->window_size)
        enc->hash_chain[pos & (enc->window_size - 1)] = chain;

    int max_attempts = 16 + (enc->level * 8);
    uint32_t max_dist = (pos < enc->window_size) ? (uint32_t)pos : enc->window_size;
    size_t max_len = data_size - pos;
    if (max_len > 273) max_len = 273;  /* LZMA max match length */

    uint32_t cur = chain;
    for (int attempt = 0; attempt < max_attempts && cur != 0xFFFFFFFF; attempt++) {
        if (cur >= pos) break;
        uint32_t dist = (uint32_t)(pos - cur);
        if (dist > max_dist || dist == 0) break;

        /* Compare */
        size_t len = 0;
        while (len < max_len && data[pos + len] == data[cur + len])
            len++;

        if (len >= LZMA_MIN_MATCH_LEN && len > best.length) {
            best.offset = dist;
            best.length = (uint32_t)len;
            if (len == max_len) break;
        }

        /* Follow chain */
        if (cur < enc->window_size)
            cur = enc->hash_chain[cur & (enc->window_size - 1)];
        else
            break;
    }
    return best;
}

/* ===== Distance encoding ===== */

static void encode_distance(LzmaEncoder *enc, uint32_t dist, uint32_t len) {
    int len_state = get_len_to_pos_state(len);
    uint32_t slot = get_dist_slot(dist);

    rc_encode_tree(&enc->rc, enc->dist_slot[len_state], 6, slot);

    if (slot >= 4) {
        int footer_bits = (int)(slot >> 1) - 1;
        uint32_t base = (2 | (slot & 1)) << footer_bits;
        uint32_t remainder = dist - base;

        if (slot < 14) {
            /* Use special probability-coded bits */
            rc_encode_tree_reverse(&enc->rc, enc->dist_special + base - slot - 1,
                                   footer_bits, remainder);
        } else {
            /* Direct bits + alignment bits */
            rc_encode_direct(&enc->rc, remainder >> 4, footer_bits - 4);
            rc_encode_tree_reverse(&enc->rc, enc->dist_align, 4, remainder & 0xF);
        }
    }
}

static uint32_t decode_distance(LzmaDecoder *dec, uint32_t len) {
    int len_state = get_len_to_pos_state(len);
    uint32_t slot = rc_decode_tree(&dec->rd, dec->dist_slot[len_state], 6);

    if (slot < 4) return slot;

    int footer_bits = (int)(slot >> 1) - 1;
    uint32_t base = (2 | (slot & 1)) << footer_bits;

    uint32_t remainder;
    if (slot < 14) {
        remainder = rc_decode_tree_reverse(&dec->rd,
                        dec->dist_special + base - slot - 1, footer_bits);
    } else {
        remainder = rc_decode_direct(&dec->rd, footer_bits - 4) << 4;
        remainder |= rc_decode_tree_reverse(&dec->rd, dec->dist_align, 4);
    }
    return base + remainder;
}

/* ===== Encoder ===== */

bool lzma_encoder_init(LzmaEncoder *enc, uint32_t dict_size, int level) {
    memset(enc, 0, sizeof(LzmaEncoder));

    enc->level = level;
    enc->window_size = dict_size;
    enc->state = 0;
    enc->reps[0] = enc->reps[1] = enc->reps[2] = enc->reps[3] = 1;

    /* Initialize probability models */
    prob_init(&enc->is_match[0][0], LZMA_NUM_STATES * LZMA_NUM_POS_STATES);
    prob_init(enc->is_rep, LZMA_NUM_STATES);
    prob_init(enc->is_rep_g0, LZMA_NUM_STATES);
    prob_init(enc->is_rep_g1, LZMA_NUM_STATES);
    prob_init(enc->is_rep_g2, LZMA_NUM_STATES);
    prob_init(&enc->is_rep0_long[0][0], LZMA_NUM_STATES * LZMA_NUM_POS_STATES);
    len_coder_init(&enc->match_len_coder);
    len_coder_init(&enc->rep_len_coder);
    prob_init(&enc->dist_slot[0][0], 4 * LZMA_NUM_DIST_SLOTS);
    prob_init(enc->dist_special, sizeof(enc->dist_special) / sizeof(Prob));
    prob_init(enc->dist_align, 16);

    /* Allocate hash table and chain */
    uint32_t hash_size = 65536;
    enc->hash_mask = hash_size - 1;
    enc->hash_table = (uint32_t *)malloc(sizeof(uint32_t) * hash_size);
    enc->hash_chain = (uint32_t *)malloc(sizeof(uint32_t) * dict_size);
    if (!enc->hash_table || !enc->hash_chain) {
        free(enc->hash_table);
        free(enc->hash_chain);
        return false;
    }
    memset(enc->hash_table, 0xFF, sizeof(uint32_t) * hash_size);
    memset(enc->hash_chain, 0xFF, sizeof(uint32_t) * dict_size);

    rc_encoder_init(&enc->rc);
    return true;
}

void lzma_encoder_free(LzmaEncoder *enc) {
    if (!enc) return;
    free(enc->hash_table);
    free(enc->hash_chain);
    free(enc->rc.out_buf);
    memset(enc, 0, sizeof(LzmaEncoder));
}

bool lzma_encode_block(LzmaEncoder *enc, const uint8_t *data, size_t size,
                       uint8_t **out, size_t *out_size) {
    if (!enc || !data || !out || !out_size) return false;

    /* Reset range coder output */
    enc->rc.out_size = 0;

    /* Write initial byte for range coder */
    rc_out_byte(&enc->rc, 0);

    size_t pos = 0;
    while (pos < size) {
        uint32_t pos_state = (uint32_t)pos & (LZMA_NUM_POS_STATES - 1);

        /* Try to find a match */
        Lz77Match match = find_match(enc, data, size, pos);

        /* Check for rep matches */
        int best_rep = -1;
        uint32_t best_rep_len = 0;
        for (int r = 0; r < 4; r++) {
            uint32_t dist = enc->reps[r];
            if (dist > pos || dist == 0) continue;
            size_t rpos = pos - dist;
            size_t rlen = 0;
            size_t max_rlen = size - pos;
            if (max_rlen > 273) max_rlen = 273;
            while (rlen < max_rlen && data[pos + rlen] == data[rpos + rlen])
                rlen++;
            if (rlen > best_rep_len && rlen >= LZMA_MIN_MATCH_LEN) {
                best_rep = r;
                best_rep_len = (uint32_t)rlen;
            }
        }

        /* Decide: literal, match, or rep match */
        if (best_rep_len >= match.length && best_rep_len >= LZMA_MIN_MATCH_LEN) {
            /* Rep match */
            rc_encode_bit(&enc->rc, &enc->is_match[enc->state][pos_state], 1);
            rc_encode_bit(&enc->rc, &enc->is_rep[enc->state], 1);

            if (best_rep == 0) {
                rc_encode_bit(&enc->rc, &enc->is_rep_g0[enc->state], 0);
                if (best_rep_len == 1) {
                    rc_encode_bit(&enc->rc, &enc->is_rep0_long[enc->state][pos_state], 0);
                    enc->state = state_after_short_rep(enc->state);
                    pos++;
                    continue;
                } else {
                    rc_encode_bit(&enc->rc, &enc->is_rep0_long[enc->state][pos_state], 1);
                }
            } else {
                rc_encode_bit(&enc->rc, &enc->is_rep_g0[enc->state], 1);
                if (best_rep == 1) {
                    rc_encode_bit(&enc->rc, &enc->is_rep_g1[enc->state], 0);
                } else {
                    rc_encode_bit(&enc->rc, &enc->is_rep_g1[enc->state], 1);
                    rc_encode_bit(&enc->rc, &enc->is_rep_g2[enc->state], best_rep == 2 ? 0 : 1);
                }
                /* Shift reps */
                uint32_t dist = enc->reps[best_rep];
                for (int j = best_rep; j > 0; j--)
                    enc->reps[j] = enc->reps[j - 1];
                enc->reps[0] = dist;
            }

            len_encode(&enc->rc, &enc->rep_len_coder, best_rep_len, pos_state);
            enc->state = state_after_rep(enc->state);
            pos += best_rep_len;

        } else if (match.length >= LZMA_MIN_MATCH_LEN) {
            /* Normal match */
            rc_encode_bit(&enc->rc, &enc->is_match[enc->state][pos_state], 1);
            rc_encode_bit(&enc->rc, &enc->is_rep[enc->state], 0);

            len_encode(&enc->rc, &enc->match_len_coder, match.length, pos_state);
            encode_distance(enc, match.offset - 1, match.length);

            /* Update reps */
            enc->reps[3] = enc->reps[2];
            enc->reps[2] = enc->reps[1];
            enc->reps[1] = enc->reps[0];
            enc->reps[0] = match.offset;

            enc->state = state_after_match(enc->state);

            /* Update hash chains for skipped positions */
            for (uint32_t k = 1; k < match.length && pos + k + 2 < size; k++) {
                uint32_t h = hash3(data + pos + k);
                enc->hash_chain[(pos + k) & (enc->window_size - 1)] = enc->hash_table[h];
                enc->hash_table[h] = (uint32_t)(pos + k);
            }

            pos += match.length;

        } else {
            /* Literal */
            rc_encode_bit(&enc->rc, &enc->is_match[enc->state][pos_state], 0);

            /* Encode literal byte directly using 8 bits via fixed probability */
            uint8_t byte = data[pos];
            uint32_t context = 1;
            for (int bit_idx = 7; bit_idx >= 0; bit_idx--) {
                int bit = (byte >> bit_idx) & 1;
                rc_encode_direct(&enc->rc, bit, 1);
                context = (context << 1) | bit;
            }

            enc->state = state_after_literal(enc->state);
            pos++;
        }
    }

    rc_encoder_flush(&enc->rc);

    *out = enc->rc.out_buf;
    *out_size = enc->rc.out_size;
    /* Detach buffer from encoder so caller owns it */
    enc->rc.out_buf = NULL;
    enc->rc.out_size = 0;
    enc->rc.out_cap = 0;
    return true;
}

/* ===== Decoder ===== */

bool lzma_decoder_init(LzmaDecoder *dec, uint32_t dict_size) {
    memset(dec, 0, sizeof(LzmaDecoder));

    dec->window_size = dict_size;
    dec->state = 0;
    dec->reps[0] = dec->reps[1] = dec->reps[2] = dec->reps[3] = 1;

    prob_init(&dec->is_match[0][0], LZMA_NUM_STATES * LZMA_NUM_POS_STATES);
    prob_init(dec->is_rep, LZMA_NUM_STATES);
    prob_init(dec->is_rep_g0, LZMA_NUM_STATES);
    prob_init(dec->is_rep_g1, LZMA_NUM_STATES);
    prob_init(dec->is_rep_g2, LZMA_NUM_STATES);
    prob_init(&dec->is_rep0_long[0][0], LZMA_NUM_STATES * LZMA_NUM_POS_STATES);
    len_coder_init(&dec->match_len_coder);
    len_coder_init(&dec->rep_len_coder);
    prob_init(&dec->dist_slot[0][0], 4 * LZMA_NUM_DIST_SLOTS);
    prob_init(dec->dist_special, sizeof(dec->dist_special) / sizeof(Prob));
    prob_init(dec->dist_align, 16);

    dec->window = (uint8_t *)calloc(dict_size, 1);
    if (!dec->window) return false;
    dec->window_pos = 0;
    dec->total_decoded = 0;

    return true;
}

void lzma_decoder_free(LzmaDecoder *dec) {
    if (!dec) return;
    free(dec->window);
    memset(dec, 0, sizeof(LzmaDecoder));
}

bool lzma_decode_block(LzmaDecoder *dec, const uint8_t *comp, size_t comp_size,
                       uint8_t *out, size_t out_size) {
    if (!dec || !comp || !out) return false;

    rc_decoder_init(&dec->rd, comp, comp_size);

    size_t pos = 0;
    while (pos < out_size) {
        uint32_t pos_state = (uint32_t)pos & (LZMA_NUM_POS_STATES - 1);

        if (rc_decode_bit(&dec->rd, &dec->is_match[dec->state][pos_state]) == 0) {
            /* Literal */
            uint8_t byte = 0;
            for (int bit_idx = 7; bit_idx >= 0; bit_idx--) {
                uint32_t bit = rc_decode_direct(&dec->rd, 1);
                byte |= (uint8_t)(bit << bit_idx);
            }
            out[pos] = byte;
            dec->window[dec->window_pos] = byte;
            dec->window_pos = (dec->window_pos + 1) % dec->window_size;
            dec->state = state_after_literal(dec->state);
            pos++;
        } else {
            /* Match or rep */
            uint32_t len;
            uint32_t dist;

            if (rc_decode_bit(&dec->rd, &dec->is_rep[dec->state]) == 0) {
                /* Normal match */
                len = len_decode(&dec->rd, &dec->match_len_coder, pos_state);
                dist = decode_distance(dec, len) + 1;

                dec->reps[3] = dec->reps[2];
                dec->reps[2] = dec->reps[1];
                dec->reps[1] = dec->reps[0];
                dec->reps[0] = dist;
                dec->state = state_after_match(dec->state);
            } else {
                /* Rep match */
                if (rc_decode_bit(&dec->rd, &dec->is_rep_g0[dec->state]) == 0) {
                    if (rc_decode_bit(&dec->rd, &dec->is_rep0_long[dec->state][pos_state]) == 0) {
                        /* Short rep (length 1) */
                        dist = dec->reps[0];
                        if (dist > pos) return false;
                        out[pos] = out[pos - dist];
                        dec->window[dec->window_pos] = out[pos];
                        dec->window_pos = (dec->window_pos + 1) % dec->window_size;
                        dec->state = state_after_short_rep(dec->state);
                        pos++;
                        continue;
                    }
                    dist = dec->reps[0];
                } else {
                    int rep_idx;
                    if (rc_decode_bit(&dec->rd, &dec->is_rep_g1[dec->state]) == 0) {
                        rep_idx = 1;
                    } else {
                        if (rc_decode_bit(&dec->rd, &dec->is_rep_g2[dec->state]) == 0) {
                            rep_idx = 2;
                        } else {
                            rep_idx = 3;
                        }
                    }
                    dist = dec->reps[rep_idx];
                    for (int j = rep_idx; j > 0; j--)
                        dec->reps[j] = dec->reps[j - 1];
                    dec->reps[0] = dist;
                }

                len = len_decode(&dec->rd, &dec->rep_len_coder, pos_state);
                dec->state = state_after_rep(dec->state);
            }

            /* Copy match bytes */
            if (dist > pos) return false;
            for (uint32_t k = 0; k < len && pos < out_size; k++, pos++) {
                out[pos] = out[pos - dist];
                dec->window[dec->window_pos] = out[pos];
                dec->window_pos = (dec->window_pos + 1) % dec->window_size;
            }
        }
    }

    return true;
}

/* ===== High-level LZMA2 API ===== */

/*
 * LZMA2 chunk format:
 *   [type: 1 byte]
 *   If type == LZMA2_CHUNK_LZMA:
 *     [uncompressed_size_hi: 1 byte][uncompressed_size_lo: 2 bytes]
 *     [compressed_size: 2 bytes]
 *     [lzma data...]
 *   If type == LZMA2_CHUNK_UNCOMPRESSED:
 *     [size: 2 bytes]
 *     [raw data...]
 *   If type == LZMA2_CHUNK_END:
 *     (end of stream)
 */

#define LZMA2_CHUNK_MAX_UNCOMPRESSED  (1 << 16)

bool lzma2_compress(const uint8_t *data, size_t data_size,
                    uint8_t **out, size_t *out_size,
                    int level, uint32_t dict_size) {
    if (!data || !out || !out_size) return false;
    if (data_size == 0) {
        /* Empty data: just end marker */
        *out = (uint8_t *)malloc(1);
        if (!*out) return false;
        (*out)[0] = LZMA2_CHUNK_END;
        *out_size = 1;
        return true;
    }

    if (level < LZMA2_MIN_LEVEL) level = LZMA2_MIN_LEVEL;
    if (level > LZMA2_MAX_LEVEL) level = LZMA2_MAX_LEVEL;
    if (dict_size == 0) dict_size = lzma2_dict_size_for_level(level);

    /* Output buffer */
    size_t out_cap = data_size + data_size / 4 + 1024;
    uint8_t *obuf = (uint8_t *)malloc(out_cap);
    if (!obuf) return false;
    size_t opos = 0;

    /* Process in chunks */
    size_t pos = 0;
    while (pos < data_size) {
        size_t chunk_size = data_size - pos;
        if (chunk_size > LZMA2_CHUNK_MAX_UNCOMPRESSED)
            chunk_size = LZMA2_CHUNK_MAX_UNCOMPRESSED;

        /* Try LZMA compression */
        LzmaEncoder enc;
        if (!lzma_encoder_init(&enc, dict_size, level)) {
            free(obuf);
            return false;
        }

        uint8_t *comp_data = NULL;
        size_t comp_size = 0;
        bool compressed = lzma_encode_block(&enc, data + pos, chunk_size,
                                            &comp_data, &comp_size);
        lzma_encoder_free(&enc);

        /* Use compressed if smaller, otherwise store uncompressed */
        if (compressed && comp_size < chunk_size) {
            /* LZMA chunk */
            size_t need = 6 + comp_size;
            while (opos + need >= out_cap) {
                out_cap *= 2;
                uint8_t *tmp = (uint8_t *)realloc(obuf, out_cap);
                if (!tmp) { free(comp_data); free(obuf); return false; }
                obuf = tmp;
            }
            obuf[opos++] = LZMA2_CHUNK_LZMA;
            obuf[opos++] = (uint8_t)((chunk_size >> 16) & 0xFF);
            obuf[opos++] = (uint8_t)((chunk_size >> 8) & 0xFF);
            obuf[opos++] = (uint8_t)(chunk_size & 0xFF);
            obuf[opos++] = (uint8_t)((comp_size >> 8) & 0xFF);
            obuf[opos++] = (uint8_t)(comp_size & 0xFF);
            memcpy(obuf + opos, comp_data, comp_size);
            opos += comp_size;
        } else {
            /* Uncompressed chunk */
            size_t need = 3 + chunk_size;
            while (opos + need >= out_cap) {
                out_cap *= 2;
                uint8_t *tmp = (uint8_t *)realloc(obuf, out_cap);
                if (!tmp) { free(comp_data); free(obuf); return false; }
                obuf = tmp;
            }
            obuf[opos++] = LZMA2_CHUNK_UNCOMPRESSED;
            obuf[opos++] = (uint8_t)((chunk_size >> 8) & 0xFF);
            obuf[opos++] = (uint8_t)(chunk_size & 0xFF);
            memcpy(obuf + opos, data + pos, chunk_size);
            opos += chunk_size;
        }

        free(comp_data);
        pos += chunk_size;
    }

    /* End marker */
    if (opos >= out_cap) {
        out_cap = opos + 1;
        uint8_t *tmp = (uint8_t *)realloc(obuf, out_cap);
        if (!tmp) { free(obuf); return false; }
        obuf = tmp;
    }
    obuf[opos++] = LZMA2_CHUNK_END;

    *out = obuf;
    *out_size = opos;
    return true;
}

bool lzma2_decompress(const uint8_t *comp, size_t comp_size,
                      uint8_t *out, size_t out_size,
                      uint32_t dict_size) {
    if (!comp || !out) return false;
    if (out_size == 0) return true;

    size_t cpos = 0;
    size_t opos = 0;

    while (cpos < comp_size) {
        uint8_t chunk_type = comp[cpos++];

        if (chunk_type == LZMA2_CHUNK_END) {
            break;
        } else if (chunk_type == LZMA2_CHUNK_UNCOMPRESSED) {
            if (cpos + 2 > comp_size) return false;
            uint16_t size = ((uint16_t)comp[cpos] << 8) | comp[cpos + 1];
            cpos += 2;
            if (cpos + size > comp_size) return false;
            if (opos + size > out_size) return false;
            memcpy(out + opos, comp + cpos, size);
            cpos += size;
            opos += size;
        } else if (chunk_type == LZMA2_CHUNK_LZMA) {
            if (cpos + 5 > comp_size) return false;
            uint32_t uncomp_size = ((uint32_t)comp[cpos] << 16) |
                                   ((uint32_t)comp[cpos + 1] << 8) |
                                   comp[cpos + 2];
            cpos += 3;
            uint16_t comp_chunk_size = ((uint16_t)comp[cpos] << 8) | comp[cpos + 1];
            cpos += 2;
            if (cpos + comp_chunk_size > comp_size) return false;
            if (opos + uncomp_size > out_size) return false;

            LzmaDecoder dec;
            if (!lzma_decoder_init(&dec, dict_size)) return false;
            bool ok = lzma_decode_block(&dec, comp + cpos, comp_chunk_size,
                                        out + opos, uncomp_size);
            lzma_decoder_free(&dec);
            if (!ok) return false;

            cpos += comp_chunk_size;
            opos += uncomp_size;
        } else {
            return false; /* Unknown chunk type */
        }
    }

    return opos == out_size;
}
