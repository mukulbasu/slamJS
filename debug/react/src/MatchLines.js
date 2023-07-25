/**
 * @file MatchLines.js.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
import React from 'react';

const RowStyle = {
  display: 'flex',
  'flexDirection': 'row'
}

const ColStyle = {
  display: 'flex',
  'flexDirection': 'column',
  'alignItems': 'flex-start',
  'justifyContent': 'center',
}

const MatchLinesStyle = {
  position: 'absolute',
  alignSelf: 'flexStart'
}


const VALID_COLOR = 'rgb(0,255,0)';
const FIXED_COLOR = 'rgb(0,128,255)';
const UNSET_COLOR = 'rgb(255, 128, 0)';
const INVALID_COLOR = 'rgb(255, 0, 0)';
const getColor = (result) => {
  if (result == "UNSET") return UNSET_COLOR;
  else if (result == "INVALID") return INVALID_COLOR;
  else if (result == "VALID") return VALID_COLOR;
  else if (result == "FIXED") return FIXED_COLOR;
  else return UNSET_COLOR;
}

function MatchLines(props) {
  
  return <svg width={(props.width*2).toString()+"px"} height={props.height.toString()+"px"} style={MatchLinesStyle}>
    {Object.values(props.landmarks).map(ele => {
      const [poseFp, matchFp] = [ele.fps[props.poseFid], ele.fps[props.matchFid]];
      if (!matchFp) console.log("No MatchFP", ele.fps);
      const showLine = {
        key: 'show_'+ele.lId,
        width: 0,
        height: 0,
        x1: 480+props.mult*(poseFp?poseFp.x:0) + props.offsetX,
        y1: props.mult*(poseFp?poseFp.y:0) + props.offsetY,
        x2: props.mult*(matchFp?matchFp.x:0) + props.offsetX,
        y2: props.mult*(matchFp?matchFp.y:0) + props.offsetY,
        // x1: 0,
        // y1: 100,
        // x2: 680,
        // y2: 100,
        strokeWidth: ele.selected? 20:2, 
        strokeOpacity: '1.0', 
        className: 'showLine',
        strokeLinecap: 'round',
        stroke: getColor(ele.result)
      };
      return <line {...showLine}></line>;
    })}
    {Object.values(props.landmarks).map(ele => {
      const [poseFp, matchFp] = [ele.fps[props.poseFid], ele.fps[props.matchFid]];
      const hiddenLine = {
        key: 'hide_'+ele.lId,
        width: 0,
        height: 0,
        x1: 480+props.mult*(poseFp?poseFp.x:0) + props.offsetX,
        y1: props.mult*(poseFp?poseFp.y:0) + props.offsetY,
        x2: props.mult*(matchFp?matchFp.x:0) + props.offsetX,
        y2: props.mult*(matchFp?matchFp.y:0) + props.offsetY,
        strokeWidth: 10, 
        strokeOpacity: '0.0', 
        className: 'hiddenLine', 
        stroke: 'rgb(255,128,0)'};
      return <line {...hiddenLine}></line>;
    })}
  </svg>;
}

export default MatchLines;