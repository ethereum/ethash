/*
 * Copyright (C) 2012 Vincent Hanquez <vincent@snarc.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "compiler.h"
#include <stdint.h>

struct sha3_ctx
{
    uint32_t hashlen; /* in bytes */
    uint32_t bufindex;
    uint64_t state[25];
    uint32_t bufsz;
    uint32_t _padding;
    uint8_t  buf[144]; /* minimum SHA3-224, otherwise buffer need increases */
};

#define SHA3_CTX_SIZE		sizeof(struct sha3_ctx)

void sha3_init(struct sha3_ctx * const restrict ctx, const uint32_t hashlen);
void sha3_update(struct sha3_ctx * const restrict ctx, const uint8_t *data, const uint32_t len);
void sha3_finalize(struct sha3_ctx * const restrict sctx, uint8_t * const restrict out);

static inline void sha3_256(void *const ret, void const *data, const uint32_t size) {
    struct sha3_ctx ctx;
    sha3_init(&ctx, 256);
    sha3_update(&ctx, (uint8_t const *) data, size);
    sha3_finalize(&ctx, (uint8_t *) ret);
}

static inline void sha3_512(void *const ret, void const *data, const uint32_t size) {
    struct sha3_ctx ctx;
    sha3_init(&ctx, 512);
    sha3_update(&ctx, (uint8_t const *) data, size);
    sha3_finalize(&ctx, (uint8_t *) ret);
}

static inline void sha3_512_2(void *const ret, void const *data1, void const *data2, const uint32_t size) {
    struct sha3_ctx ctx;
    sha3_init(&ctx, 512);
    sha3_update(&ctx, (uint8_t const *) data1, size);
    sha3_update(&ctx, (uint8_t const *) data2, size);
    sha3_finalize(&ctx, (uint8_t *) ret);
}


#ifdef __cplusplus
}
#endif