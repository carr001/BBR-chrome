#include <iostream>

#include "stddef.h"
#include "src/congestion_control/chrome_windowed_filter.h"
#include "src/common/defs.h"


using MaxBandwidthFilter = WindowedFilter<double,
                                        MaxFilter<double>,
                                        size_t,
                                        size_t>;

int main(int argc, char* argv[]){
    MaxBandwidthFilter* max_bandwidth = new MaxBandwidthFilter(8,0,0);

    double bw_samples[19] = {22, 53, 80, 80, 42, 24,  9, 89, 89, 80,79,79,79,79,79,79,79,9};

    for (int i = 0; i < 19; i++)
    {
        max_bandwidth->Update(bw_samples[i],(size_t)i);
        std::cout
            <<"Best sample so far: "
            <<max_bandwidth->GetBest()
            <<std::endl;
    }
    
    std::cout<<"hello"<<std::endl;
}


