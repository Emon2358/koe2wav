/*
 * koe_dos.c
 * MS-DOS用 Visual Arts系 .koe -> .wav/.ogg 変換ツール
 * Compiler: DJGPP (GCC for DOS), Open Watcom, Turbo C++
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 4096   /* 4KB buffer (Safe for DOS) */
#define SCAN_LIMIT 32768   /* 32KB Header scan limit */

typedef enum {
    FORMAT_UNKNOWN,
    FORMAT_WAV,
    FORMAT_OGG
} AudioFormat;

/* * ファイルパスからファイル名本体を取り出し、
 * 8.3形式の出力ファイル名を生成する関数
 */
void make_dos_filename(const char *input_path, char *output_name, const char *ext) {
    char base_name[9]; /* 8 chars + null */
    const char *p = input_path;
    const char *name_start = input_path;
    int i;

    /* パス区切り文字を探して、ファイル名の先頭を見つける */
    while (*p) {
        if (*p == '\\' || *p == '/' || *p == ':') {
            name_start = p + 1;
        }
        p++;
    }

    /* 拡張子の手前まで、最大8文字をコピー */
    for (i = 0; i < 8; i++) {
        char c = name_start[i];
        if (c == '.' || c == '\0') {
            break;
        }
        /* DOSはファイル名を大文字で扱うのが一般的 */
        base_name[i] = toupper((unsigned char)c);
    }
    base_name[i] = '\0';

    /* 出力ファイル名を構成 (BASE.EXT) */
    sprintf(output_name, "%s%s", base_name, ext);
}

int main(int argc, char *argv[]) {
    FILE *fp_in, *fp_out;
    unsigned char buffer[BUFFER_SIZE];
    unsigned char *scan_buf;
    char out_filename[13]; /* 8.3 format max: 8+1+3+1 = 13 */
    long file_size;
    long data_offset = -1;
    long scan_size;
    long i;
    size_t bytes_read;
    AudioFormat format = FORMAT_UNKNOWN;

    printf("KOE2WAV for MS-DOS\n");

    if (argc < 2) {
        printf("USAGE: KOE_DOS.EXE INPUT.KOE\n");
        return 1;
    }

    /* バイナリモードで開くことが必須 */
    fp_in = fopen(argv[1], "rb");
    if (fp_in == NULL) {
        printf("ERR: Cannot open %s\n", argv[1]);
        return 1;
    }

    /* ファイルサイズ取得 */
    fseek(fp_in, 0, SEEK_END);
    file_size = ftell(fp_in);
    fseek(fp_in, 0, SEEK_SET);

    /* ヘッダースキャン */
    scan_size = (file_size < SCAN_LIMIT) ? file_size : SCAN_LIMIT;
    scan_buf = (unsigned char *)malloc(scan_size);
    
    if (scan_buf == NULL) {
        printf("ERR: Out of memory\n");
        fclose(fp_in);
        return 1;
    }

    if (fread(scan_buf, 1, scan_size, fp_in) != scan_size) {
        printf("ERR: Read error\n");
        free(scan_buf);
        fclose(fp_in);
        return 1;
    }

    /* RIFF / OggS を探す */
    for (i = 0; i < scan_size - 4; i++) {
        if (scan_buf[i] == 'R' && scan_buf[i+1] == 'I' && 
            scan_buf[i+2] == 'F' && scan_buf[i+3] == 'F') {
            data_offset = i;
            format = FORMAT_WAV;
            break;
        }
        if (scan_buf[i] == 'O' && scan_buf[i+1] == 'g' && 
            scan_buf[i+2] == 'g' && scan_buf[i+3] == 'S') {
            data_offset = i;
            format = FORMAT_OGG;
            break;
        }
    }
    free(scan_buf);

    if (format == FORMAT_UNKNOWN) {
        printf("ERR: No WAV/OGG header found.\n");
        fclose(fp_in);
        return 1;
    }

    /* 出力ファイル名の決定 (8.3形式) */
    if (format == FORMAT_WAV) {
        make_dos_filename(argv[1], out_filename, ".WAV");
        printf("WAV found at offset %ld\n", data_offset);
    } else {
        make_dos_filename(argv[1], out_filename, ".OGG");
        printf("OGG found at offset %ld\n", data_offset);
    }

    fp_out = fopen(out_filename, "wb");
    if (fp_out == NULL) {
        printf("ERR: Cannot write %s\n", out_filename);
        fclose(fp_in);
        return 1;
    }

    /* データ抽出 */
    fseek(fp_in, data_offset, SEEK_SET);
    printf("Extracting to %s...\n", out_filename);

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp_in)) > 0) {
        if (fwrite(buffer, 1, bytes_read, fp_out) != bytes_read) {
            printf("ERR: Write failed (Disk full?)\n");
            break;
        }
    }

    printf("Done.\n");

    fclose(fp_in);
    fclose(fp_out);

    return 0;
}
