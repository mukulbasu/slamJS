/**
 * @file types.hpp
 * @brief Contains all the key class types that are used for SLAM.
 * The key types are:
 * 1. Frame : Holds the data extracted from Image frames. Primarily contains a list of framePoints.
 * 2. FramePoint (FramePointTemplate): This represents a keypoint that has been identified in an image. 
 *      Primarily holds the u,v/x,y pixel coordinates of the keypoint in the image.
 * 3. Landmark (LandmarkTemplate): This holds a set of related or matched framepoints belonging to different frames. 
 *      Ideally it should not contain 2 framepoints related to the same frame.
 * 
 * The other key types are the result classes for:
 * 1. Estimate Validator: Contains the validation results of the landmarks, frames and framepoints used for BA
 * 2. BaHelper: Contains the overall BA output
 * 3. PoseManager: Contains the overall Pose output
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __TYPES_HPP__
#define __TYPES_HPP__

// for opencv 
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <set>
#include <map>

#include "slamConfig.hpp"

using namespace std;
using namespace cv;
using namespace Eigen;

#define SP shared_ptr
#define WP weak_ptr

//Core Types
template <typename T> class LandmarkTemplate {
    public:
        int id;
        set<SP<T>> fps;
        Vector3d trans{0, 0, 0};
        int baIterCount = 0;
        bool valid = false;
};
class Frame;
template <typename T> class FramePointTemplate;
using FramePoint = FramePointTemplate<Frame>;
using Landmark = LandmarkTemplate<FramePoint>;
using LandmarkSet = set<SP<Landmark>>;
using LandmarkVec = vector<SP<Landmark>>;
using LandmarkPair = pair<SP<Landmark>, SP<Landmark>>;
using LandmarkPairVec = vector<LandmarkPair>;

#define INITIAL_DISTANCE 99999
#define NLEVELS 6 //8
using DESC = vector<Mat>;
template <typename T> class FramePointTemplate {
    public:
        const int id;
        KeyPoint kp;
        float x;
        float y;
        Mat desc;
        SP<T> frame;
        WP<Landmark> landmark;
        double matchDistance = INITIAL_DISTANCE;
        // bool valid = false;

        FramePointTemplate(int idArg, KeyPoint& kpArg, Mat& descArg, SP<Frame> frameArg, float cx, float cy, float focal):
            id(idArg), kp(kpArg), desc(descArg), frame(frameArg) {
            x = (kpArg.pt.x - cx)/focal;
            y = (kpArg.pt.y - cy)/focal;
            // cout<<"KP "<<kp.pt.x<<" "<<kp.pt.y<<endl;
        }
};

using FramePointSet = set<SP<FramePoint>>;
using FramePointVec = vector<SP<FramePoint>>;

class Pose {
    public:
        Vector3d trans{0,0,0};
        Quaterniond rot;

        void init(const Vector3d& transArg, const Quaterniond& rotArg) {
            trans[0] = transArg[0];
            trans[1] = transArg[1];
            trans[2] = transArg[2];
            rot = Quaterniond(rotArg);
        }

        Pose() {}

        Pose(const Vector3d& transArg, const Quaterniond& rotArg) {
            init(transArg, rotArg);
        }

        Pose(const Pose& pose) {
            init(pose.trans, pose.rot);
        }

        Pose(const SP<Pose> pose) {
            init(pose->trans, pose->rot);
        }
};

class MatchNode;
using MatchNodeVec = vector<SP<MatchNode>>;

class MatchNode {
    public:
        SP<FramePoint> fp;
        MatchNodeVec children;

        MatchNode(SP<FramePoint> fpArg) : fp(fpArg) {}
};

using MatchNodeDist = pair<SP<MatchNode>, double>;
using MatchPriorityQueue = vector<MatchNodeDist>;

class MatchNodeInt {
    public:
        int index;
        vector<SP<MatchNodeInt>> children;

        MatchNodeInt(int indexArg) : index(indexArg) {}
};

class Frame {
    public:
        const int id;
        int64_t timestamp;
        FramePointSet fps;
        SP<Pose> pose;
        double deg[3];
        int baIterCount = 0;
        int level = 999;
        double landmarkDistThreshold = 0;
        bool valid = false;
        SP<Mat> descs;
        vector<MatchNodeVec> matchTree;
        bool isCurrFrame = false;
        bool isKeyFrame = false;

        // SP<Mat> bowDesc;
        // bool fixed = false;

        // Frame(int idArg, SP<Pose> poseArg, const double degArg[3]): 
        //         id(idArg), pose(make_shared<Pose>(poseArg)) {
        //     for (int i = 0; i <3; i++) deg[i] = degArg[i];
        // }
        
        Frame(int idArg, const Vector3d& transArg, const Quaterniond& rotArg, const double degArg[3], int64_t timestampArg): 
                id(idArg), timestamp(timestampArg), pose(make_shared<Pose>(transArg, rotArg)) {
            for (int i = 0; i <3; i++) deg[i] = degArg[i];
        }
};
using FrameSet = set<SP<Frame>>;
using FrameVec = vector<SP<Frame>>;
using FrameRank = map<SP<Frame>, int>;

using FramePoseMap = map<SP<Frame>, SP<Pose>>;
using LandmarkTransMap = map<SP<Landmark>, Vector3d>;


using FramePointLandmarkPair = pair<SP<FramePoint>, SP<Landmark>>;
using FramePointLandmarkPairSet = set<FramePointLandmarkPair>;
using FrameFpLandmarks = map<SP<Frame>, SP<FramePointLandmarkPairSet>>;
using FrameLandmarksMap = map<SP<Frame>, SP<LandmarkSet>>;
using FrameLandmarksPair = pair<SP<Frame>, SP<LandmarkSet>>;
using FrameLandmarksPairVec = vector<FrameLandmarksPair>;
using LandmarkFramesMap = map<SP<Landmark>, SP<FrameSet>>;
using LandmarkFramesPair = pair<SP<Landmark>, SP<FrameSet>>;
using LandmarkFramesPairVec = vector<LandmarkFramesPair>;
using LandmarkDistancePair = pair<SP<Landmark>, double>;
using LandmarkDistancePairVec = vector<LandmarkDistancePair>;
using FrameIntVec = vector<pair<SP<Frame>, int>>;

class FrameSort {
    public:
        SP<Frame> frame;
        double degDiff;
        double distRatio;
        double dist;

        FrameSort(SP<Frame> frameArg, double degDiffArg, 
                double distRatioArg, double distArg) :
                frame(frameArg), degDiff(degDiffArg), 
                distRatio(distRatioArg), dist(distArg) {}
};
using FrameSortVec = vector<SP<FrameSort>>;

#define LOOP_FP_THRU_LANDMARK(FP, LANDMARK, LANDMARK_SET) for (auto LANDMARK : *LANDMARK_SET) for (auto FP : LANDMARK->point2Ds)

enum ValidateResultType {
    UNSET = -1,
    INVALID = 0, 
    VALID = 1,
    FIXED = 2
};

class FpValidResult {
    public:
        ValidateResultType  result = ValidateResultType::UNSET;
        bool                isTooFar = false;
        bool                isTooClose = false;
        bool                isBehind = false;
        bool                isWithinRange = true;
        Vector3d            behindTrans;
        double              px;
        double              py;
};

template <typename T> class ValidResult {
    protected:
        map<T, ValidateResultType> results;
        map<ValidateResultType, SP<set<T>>> sets;

    public:
        void put(T t, ValidateResultType result) {
            if (sets.count(result) == 0) sets[result] = make_shared<set<T>>();
            if (results.count(t) > 0) {
                auto prevResult = results[t];
                sets[prevResult]->erase(t);
            }
            results[t] = result;
            sets[result]->insert(t);
        }

        ValidateResultType get(T t) {
            assert(results.count(t) > 0);
            return results[t];
        }

        bool exists(T t) {return results.count(t) > 0;}

        void replace(T orig, T repl) {
            if (results.count(orig) == 0) return;
            auto result = results[orig];
            sets[result]->erase(orig);
            sets[result]->insert(repl);
            results[repl] = result;
            results.erase(orig);
        }

        SP<set<T>> get(ValidateResultType result) {
            if (sets.count(result) == 0) sets[result] = make_shared<set<T>>();
            return sets[result];
        }

        int size(ValidateResultType result) { 
            return sets.count(result) > 0? sets[result]->size(): 0;
        }
        int size() { return (int)results.size();}
};

using FpLandmarkResult = map<SP<FramePoint>, map<SP<Landmark>, SP<FpValidResult>>>;
using LandmarkResult = ValidResult<SP<Landmark>>;
using FrameResult = ValidResult<SP<Frame>>;

class ValidatorOutput {
    public:
        SP<LandmarkTransMap> landmarkTransMap;
        SP<FramePoseMap> framePoseMap;
        SP<LandmarkResult> landmarkResult;
        SP<FrameResult> frameResult;
        SP<FpLandmarkResult> fpLandmarkResult;
        float avgInlierRatio;
        float validFrameRatio;
        bool valid = false;

        ValidatorOutput(
            SP<LandmarkTransMap> landmarkTransMapArg, 
            SP<FramePoseMap> framePoseMapArg,
            SP<LandmarkResult> landmarkResultArg, 
            SP<FrameResult> frameResultArg, 
            SP<FpLandmarkResult> fpLandmarkResultArg,
            float avgInlierRatioArg, 
            float validFrameRatioArg, 
            bool validArg) :
            landmarkTransMap(landmarkTransMapArg), 
            framePoseMap(framePoseMapArg), 
            landmarkResult(landmarkResultArg),
            frameResult(frameResultArg), 
            fpLandmarkResult(fpLandmarkResultArg), 
            avgInlierRatio(avgInlierRatioArg),
            validFrameRatio(validFrameRatioArg), 
            valid(validArg) 
        {}
};

class BaHelperOutput {
    public:
        SP<LandmarkSet> landmarkSet;
        SP<FrameSet> frameSet;
        SP<LandmarkSet> fixedLandmarks;
        SP<FrameSet> fixedFrames;
        SP<FrameRank> frameRank;
        int maxRank;
        SP<ValidatorOutput> validatorOutput;
        
        BaHelperOutput() {}

        BaHelperOutput(
            SP<LandmarkSet> landmarkSetArg, 
            SP<FrameSet> frameSetArg,
            SP<LandmarkSet> fixedLandmarksArg, 
            SP<FrameSet> fixedFramesArg, 
            SP<FrameRank> frameRankArg,
            int maxRankArg,
            SP<ValidatorOutput> validatorOutputArg) :
            //Explicitly making a copy of the elements here. Otherwise it messes with the
            // landmark replace logic. Changing in one place was changing in multiple 
            // outputs, since they were sharing the same landmarkSet.
            landmarkSet(make_shared<LandmarkSet>()), 
            frameSet(frameSetArg),
            fixedLandmarks(make_shared<LandmarkSet>()), 
            fixedFrames(fixedFramesArg),
            frameRank(frameRankArg),
            maxRank(maxRankArg),
            validatorOutput(validatorOutputArg) 
        {
            landmarkSet->insert(landmarkSetArg->begin(), landmarkSetArg->end());
            fixedLandmarks->insert(fixedLandmarksArg->begin(), fixedLandmarksArg->end());
        }
};

enum ProfileType {
    OVERALL_TIME = 0,
    KP_TIME = 1, 
    FRAME_CREATE_TIME = 2,
    POSE_TIME = 3,
    POSE_FRAME_EXTRACTION_TIME = 4,
    POSE_MATCH_TIME = 5,
    POSE_RANSAC_INIT_TIME = 6,
    POSE_WINNER_TIME = 7,
    POSE_VALID_TIME = 8,
    POSE_EST_TIME = 9
};

enum PoseManagerStatusType {
    NOT_ENOUGH_LANDMARKS_FOR_VALID = -7,
    MATCH_INVALID = -6,
    NOT_ENOUGH_MATCH_FRAMES = -5,
    DID_NOT_MATCH_ALL_FRAMES = -4,
    NOT_ENOUGH_LANDMARKS = -3,
    ALREADY_INITIALIZED = -2,
    NOT_INITIALIZED = -1,
    DEFAULT = 0,
    VALID_MATCH = 1
};

class PoseManagerOutput {
    public:
        bool valid = false;
        PoseManagerStatusType status = DEFAULT;
        SP<Frame> frame;
        SP<FrameSet> matchFrames = make_shared<FrameSet>();
        vector<vector<SP<BaHelperOutput>>> results;
        int winnerRansacIndex;
        map<ProfileType, int64_t> profile;
        SP<LandmarkPairVec> replacements = make_shared<LandmarkPairVec>();
};

#define MAX_KPS 1500
#define MAX_TREES 5
struct ExportData {
    float imgWidth = 0;
    float imgHeight = 0;
    float kpSize = 0;
    float treeSize = 0;
    float x[MAX_KPS];
    float y[MAX_KPS];
    float desc[MAX_KPS][8];
    float trees[MAX_TREES][MAX_KPS*2];
} ;

#define LOG_START "LOG_START"
#define LOG_END "LOG_END"


#if WASM_COMPILE
#define DEBUG_COUT(X)
#define LOG(X) cout<<X
#else
#define DEBUG_COUT(X) cout<<X
#define LOG(X) cout<<X
#endif
#endif /* __TYPES_HPP__*/