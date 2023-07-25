/**
 * @file slamTester.cpp
 * @brief Executable file to run and debug SLAM operation offline
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <iostream>
#include <fstream>

#include "slam/slam.hpp"
#include "utils/configReader.hpp"
#include "utils/timer.hpp"

using namespace std;
using namespace cv;

void print_pose(SP<Frame> frame, ostream& posesFile, Slam& slam) {
    auto trans = frame->pose->trans;
    double* deg = frame->deg;
    posesFile<<"FramePose "<<frame->id<<" : "<<trans[0]<<", "<<trans[1]<<", "<<trans[2];
    posesFile<<" | "<<deg[0]<<", "<<deg[1]<<", "<<deg[2]<<endl;
}


void read_orientation(double* orientation, string orientName) {
    fstream orientFile;
    orientFile.open(orientName, ios::in);
    char orientationStr[100];
    char tempStr[100];
    if (orientFile.is_open()) {
        orientFile.getline(orientationStr, 100);
        orientFile.close();
        int j = 0;
        int k = 0;
        for (int i = 0; i < 100 && k < 3; i++) {
            if (orientationStr[i] == ',') {
                tempStr[j] = '\0';
                orientation[k] = atof(tempStr);
                k++;
                j = 0;
            } else if (orientationStr[i] == '\0') {
                tempStr[j] = '\0';
                orientation[k] = atof(tempStr);
                break;
            } else {
                // Append the char to the temp string.
                tempStr[j] = orientationStr[i];
                j++;
            }
        }
    }
}

void debug_trans(Vector3d& t, ostream& file) {
    file<<";TX="<<t[0]<<";TY="<<t[1]<<";TZ="<<t[2];
}

void debug_pose(SP<Pose> pose, ostream& file) {
    auto t = pose->trans;
    double deg[3];
    TransformUtils::quaterniondToDeg(pose->rot, deg);
    debug_trans(t, file);
    file<<";RX="<<deg[1]<<";RY="<<deg[0]<<";RZ="<<deg[2];
}

static bool initialized = false;
static vector<string> pathNames;
static vector<SP<PoseManagerOutput>> outputs;
void debug_frame(SlamConfig& _cfg, 
    ofstream& file, 
    string pathName, 
    SP<PoseManagerOutput> output,
    SP<Frame> originFrame,
    Slam& slam) 
{
    if (!_cfg.debugEstimateValidation) return;
    auto currFrame = output->frame;
     
    //Print Keypoints
    // {
    //     file<<"KPS_START:FID="<<currFrame->id<<endl;
    //     for (auto fp : currFrame->fps) {
    //         file<<"KPS:FID="<<currFrame->id<<";FPID="<<fp->id;
    //         if (!fp->landmark.expired()) {
    //             file<<";LID="<<fp->landmark.lock()->id;
    //         } else {
    //             file<<";LID="<<-1;
    //         }
    //         file<<";X="<<fp->kp.pt.x<<";Y="<<fp->kp.pt.y<<endl;
    //     }
    //     file<<"KPS_END:FID="<<currFrame->id<<endl;
    // }

    //Print Pose
    if (initialized) {
        file<<"FRAME_POSE:POSE_FID="<<currFrame->id;
        debug_pose(currFrame->pose, file);
        file<<";VALID="<<currFrame->valid;
        int matchSize = 0;
        if ((int) output->results.size() && (int)output->results[output->results.size() - 1].size() > 0) 
            matchSize = output->results[output->results.size() - 1][0]->validatorOutput->landmarkResult->size(VALID);
        file<<";MATCH_SIZE="<<matchSize;
        file<<";MFIDS=";
        for (auto f : *output->matchFrames) file<<f->id<<",";
        file<<";PATH=/"<<pathName;
        file<<";WINNER_RID="<<output->winnerRansacIndex;
        file<<";INIT=1";
        file<<endl;
    } else if (slam.initialized || (int)outputs.size() > 10) {
        int i = 0;
        pathNames.push_back(pathName);
        outputs.push_back(output);
        for (auto f : *slam.fm->frameList) {
            file<<"FRAME_POSE:POSE_FID="<<f->id;
            debug_pose(f->pose, file);
            file<<";VALID="<<f->valid;
            int matchSize = 0;
            if ((int) outputs[i]->results.size() && (int)outputs[i]->results[outputs[i]->results.size() - 1].size() > 0) 
                matchSize = outputs[i]->results[outputs[i]->results.size() - 1][0]->validatorOutput->landmarkResult->size(VALID);
            file<<";MATCH_SIZE="<<matchSize;
            file<<";MFIDS=";
            for (auto f : *outputs[i]->matchFrames) file<<f->id<<",";
            file<<";PATH=/"<<pathNames[i];
            file<<";WINNER_RID="<<outputs[i]->winnerRansacIndex;
            file<<";INIT=0";
            file<<endl;
            i++;
        }
        initialized = true;
        pathNames.clear();
        outputs.clear();
    } else {
        pathNames.push_back(pathName);
        outputs.push_back(output);
    }

    //Print Results
    for (auto matchFrame : *output->matchFrames) {
        int stageCnt = 0;
        for (auto& outputVec : output->results) {
            int ransacIter = 0;
            for (auto baHelperOut : outputVec) {
                if (baHelperOut->frameSet->count(matchFrame) > 0) {
                    auto vo = baHelperOut->validatorOutput;

                    file<<"VO:POSE_FID="<<currFrame->id<<";STAGE="<<stageCnt<<";RID="<<ransacIter;
                    file<<";MFID="<<matchFrame->id;
                    file<<";MATCH_SIZE="<<baHelperOut->landmarkSet->size();
                    file<<";AVG_IN_RATIO="<<vo->avgInlierRatio;
                    file<<";VALID_FR_RATIO="<<vo->validFrameRatio;
                    file<<endl;
                    
                    for (auto l : *baHelperOut->landmarkSet) {
                        auto trans = vo->landmarkTransMap->count(l) > 0?(*vo->landmarkTransMap)[l]:l->trans;
                        file<<"LANDMARK:POSE_FID="<<currFrame->id<<";STAGE="<<stageCnt<<";RID="<<ransacIter;
                        file<<";MFID="<<matchFrame->id;
                        file<<";LID="<<l->id;
                        file<<";FIXED="<<baHelperOut->fixedLandmarks->count(l);
                        file<<";RESULT="<<vo->landmarkResult->get(l);
                        debug_trans(trans, file);
                        file<<endl;
                    }
                    
                    // for (auto f : *baHelperOut->frameSet) {
                    {
                        assert(vo->framePoseMap->count(matchFrame) > 0);
                        assert(vo->framePoseMap->count(currFrame) > 0);
                        auto matchPose = (*vo->framePoseMap)[matchFrame];
                        auto currPose = (*vo->framePoseMap)[currFrame];
                        file<<"FRAME:POSE_FID="<<currFrame->id<<";STAGE="<<stageCnt<<";RID="<<ransacIter;
                        file<<";FID="<<matchFrame->id<<";MFID="<<matchFrame->id;
                        file<<";RANK="<<(*baHelperOut->frameRank)[matchFrame];
                        file<<";FIXED="<<baHelperOut->fixedFrames->count(matchFrame);
                        file<<";RESULT="<<vo->frameResult->get(matchFrame);
                        debug_pose(matchPose, file);
                        file<<endl;

                        file<<"FRAME:POSE_FID="<<currFrame->id<<";STAGE="<<stageCnt<<";RID="<<ransacIter;
                        file<<";FID="<<currFrame->id<<";MFID="<<matchFrame->id;
                        file<<";RANK="<<(*baHelperOut->frameRank)[currFrame];
                        file<<";FIXED="<<baHelperOut->fixedFrames->count(currFrame);
                        file<<";RESULT="<<vo->frameResult->get(currFrame);
                        debug_pose(currPose, file);
                        file<<endl;
                    }
                    for (auto& [fp, lFpValid] : *baHelperOut->validatorOutput->fpLandmarkResult) {
                        if (fp->frame != currFrame && fp->frame != matchFrame) continue;
                        for (auto& [l, fpValid] : lFpValid) {
                            if (baHelperOut->landmarkSet->count(l) == 0) {
                                cout<<"Frame "<<currFrame->id<<endl;
                                cout<<"Fp "<<fp->id<<" FP Frame "<<fp->frame->id<<endl;
                                cout<<"Landmark "<<l->id<<endl;
                                cout<<"LandmarkSet ";
                                for (auto l2 : *baHelperOut->landmarkSet) {
                                    for (auto fp2 : l2->fps) {
                                        if (fp2 == fp) {
                                            cout<<l2->id<<", "<<" FP2 "<<fp2->id<<", ";
                                        }
                                    }
                                }
                                cout<<endl;
                                cout<<"Ransac "<<ransacIter<<endl;
                                cout<<"Stage "<<stageCnt<<endl;
                            }
                            assert(baHelperOut->landmarkSet->count(l) > 0);
                            file<<"FP:POSE_FID="<<currFrame->id<<";STAGE="<<stageCnt<<";RID="<<ransacIter;
                            file<<";MFID="<<matchFrame->id;
                            file<<";FPID="<<fp->id<<";FID="<<fp->frame->id<<";LID="<<l->id;
                            file<<";RESULT="<<fpValid->result;
                            file<<";BEHIND="<<fpValid->isBehind<<";CLOSE="<<fpValid->isTooClose<<";FAR="<<fpValid->isTooFar;
                            file<<";PX="<<fpValid->px*slam.cfg.fx+slam.cfg.cx;
                            file<<";PY="<<fpValid->py*slam.cfg.fx+slam.cfg.cy;
                            file<<";X="<<fp->kp.pt.x<<";Y="<<fp->kp.pt.y;
                            file<<endl;
                        }
                    }
                    ransacIter++;
                }
            }
            stageCnt++;
        }
    }

    // Print Replacements
    {
        for (auto& [l1, l2] : *output->replacements) {
            file<<"REPL:POSE_FID="<<currFrame->id;
            file<<";L1="<<l1->id<<";L2="<<l2->id;
            file<<";L1FPS=";
            for (auto fp : l1->fps) file<<fp->frame->id<<"_"<<fp->id<<",";
            file<<";L2FPS=";
            for (auto fp : l2->fps) file<<fp->frame->id<<"_"<<fp->id<<",";
            file<<endl;
        }
    }

    //Print origin
    if (originFrame) {
        file<<"FRAME_ORIGIN:POSE_FID="<<currFrame->id<<";OID="<<originFrame->id;
        debug_pose(originFrame->pose, file);
        file<<endl;
    }

    //Print Pose time
    {
        file<<"PROFILE:POSE_FID="<<currFrame->id;
        for (auto [type, time] : output->profile) {
            switch(type) {
                case OVERALL_TIME: file<<";OVERALL_TIME="; break;
                case KP_TIME: file<<";KP_TIME="; break;
                case FRAME_CREATE_TIME: file<<";FRAME_CREATE_TIME="; break;
                case POSE_TIME: file<<";POSE_TIME="; break;
                case POSE_FRAME_EXTRACTION_TIME: file<<";POSE_FRAME_EXTRACTION_TIME="; break;
                case POSE_MATCH_TIME: file<<";POSE_MATCH_TIME=";break;
                case POSE_RANSAC_INIT_TIME: file<<";POSE_RANSAC_INIT_TIME="; break;
                case POSE_WINNER_TIME: file<<";POSE_WINNER_TIME="; break;
                case POSE_VALID_TIME: file<<";POSE_VALID_TIME="; break;
                case POSE_EST_TIME: file<<";POSE_EST_TIME="; break;
                default:cout<<"Unhandled profile type found "<<type<<endl; assert(false);
            }
            file<<Timer::print(time);
        }
        file<<endl;
    }
}


int main( int argc, char** argv )
{    
    cout << "Arg "<<argv[1]<<endl;
    ConfigReader configReader(argv[1]);
    // vector<string> paths = configReader.read_a("paths");
    string pathStr = configReader.read_s("path");
    string orientStr = configReader.read_s("orient");
    int pathStart = configReader.read_i("pathStart");
    int pathEnd = configReader.read_i("pathEnd");

    SlamConfig _cfg{&configReader};
    Slam slam(_cfg);
    string debugFilename = "debug/logs/debug.txt";
    ofstream debugFile = ofstream(debugFilename);

    int offset = 1;
    int badFrameCount = 0, goodFrameCount = 0;
    auto startTimer = Timer::time();
    Timer timer;
    
    debugFile<<"LIMITS:START="<<pathStart<<";END="<<pathEnd<<";MULTIPLIER="<<1
            <<";OFFSETX="<<0<<";OFFSETY="<<0<<endl;
    for (int pathIdx = pathStart; pathIdx <= pathEnd; pathIdx+=offset) {
        stringstream pathName;
        stringstream orientName;
        pathName << pathStr << pathIdx <<"."<<configReader.read_s("fileExtension");
        orientName << orientStr << pathIdx << ".txt";
        cout<<"##"<<endl<<"##   ============================="<<endl;
        cout << "##"<<pathName.str()<<endl;

        auto imgColor = imread(pathName.str());
        Mat img;
        // auto img = make_shared<Mat>();
        cvtColor(imgColor, img, COLOR_BGR2GRAY);
        // auto img = make_shared<Mat>(imread(pathName.str()));
        if (pathIdx == pathStart) {
            debugFile<<"IMG_DIMS:WIDTH="<<img.cols<<";HEIGHT="<<img.rows<<endl;
        }

        double orientation[3];
        orientation[0] = 0;
        orientation[1] = 0;
        orientation[2] = 0;
        read_orientation(orientation, orientName.str());
        
        timer.start();
        ExportData data;
        slam.extract_keypoints(img, &data);
        auto result = slam.process(orientation, pathIdx, Timer::time(), &data);
        auto currFrame = result->frame;
        cout<<"Overall Landmarks Size "<<slam.lm->get_landmarks()->size()<<" Key Frames size "<<slam.fm->get_keyframes()->size()<<endl;
        
        if (!result->valid) {
            badFrameCount++;
            cout<<currFrame->id<<" Bad Frame... Moving on to next "<<badFrameCount<<endl;
        } else {
            goodFrameCount++;
            timer.stop();
        }

        //Print frames
        if (result->valid) {
            if ((int)slam.fm->frameList->size() == _cfg.mapInitializationFrames) {
                FrameVec frames;
                for (auto frame : *slam.fm->frameList) {
                    frames.push_back(frame);
                }
                reverse(frames.begin(), frames.end());
                for (auto frame : frames) print_pose(frame, cout, slam);
            } else if ((int)slam.fm->frameList->size() > _cfg.mapInitializationFrames) {
                print_pose(slam.fm->get_current(), cout, slam);
            }
        }
        debug_frame(_cfg, debugFile, pathName.str(), result, slam.fm->originFrame, slam);
        cout<<endl<<endl; 
    }
    
    cout<<endl<<endl;
    cout<<"Bad Frame Count "<<badFrameCount<<" Total time "<<Timer::diff(startTimer)<<" Per Frame time "<<timer.print(timer._total/goodFrameCount)<<endl;
    cout << endl<< "+++++++++"<<endl<<"All execution completed successfully" <<endl;
    return 0;
}

