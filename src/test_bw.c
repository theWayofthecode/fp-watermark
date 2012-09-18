#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pbm.h>
#include <zlib.h>
#include "flippability.h"
#include "shuffling.h"
#include "bin_watermarking.h"


int test_flip_lut(int n);
int test_shuffling(int n);
int test_bin_watermarking(char *path);

int main(int argc, char **argv) {
    int status = 0;
    pbm_init(&argc, argv);
    status += test_flip_lut(3);
    status += test_shuffling(1000000);
    status += test_bin_watermarking(argv[1]);
    if (status == 0) {
        printf("PASSED\n");
    } else {
        printf("FAILED\n");
    }
    return status;
}

int test_flip_lut(int n) {
    FILE *f;
    int i = 0;
    float *lut;
    size_t items;
    size_t N = 1 << (n * n);
    lut = (float *)calloc(N, sizeof(float));
    assert(lut != NULL);
    //check if flippalut.data is recreated
    init_flippability_lut(n);
    assert(system("rm flippalut.data") == 0);
    init_flippability_lut(n);
    f = fopen("flippalut.data", "r");
    assert(f != NULL);
    items = fread(lut, sizeof(float), N, f);
    assert(items == N);
    for (i = 0; i < N; i++) {
        if (!(lut[i] >= 0.0 && lut[i] <= 1.0)) {//check if 0.0
            printf("i=%d, lut[i]=%f\n", i, lut[i]);
            abort();
        }
    }
    fclose(f);
    return 0;
}

int test_shuffling(int n) {
    long sum = 0;
    int *sequence, idx;
    sequence = random_permutation(n);
    for (idx = 0; idx < n; idx++) {
        sum += (long)sequence[idx];
    }
    assert(sum == ((long)n - 1) * (long)n / 2);
    free(sequence);
    return 0;
}


int test_bin_watermarking(char *path) {
    int i, j, items, status;
    struct image img;
    FILE *fr, *fw, *f;
    Bytef *orig_pl = calloc(800, 1);
    Bytef *payload = calloc(800, 1);
    uLongf buflen = compressBound(800);
    uLongf dest, src = 800;
    Bytef *zipped = calloc(buflen, 1);
    //INIT
    fr = pm_openr(path);
    assert(fr != NULL);
    img.bitmap = pbm_readpbm(fr, &img.cols, &img.rows);
    assert(img.bitmap != NULL);
    //PROCESS
    f = fopen("imba.data.gz", "r");
    items = fread(payload, 1, 800, f);
    assert(items == 800);
    memcpy(orig_pl, payload, 800);
    dest = buflen;
    status = compress(zipped, &dest, payload, src);
    assert(status == Z_OK);
    embed(img, zipped, dest);
    extract(img, zipped, dest);
    status = uncompress(payload, &src, zipped, dest);
    assert(status == Z_OK);
    status = memcmp(payload, orig_pl, 800);
    assert(status == 0);
    //WRITE_BACK
    fw = pm_openw("out.pbm");
    assert(fw != NULL);
    pbm_writepbm(fw, img.bitmap, img.cols, img.rows, FALSE);
    //FREE
    pbm_freearray(img.bitmap, img.rows);
    free(zipped);
    free(payload);
    pm_close(fr);
    pm_close(fw);
    return 0;
}
