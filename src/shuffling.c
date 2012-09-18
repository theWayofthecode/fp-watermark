/**
*\file shuffling.c
*This module is responsible for generating a random sequence
*of integers.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "shuffling.h"

/**
*Yet another auxiliary structure
*used for the sequence generation.
*/
struct seq_node {
    int i;
    struct seq_node *next;
};

struct seq_node *add_node(struct seq_node *hd,  int value);
void destroy_sequence(struct seq_node *hd);

/**
*Random permutation without replacement: Floyd
*This function implements Floyd's algorithm P for N values
*ranging [1, N]. It produces a sequence with (pseudo)randomly
*shuffled values.
*\param[in] pix_N An integer indicating the total number of pixels.
*\returns An array of random integers in the range [0, pix_N - 1]
*/

int *random_permutation(int pix_N) {
    int J, i;
    int seedp = 7; //magic seed. It is randomly choosen to be lucky number 7
    int T, *final;
    struct seq_node *new, *hd;
    struct seq_node **picked;
    new = hd = NULL;
    //create sentinel node
    hd = (struct seq_node *)malloc(sizeof(struct seq_node));
    picked = (struct seq_node **)calloc(pix_N + 1, sizeof(struct seq_node *)); //picked[0] is not used
    assert(hd != NULL && picked != NULL);
    for (i = 0; i < pix_N; i++) {
        picked[i] = NULL;
    }
    hd->next = NULL;
    for (J = 1; J <= pix_N; J++) {
        T = rand_r(&seedp) % J + 1;
        if (picked[T] == NULL) {
            picked[T] = add_node(hd, T);
        } else {
            assert(picked[J] == NULL);
            picked[J] = add_node(picked[T], J);
        }
    }
    final = (int *)calloc(pix_N, sizeof(int));
    for (i = 0, new = hd->next; i < pix_N; i++, new = new->next)
        final[i] = new->i - 1; /* we have to substract every value by 1
                                  because the algorithm calculates random
                                  permutations of integers in the interval
                                  [1, pix_N]. We need [0, pix_N - 1]. */
    destroy_sequence(hd);
    free(picked);
    return final;
}

struct seq_node *add_node(struct seq_node *hd,  int value) {
    struct seq_node *new;
    new = (struct seq_node *)malloc(sizeof(struct seq_node));
    assert(new != NULL);
    new->i = value;
    new->next = hd->next;
    hd->next = new;
    return new;
}

void destroy_sequence(struct seq_node *hd) {
    struct seq_node *previous, *current;
    previous = hd;
    current = previous->next;
    free(previous);
    while (current != NULL) {
        previous = current;
        current = current->next;
        free(previous);
    }
}
