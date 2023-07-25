/**
 * @file slamWasm.cpp
 * @brief Wrapper class for SLAM to enable webassembly
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "emscripten.h"
#include "slam/slam.hpp"
#include "utils/configReader.hpp"

using namespace std;
using namespace cv;

extern "C" {
    EMSCRIPTEN_KEEPALIVE
    ConfigReader* config_reader() {
        return new ConfigReader();
    }

    EMSCRIPTEN_KEEPALIVE
    void config_set(ConfigReader* cfgReader, const char* key, const char* val) {
        cfgReader->set(key, val);
    }

    EMSCRIPTEN_KEEPALIVE
    Slam* initialize(ConfigReader* cfgReader) {
        SlamConfig _cfg = SlamConfig(cfgReader);
        return new Slam(_cfg);
    }

    EMSCRIPTEN_KEEPALIVE 
    unsigned* create_image_buffer(int width, int height) {
        return new unsigned[width*height*2];
        // return (unsigned*)malloc(width * height * 4 * sizeof(uint8_t));
    }

    EMSCRIPTEN_KEEPALIVE 
    float* create_data_buffer() {
        return (float*)new ExportData;
    }

    EMSCRIPTEN_KEEPALIVE 
    int get_data_buffer_size() {
        return sizeof(ExportData)/sizeof(float);
    }

    EMSCRIPTEN_KEEPALIVE 
    int extract_keypoints(Slam* slam, const unsigned* imgData, int height, int width, float* data) {
        auto colorMat = Mat(height, width, CV_8UC4, (unsigned*)imgData);
        // SP<Mat> greyMat = make_shared<Mat>();
        Mat greyMat;
        cvtColor(colorMat, greyMat, COLOR_RGBA2GRAY);
        slam->extract_keypoints(greyMat, (ExportData*)data);
        ExportData* dataptr = (ExportData*)data;
        std::cout<<"Data WASM "<<dataptr->imgWidth<<", "<<dataptr->imgHeight<<", "<<dataptr->kpSize<<endl;
        return 0;
    }

    EMSCRIPTEN_KEEPALIVE 
    int process_keypoints(Slam* slam, float* data, float x, float y, float z, int64_t timestamp) {
        double orientation[3];
        orientation[0] = x;
        orientation[1] = y;
        orientation[2] = z;
        ExportData* dataptr = (ExportData*)data;
        auto result = slam->process(orientation, -1, timestamp, (ExportData*)data);
        // cout<<"Overall Landmarks Size "<<slam->lm->get_landmarks()->size()<<endl;
        if (result && result->valid) {
            auto currFrame = result->frame;
            auto trans = currFrame->pose->trans;
            cout<<"Frame "<<currFrame->id<<" : "<<trans[0]<<", "<<trans[1]<<", "<<trans[2]<<endl;
            cout<<"Time Profile "<<currFrame->id<<" : ";
            for (auto [type, time] : result->profile) {
                switch(type) {
                    case OVERALL_TIME: cout<<";OVERALL_TIME="; break;
                    case KP_TIME: cout<<";KP_TIME="; break;
                    case FRAME_CREATE_TIME: cout<<";FRAME_CREATE_TIME="; break;
                    case POSE_TIME: cout<<";POSE_TIME="; break;
                    case POSE_FRAME_EXTRACTION_TIME: cout<<";POSE_FRAME_EXTRACTION_TIME="; break;
                    case POSE_MATCH_TIME: cout<<";POSE_MATCH_TIME=";break;
                    case POSE_RANSAC_INIT_TIME: cout<<";POSE_RANSAC_INIT_TIME="; break;
                    case POSE_WINNER_TIME: cout<<";POSE_WINNER_TIME="; break;
                    case POSE_VALID_TIME: cout<<";POSE_VALID_TIME="; break;
                    case POSE_EST_TIME: cout<<";POSE_EST_TIME="; break;
                    default:cout<<"Unhandled profile type found "<<type<<endl; assert(false);
                }
                cout<<Timer::print(time)<<", ";
            }
            cout<<endl;
            return result->status;
        } else {
            cout<<"Bad Frame... Moving on to next"<<endl;
            return result->status;
        }
    }

    EMSCRIPTEN_KEEPALIVE 
    int process_image(Slam* slam, const unsigned* imgData, int height, int width, float* data, float x, float y, float z, int64_t timestamp) {
        extract_keypoints(slam, imgData, height, width, data);
        return process_keypoints(slam, data, x, y, z, timestamp);
    }

    EMSCRIPTEN_KEEPALIVE 
    int is_initialized(Slam* slam) {
        return slam->initialized;
    }

    EMSCRIPTEN_KEEPALIVE
    double get_smooth_trans(Slam* slam, int index) {
        // cout<<"Returning Trans "<<slam->fm->current->id<<" index "<<index<<" value "<<slam->fm->current->estimate->trans[index]<<endl;
        return slam->fm->get_curr_trans_smoothed()[index];
    }

    EMSCRIPTEN_KEEPALIVE
    double get_smooth_vel(Slam* slam, int index) {
        // cout<<"Returning Trans "<<slam->fm->current->id<<" index "<<index<<" value "<<slam->fm->current->estimate->trans[index]<<endl;
        return slam->fm->get_curr_vel_smoothed()[index];
    }

    EMSCRIPTEN_KEEPALIVE
    double get_trans(Slam* slam, int index) {
        // cout<<"Returning Trans "<<slam->fm->current->id<<" index "<<index<<" value "<<slam->fm->current->estimate->trans[index]<<endl;
        return slam->fm->get_current()->pose->trans[index];
    }

    EMSCRIPTEN_KEEPALIVE
    double get_keyframe_trans(Slam* slam, int index, int keyframeIndex) {
        // cout<<"Returning Trans "<<slam->fm->current->id<<" index "<<index<<" value "<<slam->fm->current->estimate->trans[index]<<endl;
        if (slam->fm->get_keyframes()->size() > keyframeIndex) {
            auto keyframes = slam->fm->get_keyframes();
            vector<SP<Frame>> frameVec(keyframes->begin(), keyframes->end());
            std::sort(frameVec.begin(), frameVec.end(), [](const auto& a, const auto& b) {return a->id < b->id;});
            return frameVec[keyframeIndex]->pose->trans[index];
        } else {
            return -9999;
        }
    }

    EMSCRIPTEN_KEEPALIVE
    double get_keyframe_rot(Slam* slam, int index, int keyframeIndex) {
        // cout<<"Returning Trans "<<slam->fm->current->id<<" index "<<index<<" value "<<slam->fm->current->estimate->trans[index]<<endl;
        if (slam->fm->get_keyframes()->size() > keyframeIndex) {
            auto keyframes = slam->fm->get_keyframes();
            vector<SP<Frame>> frameVec(keyframes->begin(), keyframes->end());
            std::sort(frameVec.begin(), frameVec.end(), [](const auto& a, const auto& b) {return a->id < b->id;});
            auto rot = frameVec[keyframeIndex]->pose->rot.toRotationMatrix().eulerAngles(1, 0, 2);
            if (index == 0) return rot[1];
            else if (index == 1) return rot[0];
            else return rot[2];
        } else {
            return -9999;
        }
    }

    EMSCRIPTEN_KEEPALIVE
    int get_keyframe_count(Slam* slam) {
        return (int)slam->fm->get_keyframes()->size();
    }

    EMSCRIPTEN_KEEPALIVE
    char* allocateMemory(int size) {
        return (char*)malloc(size);
    }

    EMSCRIPTEN_KEEPALIVE
    void freeMemory(char *globalString) {
        if (globalString != NULL) {
            free(globalString);
            globalString = NULL;
        }
    }

    EMSCRIPTEN_KEEPALIVE
    void clear(Slam* slam) {
        delete(slam);
    }
}
