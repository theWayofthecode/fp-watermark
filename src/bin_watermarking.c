/**
*\file bin_watermarking.c
*This module implements the data embedding/extraction
*in/from an image.
*/
#include <stdio.h>
#include <stdlib.h>
#include <pbm.h>
#include <assert.h>
#include "flippability.h"
#include "shuffling.h"
#include "bin_watermarking.h"

#define pushbit(word, bit) (((word << 1) + bit) & 0x1ff)

/**
*This is an auxiliary data stracture
*used only for sorting the pixel positions
*in accordance with their flippability score
*/
struct pos_score {
    int pos;
    float score;
};

void sort_by_flippability(struct pos_score *flippables, int window,
        int *hd, struct image img, float *lut);
int sum_of_blacks(struct image img, int *hd, int window);
int flip_pixels(struct image img, struct pos_score *flippables, int array_size,
        int N_pix, const int color);
float evaluate(struct image img, int pos, float *lut);
int compar(const void *l, const void *r);

/**
*This function implements the data embedding functionality.
*It scans the image with the sliding window end flipps the pixels
*when needed, to establish the relationship with the payload.
*\param[in, out] img This struct represents the original image
*at the input, and at the output is the modified one.
*\param[in] payload A void * to the data to be embedded.
*\param[in] bytes The size of the payload.
*\returns Nothing.
*/
void embed(struct image img, void *payload, size_t bytes) {
    size_t items;
    int window, i, j, k, sum, status, seq_idx;
    struct pos_score *flippables;
    int *sequence;
    float *lut;
    FILE *f;
    div_t divided_sum;
    unsigned char *pl, byte;
    //INIT
    lut = (float *)calloc(1 << (3 * 3), sizeof(float));
    assert(lut != NULL);
    init_flippability_lut(3);
    f = fopen("flippalut.data", "r");
    assert(f != NULL);
    items = fread(lut, sizeof(float), 1 << (3 * 3), f);
    assert(items == (1 << (3 * 3)));
    sequence = random_permutation(img.cols * img.rows);
    assert(sequence != NULL);
    pl = (unsigned char *)payload;
    window = (img.cols * img.rows) / (8 * bytes);
    flippables = (struct pos_score *)calloc(window, sizeof(struct pos_score));
    assert(flippables != NULL);
    seq_idx = 0;
    //PROCESS
    for (k = 0; k < bytes; k++) {
        byte = pl[k];
        for(i = 0; i < 8; i++) {
            sort_by_flippability(flippables, window, sequence + seq_idx, img, lut);
            sum = sum_of_blacks(img, sequence + seq_idx, window);
            divided_sum = div(sum, 3);
            if ((divided_sum.quot % 2) == (byte & 0x1)) {
                //change divided_sum.rem pixels from black to white
                status = flip_pixels(img, flippables, window, divided_sum.rem, PBM_BLACK);
                assert(status == 0);
            } else {
                //change 3 - divided_sum.rem pixels from white to black
                status = flip_pixels(img, flippables, window, 3 - divided_sum.rem, PBM_WHITE);
                assert(status == 0);
            }
            byte = byte >> 1;
            seq_idx += window;
        }
    }
    //FREE
    free(lut);
    free(sequence);
    free(flippables);
    fclose(f);
}

/**
*  The symmetrical counterpart of embed, scans the image with the
*  exact same window as embed and counts the black pixels. If they
*  are 3(2k) it assigns to the next bit of the payload 0 and if the
*  sum of blacks is 3(2k + 1) it assignes 1.
*  \param[in] img The struct representing the watermarked image
*  \param[in] bytes The size of the payload which means the caller
*  must provide the exact size of the embedded data.
*  \param[out] payload The extracted data.
*  \returns Nothing.
*/
void extract(struct image img, void *payload, size_t bytes) {
    int window, *sequence, i, j, seq_idx, sum;
    div_t divided_sum;
    unsigned char *pl, byte;
    //INIT
    window = (img.cols * img.rows) / (8 * bytes);
    sequence = random_permutation(img.cols * img.rows);
    assert(sequence != NULL);
    pl = (unsigned char *)payload;
    seq_idx = 0;
    //PROCESS
    for (i = 0; i < bytes; i++) {
        byte = 0;
        for (j = 0; j < 8; j++) {
            sum = sum_of_blacks(img, sequence + seq_idx, window);
            divided_sum = div(sum, 3);
            if (divided_sum.rem == 2)
                divided_sum.quot += 1;
            if (divided_sum.quot % 2 == 1) {
                byte = byte | (PBM_BLACK << j);
            } else {
                byte = byte | (PBM_WHITE << j);
            }
            seq_idx += window;
        }
        pl[i] = byte;
    }
    //FREE
    free(sequence);
}

void sort_by_flippability(struct pos_score *flippables, int window, int *seq,
        struct image img, float *lut) {
    int i;
    for (i = 0; i < window; i++) {
        flippables[i].pos = seq[i];
        flippables[i].score = evaluate(img, seq[i], lut);
    }
    qsort((void *)flippables, window, sizeof(struct pos_score), compar);
}

int sum_of_blacks(struct image img, int *seq, int window) {
    int i, sum = 0;
    div_t q;
    for (i = 0; i < window; i++) {
        q = div(seq[i], img.cols);
        sum += img.bitmap[q.quot][q.rem];
    }
    return sum;
}

int flip_pixels(struct image img, struct pos_score *flippables, int array_size,
        int N_pix, const int color) {
    int i = array_size - 1, pos;
    div_t q;
    while (N_pix != 0 && i >=0) {
        q = div(flippables[i].pos, img.cols);
        if (img.bitmap[q.quot][q.rem] == color) {
            img.bitmap[q.quot][q.rem] = color ^ 1;
            N_pix--;
        }
        i--;
    }
    return N_pix;
}

float evaluate(struct image img, int pos, float *lut) {
    int i, j;
    div_t q;
    int index = 0;
    q = div(pos, img.cols);
    //[q.quot, q.rem] is the [x,y] cordinates of the central pixel of the 3x3 window
    if (q.quot == 0 || q.quot == (img.rows - 1) ||
        q.rem == 0  || q.rem  == (img.cols - 1)) {
        //if the position of the pixel is at the borders of the image
        return (0.250);
    }
    for (i = 1; i >= -1; i--) {
        for (j = 1; j >= -1; j--) {
            index = pushbit(index,
                    img.bitmap[q.quot + i][q.rem + j]);
        }
    }
    return (lut[index]);
}

int compar(const void *l, const void *r) {
    struct pos_score *ll, *rr;
    ll = (struct pos_score *)l;
    rr = (struct pos_score *)r;
    if (ll->score < rr->score) {
        return -1;
    } else if (ll->score > rr->score) {
        return 1;
    } else {
        return 0;
    }
}
