/**
*\file flippability.c
*This file containes the code responsible for initializing the flippability
*look up table.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "flippability.h"

/**
*This is an auxiliary data structure
*that aids to represent different characteristics
*regarding the various patterns.
*/
struct smooth {
    int horiz;
    int vert;
    int diag;
    int anti;
};

float compute_score(unsigned char *pattern, int n);
void print_lut(float *lut, int n);
void print_image(unsigned char *image, int n);
struct smooth smoothness(unsigned char *window, int n);
int *connectivity(unsigned char *window, const int n);
void discover_connections(int i, int j,
unsigned char *current, int *touched, const int n);

/**
*This is the main function of this module. It's role is to
*initialize the flippability look up table and save it to the
*file flippalut.data. If the flippalut.data file already exists
*it does nothing.
*\param[in] n An integer where n is the magnitude of the
*patterns (e.g n = 3 implies 3x3 patterns)
*\returns nothing (exept that it creates the file)
*/
void init_flippability_lut(int n) {
    unsigned char *buf;
    float *lut;
    int i, j;
    size_t io_items;
    FILE *f;
    //check if flippability look up table is present
    f = fopen("flippalut.data", "r");
    if ( f != NULL) {
        fclose(f);
        return;
    }
    buf = (unsigned char *)calloc(n * n, sizeof(unsigned char));
    lut = (float *)calloc(1 << (n * n), sizeof(float)); //2 ^ n*n patterns
    assert(buf != NULL && lut != NULL);
    for (i = 0; i < (1 << (n * n)); i++) {
        for (j = 0; j < n * n; j++) {
            buf[j] = (i & (1 << j)) >> j;
        }
        lut[i] = compute_score(buf, n);
    }
    f = fopen("flippalut.data", "w");
    io_items = fwrite(lut, sizeof(float), 1 << (n * n), f);
    assert(io_items == (1 << (n * n)));
    free(lut);
    free(buf);
    fclose(f);
}

float compute_score(unsigned char *pattern, int n) {
    unsigned char *flipped;
    int *con, *con_fl;
    struct smooth sm, sm_fl;
    float score;
    flipped = (unsigned char *)calloc(n * n, sizeof(unsigned char));
    memcpy(flipped, pattern, n * n * sizeof(unsigned char));
    sm = smoothness(pattern, n);
    con = connectivity(pattern, n);
    flipped[(n >> 1) * n + (n >> 1)] ^= 1; //flip the central pixel
    sm_fl = smoothness(flipped, n);
    con_fl= connectivity(flipped, n);
    //trivial cases
    //1
    if ((sm.horiz + sm.vert + sm.diag + sm.anti) == 0 ||
        (sm_fl.horiz + sm_fl.vert + sm_fl.diag + sm_fl.anti) == 0)
        return 0.0;
    //2
    if (sm.horiz == 0 || sm.vert == 0) {
        return 0.0;
    } else {
        score = 0.5;
    }
    //3
    if (sm.diag == 0 || sm.anti == 0) {
        score = score - 0.250;
    } else if (sm.diag < 3 && sm.anti < 3) {
        score = score - 0.125;
    }
    //4
    if (sm.horiz == sm_fl.horiz && sm.vert == sm_fl.vert &&
        sm.diag == sm_fl.diag && sm.anti == sm_fl.anti) {
        score = score + 0.250;
    } else if (sm.horiz < sm_fl.horiz || sm.vert < sm_fl.vert ||
            sm.diag < sm_fl.diag || sm.anti < sm_fl.anti) {
        score = score - 0.125;
    }
    //5
    if (con[0] != con_fl[0] || con[1] != con_fl[1])
        score = score - 0.125;
    return score;
}

void print_lut(float *lut, int n) {
    int i, j, k, l;
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            for (k = 0; k < 3; k++) {
                printf("{");
                for (l = 0; l < 3; l++) {
                    printf("%d, ", ((i * n + j) & (1 << k * 3 + l)) >> k * 3 + l);
                }
                printf("}\n");
            }
            printf("%f\n\n", lut[i * n + j]);
        }
    }
}

void print_image(unsigned char *image, int n) {
    int i, j;
    unsigned char px;
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            px = image[i * n + j] + 1; // {0,255} -> {1,0}
            printf("%d ", px);
        }
        printf("\n");
    }
}

struct smooth smoothness(unsigned char *window, int n) {
    struct smooth sm;
    int k, l;
    sm.horiz = sm.vert = sm.diag = sm.anti = 0;
    for (k = -1; k <= 1; k++) {
        for (l = -1; l <= 0; l++) {
            //horizontal
            if (window[(1 + k) * n + (1 + l)] != \
                window[(1 + k) * n + (1 + l + 1)]) {
                sm.horiz++;
            }
            //vertical
            if (window[(1 + l) * n + (1 + k)] != \
                window[(1 + l + 1) * n + (1 + k)]) {
                sm.vert++;
            }
        }
    }
    for (k = -1; k <= 0; k++) {
        for (l = -1; l <= 0; l++) {
            //diagonal
            if (window[(1 + k) * n + (1 + l)] != \
                window[(1 + k + 1) * n + (1 + l + 1)]) {
                sm.diag++;
            }
        }
    }
    for (k = 0; k <= 1; k++) {
        for (l = -1; l <= 0; l++) {
            //anti-diagonal
            if (window[(1 + k) * n + (1 + l)] != \
                window[(1 + k - 1) * n + (1 + l + 1)]) {
                sm.diag++;
            }
        }
    }
    return sm;
}




int *connectivity(unsigned char *window, const int n) {
    int *touched;
    int i, j, *clusters;
    touched = (int *)calloc(n * n, sizeof(int));
    clusters = (int *)calloc(2, sizeof(int));
    memset(touched, 0, n * n * sizeof(int));
    memset(clusters, 0, 2 * sizeof(int));
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            if (touched[i * n + j] == 0) {
                discover_connections(i, j, window, touched, n);
                clusters[window[i * n + j]]++; //count the white or black clusters in the window
            }
        }
    }
    return clusters;
}

void discover_connections(int i, int j,
unsigned char *window, int *touched, const int n) {
    int nxt;
    touched[i * n + j] = 1;
    //right
    if (j < n - 1) {
        nxt = i * n + j + 1;
        if ((touched[nxt] == 0) &&
            (window[i * n + j] == window[nxt])) { //if not visited yet and is the same color
            discover_connections(i, j + 1, window, touched, n);
        }
    }
    //down
    if (i < n - 1) {
        nxt = (i + 1) * n + j;
        if ((touched[nxt] == 0) &&
            (window[i * n + j] == window[nxt])) { //if not visited yet and is the same color
            discover_connections(i + 1, j, window, touched, n);
        }
    }
    //left
    if (j > 0) {
        nxt = i * n + j - 1;
        if ((touched[nxt] == 0) &&
            (window[i * n + j] == window[nxt])) { //if not visited yet and is the same color
            discover_connections(i, j - 1, window, touched, n);
        }
    }
    //up
    if (i > 0) {
        nxt = (i - 1) * n + j;
        if ((touched[nxt] == 0) &&
            (window[i * n + j] == window[nxt])) { //if not visited yet and is the same color
            discover_connections(i - 1, j, window, touched, n);
        }
    }
}


