#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits>

#include "src/scene/scene_config.h"
#include "src/spheretrace.h"
#include "src/benchmark.h"

#include "auxiliary/json.hpp"
#include "auxiliary/load_scene.h"

// folder that contains the test scenes
#define TEST_SCENES "test_scenes/"

typedef std::numeric_limits<double> dbl;
using namespace std;

// Add a name here that describes the optimization applied
#define OPT_NAME "current"

// put in names of OPTIMIZED C functions
#define OPT_RENDER render

/**
 * If this is defined, generate output images (for debugging purposes)
 */
// #define GENERATE_OUTPUT_IMAGE

unsigned long long runtime_cycles;

void export_buf(string filename, uint32_t width, uint32_t height, Vec *buffer) {
    std::ofstream ofs;
    ofs.open(filename);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (uint32_t i = 0; i < width * height; ++i) {
        unsigned char r = static_cast<unsigned char>(std::min(1.0f, buffer[i].x) * 255);
        unsigned char g = static_cast<unsigned char>(std::min(1.0f, buffer[i].y) * 255);
        unsigned char b = static_cast<unsigned char>(std::min(1.0f, buffer[i].z) * 255);
        ofs << r << g << b;
    }
    ofs.close();
}

/* Generates a filename for the benchmark of the form benchmark_Y-M-D_HM */
string get_filename(){
    time_t t = time(0);   // get time now
    struct tm * now = localtime(&t);
    char date_and_time[20];
    string filename = (char *) "benchmark_";

    strftime (date_and_time, 20, "%H:%M:%S-", now);
    filename += date_and_time;

    char suff[5];
    srand(time(0));
    sprintf(suff, "%04x", rand() % 65536);
    filename += suff;
    return "./benchmarks/" + filename + ".csv";
}

render_mode measure_runtime(Vec *buffer, const uint32_t width, const uint32_t height, scene_config *scene, int num_runs, render_mode rendermode) {


    runtime_cycles = 0;
    render_mode chosen_mode = Automatic;

    // Measure optimized implementation
    runtime_cycles = measure_whole(OPT_RENDER, buffer, width, height, scene, num_runs, rendermode, &chosen_mode);
    
    return chosen_mode;
}

unsigned long measure_flops(string &scene_name, uint32_t width, uint32_t height, render_mode chosen_mode){
#if __APPLE__ || __MACH__
    return 0;   // perf does not run on mac
#endif

	string render_mode_str;
	
	if(chosen_mode == ObjectVectorization){
	    render_mode_str = "obj";
	}else if(chosen_mode == RayVectorization){
	    render_mode_str = "ray";
	}else if(chosen_mode == MixedVectorization){
	  render_mode_str = "mixed";
   	}else{
	    render_mode_str = "auto";
	}
	
    string cmd = "python helpers/count_flops.py " + scene_name + " " + to_string(width) + " " + to_string(height) + " " + render_mode_str;

    char output_buffer[64];

    FILE *stream = popen(cmd.c_str(), "r");

    if(stream){
        if(!feof(stream)){
            fgets(output_buffer, 64, stream);
            string flops_as_str = string(output_buffer);
            return stoul(flops_as_str);
        }else{
            cerr << "aborting, flop count didn't return" << endl;
            exit(1);
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    /**
     * Renders and measures the runtime of defined scenes
     */

    int c;
    vector<string> scenes;

    int Nflag = 0;
    render_mode rendermode = Automatic;
    vector<int> N;

    int num_runs = 1;

    // parse commmand line arguments
    while ((c = getopt(argc, argv, "s:N:r:m:")) != -1) {
        char *token;
        switch (c) {
            case 's':
                // split optarg by ","
                token = strtok(optarg, ",");
                while(token != NULL) {
                    scenes.push_back(token);
                    token = strtok(NULL, ",");
                }
                break;
            case 'r':
                num_runs = stoi(optarg);
                break;
            case 'N':
                Nflag = 1;
                // split optarg by ","
                token = strtok(optarg, ",");
                while(token != NULL) {
                    N.push_back(stoi(token));
                    token = strtok(NULL, ",");
                }
                break;
            case 'm':
                rendermode = get_render_mode();
                break;
            default:
                cout << "wrong usage" << endl;
                exit(1);
        }
    }

    printf("Starting measurements. \n");
    printf("\t num_runs = %d\n", num_runs);

    // ratio: 4:3
    vector<int> widths;
    if(Nflag == 0) {
        widths.push_back(4*10);
        widths.push_back(4*32);
        widths.push_back(4*64);
    } else {
        for(int n : N) {
            widths.push_back(4 * n);
        }
    }

    vector <string> configs = get_scene_configs(TEST_SCENES);

    string filename = get_filename();
    ofstream output_file;
    output_file.open (filename);

    output_file << "scene_id,scene_name,width,height,measurement_mode,render_mode,num_runs,measure_all,flop_count\n";

    print_scene_info(scenes, rendermode, 1);

    int scene_index = 0;
    for (string &s : configs) {
        scene_config conf = load_config(TEST_SCENES, s);
        // Get test name
        int lastindex = s.find_last_of(".");
        string scene_name = s.substr(0, lastindex);
        if (!should_run(scene_name, scenes)) continue;
        cout << std::setw (20) << scene_name << ": ";
        fflush(stdout);

        for(int width : widths) {
            uint32_t height = 3 *width/4;

            Vec *impl_result = new Vec[width * height];
            render_mode chosen_mode = measure_runtime(impl_result, width, height, &conf, num_runs, rendermode);
            string chosen_mode_str = "undefined";
            if (chosen_mode == ObjectVectorization) {
                chosen_mode_str = "object";
            } else if (chosen_mode == RayVectorization) {
                chosen_mode_str = "ray";
            } else if (chosen_mode == MixedVectorization) {
                chosen_mode_str = "mixed";
            }

            unsigned long flop_count = measure_flops(s, width, height, chosen_mode);

            output_file.precision(dbl::max_digits10);
            output_file
                    << scene_index << ","
                    << scene_name << ","
                    << width << ","
                    << height << ","
                    << OPT_NAME << ","
                    << chosen_mode_str << ","
                    << num_runs << ","
                    << runtime_cycles << ","
                    << flop_count << "\n";

            delete[] impl_result;

#ifdef GENERATE_OUTPUT_IMAGE
            render(impl_result, width, height, &conf);

            export_buf("out/" + scene_name + "_" + to_string(width) + "x" + to_string(height) + ".ppm", width, height, impl_result);
#endif
            cout << " ";
        }
        cout << endl;
        free_scene(conf);
        scene_index++;
    }
    output_file << "\n";
    output_file.close();
}
