/**
*\file watermark_f.c
*This is the frontend and impelements
*the application logic by dealing with
*the fingerprint reader.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pbm.h>
#include <zlib.h>
#include <libfprint/fprint.h>
#include "bin_watermarking.h"

void watermark(struct fp_dev *dev, char *path);
void authenticate(struct fp_dev *dev, char *path);
struct fp_dscv_dev *discover_device(struct fp_dscv_dev **discovered_devs);
struct fp_print_data *enroll(struct fp_dev *dev);
int verify(struct fp_dev *dev, struct fp_print_data *data);

int main(int argc, char **argv) {
    int opt;
    int r = 1;
    struct fp_dscv_dev *ddev;
    struct fp_dscv_dev **discovered_devs;
    struct fp_dev *dev;
    //INIT
    pbm_init(&argc, argv);
    r = fp_init();
    if (r < 0) {
        fprintf(stderr, "Failed to initialize libfprint\n");
        exit(1);
    }
    fp_set_debug(3);
    discovered_devs = fp_discover_devs();
    if (!discovered_devs) {
        fprintf(stderr, "Could not discover devices\n");
        abort();
    }
    ddev = discover_device(discovered_devs);
    if (!ddev) {
        fprintf(stderr, "No devices detected.\n");
        abort();
    }
    dev = fp_dev_open(ddev);
    fp_dscv_devs_free(discovered_devs);
    if (!dev) {
        fprintf(stderr, "Could not open device.\n");
        abort();
    }
    //PROCESS
    while ((opt = getopt(argc, argv, "w:a:h")) != -1) {
        switch (opt) {
        case 'w':
            watermark(dev, optarg);
            break;
        case 'a':
            authenticate(dev,optarg);
            break;
        default:
            printf("Bad argument\n");
        }
    }
    //FREE
    fp_dev_close(dev);
    return 0;
}

/**
*  Check the source
*/
void watermark(struct fp_dev *dev, char *path) {
    FILE *fr, *fw;
    int status;
    unsigned char *buf;
    struct fp_print_data *data;
    size_t bytes;
    struct image img;
    uLongf d_len, s_len;
    Bytef *dest, *src;

    /*Get the fingerprint*/
    printf("Opened device. It's now time to enroll your finger.\n");
    data = enroll(dev);
    assert(data != NULL);

    /*Compress the fingerprint data*/
    bytes = fp_print_data_get_data(data, &buf);
    s_len = bytes;
    src = (Bytef *)buf;
    d_len = compressBound(s_len);
    dest = (Bytef *)calloc(d_len, sizeof(Bytef));
    status = compress(dest, &d_len, src, s_len);
    assert(status == Z_OK);

    /*Read image to be watermarked*/
    printf("Got it, wait...\n");
    fr = pm_openr(path);
    assert(fr != NULL);
    img.bitmap = pbm_readpbm(fr, &img.cols, &img.rows);
    assert(img.bitmap != NULL);

    /*Embed the fingerprint to the image*/
    embed(img, dest, d_len);
    fw = pm_openw("out.pbm");
    pbm_writepbm(fw, img.bitmap, img.cols, img.rows, FALSE);
    fprintf(fw, "\n#%ld", d_len);

    /*Release the resources*/
    pbm_freearray(img.bitmap, img.rows);
    free(buf);
    free(dest);
    fp_print_data_free(data);
    fclose(fr);
    fclose(fw);
}

/**
 *  Check the source
 */
