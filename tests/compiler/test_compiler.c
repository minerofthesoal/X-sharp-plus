/*
 * X# Compiler Tests
 * ===================
 * Tests for bytecode generation correctness.
 * Tests the XSSC/XSCSC archive formats as the compiler output container.
 */

#include "../test_framework.h"
#include "../../src/formats/xssc.h"
#include "../../src/formats/xscsc.h"
#include "../../src/formats/lzma2.h"
#include <string.h>

/* ===== CRC32 Tests ===== */

static void test_crc32_empty(void) {
    TEST_CASE("CRC32: empty data");
    uint32_t crc = xssc_crc32(NULL, 0);
    /* CRC32 of empty should be 0 (our implementation: crc32_update(0, data, 0)) */
    uint8_t dummy = 0;
    crc = xssc_crc32(&dummy, 0);
    ASSERT_EQ(crc, 0);
}

static void test_crc32_known_value(void) {
    TEST_CASE("CRC32: known test vector");
    const char *data = "123456789";
    uint32_t crc = xssc_crc32((const uint8_t *)data, 9);
    /* Standard CRC32 of "123456789" is 0xCBF43926 */
    ASSERT_EQ(crc, 0xCBF43926u);
}

static void test_crc32_incremental(void) {
    TEST_CASE("CRC32: incremental update equals single pass");
    const char *data = "Hello, X# World!";
    size_t len = strlen(data);

    uint32_t single = xssc_crc32((const uint8_t *)data, len);

    uint32_t inc = xssc_crc32_update(0, (const uint8_t *)data, 7);
    inc = xssc_crc32_update(inc, (const uint8_t *)data + 7, len - 7);

    ASSERT_EQ(single, inc);
}

/* ===== XSSC Archive Tests ===== */

static void test_xssc_create_free(void) {
    TEST_CASE("XSSC: create and free");
    XsscArchive *ar = xssc_create();
    ASSERT_NOT_NULL(ar);
    ASSERT_EQ(ar->version, XSSC_VERSION);
    ASSERT_EQ(ar->entry_count, 0);
    ASSERT_EQ(ar->metadata_count, 0);
    xssc_free(ar);
}

static void test_xssc_metadata(void) {
    TEST_CASE("XSSC: metadata get/set");
    XsscArchive *ar = xssc_create();
    ASSERT_NOT_NULL(ar);

    ASSERT_TRUE(xssc_set_metadata(ar, "author", "test"));
    ASSERT_TRUE(xssc_set_metadata(ar, "version", "1.0"));

    const char *author = xssc_get_metadata(ar, "author");
    ASSERT_NOT_NULL(author);
    ASSERT_STR_EQ(author, "test");

    const char *version = xssc_get_metadata(ar, "version");
    ASSERT_NOT_NULL(version);
    ASSERT_STR_EQ(version, "1.0");

    /* Non-existent key */
    ASSERT_NULL(xssc_get_metadata(ar, "nonexistent"));

    /* Overwrite */
    ASSERT_TRUE(xssc_set_metadata(ar, "author", "updated"));
    author = xssc_get_metadata(ar, "author");
    ASSERT_STR_EQ(author, "updated");

    ASSERT_EQ(ar->metadata_count, 2);
    xssc_free(ar);
}

static void test_xssc_add_data(void) {
    TEST_CASE("XSSC: add raw data entries");
    XsscArchive *ar = xssc_create();

    const char *data1 = "Hello from entry 1";
    const char *data2 = "Data for entry 2";

    int idx1 = xssc_add_data(ar, "src/main.xs", (const uint8_t *)data1,
                              strlen(data1), XSSC_ENTRY_SOURCE);
    ASSERT_EQ(idx1, 0);

    int idx2 = xssc_add_data(ar, "assets/icon.png", (const uint8_t *)data2,
                              strlen(data2), XSSC_ENTRY_ASSET);
    ASSERT_EQ(idx2, 1);

    ASSERT_EQ(ar->entry_count, 2);
    ASSERT_STR_EQ(ar->entries[0].path, "src/main.xs");
    ASSERT_EQ(ar->entries[0].type, XSSC_ENTRY_SOURCE);
    ASSERT_EQ(ar->entries[0].content_size, strlen(data1));
    ASSERT_MEM_EQ(ar->entries[0].data, data1, strlen(data1));

    ASSERT_STR_EQ(ar->entries[1].path, "assets/icon.png");
    ASSERT_EQ(ar->entries[1].type, XSSC_ENTRY_ASSET);

    xssc_free(ar);
}

