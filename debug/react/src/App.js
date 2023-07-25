/**
 * @file App.js.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
import React, { useState, useEffect, useRef } from 'react';
import Draggable, {DraggableCore} from 'react-draggable';
import { Rnd } from "react-rnd";
import Img from './Img.js';
import MatchLines from './MatchLines.js';
import Details from './Details.js';
import { fetchData, genImgViewData, genDetailsViewData, genMatchViewData, extractLandmarkDetails, extractFrameDetails, genThreeDData } from './Utils.js';
import Log from './Log.js';
import PoseView from './PoseView.js';

const AppStyle = {
  textAlign: 'center',
  display: 'flex',
  flexDirection: 'row',
  width: '99vw',
  height: '99vh'
}

const ContainerStyle = {
  display: 'flex',
  flexDirection: 'column',
  width: '100%',
}

const RowStyle = {
  display: 'flex',
  flexDirection: 'row',
}

const ColStyle = {
  display: 'flex',
  flexDirection: 'column',
}

const ImgViewerStyle = {
  display: 'flex',
  flexDirection: 'row',
  border: '2px solid grey',
  height: '1050px',
  margin: '2px'
} 

const ActionPanelStyle = {
  display: 'flex',
  flexDirection: 'row',
  marginTop: "2px",
  width: '100%',
  height: '100%',
  justifyContent: 'space-between',
  alignItems: 'flex-start',
}

const PoseInputStyle = {
  marginLeft: '5px',
  borderRadius: '9px',
  height: '50px',
  width: '100px',
  fontSize: '20px',
  textAlign: 'center', 
}

const ButtonStyle = {
  margin: '3px',
  marginTop: '20px',
  borderRadius: '9px',
  height: '50px',
  width: '150px',
  fontSize: '18px'
}

const RnDStyle = {
  display: "flex",
  alignItems: "center",
  justifyContent: "center",
  border: "solid 2px grey",
  background: "#f0f0f0",
  position: 'relative'
};

const PoseViewerStyle = {
  flexGrow: 1, 
  border: '2px solid grey', 
  overflow: 'hidden',
  position: 'relative',
  margin: '2px',
}

const DetailsStyle = {
  border: '2px solid grey',
  // width: 'fit-content',
  height: 'fit-content',
  zIndex: '100',
  width: '450px',
  position: 'absolute', 
  top: '0px', 
  left: '0px',
}

const InstructionsStyle = {
  position: 'absolute', 
  top: '0px', 
  right: '0px',
  width: '450px',
  border: '2px solid black',
}

const getMatchFrames = (stageMap, poseFid, stageId, rId) => {
  let matchFrames = new Set();
  for (const fId in stageMap) {
    if (fId == poseFid) continue;
    for (const stageIdArg in stageMap[fId]) {
      for (const rIdArg in stageMap[fId][stageIdArg]) {
        if (Number(stageId) === Number(stageIdArg) && Number(rIdArg) === Number(rId)) {
          matchFrames.add(fId);
        }
      }
    }
  }
  return Array.from(matchFrames);
}


function App() {
  const [debugFile, setDebugFile] = useState("debug");
  const [reload, setReload] = useState(true);
  const [limits, setLimits] = useState();
  const [framePoses, setFramePoses] = useState([]);
  const [poseFid, setPoseFid] = useState(null);
  const [frameData, setFrameData] = useState({});
  const [matchFid, setMatchFid] = useState(null);
  const [stageId, setStageId] = useState("0");
  const [rId, setRId] = useState("0");
  const [landmarkIndex, setLandmarkIndex] = useState(0);

  const [matchViewData, setMatchViewData] = useState({landmarks: {}});
  const [detailsViewData, setDetailsViewData] = useState();
  const [poseImageData, setPoseImageData] = useState(null);
  const [matchImageData, setMatchImageData] = useState(null);
  const [threeDData, setThreeDData] = useState([{trans: [0,0,0], rots: {x: 0, y: 0, z:0}}]);

  const [viewerDims, setViewerDims] = useState({width: 480, height: 640});
  const [poseViewDims, setPoseViewDims] = useState({width: 100, height: 100});
  const imageViewerRef = useRef(null);
  const poseViewerRef = useRef(null);

  useEffect(() => {
    if (!reload) return;
    {
      console.log("Reloading");
      const call = async() => {
        const limitsData = await fetchData(debugFile, "LIMITS:");
        const framePosesData = await fetchData(debugFile, "FRAME_POSE:");
        console.log("FramePosesData", framePosesData);
        framePosesData.sort((a, b) => Number(a.POSE_FID) - Number(b.POSE_FID));
        setLimits(limitsData[0]);
        setFramePoses(framePosesData);
        setPoseFid(framePosesData[0].POSE_FID);
        setStageId("0");
        setRId("0");
        setLandmarkIndex(0);
        setReload(false);
      };
      call();
    }
  }, [reload, debugFile]);

  useEffect(() => {
    if (poseFid == null) return;
    console.log("Pose fid", poseFid);
    const call = async() => {
      const landmarks = await fetchData(debugFile, "LANDMARK:POSE_FID="+poseFid+";");
      console.log("Landmarks", landmarks);
      const frames = await fetchData(debugFile, "FRAME:POSE_FID="+poseFid+";");
      console.log("Frames", frames);
      const fps = await fetchData(debugFile, "FP:POSE_FID="+poseFid+";");
      console.log("Fps", fps);
      const vos = await fetchData(debugFile, "VO:POSE_FID="+poseFid+";");
      console.log("Vos", vos);
      const repls = await fetchData(debugFile, "REPL:POSE_FID="+poseFid+";");
      console.log("Repl", repls);
      const profiles = await fetchData(debugFile, "PROFILE:POSE_FID="+poseFid+";");
      console.log("Profile", profiles);
      const profile = (profiles && profiles.length)? profiles[0] : null;

      const stageMap = {};
      let maxStage = 0;
      vos.forEach(vo => {
        const fid = vo.MFID;
        if (!(fid in stageMap)) stageMap[fid] = {};
        if (!(vo.STAGE in stageMap[fid])) stageMap[fid][vo.STAGE] = {};
        stageMap[fid][vo.STAGE][vo.RID] = {avgInRatio : vo.AVG_IN_RATIO, validFrRatio: vo.VALID_FR_RATIO}
        if (Number(vo.STAGE) > maxStage) maxStage = vo.STAGE;
      });

      const matchImageData = frames.find(frame => frame.FID !== poseFid);
      const matchFidVal = matchImageData?matchImageData.FID: null;

      const refFramePose = framePoses.find(fp => fp.POSE_FID === poseFid);
      const matchFrames = refFramePose.MFIDS.split(",");
      const mapInit = refFramePose.INIT === "1";

      if (matchFidVal) {
        setMatchFid(matchFidVal);
        setStageId(Math.max(...Object.keys(stageMap[matchFidVal]).map(sId => Number(sId))).toString());
      } else {
        setMatchFid(null);
        setStageId(null);
      }
      setRId(0);
      setLandmarkIndex(0);
      setFrameData({landmarks, frames, fps, vos, repls, profile, refFramePose, matchFrames, mapInit, stageMap});
      
    };
    call();
  }, [poseFid]);

  const compare = (a, b, isGreater) => {
    if (isGreater) return Number(a) > Number(b);
    else return Number(a) < Number(b);
  }

  useEffect(() => {
    if (poseFid == null) return;
    console.log(`Computing genData for ${stageId} ${rId}`);

    const {landmarks, frames, fps, 
        vos, repls, profile, refFramePose, 
        matchFrames, mapInit, stageMap} = frameData;

    const {relevantLandmarks, selectedLandmark, relevantFps, poseFp, matchFp} = 
        extractLandmarkDetails({landmarks, fps, poseFid, matchFid, stageId, rId, landmarkIndex});
    
    console.log("poseFid", poseFid);
    console.log("matchFid", matchFid);
    console.log("selectedLandmark", selectedLandmark);
    console.log("Relevant Landmarks", relevantLandmarks);
    console.log("relevantFps", relevantFps);
    console.log("poseFp", poseFp);
      
    const {matchFrame, poseFrame} = extractFrameDetails({frames, poseFid, matchFid, stageId, rId});
    const matchFramePose = framePoses.find(fp => fp.POSE_FID === matchFid);

    if (relevantLandmarks.length > 0) {
      setMatchViewData(genMatchViewData({
        poseFid, matchFid, limits, 
        relevantLandmarks, selectedLandmark, relevantFps}));
    } else {
      setMatchViewData(null);
    }
    setPoseImageData(genImgViewData({
      titlePrepend: "Ref", 
      framePose: refFramePose, 
      frame: poseFrame, 
      landmark: selectedLandmark, 
      fp: poseFp
    }));
    setMatchImageData(genImgViewData({
      titlePrepend: "Match", 
      framePose: matchFramePose, 
      frame: matchFrame, 
      landmark: selectedLandmark, 
      fp: matchFp
    }));
    setDetailsViewData(genDetailsViewData({
      poseFid, matchFid, stageId, rId, 
      refFramePose, vos, matchFrames, stageMap, 
      selectedLandmark, repls, poseFp, matchFp, profile
    }));
    setThreeDData(genThreeDData({framePoses, refFramePose, poseFid, poseFrame}));
  }, [frameData, matchFid, stageId, rId, landmarkIndex]);

  const findNext = (current, list, incr) => {
    let next = null;
    list.forEach(ele => {
      if (compare(current, ele, !incr))
        if (!next || compare(next, ele, incr))
          next = ele;
    });
    return next;
  }

  const poseFidInput = (event) => {
    if (framePoses.some(framePose => framePose.POSE_FID === event.target.value)) {
      setPoseFid(event.target.value);
      document.getElementById("poseFidInput").style.color = "#000000";
    } else {
      document.getElementById("poseFidInput").style.color = "#ff0000";
    }
  }

  const poseClick = (incr) => {
    let newPoseFid = findNext(poseFid, 
        framePoses.map(framePose => framePose.POSE_FID), incr);
    if (newPoseFid) {
      console.log("New Pose FID ", newPoseFid);
      setPoseFid(newPoseFid);
    } else {
      alert("No other Pose Frame exists");
    }
  }

  const matchFrameClick = (incr) => {
    let matchFrames = getMatchFrames(frameData.stageMap, poseFid, stageId, rId);
    let newMatchFid = findNext(matchFid, matchFrames, incr);
    if (null != newMatchFid) {
      console.log("New Match FID ", newMatchFid);
      setMatchFid(newMatchFid);
      setLandmarkIndex(0);
    } else {
      alert("No other Match Frame exists");
    }
  }

  const stageClick = (incr) => {
    let newStageId = findNext(stageId, Object.keys(frameData.stageMap[matchFid]), incr);
    if (null != newStageId) {
      console.log("New Stage ID ", newStageId);
      setRId("0");
      setStageId(newStageId);
      setLandmarkIndex(0);
    } else {
      alert("No other Stage ID exists");
    }
  }

  const rIdClick = (incr) => {
    let newRId = findNext(rId, Object.keys(frameData.stageMap[matchFid][stageId]), incr);
    if (null != newRId) {
      console.log("New RID ", newRId);
      setRId(newRId);
      setLandmarkIndex(0);
    } else {
      alert("No other RID exists");
    }
  }

  const landmarkClick = (incr) => {
    const maxLandmarks = Object.keys(matchViewData.landmarks).length;
    let newLandmarkIndex = landmarkIndex + (incr? 1 : -1);
    if (newLandmarkIndex < 0) newLandmarkIndex = maxLandmarks + newLandmarkIndex;
    newLandmarkIndex = newLandmarkIndex % maxLandmarks;
    setLandmarkIndex(newLandmarkIndex);
  }

  
  useEffect(() => {
    const keyEventHandler = (e) => {
      if (e.key === "L") landmarkClick(false);
      else if (e.key === "l") landmarkClick(true);
      else if (e.key === "R") rIdClick(false);
      else if (e.key === "r") rIdClick(true);
      else if (e.key === "S") stageClick(false);
      else if (e.key === "s") stageClick(true);
      else if (e.key === "M") matchFrameClick(false);
      else if (e.key === "m") matchFrameClick(true);
      else if (e.key === "P") poseClick(false);
      else if (e.key === "p") poseClick(true);
    }
    document.addEventListener("keydown", keyEventHandler);
    return () => {
      document.removeEventListener("keydown", keyEventHandler);
    }
  }, [poseFid, matchFid, stageId, rId, landmarkIndex]);


  const onPoseViewResize = () => {
    if (imageViewerRef && imageViewerRef.current && poseViewerRef && poseViewerRef.current)
      setPoseViewDims({height: imageViewerRef.current.clientHeight, width:poseViewerRef.current.clientWidth});
  }

  useEffect(() => { 
    window.addEventListener('resize', onPoseViewResize);
    onPoseViewResize();
    return () => {
      window.removeEventListener('resize', onPoseViewResize);
    };
  }, [imageViewerRef, poseViewerRef, poseImageData]);


  return (
    <div className="App" style={AppStyle}>
      <div style={ContainerStyle}>
        <div style={RowStyle}>
            <div className="handle" style={ImgViewerStyle} ref={imageViewerRef}>
              {matchImageData && <Img width={viewerDims.width} height={viewerDims.height} {...matchImageData}></Img>}
              {poseImageData && <Img width={viewerDims.width} height={viewerDims.height}  {...poseImageData}></Img>}
              {matchViewData && <MatchLines width={viewerDims.width} height={viewerDims.height} {...matchViewData}></MatchLines>}
            </div>
          <div ref={poseViewerRef} style={PoseViewerStyle}>
            {threeDData && 
              <PoseView 
              dims={poseViewDims} 
              poses={threeDData} 
              landmarks={matchViewData?matchViewData.landmarks:[]} 
              scale={1}/>
            }
            <div className='handle' style={DetailsStyle}>
              {detailsViewData && <Details {...detailsViewData}></Details>}
            </div>
            <div style={InstructionsStyle}>
              <h2>Instructions</h2>
              <p>* Press P or Shift+P to scroll through the Frames and their Poses</p>
              <p>* Press M or Shift+M to scroll through the Match Frames of Frames</p>
              <p>* Press S or Shift+S to scroll through the Stages of a particular Frame and Match Frame combo</p>
              <p>* Press R or Shift+R to scroll through the RANSAC iteration within a particular Stage, Frame and Match Frame combo</p>
              <p>* Press L or Shift+L to scroll through the matches</p>
              <p>The Logs Box can be dragged and moved anywhere on the screen. It can also be resized</p>
            </div>
          </div>
        </div>
        <div style={ActionPanelStyle}>
          <div style={{margin: '3px',width:'70%', height: '100%'}}>
            <Rnd
              style={RnDStyle}
              default={{
                x: 0,
                y: 0,
                width: '100%',
                height: '100%'
              }}
            >
              {frameData && <Log init={frameData.mapInit} frameId={poseFid} stageId={stageId} rId={rId} matc></Log>}
            </Rnd>
          </div>
          <div style={{display: 'flex', flexDirection: 'row', alignItems: 'center', marginTop: '20px'}}>
            <p style={{fontSize: '18px'}}>Jump to Pose Frame Id</p>
            <input id="poseFidInput" 
              type="text" 
              style={PoseInputStyle}
              onChange={poseFidInput}>
            </input>
          </div>
          <button id="reload" style={ButtonStyle} onClick={() => setReload(true)}>Reload From Start</button>
        </div>
      </div>
    </div>
  );
}

export default App;
