#pragma once

#include <math.h>

//extern "C" int nk_vc_printf(...);

double myF (std::int64_t iters){
  nk_vc_printf("In myF\n");
  double v = static_cast<double>(iters);

  for (auto i=0; i < iters; i++){
    for (auto i=0; i < iters; i++){
      for (auto i=0; i < iters; i++){
        v = sqrt(v);
      }
    }
  }

  nk_vc_printf("Leaving myF\n");
  return v;
}
