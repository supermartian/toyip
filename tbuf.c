#include <stdlib.h>
#include <memory.h>

#include "ip.h"
#include "tbuf.h"

int tbuf_free(struct tbuf *buf)
{
    buf->payload = buf->start;
    if (buf != NULL) {
        if (buf->payload != NULL) {
            free(buf->payload);
        }
        free(buf);
        return TBUF_OK;
    }

    return TBUF_FAIL;
}

struct tbuf *tbuf_malloc(__u16 len)
{
    struct tbuf *buf = NULL;
    buf = malloc(sizeof(struct tbuf));
    if (buf == NULL) {
        return buf;
    }

    buf->payload = malloc(MTU_LEN * sizeof(__u8));
    buf->start = buf->payload;
    memset(buf->payload, 0, len);
    buf->len = len;
    if (buf->payload == NULL) {
        tbuf_free(buf);
        buf = NULL;
        return buf;
    }

    return buf;
}

int tbuf_header(struct tbuf *buf, __u16 offset)
{
    __u8 *p;
    p = buf->start + offset;
    if (p < buf->start ||
            p > (buf->start + buf->len)) {
        return 1;
    }

    buf->payload = p;
    return 0;
}

int tbuf_copy(struct tbuf *dst, struct tbuf *src)
{
    if(dst != NULL) {
        tbuf_free(dst);
    }
    dst = tbuf_malloc(src->len);
    dst->len = src->len;
    dst->payload = dst->start + (src->payload - src->start);
    memcpy(dst->start, src->start, src->len);

    return 0;
}
