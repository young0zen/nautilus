#include <nautilus/nautilus.h>
#include <nautilus/thread.h>

void *nk_virgil_get_cur_thread()
{
  return get_cur_thread();
}

