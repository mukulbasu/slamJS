/**
 * @file timer.hpp
 * @brief Helper class to profile various operations
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __TIMER_HPP__
#define __TIMER_HPP__

#include <iostream>
#include <sstream>
#include <chrono>
#include <sys/time.h>

using namespace std;

class Timer {
    public:
        int64_t _total = 0;
        int64_t _last = 0;

        static string print(int64_t diff) {
            stringstream str;
            if (diff > 5000) {
                str<<diff/1000<<" ms";
            } else {
                str<<diff<<" us";
            }
            return str.str();
        }
        
        static int64_t time() {
            return chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            // chrono::high_resolution_clock::now();
        }

        static string diff(int64_t startTime) {
            auto stopTime = time();
            return print(stopTime-startTime);
        }

        void start() { _last = time();}
        void stop() { _total += (time() - _last);}
        int64_t total() { return (_total); }
};

#endif /* __TIMER_HPP__ */