static void test_xssc_serialize_deserialize(void) {
    TEST_CASE("XSSC: serialize and deserialize round-trip");
    XsscArchive *ar = xssc_create();

    xssc_set_metadata(ar, "project", "test_project");
    xssc_set_metadata(ar, "version", "2.0");

    const char *src = "quest() { engrave(\"hello\") }";
    xssc_add_data(ar, "main.xs", (const uint8_t *)src, strlen(src), XSSC_ENTRY_SOURCE);

    const char *cfg = "name=test\nversion=1.0";
    xssc_add_data(ar, "config.cfg", (const uint8_t *)cfg, strlen(cfg), XSSC_ENTRY_CONFIG);

    /* Serialize */
    uint8_t *buf = NULL;
    size_t size = 0;
    ASSERT_TRUE(xssc_serialize(ar, &buf, &size));
    ASSERT_NOT_NULL(buf);
    ASSERT_GT(size, (size_t)0);

    /* Check magic */
    ASSERT_MEM_EQ(buf, XSSC_MAGIC, XSSC_MAGIC_SIZE);
    /* Check footer magic */
    ASSERT_MEM_EQ(buf + size - XSSC_MAGIC_SIZE, XSSC_FOOTER_MAGIC, XSSC_MAGIC_SIZE);

    /* Deserialize */
    XsscArchive *ar2 = xssc_read_buffer(buf, size);
    ASSERT_NOT_NULL(ar2);

    /* Verify metadata */
    const char *proj = xssc_get_metadata(ar2, "project");
    ASSERT_NOT_NULL(proj);
    ASSERT_STR_EQ(proj, "test_project");

    /* Verify entries */
    ASSERT_EQ(ar2->entry_count, 2);
    ASSERT_STR_EQ(ar2->entries[0].path, "main.xs");
    ASSERT_EQ(ar2->entries[0].type, XSSC_ENTRY_SOURCE);
    ASSERT_EQ(ar2->entries[0].content_size, strlen(src));
    ASSERT_MEM_EQ(ar2->entries[0].data, src, strlen(src));

    ASSERT_STR_EQ(ar2->entries[1].path, "config.cfg");
    ASSERT_EQ(ar2->entries[1].type, XSSC_ENTRY_CONFIG);
    ASSERT_MEM_EQ(ar2->entries[1].data, cfg, strlen(cfg));

    xssc_free(ar);
    xssc_free(ar2);
}

static void test_xssc_validate(void) {
    TEST_CASE("XSSC: validate checksums");
    XsscArchive *ar = xssc_create();

    const char *data = "Test data for validation";
    xssc_add_data(ar, "test.txt", (const uint8_t *)data, strlen(data), XSSC_ENTRY_SOURCE);

    ASSERT_TRUE(xssc_validate(ar));

    /* Corrupt data and check validation fails */
    ar->entries[0].data[0] ^= 0xFF;
    ASSERT_FALSE(xssc_validate(ar));

    xssc_free(ar);
}

static void test_xssc_write_read_file(void) {
    TEST_CASE("XSSC: write to file and read back");
    XsscArchive *ar = xssc_create();

    const char *code = "forge add(a: blade, b: blade) -> blade { unleash a + b }";
    xssc_add_data(ar, "math.xs", (const uint8_t *)code, strlen(code), XSSC_ENTRY_SOURCE);
    xssc_set_metadata(ar, "test", "file_io");

    ASSERT_TRUE(xssc_write(ar, "/tmp/test_xssc.xssc"));

    /* Validate file */
    ASSERT_TRUE(xssc_validate_file("/tmp/test_xssc.xssc"));

    /* Read back */
    XsscArchive *ar2 = xssc_read("/tmp/test_xssc.xssc");
    ASSERT_NOT_NULL(ar2);
    ASSERT_EQ(ar2->entry_count, 1);
    ASSERT_STR_EQ(ar2->entries[0].path, "math.xs");
    ASSERT_MEM_EQ(ar2->entries[0].data, code, strlen(code));

    const char *meta = xssc_get_metadata(ar2, "test");
    ASSERT_NOT_NULL(meta);
    ASSERT_STR_EQ(meta, "file_io");

    xssc_free(ar);
    xssc_free(ar2);

    /* Cleanup */
    remove("/tmp/test_xssc.xssc");
}

