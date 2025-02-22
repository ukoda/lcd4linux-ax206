#define ERREXIT(cinfo,code)  \
  ((cinfo)->err->msg_code = (code), \
   (*(cinfo)->err->error_exit) ((j_common_ptr) (cinfo)))
#define ERREXIT1(cinfo,code,p1)  \
  ((cinfo)->err->msg_code = (code), \
   (cinfo)->err->msg_parm.i[0] = (p1), \
   (*(cinfo)->err->error_exit) ((j_common_ptr) (cinfo)))

#define JERR_OUT_OF_MEMORY "Insufficient memory (case %d)"
#define JERR_BUFFER_SIZE "Buffer passed to JPEG library is too small"

#define OUTPUT_BUF_SIZE  4096   /* choose an efficiently fwrite'able size */


/* Expanded data destination object for memory output */

typedef struct {
    struct jpeg_destination_mgr pub;    /* public fields */

    unsigned char **outbuffer;  /* target buffer */
    unsigned long *outsize;
    unsigned char *newbuffer;   /* newly allocated buffer */
    JOCTET *buffer;             /* start of buffer */
    size_t bufsize;
} my_mem_destination_mgr;

typedef my_mem_destination_mgr *my_mem_dest_ptr;


METHODDEF(void) init_mem_destination(j_compress_ptr cinfo)
{
    /* no work necessary here */
}

METHODDEF(boolean)
    empty_mem_output_buffer(j_compress_ptr cinfo)
{
    size_t nextsize;
    JOCTET *nextbuffer;
    my_mem_dest_ptr dest = (my_mem_dest_ptr) cinfo->dest;

    /* Try to allocate new buffer with double size */
    nextsize = dest->bufsize * 2;
    nextbuffer = (JOCTET *) malloc(nextsize);

    if (nextbuffer == NULL)
        ERREXIT1(cinfo, JERR_OUT_OF_MEMORY, 10);

    memcpy(nextbuffer, dest->buffer, dest->bufsize);

    if (dest->newbuffer != NULL)
        free(dest->newbuffer);

    dest->newbuffer = nextbuffer;

    dest->pub.next_output_byte = nextbuffer + dest->bufsize;
    dest->pub.free_in_buffer = dest->bufsize;

    dest->buffer = nextbuffer;
    dest->bufsize = nextsize;

    return TRUE;
}

METHODDEF(void) term_mem_destination(j_compress_ptr cinfo)
{
    my_mem_dest_ptr dest = (my_mem_dest_ptr) cinfo->dest;

    *dest->outbuffer = dest->buffer;
    *dest->outsize = dest->bufsize - dest->pub.free_in_buffer;
}

GLOBAL(void) jpeg_mem_dest(j_compress_ptr cinfo, unsigned char **outbuffer, unsigned long *outsize)
{
    my_mem_dest_ptr dest;

    if (outbuffer == NULL || outsize == NULL)   /* sanity check */
        ERREXIT(cinfo, JERR_BUFFER_SIZE);

    /* The destination object is made permanent so that multiple JPEG images
     * can be written to the same buffer without re-executing jpeg_mem_dest.
     */
    if (cinfo->dest == NULL) {  /* first time for this JPEG object? */
        cinfo->dest = (struct jpeg_destination_mgr *)
            (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(my_mem_destination_mgr));
    }

    dest = (my_mem_dest_ptr) cinfo->dest;
    dest->pub.init_destination = init_mem_destination;
    dest->pub.empty_output_buffer = empty_mem_output_buffer;
    dest->pub.term_destination = term_mem_destination;
    dest->outbuffer = outbuffer;
    dest->outsize = outsize;
    dest->newbuffer = NULL;

    if (*outbuffer == NULL || *outsize == 0) {
        /* Allocate initial buffer */
        dest->newbuffer = *outbuffer = (unsigned char *) malloc(OUTPUT_BUF_SIZE);
        if (dest->newbuffer == NULL)
            ERREXIT1(cinfo, JERR_OUT_OF_MEMORY, 10);
        *outsize = OUTPUT_BUF_SIZE;
    }

    dest->pub.next_output_byte = dest->buffer = *outbuffer;
    dest->pub.free_in_buffer = dest->bufsize = *outsize;
}
