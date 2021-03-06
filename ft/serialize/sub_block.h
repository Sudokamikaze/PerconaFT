/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:
#ident "$Id$"
/*======
This file is part of PerconaFT.


Copyright (c) 2006, 2015, Percona and/or its affiliates. All rights reserved.

    PerconaFT is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License, version 2,
    as published by the Free Software Foundation.

    PerconaFT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PerconaFT.  If not, see <http://www.gnu.org/licenses/>.

----------------------------------------

    PerconaFT is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License, version 3,
    as published by the Free Software Foundation.

    PerconaFT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with PerconaFT.  If not, see <http://www.gnu.org/licenses/>.
======= */

#ident "Copyright (c) 2006, 2015, Percona and/or its affiliates. All rights reserved."

#pragma once

#include "ft/serialize/compress.h"

// TODO: Clean this abstraciton up
static const int max_sub_blocks = 8;
static const int target_sub_block_size = 512 * 1024;
static const int max_basement_nodes = 32;
static const int max_basement_node_uncompressed_size = 256 * 1024;
static const int max_basement_node_compressed_size = 64 * 1024;

struct sub_block {
    void *uncompressed_ptr;
    uint32_t uncompressed_size;

    void *compressed_ptr;
    uint32_t compressed_size;         // real compressed size
    uint32_t compressed_size_bound;   // estimated compressed size

    uint32_t xsum;                    // sub block checksum
};
typedef struct sub_block *SUB_BLOCK;

struct stored_sub_block {
    uint32_t uncompressed_size;
    uint32_t compressed_size;
    uint32_t xsum;
};

void sub_block_init(SUB_BLOCK);
SUB_BLOCK sub_block_creat(void);

// get the size of the compression header
size_t 
sub_block_header_size(int n_sub_blocks);

void
set_compressed_size_bound(struct sub_block *se, enum toku_compression_method method);

// get the sum of the sub block compressed bound sizes 
size_t 
get_sum_compressed_size_bound(int n_sub_blocks, struct sub_block sub_block[], enum toku_compression_method method);

// get the sum of the sub block uncompressed sizes 
size_t 
get_sum_uncompressed_size(int n_sub_blocks, struct sub_block sub_block[]);

// Choose n_sub_blocks and sub_block_size such that the product is >= total_size and the sub_block_size is at
// least >= the target_sub_block_size.
int
choose_sub_block_size(int total_size, int n_sub_blocks_limit, int *sub_block_size_ret, int *n_sub_blocks_ret);

int
choose_basement_node_size(int total_size, int *sub_block_size_ret, int *n_sub_blocks_ret);

void
set_all_sub_block_sizes(int total_size, int sub_block_size, int n_sub_blocks, struct sub_block sub_block[]);

// find the index of the first sub block that contains the offset
// Returns the index if found, else returns -1
int
get_sub_block_index(int n_sub_blocks, struct sub_block sub_block[], size_t offset);

#include "workset.h"

struct compress_work {
    struct work base;
    enum toku_compression_method method;
    struct sub_block *sub_block;
};

void
compress_work_init(struct compress_work *w, enum toku_compression_method method, struct sub_block *sub_block);

uint32_t
compress_nocrc_sub_block(
    struct sub_block *sub_block,
    void* sb_compressed_ptr,
    uint32_t cs_bound,
    enum toku_compression_method method
    );

void
compress_sub_block(struct sub_block *sub_block, enum toku_compression_method method);

void *
compress_worker(void *arg);

size_t
compress_all_sub_blocks(int n_sub_blocks, struct sub_block sub_block[], char *uncompressed_ptr, char *compressed_ptr, int num_cores, struct toku_thread_pool *pool, enum toku_compression_method method);

struct decompress_work {
    struct work base;
    void *compress_ptr;
    void *uncompress_ptr;
    uint32_t compress_size;
    uint32_t uncompress_size;
    uint32_t xsum;
    int error;
};

// initialize the decompression work
void 
decompress_work_init(struct decompress_work *dw,
                     void *compress_ptr, uint32_t compress_size,
                     void *uncompress_ptr, uint32_t uncompress_size,
                     uint32_t xsum);

// decompress one block
int
decompress_sub_block(void *compress_ptr, uint32_t compress_size, void *uncompress_ptr, uint32_t uncompress_size, uint32_t expected_xsum);

// decompress blocks until there is no more work to do
void *
decompress_worker(void *arg);

// decompress all sub blocks from the compressed_data buffer to the uncompressed_data buffer
// Returns 0 if success, otherwise an error
int
decompress_all_sub_blocks(int n_sub_blocks, struct sub_block sub_block[], unsigned char *compressed_data, unsigned char *uncompressed_data, int num_cores, struct toku_thread_pool *pool);

extern int verbose_decompress_sub_block;
