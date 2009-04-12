/*
 *  Arch specifics for POSIX (and equivivalent systems)
 *
 *  Copyright (C) 2008 Andreas Öman
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifdef linux

#define _GNU_SOURCE
#include <sched.h>
#include <pthread.h>
#include <string.h>

static int
get_system_concurrency(void)
{
  cpu_set_t mask;
  int i, r = 0;

  memset(&mask, 0, sizeof(mask));
  sched_getaffinity(0, sizeof(mask), &mask);
  for(i = 0; i < CPU_SETSIZE; i++)
    if(CPU_ISSET(i, &mask))
      r++;
  return r?:1;
}

#elif defined(__APPLE__)

#include <sys/types.h>
#include <sys/sysctl.h>

static int
get_system_concurrency(void)
{
  int mib[2];
  int ncpu;
  size_t len;

  mib[0] = CTL_HW;
  mib[1] = HW_NCPU;
  len = sizeof(ncpu);
  sysctl(mib, 2, &ncpu, &len, NULL, 0);

  return ncpu;

}

#else /* linux */

static int
get_system_concurrency(void)
{
  return 1;
}

#endif /* linux */


#include "arch.h"
#include "showtime.h"
#include <stdio.h>

extern int concurrency;

/**
 *
 */
void
arch_init(void)
{
  concurrency = get_system_concurrency();

  TRACE(TRACE_DEBUG, "core", "Using %d CPU(s)", concurrency);

#ifdef RLIMIT_AS
  do {
    struct rlimit rlim;
    getrlimit(RLIMIT_AS, &rlim);
    rlim.rlim_cur = 512 * 1024 * 1024;
    setrlimit(RLIMIT_AS, &rlim);
  } while(0);
#endif

#ifdef RLIMIT_DATA
  do {
    struct rlimit rlim;
    getrlimit(RLIMIT_DATA, &rlim);
    rlim.rlim_cur = 512 * 1024 * 1024;
    setrlimit(RLIMIT_DATA, &rlim);
  } while(0);
#endif
}


extern int trace_level;

/**
 *
 */
void
tracev(int level, const char *subsys, const char *fmt, va_list ap)
{
  char buf[1024];
  char buf2[64];
  char *s, *p;
  const char *leveltxt, *sgr;
  int l;

  if(level > trace_level)
    return;

  switch(level) {
  case TRACE_ERROR: leveltxt = "ERROR"; sgr = "31"; break;
  case TRACE_INFO:  leveltxt = "INFO";  sgr = "33"; break;
  case TRACE_DEBUG: leveltxt = "DEBUG"; sgr = "32"; break;
  default:          leveltxt = "?????"; sgr = "35"; break;
  }

  vsnprintf(buf, sizeof(buf), fmt, ap);

  p = buf;

  snprintf(buf2, sizeof(buf2), "%s [%s]:", subsys, leveltxt);
  l = strlen(buf2);

  while((s = strsep(&p, "\n")) != NULL) {
    fprintf(stderr, "\033[%sm%s %s\033[0m\n", sgr, buf2, s);
    memset(buf2, ' ', l);
  }
}


/**
 *
 */
void
trace(int level, const char *subsys, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  tracev(level, subsys, fmt, ap);
  va_end(ap);
}
