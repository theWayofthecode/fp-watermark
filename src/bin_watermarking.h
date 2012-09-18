#ifndef BIN_WATERMARKING_H
#define BIN_WATERMARKING_H 1

struct image {
    int cols;
    int rows;
    bit **bitmap;
};

void embed(struct image img, void *payload, size_t bytes);
void extract(struct image img, void *payload, size_t bytes);

#endif
