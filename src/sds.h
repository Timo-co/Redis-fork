/**
 *  SDS Lib -- Dynamic String library
 *
 */

#ifndef __SDS_H
#define __SDS_H

#include <stdint.h>

typedef char *sds;

struct __attribute__((__packed__)) sdshdr8
{
    uint8_t len;
    uint8_t alloc;
    unsigned char flags; // type of sds
    char buf[];
};

#define SDS_TYPE_8 1
#define SDS_TYPE_MASK 7
#define SDS_VAR(T, s) struct sdshdr##T *sh = (void *)(s - sizeof(struct sdshdr##T))
#define SDS_HDR(T, s) ((struct sdshdr##T *)(s - sizeof(struct sdshdr##T)))

static inline size_t sdslen(const sds s)
{
    unsigned char flags = s[-1]; // remove the pointer backward
    switch (flags & SDS_TYPE_MASK)
    {
    case SDS_TYPE_8:
        return SDS_HDR(8, s)->len;
    }

    return 0;
}

static inline size_t sdsavail(const sds s)
{
    unsigned char flags = s[-1];
    switch (flags & SDS_TYPE_MASK)
    {
    case SDS_TYPE_8:
        SDS_VAR(8, s);
        return sh->alloc - sh->len;
    }
    return 0;
}

static inline void sdssetlen(sds s, size_t newlen)
{
    unsigned char flags = s[-1];
    switch (flags & SDS_TYPE_MASK)
    {
    case SDS_TYPE_8:
        SDS_HDR(8, s)->len = newlen;
        break;
    }
}

// sds increase len
static inline void sdsinclen(sds s, size_t inc)
{
    unsigned char flags = s[-1];
    switch (flags & SDS_TYPE_MASK)
    {
    case SDS_TYPE_8:
        SDS_HDR(8, s)->len += inc;
        break;
    }
}

static inline size_t sdsalloc(const sds s)
{
    unsigned char flags = s[-1];
    switch (flags & SDS_TYPE_MASK)
    {
    case SDS_TYPE_8:
        return SDS_HDR(8, s)->alloc;
    }
}

static inline void sdssetalloc(sds s, size_t newlen)
{
    unsigned char flags = s[-1];
    switch (flags & SDS_TYPE_MASK)
    {
    case SDS_TYPE_8:
        SDS_HDR(8, s)->alloc = newlen;
        break;
    }
}
#endif