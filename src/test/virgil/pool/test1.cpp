#include <vector>
#include <math.h>
 
#include "ThreadPoolNautilus.hpp" 
#include "work.hpp"

extern "C" 
int virgil_test1(int argc, char *argv[]) {  
   
  /*
   * Fetch the inputs.
   */
  if (argc < 4){ 
    VIRGIL_ERROR("incorrect usage\n");
    return  1;
  }
  auto tasks = atoi(argv[1]);
  auto iters = atoi(argv[2]);
  auto threads = (std::uint32_t) atoi(argv[3]); 
 
  /*
   * Create a thread pool.
   */
    MARC::ThreadPool pool{false, threads};

  /*
   * Submit jobs.
   */
  std::vector<MARC::TaskFuture<double>> results;
  for (auto i=0; i < tasks; i++){
    results.push_back(pool.submit(myF, iters));
  }
  
       VIRGIL_DEBUG("Done with task gen\n");
	     
  while (1) {
  }
    
  return 0;
} 
