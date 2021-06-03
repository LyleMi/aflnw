#include "config.h"
#include "types.h"
#include "utils.h"
#include "afl.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Fork server logic. */

static void __afl_map_shm(void) {

  char *id_str = getenv(SHM_ENV_VAR);
  char *ptr;

}

static void __afl_start_forkserver(void) {

  u8  tmp[4] = {0, 0, 0, 0};
  u32 status = 0;

  /*
  if (__afl_map_size <= FS_OPT_MAX_MAPSIZE) {
    status |= (FS_OPT_SET_MAPSIZE(__afl_map_size) | FS_OPT_MAPSIZE);
  }
  */
  if (status) status |= (FS_OPT_ENABLED);
  memcpy(tmp, &status, 4);

  /* Phone home and tell the parent that we're OK. */

  if (write(FORKSRV_FD + 1, tmp, 4) != 4) return;

}

static u32 __afl_next_testcase(u8 *buf, u32 max_len) {

  s32 status, res = 0xffffff;

  /* Wait for parent by reading from the pipe. Abort if read fails. */
  if (read(FORKSRV_FD, &status, 4) != 4) return 0;

  /* we have a testcase - read it */
  status = read(0, buf, max_len);

  /* report that we are starting the target */
  if (write(FORKSRV_FD + 1, &res, 4) != 4) return 0;

  if (status < 1)
    return 0;
  else
    return status;

}

static void __afl_end_testcase(void) {

  int status = 0xffffff;

  if (write(FORKSRV_FD + 1, &status, 4) != 4) exit(1);

}

int count_non_zero_bytes(u8* bytes)
{
    int count = 0;
    for (int i = 0; i < MAP_SIZE; ++i)
    {
        if (*bytes != 0) {
            count++;
        }
        bytes++;
    }
    return count;
}
