/*
 * Benchmarking Infrastructure used to measure and compare the runtimes of the following functions of the referenced
 * and the optimized implementation:
 *  - Overall runtime in cycles (just hand a)
 */

#include "geometry.h"
#include "spheretrace.h"
#include "benchmark.h"
#include "tsc_x86.h"
#include <stdio.h>

unsigned long long measure_whole(render_mode (*render_func)(Vec *buffer,
                                         const uint32_t width,
                                         const uint32_t height,
                                         scene_config *scene,
                                         render_mode rendermode),
                     Vec *buffer,
                     const uint32_t width,
                     const uint32_t height,
                     scene_config *scene,
                     int input_num_runs,
                     render_mode rendermode,
                     render_mode *chosen_mode)
{    
    myInt64 cycles;
    myInt64 start;
    int i, num_runs;
    num_runs = input_num_runs;


/*
 *   Adapted from benchmarking infrastructure of Homework 1
 */
#ifdef CALIBRATE
    while(num_runs < (1 << 14)) {
        start = start_tsc();
        for (i = 0; i < num_runs; ++i) {
            *chosen_mode = render_func(buffer, width, height, scene, rendermode);
        }
        cycles = stop_tsc(start);

        if(cycles >= CYCLES_REQUIRED) break;

        num_runs *= 2;
    }
#endif

    start = start_tsc();
    for (i = 0; i < num_runs; ++i) {
        *chosen_mode = render_func(buffer, width, height, scene, rendermode);
    }

    cycles = stop_tsc(start)/num_runs;
    return cycles;
}