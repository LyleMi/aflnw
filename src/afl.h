#ifndef _AFL_H_
#define _AFL_H_

#include "types.h"

// u32 __afl_map_size = MAP_SIZE;

static void __afl_map_shm(void);
static void __afl_start_forkserver(void);
static u32 __afl_next_testcase(u8 *buf, u32 max_len);
static void __afl_end_testcase(void);

int count_non_zero_bytes(u8 *bytes);

#endif