void authenticate(struct fp_dev *dev, char *path) {
    FILE *fr;
    int status;
    struct image img;
    struct fp_print_data *data;
    uLongf d_len, s_len = 0;
    Bytef *dest, *src;

    /*Read the watermarked image*/
    fr = pm_openr(path);
    assert(fr != NULL);
    img.bitmap = pbm_readpbm(fr, &img.cols, &img.rows);
    assert(img.bitmap != NULL);
    fscanf(fr, "\n#%ld", &s_len);

    /*Extract the fingerpint data*/
    src = (Bytef *)calloc(s_len, sizeof(Bytef));
    assert(src != NULL);
    extract(img, src, s_len);

    /*Uncompress the extracted data*/
    d_len = 2414; //fingerprint data standard size
    dest = (Bytef *)calloc(d_len, sizeof(Bytef));
    assert(dest != NULL);
    status = uncompress(dest, &d_len, src, s_len);
    assert(status == Z_OK);

    /*Authenticate the fingerprint*/
    data = fp_print_data_from_data(dest, d_len);
    verify(dev, data);

    /*Release the resources*/
    pbm_freearray(img.bitmap, img.rows);
    free(dest);
    free(src);
    fp_print_data_free(data);
    fclose(fr);
}

struct fp_dscv_dev *discover_device(struct fp_dscv_dev **discovered_devs)
{
    struct fp_dscv_dev *ddev = discovered_devs[0];
    struct fp_driver *drv;
    if (!ddev)
        return NULL;

    drv = fp_dscv_dev_get_driver(ddev);
    printf("Found device claimed by %s driver\n", fp_driver_get_full_name(drv));
    return ddev;
}

struct fp_print_data *enroll(struct fp_dev *dev) {
    struct fp_print_data *enrolled_print = NULL;
    int r;

    printf("You will need to successfully scan your finger %d times to "
            "complete the process.\n", fp_dev_get_nr_enroll_stages(dev));

    do {
        struct fp_img *img = NULL;

        sleep(1);
        printf("\nScan your finger now.\n");

        r = fp_enroll_finger(dev, &enrolled_print);
        if (r < 0) {
            printf("Enroll failed with error %d\n", r);
            return NULL;
        }
        switch (r) {
            case FP_ENROLL_COMPLETE:
                printf("Enroll complete!\n");
                break;
            case FP_ENROLL_FAIL:
                printf("Enroll failed, something wen't wrong :(\n");
                return NULL;
            case FP_ENROLL_PASS:
                printf("Enroll stage passed. Yay!\n");
                break;
            case FP_ENROLL_RETRY:
                printf("Didn't quite catch that. Please try again.\n");
                break;
            case FP_ENROLL_RETRY_TOO_SHORT:
                printf("Your swipe was too short, please try again.\n");
                break;
            case FP_ENROLL_RETRY_CENTER_FINGER:
                printf("Didn't catch that, please center your finger on the "
                        "sensor and try again.\n");
                break;
            case FP_ENROLL_RETRY_REMOVE_FINGER:
                printf("Scan failed, please remove your finger and then try "
                        "again.\n");
                break;
        }
    } while (r != FP_ENROLL_COMPLETE);

    if (!enrolled_print) {
        fprintf(stderr, "Enroll complete but no print?\n");
        return NULL;
    }

    printf("Enrollment completed!\n\n");
    return enrolled_print;
}

int verify(struct fp_dev *dev, struct fp_print_data *data)
{
    int r;
    do {
        sleep(1);
        printf("\nScan your finger now.\n");
        r = fp_verify_finger(dev, data);
        if (r < 0) {
            printf("verification failed with error %d :(\n", r);
            return r;
        }
        switch (r) {
            case FP_VERIFY_NO_MATCH:
                printf("NO MATCH!\n");
                return 0;
            case FP_VERIFY_MATCH:
                printf("MATCH!\n");
                return 0;
            case FP_VERIFY_RETRY:
                printf("Scan didn't quite work. Please try again.\n");
                break;
            case FP_VERIFY_RETRY_TOO_SHORT:
                printf("Swipe was too short, please try again.\n");
                break;
            case FP_VERIFY_RETRY_CENTER_FINGER:
                printf("Please center your finger on the sensor and try again.\n");
                break;
            case FP_VERIFY_RETRY_REMOVE_FINGER:
                printf("Please remove finger from the sensor and try again.\n");
                break;
        }
    } while (1);
}