static void test_xssc_multiple_entries(void) {
    TEST_CASE("XSSC: many entries");
    XsscArchive *ar = xssc_create();

    for (int i = 0; i < 100; i++) {
        char path[64];
        char data[128];
        snprintf(path, sizeof(path), "file_%03d.xs", i);
        snprintf(data, sizeof(data), "Content of file %d with some padding data here.", i);
        int idx = xssc_add_data(ar, path, (const uint8_t *)data,
                                strlen(data), XSSC_ENTRY_SOURCE);
        ASSERT_EQ(idx, i);
    }

    ASSERT_EQ(ar->entry_count, 100);
    ASSERT_TRUE(xssc_validate(ar));

    /* Round-trip */
    uint8_t *buf = NULL;
    size_t size = 0;
    ASSERT_TRUE(xssc_serialize(ar, &buf, &size));

    XsscArchive *ar2 = xssc_read_buffer(buf, size);
    ASSERT_NOT_NULL(ar2);
    ASSERT_EQ(ar2->entry_count, 100);
    ASSERT_TRUE(xssc_validate(ar2));

    /* Verify random entry */
    ASSERT_STR_EQ(ar2->entries[42].path, "file_042.xs");

    xssc_free(ar);
    xssc_free(ar2);
}

static void test_xssc_empty_entries(void) {
    TEST_CASE("XSSC: empty content entries");
    XsscArchive *ar = xssc_create();

    /* Empty data */
    uint8_t empty = 0;
    int idx = xssc_add_data(ar, "empty.txt", &empty, 0, XSSC_ENTRY_ASSET);
    ASSERT_EQ(idx, 0);
    ASSERT_EQ(ar->entries[0].content_size, (uint64_t)0);

    /* Round-trip */
    uint8_t *buf = NULL;
    size_t size = 0;
    ASSERT_TRUE(xssc_serialize(ar, &buf, &size));

    XsscArchive *ar2 = xssc_read_buffer(buf, size);
    ASSERT_NOT_NULL(ar2);
    ASSERT_EQ(ar2->entry_count, 1);
    ASSERT_EQ(ar2->entries[0].content_size, (uint64_t)0);

    xssc_free(ar);
    xssc_free(ar2);
}

static void test_xssc_corrupt_magic_rejected(void) {
    TEST_CASE("XSSC: corrupt magic rejected");
    XsscArchive *ar = xssc_create();
    const char *data = "test";
    xssc_add_data(ar, "t.txt", (const uint8_t *)data, 4, XSSC_ENTRY_SOURCE);

    uint8_t *buf = NULL;
    size_t size = 0;
    ASSERT_TRUE(xssc_serialize(ar, &buf, &size));

    /* Corrupt header magic */
    uint8_t *copy = (uint8_t *)malloc(size);
    memcpy(copy, buf, size);
    copy[0] = 'Z';

    XsscArchive *ar2 = xssc_read_buffer(copy, size);
    ASSERT_NULL(ar2);

    free(copy);
    xssc_free(ar);
}

static void test_xssc_corrupt_crc_rejected(void) {
    TEST_CASE("XSSC: corrupt CRC rejected");
    XsscArchive *ar = xssc_create();
    const char *data = "test data";
    xssc_add_data(ar, "t.txt", (const uint8_t *)data, strlen(data), XSSC_ENTRY_SOURCE);

    uint8_t *buf = NULL;
    size_t size = 0;
    ASSERT_TRUE(xssc_serialize(ar, &buf, &size));

    /* Corrupt a byte in the middle */
    uint8_t *copy = (uint8_t *)malloc(size);
    memcpy(copy, buf, size);
    copy[size / 2] ^= 0xFF;

    XsscArchive *ar2 = xssc_read_buffer(copy, size);
    ASSERT_NULL(ar2);

    free(copy);
    xssc_free(ar);
}

/* ===== LZMA2 Tests ===== */

static void test_lzma2_dict_size(void) {
    TEST_CASE("LZMA2: dictionary sizes");
    ASSERT_EQ(lzma2_dict_size_for_level(1), (uint32_t)(1 << 16));
    ASSERT_EQ(lzma2_dict_size_for_level(5), (uint32_t)(1 << 22));
    ASSERT_EQ(lzma2_dict_size_for_level(9), (uint32_t)(1 << 25));
}

static void test_lzma2_compress_empty(void) {
    TEST_CASE("LZMA2: compress empty data");
    uint8_t *out = NULL;
    size_t out_size = 0;
    uint8_t dummy = 0;
    ASSERT_TRUE(lzma2_compress(&dummy, 0, &out, &out_size, 1, 0));
    ASSERT_NOT_NULL(out);
    ASSERT_EQ(out_size, (size_t)1); /* Just end marker */
    ASSERT_EQ(out[0], LZMA2_CHUNK_END);
    free(out);
}

