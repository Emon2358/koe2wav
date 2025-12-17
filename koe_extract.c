/*
 * koe_extract.c
 * Visual Arts系 .koe ファイル抽出ツール (Enhanced Version)
 * Target: Windows 95/98/Me/2000/XP (x86)
 * Features: Auto-detect WAV/OGG, robust scanning
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 4096
// ヘッダー検索範囲（先頭から何バイトまで探すか）。通常は先頭数KBにある。
#define SCAN_LIMIT 65536 

typedef enum {
    FORMAT_UNKNOWN,
    FORMAT_WAV,
    FORMAT_OGG
} AudioFormat;

// 拡張子を変更するヘルパー関数
void change_extension(char *filename, const char *new_ext) {
    char *dot = strrchr(filename, '.');
    if (dot) {
        strcpy(dot, new_ext);
    } else {
        strcat(filename, new_ext);
    }
}

int main(int argc, char *argv[]) {
    FILE *fp_in, *fp_out;
    unsigned char buffer[BUFFER_SIZE];
    char out_filename[256];
    long file_size;
    long data_offset = -1;
    AudioFormat format = FORMAT_UNKNOWN;
    size_t bytes_read;
    long i;

    // --- ヘッダー表示 ---
    printf("KOE Extractor for Windows 95 (Legacy Edition)\n");
    printf("Supports: RIFF(WAV) / OggS(OGG)\n");
    printf("--------------------------------------------\n");

    if (argc < 2) {
        printf("Usage: koe_extract.exe <input_file.koe>\n");
        return 1;
    }

    // --- 入力ファイル処理 ---
    fp_in = fopen(argv[1], "rb");
    if (fp_in == NULL) {
        fprintf(stderr, "Error: Could not open input file '%s'.\n", argv[1]);
        return 1;
    }

    fseek(fp_in, 0, SEEK_END);
    file_size = ftell(fp_in);
    fseek(fp_in, 0, SEEK_SET);

    printf("File: %s (%ld bytes)\n", argv[1], file_size);

    // --- シグネチャ検索 ---
    // SCAN_LIMIT またはファイルサイズの小さい方までスキャン
    long scan_size = (file_size < SCAN_LIMIT) ? file_size : SCAN_LIMIT;
    unsigned char *scan_buf = (unsigned char *)malloc(scan_size);
    
    if (scan_buf == NULL) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        fclose(fp_in);
        return 1;
    }

    if (fread(scan_buf, 1, scan_size, fp_in) != scan_size) {
        fprintf(stderr, "Error: Failed to read file header.\n");
        free(scan_buf);
        fclose(fp_in);
        return 1;
    }

    for (i = 0; i < scan_size - 4; i++) {
        // RIFF (WAV) check
        if (scan_buf[i] == 'R' && scan_buf[i+1] == 'I' && 
            scan_buf[i+2] == 'F' && scan_buf[i+3] == 'F') {
            data_offset = i;
            format = FORMAT_WAV;
            break;
        }
        // OggS (OGG) check
        if (scan_buf[i] == 'O' && scan_buf[i+1] == 'g' && 
            scan_buf[i+2] == 'g' && scan_buf[i+3] == 'S') {
            data_offset = i;
            format = FORMAT_OGG;
            break;
        }
    }

    free(scan_buf); // スキャン用バッファはもう不要

    if (format == FORMAT_UNKNOWN) {
        fprintf(stderr, "Error: No known audio header (RIFF or OggS) found in the first %d bytes.\n", SCAN_LIMIT);
        fclose(fp_in);
        return 1;
    }

    // --- 出力ファイル名決定 ---
    strcpy(out_filename, argv[1]);
    if (format == FORMAT_WAV) {
        printf("Detected: WAV format (Offset: 0x%lX)\n", data_offset);
        change_extension(out_filename, ".wav");
    } else {
        printf("Detected: OGG format (Offset: 0x%lX)\n", data_offset);
        change_extension(out_filename, ".ogg");
    }

    // --- データ抽出 ---
    fp_out = fopen(out_filename, "wb");
    if (fp_out == NULL) {
        fprintf(stderr, "Error: Could not create output file '%s'. Check disk space or write permissions.\n", out_filename);
        fclose(fp_in);
        return 1;
    }

    fseek(fp_in, data_offset, SEEK_SET);

    long total_written = 0;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp_in)) > 0) {
        fwrite(buffer, 1, bytes_read, fp_out);
        total_written += bytes_read;
    }

    printf("Extracted: %ld bytes -> %s\n", total_written, out_filename);
    printf("Done.\n");

    fclose(fp_in);
    fclose(fp_out);

    return 0;
}
