#include <vector>
#include <math.h>
 
#include "ThreadPoolNautilus.hpp" 
#include "work.hpp"

/*
extern "C" int __pthread_key_create(pthread_key_t *key, void (*destructor)(void*)) {
  VIRGIL_DEBUG("key create\n");
  return 0;
}

extern "C" int pthread_once(pthread_once_t *whatever, void (*whatever2)(void)) {
  VIRGIL_DEBUG("pthread once\n");
  return 0;
}
*/

int simone_main (int x){

  VIRGIL_DEBUG("In simone_main\n");

    std::function<int (int)> myF = [](int v)->int{
      return v+1;
    };

  VIRGIL_DEBUG("In simone_main - done with constructor\n");
  
    auto r = myF(x);

    VIRGIL_DEBUG("Leaving simone_main r=%d\n",r);
    

    return r;
}


//#include <iostream>
//#include <cmath>
//#include <math.h>
#include <thread>
#include <future>
#include <functional>

int simone_main2 (int argc, char *argv[]){
  std::packaged_task<int(int)> task([] (int v) ->int {
      return v+1;
    });

  std::future<int> result = task.get_future();

  task(argc);

  auto r = result.get();

  return r;
}

extern "C" 
int virgil_test1(int argc, char *argv[])
{
  VIRGIL_DEBUG("In virgil_test\n");
  
 //  int rc= simone_main2(argc,argv);

 // VIRGIL_DEBUG("returned from simone_main - rc=%d\n",rc);

  //  return rc;
  
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
