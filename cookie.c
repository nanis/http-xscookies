#include <ctype.h>
#include <memory.h>
#include <stdio.h>
#include "buffer.h"
#include "uri.h"
#include "date.h"
#include "cookie.h"

/*
 * This file is generated automatically with program "encode".
 * We include it because we will do our own URL decoding.
 */
#include "tables.h"

static Buffer* cookie_put_value(Buffer* cookie,
                                const char* name, int nlen,
                                const char* value, int vlen,
                                int boolean, int encode)
{
    Buffer dnam;
    Buffer dval;
    buffer_wrap(&dnam, name , nlen);
    buffer_wrap(&dval, value, vlen);

    /* output each part into the cookie */
    do {
        if (cookie->pos > 0) {
            buffer_append(cookie, "; ", 2);
        }

        if (!encode) {
            buffer_append(cookie, dnam.data, dnam.size);
        } else {
            url_encode(&dnam, dnam.size, cookie);
        }

        if (!boolean) {
            buffer_append(cookie, "=", 1);

            if (!encode) {
                buffer_append(cookie, dval.data, dval.size);
            } else {
                url_encode(&dval, dval.size, cookie);
            }
        }
    } while (0);

    buffer_terminate(cookie);
    return cookie;
}

Buffer* cookie_put_string(Buffer* cookie,
                          const char* name, int nlen,
                          const char* value, int vlen,
                          int encode)
{
    return cookie_put_value(cookie, name, nlen, value, vlen, 0, encode);
}

Buffer* cookie_put_date(Buffer* cookie,
                        const char* name, int nlen,
                        const char* value)
{
    double date = date_compute(value);

    Buffer format;
    buffer_init(&format, 0);
    date_format(date, &format);
    cookie_put_value(cookie, name, nlen, format.data, format.pos, 0, 0);
    buffer_fini(&format);

    return cookie;
}

Buffer* cookie_put_integer(Buffer* cookie,
                           const char* name, int nlen,
                           long value)
{
    char buf[50]; /* FIXED BUFFER OK: to format a long */
    int blen = 0;
    sprintf(buf, "%ld", value);
    blen = strlen(buf);
    return cookie_put_value(cookie, name, nlen, buf, blen, 0, 0);
}

Buffer* cookie_put_boolean(Buffer* cookie,
                           const char* name, int nlen,
                           int value)
{
    if (!value) {
        return cookie;
    }

    char buf[50]; /* FIXED BUFFER OK: to format a boolean */
    int blen = 0;
    strcpy(buf, "1");
    blen = strlen(buf);
    return cookie_put_value(cookie, name, nlen, buf, blen, 1, 0);
}

Buffer* cookie_get_pair(Buffer* cookie,
                        Buffer* name, Buffer* value)
{
    int ncur = name->pos;
    int vcur = value->pos;
    for (int state = URI_STATE_START; state != URI_STATE_END; ) {
        char c = cookie->data[cookie->pos];
        state = uri_state_tbl[(int)c][state];

        switch (state) {
            case URI_STATE_NAME:
                buffer_ensure_unused(name, 3);
                if (c == '%' &&
                    isxdigit(cookie->data[cookie->pos+1]) &&
                    isxdigit(cookie->data[cookie->pos+2])) {
                    /* put a byte together from the next two hex digits */
                    c = MAKE_BYTE(uri_decode_tbl[(int)cookie->data[cookie->pos+1]],
                                  uri_decode_tbl[(int)cookie->data[cookie->pos+2]]);
                    cookie->pos += 2;
                }
                name->data[name->pos++] = c;
                ++cookie->pos;
                break;

            case URI_STATE_VALUE:
                buffer_ensure_unused(value, 3);
                if (c == '%' &&
                    isxdigit(cookie->data[cookie->pos+1]) &&
                    isxdigit(cookie->data[cookie->pos+2])) {
                    /* put a byte together from the next two hex digits */
                    c = MAKE_BYTE(uri_decode_tbl[(int)cookie->data[cookie->pos+1]],
                                  uri_decode_tbl[(int)cookie->data[cookie->pos+2]]);
                    cookie->pos += 2;
                }
                value->data[value->pos++] = c;
                ++cookie->pos;
                break;

            case URI_STATE_END:
                if (c == '=') {
                    name->pos = ncur;
                    value->pos = vcur;
                }
                if (c != '\0') {
                    ++cookie->pos;
                }
                break;

            default:
                ++cookie->pos;
                break;
        }
    }

    buffer_terminate(name);
    buffer_terminate(value);
    return cookie;
}