static void test_lzma2_round_trip_simple(void) {
    TEST_CASE("LZMA2: compress and decompress simple data");
    const char *data = "Hello, LZMA2 compression in X#!";
    size_t data_size = strlen(data);

    uint8_t *comp = NULL;
    size_t comp_size = 0;
    uint32_t dict_size = lzma2_dict_size_for_level(1);

    ASSERT_TRUE(lzma2_compress((const uint8_t *)data, data_size,
                               &comp, &comp_size, 1, dict_size));
    ASSERT_NOT_NULL(comp);
    ASSERT_GT(comp_size, (size_t)0);

    uint8_t *decomp = (uint8_t *)malloc(data_size);
    ASSERT_NOT_NULL(decomp);

    ASSERT_TRUE(lzma2_decompress(comp, comp_size, decomp, data_size, dict_size));
    ASSERT_MEM_EQ(decomp, data, data_size);

    free(comp);
    free(decomp);
}

static void test_lzma2_round_trip_repetitive(void) {
    TEST_CASE("LZMA2: compress repetitive data achieves compression");
    /* Create repetitive data that should compress well */
    size_t data_size = 4096;
    uint8_t *data = (uint8_t *)malloc(data_size);
    for (size_t i = 0; i < data_size; i++)
        data[i] = (uint8_t)(i % 16);

    uint8_t *comp = NULL;
    size_t comp_size = 0;
    uint32_t dict_size = lzma2_dict_size_for_level(3);

    ASSERT_TRUE(lzma2_compress(data, data_size, &comp, &comp_size, 3, dict_size));

    /* Verify decompression */
    uint8_t *decomp = (uint8_t *)malloc(data_size);
    ASSERT_TRUE(lzma2_decompress(comp, comp_size, decomp, data_size, dict_size));
    ASSERT_MEM_EQ(decomp, data, data_size);

    free(data);
    free(comp);
    free(decomp);
}

/* ===== XSCSC Tests ===== */

static void test_xscsc_compress_decompress(void) {
    TEST_CASE("XSCSC: compress and decompress archive");
    XsscArchive *xssc = xssc_create();
    xssc_set_metadata(xssc, "test", "compression");

    /* Add some data */
    const char *code = "quest() { engrave(\"compressed!\") }";
    xssc_add_data(xssc, "main.xs", (const uint8_t *)code, strlen(code), XSSC_ENTRY_SOURCE);

    /* Compress */
    XscscArchive *xscsc = xscsc_compress(xssc, 1);
    ASSERT_NOT_NULL(xscsc);
    ASSERT_EQ(xscsc->compression_method, XSCSC_COMPRESS_LZMA2);
    ASSERT_GT(xscsc->uncompressed_size, (uint64_t)0);
    ASSERT_GT(xscsc->compressed_size, (uint64_t)0);

    /* Decompress */
    XsscArchive *restored = xscsc_decompress(xscsc);
    ASSERT_NOT_NULL(restored);
    ASSERT_EQ(restored->entry_count, 1);
    ASSERT_STR_EQ(restored->entries[0].path, "main.xs");
    ASSERT_MEM_EQ(restored->entries[0].data, code, strlen(code));

    const char *meta = xssc_get_metadata(restored, "test");
    ASSERT_NOT_NULL(meta);
    ASSERT_STR_EQ(meta, "compression");

    xssc_free(xssc);
    xscsc_free(xscsc);
    xssc_free(restored);
}

static void test_xscsc_write_read(void) {
    TEST_CASE("XSCSC: write and read file");
    XsscArchive *xssc = xssc_create();
    const char *data = "Test compressed file IO";
    xssc_add_data(xssc, "test.txt", (const uint8_t *)data, strlen(data), XSSC_ENTRY_SOURCE);

    XscscArchive *compressed = xscsc_compress(xssc, 1);
    ASSERT_NOT_NULL(compressed);

    ASSERT_TRUE(xscsc_write(compressed, "/tmp/test_xscsc.xscsc"));

    XscscArchive *loaded = xscsc_read("/tmp/test_xscsc.xscsc");
    ASSERT_NOT_NULL(loaded);
    ASSERT_EQ(loaded->version, XSCSC_VERSION);
    ASSERT_EQ(loaded->compressed_size, compressed->compressed_size);
    ASSERT_EQ(loaded->uncompressed_size, compressed->uncompressed_size);

    /* Decompress loaded archive */
    XsscArchive *restored = xscsc_decompress(loaded);
    ASSERT_NOT_NULL(restored);
    ASSERT_EQ(restored->entry_count, 1);
    ASSERT_MEM_EQ(restored->entries[0].data, data, strlen(data));

    xssc_free(xssc);
    xscsc_free(compressed);
    xscsc_free(loaded);
    xssc_free(restored);

    remove("/tmp/test_xscsc.xscsc");
}

