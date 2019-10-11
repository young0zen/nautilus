extern "C" {
#include <nautilus/naut_string.h>
#include <nautilus/thread.h>

  unsigned long strtoul(const char *str, char **endptr, int base) {
    return (unsigned long) strtol(str, endptr, base);
  }

  int gettimeofday(struct timeval *tv, struct timezone *tz) {
    return -1;
  }

  int settimeofday(const struct timeval *tv, const struct timezone *tz) {
    return -1;
  }

  void* stderr;

  void atexit(void) {}

}
