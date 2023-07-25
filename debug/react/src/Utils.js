/**
 * @file Utils.js.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
export const trim = (val) => {
  return Math.round(Number(val)*100)/100;
}

const getResult = (result) => {
  const rn = Number(result);
  if (rn == -1) return "UNSET";
  else if (rn == 0) return "INVALID";
  else if (rn == 1) return "VALID";
  else if (rn == 2) return "FIXED";
  else return "NA";
}

export const fetchData = async (debugFile, search) => {
  const response = await fetch('http://localhost:8080/debug/logs/'+debugFile+'/'+search);
  const respJson = await response.json();
  const resp = respJson.lines.map((line) => {
      // console.log("Line ", line);
      const values = line.split(":");
      // console.log("Values", values);
      const name = values[0];
      const result = {name};
      values[1].split(";").forEach(value => {
          const list = value.split("=");
          result[list[0]] = list[1];
      });
      return result;
  })
  return resp;
}

export const fetchLogs = async (frameId, stageId, rId) => {
  console.log("Fetch logs", frameId, stageId, rId);
  const response = await fetch('http://localhost:8080/cout/'+(frameId?frameId:"")+
    (stageId?'/'+stageId:"")+(stageId && rId?'/'+rId:""));
  const respJson = await response.json();
  return respJson.lines;
}


export const extractLandmarkDetails = ({landmarks, fps, poseFid, matchFid, stageId, rId, landmarkIndex}) => {
  let relevantFps = fps.filter(fp => 
    (fp.FID === poseFid || fp.FID === matchFid) && 
    fp.MFID === matchFid && 
    Number(fp.STAGE) === Number(stageId) &&
    Number(fp.RID) === Number(rId)
  );

  const landmarkVsRefY = {};
  const landmarkFps = {};
  relevantFps.forEach(fp => {
    if (!(fp.LID in landmarkFps)) landmarkFps[fp.LID] = new Set();
    landmarkFps[fp.LID].add(fp.FID);
    if (fp.FID === poseFid) landmarkVsRefY[fp.LID] = Number(fp.Y);
  });

  relevantFps = relevantFps.filter(fp => landmarkFps[fp.LID].size >= 2);

  const relevantLandmarks = landmarks.filter(landmark => 
    landmark.MFID === matchFid && 
    Number(landmark.STAGE) === Number(stageId) && 
    Number(landmark.RID) === Number(rId) &&
    landmark.LID in landmarkFps &&
    landmarkFps[landmark.LID].size >= 2
  );

  relevantLandmarks.sort((a, b) => landmarkVsRefY[a.LID] - landmarkVsRefY[b.LID]);

  let selectedLandmark = null;
  if (relevantLandmarks.length > landmarkIndex) {
    selectedLandmark = relevantLandmarks[landmarkIndex];
  }

  const poseFp = (selectedLandmark? relevantFps.find(fp => fp.FID === poseFid && fp.LID === selectedLandmark.LID) : null);
  const matchFp = (selectedLandmark? relevantFps.find(fp => fp.FID === matchFid && fp.LID === selectedLandmark.LID) : null);

  return {relevantLandmarks, selectedLandmark, relevantFps, poseFp, matchFp};
}

export const extractFrameDetails = ({frames, poseFid, matchFid, stageId, rId}) => {
  const poseFrame = frames.find(frame => frame.FID == poseFid &&
    frame.MFID === matchFid && 
    Number(frame.STAGE) === Number(stageId) && 
    Number(frame.RID) === Number(rId));
  const matchFrame = frames.find(frame => frame.FID == matchFid &&
    frame.MFID === matchFid && 
    Number(frame.STAGE) === Number(stageId) && 
    Number(frame.RID) === Number(rId));
  
  return {poseFrame, matchFrame};
}

export const genMatchViewData = ({poseFid, matchFid, limits, relevantLandmarks, selectedLandmark, relevantFps}) => {
  const matchData = {
    poseFid, 
    matchFid, 
    mult: Number(limits.MULTIPLIER), 
    offsetX: Number(limits.OFFSETX),
    offsetY: Number(limits.OFFSETY),
    landmarks: {}
  };
  relevantLandmarks.forEach(landmark => {
    matchData.landmarks[landmark.LID] = {
      result: getResult(landmark.RESULT), 
      lId: landmark.LID,
      trans: [landmark.TX, landmark.TY, landmark.TZ],
      selected: landmark.LID === selectedLandmark.LID,
      fps: {}
    }
  });

  relevantFps.forEach(fp => {
    // console.log("Filling up Landmark ", fp.LID, fp.FID, fp.FPID, poseFid, matchFid);
    matchData.landmarks[fp.LID].fps[fp.FID] = {
      fpId: fp.FPID,
      fId: fp.FID,
      lId: fp.LID,
      x: Number(fp.X),
      y: Number(fp.Y),
      px: (fp.RESULT !== "-1" && fp.RESULT !== "2")? fp.PX : "-",
      py: (fp.RESULT !== "-1" && fp.RESULT !== "2")? fp.PY : "-",
      result: getResult(fp.RESULT),
      isBehind: fp.BEHIND === "1",
      isTooClose: fp.CLOSE === "1",
      isTooFar: fp.FAR === "1",
      selected: false,
    };
  });

  const deleteLids = [];
  for (const lId in matchData.landmarks) {
    if (Object.keys(matchData.landmarks[lId].fps).length < 2)
      deleteLids.push(lId);
  }
  deleteLids.forEach(lId => delete matchData.landmarks[lId]);

  return matchData;
}

export const genImgViewData = ({titlePrepend, framePose, frame, landmark, fp}) => {
  const resp = {title: `${titlePrepend} NA`, path: "", details: []};
  const details = [];
  let title, path;
  if (framePose) {
    title = `${titlePrepend} Frame: ${framePose.POSE_FID}`;
    path = `http://localhost:8080${framePose.PATH}`;
  }
  if (frame) {
    details.push("Valid : " + getResult(frame.RESULT));
    details.push("Pose : " + [frame.TX, frame.TY, frame.TZ].join(", "));
    details.push("Rot : " + [frame.RX, frame.RY, frame.RZ].join(", "));
  }
  if (fp) {
    details.push("FP ID : " + fp.FPID);
    details.push(`FP Result : ${getResult(fp.RESULT)} (${fp.BEHIND === "1"?"Is Behind/":""}${fp.CLOSE === "1"?"Too Close/":""}${fp.FAR === "1"?"Too Far/":""}${getResult(fp.RESULT)==="INVALID"&& fp.FAR !== "1" && fp.CLOSE !== "1" && fp.BEHIND !== "1"?"Exceeds Gap":""})`);
    details.push("FP x, y : " + [fp.X, fp.Y].join(", "));
    details.push("Projected x, y : " + [fp.PX, fp.PY].join(", "));
  }
  if (landmark) {
    details.push("L ID : " + landmark.LID);
    details.push("L Result : " + getResult(landmark.RESULT));
    details.push("L Trans : " + [landmark.TX, landmark.TY, landmark.TZ].join(", "));
  }
  return { title, path, details};
}

export const genDetailsViewData = ({poseFid, matchFid, stageId, rId,
    refFramePose, vos, matchFrames, stageMap, 
    selectedLandmark, repls, poseFp, matchFp, profile}) => {
  const detailsData = {
    title: "Details",
    details: [
      `Selected Pose Frame : ${poseFid}`,
      `Selected Match Frame : ${matchFid}`,
      `Stage/RID Selected : ${stageId}/${rId}`,
      `Winner RID : ${refFramePose.WINNER_RID}`,
      `Frame Validity : ${refFramePose.VALID === "1"}`,
      `Map Initialized : ${refFramePose.INIT === "1"}`,
      `Match Size : ${refFramePose.MATCH_SIZE}`,
    ]
  }

  if (profile) {
    const {OVERALL_TIME, KP_TIME, FRAME_CREATE_TIME, 
      POSE_TIME, POSE_FRAME_EXTRACTION_TIME, POSE_MATCH_TIME,
      POSE_RANSAC_INIT_TIME, POSE_WINNER_TIME, 
      POSE_VALID_TIME, POSE_EST_TIME} = profile;
    detailsData.details.push(`Perf : Overall = ${OVERALL_TIME}, Kp = ${KP_TIME}, Pose = ${POSE_TIME}`);
    detailsData.details.push(`Pose Perf : Match = ${POSE_MATCH_TIME}, RansacInit = ${POSE_RANSAC_INIT_TIME}`);
    detailsData.details.push(`Pose Perf2 : Winner = ${POSE_WINNER_TIME}, Valid=${POSE_VALID_TIME}, Est=${POSE_EST_TIME}`);
  }

  const vo = vos.find(vo => vo.STAGE===stageId && vo.RID===rId && vo.MFID === matchFid);
  if (vo) {
    detailsData.details.push(`Num Matches : ${vo.MATCH_SIZE}`);
    detailsData.details.push(`Avg Inlier Ratio: ${vo.AVG_IN_RATIO}`);
    detailsData.details.push(`Valid Frame Ratio: ${vo.VALID_FR_RATIO}`);
  }

  if (matchFid && matchFid in stageMap) {
    detailsData.details.push(`Match Frames : ${Array.from(matchFrames).join(", ")}`);
    detailsData.details.push(`Max Stages : ${Object.keys(stageMap[matchFid]).length}`);
    for (let i = 0; i < Object.keys(stageMap[matchFid]).length; i++) {
      console.log("Stage Map ", stageMap[matchFid][i]);
      detailsData.details.push("STAGE "+i.toString()+" RANSAC : "+Object.keys(stageMap[matchFid][i]).join(", "));
      detailsData.details.push("STAGE "+i.toString()+" AVG IN RATIO : "+Object.values(stageMap[matchFid][i]).map(ele => trim(ele.avgInRatio)).join(", "));
      detailsData.details.push("STAGE "+i.toString()+" VALID FR RATIO : "+Object.values(stageMap[matchFid][i]).map(ele => ele.validFrRatio).join(", "));
    }
    if (selectedLandmark) {
      detailsData.details.push(`Selected Landmark : ${selectedLandmark.LID}`);
      
      const replMatches = [];
      const matchFIdFpId = (replFp, frameId, fpId) => {
        if (!replFp) return false;
        const [replFId, replFpId] = replFp.split("_");
        console.log("FPID ", replFp, replFId, replFpId, frameId, fpId);
        return (replFId === frameId && fpId === replFpId);
      }
      repls.forEach(repl => {
        if (repl.L2 === selectedLandmark.LID) {
          const replFps = repl.L1FPS.split(",");
          const posePresent = replFps.some(replFp => matchFIdFpId(replFp, poseFid, poseFp.FPID));
          const matchPresent = replFps.some(replFp => matchFIdFpId(replFp, matchFid, matchFp.FPID));
          posePresent && matchPresent && detailsData.details.push(`Alternate Landmarks : ${repl.L1}`);
        }
      });
    }
  }
  return detailsData;
}

export const genThreeDData = ({framePoses, refFramePose, poseFid, poseFrame}) => {
  const posesData = [];
  framePoses.sort((a, b) => Number(a.POSE_FID) < Number(b.POSE_FID));
  for (let i = 0; i < framePoses.length; i++) {
    const framePose = framePoses[i];
    if (framePose.VALID !== "1") continue;
    if (Number(framePose.POSE_FID) >= Number(poseFid)) continue;
    posesData.push({
      trans: [framePose.TX, framePose.TY, framePose.TZ],
      rots: {x: framePose.RX, y: framePose.RY, z: framePose.RZ},
      valid: framePose.VALID === "1",
      isKeyframe: false,
      isCurrent: false,
    })
  }
  {
    if (poseFrame) {
      posesData.push({
        trans: [poseFrame.TX, poseFrame.TY, poseFrame.TZ],
        rots: {x: poseFrame.RX, y: poseFrame.RY, z: poseFrame.RZ},
        valid: poseFrame.RESULT === "1",
        isKeyframe: false,
        isCurrent: true,
      })
    } else {
      posesData.push({
        trans: [0, 0, 0],
        rots: {x: refFramePose.RX, y: refFramePose.RY, z: refFramePose.RZ},
        valid: false,
        isKeyframe: false,
        isCurrent: true,
      })
    }
  }
  console.log("posesData", posesData);
  return posesData;
}


// export const genData = (frameData, poseFid, matchFid, stageId, rId, limits, framePoses, selectedLandmarkIndex) => {
//   if (poseFid == null) return;
//   const {landmarks, frames, fps, vos, repls, matchFrames, stageMap} = frameData;

//   console.log("Frames ", frames);
//   const matchData = {
//     poseFid, 
//     matchFid, 
//     mult: Number(limits.MULTIPLIER), 
//     offsetX: Number(limits.OFFSETX),
//     offsetY: Number(limits.OFFSETY),
//     landmarks: {}
//   };
//   landmarks.forEach(landmark => {
//     if (Number(landmark.STAGE) !== Number(stageId) || Number(landmark.RID) !== Number(rId)) return;
//     matchData.landmarks[landmark.LID] = {
//       result: getResult(landmark.RESULT), 
//       lId: landmark.LID,
//       trans: [landmark.TX, landmark.TY, landmark.TZ],
//       fps: {}
//     }
//   });

//   const landmarkVsRefY = {};
//   fps.forEach(fp => {
//     if (fp.FID !== matchFid && fp.FID !== poseFid) return;
//     if (Number(fp.STAGE) !== Number(stageId) || Number(fp.RID) !== Number(rId)) return;
//     // console.log("Filling up Landmark ", fp.LID, fp.FID, fp.FPID, poseFid, matchFid);
//     matchData.landmarks[fp.LID].fps[fp.FID] = {
//       fpId: fp.FPID,
//       fId: fp.FID,
//       lId: fp.LID,
//       x: Number(fp.X),
//       y: Number(fp.Y),
//       px: (fp.RESULT !== "-1" && fp.RESULT !== "2")? fp.PX : "-",
//       py: (fp.RESULT !== "-1" && fp.RESULT !== "2")? fp.PY : "-",
//       result: getResult(fp.RESULT),
//       isBehind: fp.BEHIND === "1",
//       isTooClose: fp.CLOSE === "1",
//       isTooFar: fp.FAR === "1",
//       selected: false,
//     };
//     if (fp.FID === poseFid) landmarkVsRefY[fp.LID] = Number(fp.Y);
//   });

//   const deleteLids = [];
//   for (const lId in matchData.landmarks) {
//     if (Object.keys(matchData.landmarks[lId].fps).length < 2)
//       deleteLids.push(lId);
//   }
//   deleteLids.forEach(lId => delete matchData.landmarks[lId]);


//   const sortedLandmarks = Object.keys(matchData.landmarks);
//   sortedLandmarks.sort((a, b) => landmarkVsRefY[a] - landmarkVsRefY[b]);

//   let selectedLandmark = null;
//   if (Object.keys(matchData.landmarks).length > selectedLandmarkIndex) {
//     const selectedLandmarkId = sortedLandmarks[selectedLandmarkIndex];
//     matchData.landmarks[selectedLandmarkId].selected = true;
//     selectedLandmark = landmarks.find(landmark => {
//       return landmark.LID === selectedLandmarkId &&
//         landmark.STAGE === stageId &&
//         landmark.RID === rId;
//     });
//     landmarks.forEach((l, index) => {
//       if (l.LID === selectedLandmarkId &&
//           l.STAGE === stageId &&
//           l.RID === rId) console.log("Found landmark ", index);
//     })
//   }
//   console.log("MatchData", matchData);


//   const refFrame = frames.find((f => f.POSE_FID === poseFid && f.FID === poseFid && f.STAGE === stageId && f.RID === rId));
//   const refFramePose = framePoses.find(fp => fp.POSE_FID === poseFid);
//   const poseFrameData = genImgData("Ref", refFramePose, refFrame, matchData, selectedLandmark);
//   const matchFrame = frames.find((f => f.POSE_FID === poseFid && f.FID === matchFid && f.STAGE === stageId && f.RID === rId));
//   const matchFramePose = framePoses.find(fp => fp.POSE_FID === matchFid);
//   const matchFrameData = genImgData("Match", matchFramePose, matchFrame, matchData, selectedLandmark);

//   const detailsData = genDetailsData(poseFid, matchFid, stageId, rId, selectedLandmark, matchData, frameData, refFramePose);

//   const posesData = [];
//   framePoses.sort((a, b) => Number(a.POSE_FID) < Number(b.POSE_FID));
//   for (let i = 0; i < framePoses.length; i++) {
//     const framePose = framePoses[i];
//     if (framePose.VALID !== "1") continue;
//     if (Number(framePose.POSE_FID) >= Number(poseFid)) continue;
//     posesData.push({
//       trans: [framePose.TX, framePose.TY, framePose.TZ],
//       rots: {x: framePose.RX, y: framePose.RY, z: framePose.RZ},
//       valid: framePose.VALID === "1",
//       isKeyframe: false,
//       isCurrent: false,
//     })
//   }
//   {
//     let currFrameRANSAC = frames.find(frame => frame.POSE_FID === poseFid && frame.FID === poseFid && frame.STAGE == stageId && frame.RID == rId);
//     if (currFrameRANSAC) {
//       posesData.push({
//         trans: [currFrameRANSAC.TX, currFrameRANSAC.TY, currFrameRANSAC.TZ],
//         rots: {x: currFrameRANSAC.RX, y: currFrameRANSAC.RY, z: currFrameRANSAC.RZ},
//         valid: currFrameRANSAC.RESULT === "1",
//         isKeyframe: false,
//         isCurrent: true,
//       })
//     } else {
//       posesData.push({
//         trans: [0, 0, 0],
//         rots: {x: refFramePose.RX, y: refFramePose.RY, z: refFramePose.RZ},
//         valid: false,
//         isKeyframe: false,
//         isCurrent: true,
//       })
//     }
//   }
//   console.log("posesData", posesData);

//   return {matchData, detailsData, poseFrameData, matchFrameData, selectedLandmark, posesData};
// };


// const genDetailsData = (poseFid, matchFid, stageId, rId, selectedLandmark, matchData, frameData, refFramePose) => {
//   const {landmarks, frames, fps, vos, repls, matchFrames, stageMap} = frameData;
//   const detailsData = {
//     title: "Details",
//     details: [
//       `Selected Pose Frame : ${poseFid}`,
//       `Selected Match Frame : ${matchFid}`,
//       `Stage/RID Selected : ${stageId}/${rId}`,
//       `Winner RID : ${refFramePose.WINNER_RID}`,
//       `Frame Validity : ${refFramePose.VALID === "1"}`,
//       `Map Initialized : ${refFramePose.INIT === "1"}`,
//     ]
//   }
//   const vo = vos.find(vo => vo.STAGE===stageId && vo.RID===rId);
//   if (vo) detailsData.details.push(`Num Matches : ${vo.MATCH_SIZE}`);

//   if (matchFid && matchFid in stageMap) {
//     if (stageId in stageMap[matchFid] && rId in stageMap[matchFid][stageId]) {
//       detailsData.details.push(`Avg Inlier Ratio: ${stageMap[matchFid][stageId][rId].avgInRatio}`);
//       detailsData.details.push(`Valid Frame Ratio: ${stageMap[matchFid][stageId][rId].validFrRatio}`);
//     }
//     detailsData.details.push(`Match Frames : ${Array.from(matchFrames).join(", ")}`);
//     detailsData.details.push(`Max Stages : ${Object.keys(stageMap[matchFid]).length}`);
//     for (let i = 0; i < Object.keys(stageMap[matchFid]).length; i++) {
//       console.log("Stage Map ", stageMap[matchFid][i]);
//       detailsData.details.push("STAGE "+i.toString()+" RANSAC : "+Object.keys(stageMap[matchFid][i]).join(", "));
//       detailsData.details.push("STAGE "+i.toString()+" AVG IN RATIO : "+Object.values(stageMap[matchFid][i]).map(ele => trim(ele.avgInRatio)).join(", "));
//       detailsData.details.push("STAGE "+i.toString()+" VALID FR RATIO : "+Object.values(stageMap[matchFid][i]).map(ele => ele.validFrRatio).join(", "));
//     }
//     if (selectedLandmark) {
//       detailsData.details.push(`Selected Landmark : ${selectedLandmark.LID}`);
      
//       const replMatches = [];
//       const matchFIdFpId = (replFp, frameId) => {
//         if (!replFp) return false;
//         const [fId, fpId] = replFp.split("_");
//         const matchFpId = matchData.landmarks[selectedLandmark.LID].fps[frameId].fpId;
//         console.log("FPID ", replFp, fId, fpId, frameId, matchFpId);
//         return (fId === frameId && fpId === matchFpId);
//       }
//       repls.forEach(repl => {
//         if (repl.L2 === selectedLandmark.LID) {
//           const replFps = repl.L1FPS.split(",");
//           const posePresent = replFps.some(replFp => matchFIdFpId(replFp, poseFid));
//           const matchPresent = replFps.some(replFp => matchFIdFpId(replFp, matchFid));
//           posePresent && matchPresent && detailsData.details.push(`Alternate Landmarks : ${repl.L1}`);
//         }
//       });
//     }
//   }
//   return detailsData;
// }
