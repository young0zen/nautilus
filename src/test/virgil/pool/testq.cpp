#include <vector>
#include <math.h>

// Note that this should bring in ThreadSafeNautilusQueue as well
#include "ThreadPoolNautilus.hpp" 
#include "work.hpp"

extern "C"  int virgil_test1(int argc, char *argv[])
{
  VIRGIL_DEBUG("Create queue\n");

  MARC::ThreadSafeNautilusQueue<int> q;

  VIRGIL_DEBUG("Push 100\n");
  for (auto i=0; i < 100; i++){
    q.push(i);
  }

  VIRGIL_DEBUG("Queue now has %d elements\n",q.size());

  for (auto i=0; i < 100; i++) {
    int x;
    if (q.waitPop(x)) {
      if (x!=i) {
	VIRGIL_ERROR("Pop expected %d but got %d\n",i,x);
      } else {
	// good
      }
    } else {
      VIRGIL_ERROR("Failed to pop round %d\n",i);
    }
  }

  VIRGIL_DEBUG("Popped 100 elements\n");
  
  VIRGIL_DEBUG("Done with queue test\n");
	     
    
  return 0;
} 