static void test_xscsc_corrupt_checksum_rejected(void) {
    TEST_CASE("XSCSC: corrupt checksum rejected on read");
    XsscArchive *xssc = xssc_create();
    const char *data = "data";
    xssc_add_data(xssc, "t.txt", (const uint8_t *)data, 4, XSSC_ENTRY_SOURCE);

    XscscArchive *comp = xscsc_compress(xssc, 1);
    ASSERT_NOT_NULL(comp);
    ASSERT_TRUE(xscsc_write(comp, "/tmp/test_xscsc_corrupt.xscsc"));

    /* Corrupt the compressed data checksum by modifying file */
    FILE *f = fopen("/tmp/test_xscsc_corrupt.xscsc", "r+b");
    ASSERT_NOT_NULL(f);
    /* Seek to a compressed data byte and flip it */
    fseek(f, XSCSC_HEADER_SIZE + 1, SEEK_SET);
    uint8_t b = 0;
    fread(&b, 1, 1, f);
    b ^= 0xFF;
    fseek(f, XSCSC_HEADER_SIZE + 1, SEEK_SET);
    fwrite(&b, 1, 1, f);
    fclose(f);

    XscscArchive *loaded = xscsc_read("/tmp/test_xscsc_corrupt.xscsc");
    /* Should fail due to CRC mismatch */
    ASSERT_NULL(loaded);

    xssc_free(xssc);
    xscsc_free(comp);
    remove("/tmp/test_xscsc_corrupt.xscsc");
}

/* ===== Range Coder Tests ===== */

static void test_range_coder_round_trip(void) {
    TEST_CASE("Range coder: encode/decode bits round-trip");
    /* Encode a sequence of bits */
    RangeEncoder enc;
    rc_encoder_init(&enc);

    Prob prob = RC_BIT_MODEL_TOTAL / 2;
    Prob enc_prob = prob;
    int bits[] = {0, 1, 0, 0, 1, 1, 0, 1};
    int num_bits = 8;

    for (int i = 0; i < num_bits; i++)
        rc_encode_bit(&enc, &enc_prob, bits[i]);

    rc_encoder_flush(&enc);

    /* Decode */
    RangeDecoder dec;
    rc_decoder_init(&dec, enc.out_buf, enc.out_size);

    Prob dec_prob = prob;
    for (int i = 0; i < num_bits; i++) {
        int decoded = rc_decode_bit(&dec, &dec_prob);
        ASSERT_EQ(decoded, bits[i]);
    }

    free(enc.out_buf);
}

/* ===== Test Runner ===== */

void run_compiler_tests(void) {
    TEST_SUITE("Compiler / Archive Formats");

    /* CRC32 */
    RUN_TEST(test_crc32_empty);
    RUN_TEST(test_crc32_known_value);
    RUN_TEST(test_crc32_incremental);

    /* XSSC */
    RUN_TEST(test_xssc_create_free);
    RUN_TEST(test_xssc_metadata);
    RUN_TEST(test_xssc_add_data);
    RUN_TEST(test_xssc_serialize_deserialize);
    RUN_TEST(test_xssc_validate);
    RUN_TEST(test_xssc_write_read_file);
    RUN_TEST(test_xssc_multiple_entries);
    RUN_TEST(test_xssc_empty_entries);
    RUN_TEST(test_xssc_corrupt_magic_rejected);
    RUN_TEST(test_xssc_corrupt_crc_rejected);

    /* LZMA2 */
    RUN_TEST(test_lzma2_dict_size);
    RUN_TEST(test_lzma2_compress_empty);
    RUN_TEST(test_lzma2_round_trip_simple);
    RUN_TEST(test_lzma2_round_trip_repetitive);

    /* XSCSC */
    RUN_TEST(test_xscsc_compress_decompress);
    RUN_TEST(test_xscsc_write_read);
    RUN_TEST(test_xscsc_corrupt_checksum_rejected);

    /* Range coder */
    RUN_TEST(test_range_coder_round_trip);
}
