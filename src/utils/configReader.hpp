/**
 * @file configReader.hpp
 * @brief Used by slamTester to read JS config
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __CONFIG_READER_HPP__
#define __CONFIG_READER_HPP__

// for std
#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <memory>
#include <sstream>

using namespace std;

class ConfigReader {
    public:
        const char* _configFile;

        map<string, vector<string>> _keyVal;
#if !WASM_COMPILE
        void read_file(const string& cmdstr) {
            const char* cmd = (const char*)(cmdstr.c_str());
            std::array<char, 4096> buffer;
            int return_code = -1;
            string dataEnd = "DATA_END";
            bool isNextKey = true;
            string key = "NO_KEY";
            vector<string> val = vector<string>();
            auto pclose_wrapper = [&return_code](FILE* cmd){ return_code = pclose(cmd); };
            { // scope is important, have to make sure the ptr goes out of scope first
                const unique_ptr<FILE, decltype(pclose_wrapper)> pipe(popen(cmd, "r"), pclose_wrapper);
                if (pipe) {
                    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                        string data = buffer.data();
                        data.pop_back();
                        if (dataEnd.compare(data) == 0) {
                            _keyVal.insert(pair<string, vector<string>>(key, val));
                            val = vector<string>();
                            isNextKey = true;
                        } else if (isNextKey) {
                            key = string(data);
                            isNextKey = false;
                        } else {
                            val.push_back(string(data));
                        }
                    }
                }
            }
        }

        ConfigReader(const char* configFile) : _configFile(configFile) {
            stringstream cmd;
            cmd<<"node src/utils/configReader.js "<<_configFile;
            read_file(cmd.str());
        }
#endif
        ConfigReader() {}
        void set(const char* keyArg, const char* valArg) {
            string key = keyArg;
            string val = valArg;
            auto vals = vector<string>();
            vals.push_back(val);
            _keyVal[key] = vals;
        }

        vector<string>& read_a(const char* key) {
            // for (auto& val : _keyVal[key]) cout<<val<<endl;
            // cout<<endl;
            return _keyVal[key];
        }

        string& read_s(const char* key) {
            return read_a(key)[0];
        }

        string& read_string(const char* key) {
            return read_a(key)[0];
        }

        int read_i(const char* key) {
            return stoi(read_a(key)[0]);
        }

        int read_int(const char* key) {
            return stoi(read_a(key)[0]);
        }

        float read_f(const char* key) {
            return stof(read_a(key)[0]);
        }

        float read_float(const char* key) {
            return stof(read_a(key)[0]);
        }

        double read_d(const char* key) {
            return stod(read_a(key)[0]);
        }

        double read_double(const char* key) {
            return stod(read_a(key)[0]);
        }

        bool read_b(const char* key) {
            vector<string> trues {"TRUE", "True", "true", "T", "t"};
            bool result = false;
            auto val = read_a(key)[0];
            // cout<<"vl:"<<val<<endl;

            for (auto& str : trues) if (0 == str.compare(val)) result = true;
            return result;
        }

        bool read_bool(const char* key) {
            vector<string> trues {"TRUE", "True", "true", "T", "t"};
            bool result = false;
            auto val = read_a(key)[0];
            // cout<<"vl:"<<val<<endl;

            for (auto& str : trues) if (0 == str.compare(val)) result = true;
            return result;
        }

};


template <typename T> class StringToVector {
    public:
        vector<T> v;
        StringToVector(const string& str) {
            istringstream it(str);
            string word;
            while(it) {
                T n;
                it >> n;
                if (it) v.push_back(n);
            }
        }
};

#endif /* __CONFIG_READER_HPP__ */