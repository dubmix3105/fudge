#include <memory.h>
#include <string.h>
#include <tar.h>
#include <ramdisk.h>

static struct ramdisk_image image;

static unsigned int parse(struct ramdisk_image *image, void *address)
{

    char *current = address;

    for (; *current; image->count++)
    {

        unsigned int size;

        image->headers[image->count] = (struct tar_header *)current;
        size = string_read_num(image->headers[image->count]->size, 8);

        current += ((size / TAR_BLOCK_SIZE) + ((size % TAR_BLOCK_SIZE) ? 2 : 1)) * TAR_BLOCK_SIZE;

    }

    return image->count;

}

void ramdisk_image_init(struct ramdisk_image *image)
{

    memory_clear(image, sizeof (struct ramdisk_image));

    image->parse = parse;

}

struct ramdisk_image *ramdisk_setup(unsigned int ramdiskc, void **ramdiskv)
{

    unsigned int i;

    ramdisk_image_init(&image);

    for (i = 0; i < ramdiskc; i++)
        image.parse(&image, *(ramdiskv + i));

    return &image;

}

