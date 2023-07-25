/**
 * @file PoseView.js.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
import React, { useEffect, useState, useRef } from 'react';
import { initialize, updatePoses, resize } from './ThreeJSUtils';

const PoseViewStyle = {
    width: '0px',
    height: '0px'
}

function PoseView({poses, landmarks, dims, scale}) {
    const canvasRef = useRef();
    const [sceneRef, setSceneRef] = useState(null);

    useEffect(() => {
        if (!canvasRef.current) return;
        setSceneRef(initialize(canvasRef));
        console.log("Scene ref");
    }, []);

    useEffect(() => {
        if (!sceneRef) return;
        resize(sceneRef, dims);
    }, [dims]);

    useEffect(() => {
        if (!sceneRef) return;
        updatePoses(sceneRef, poses, landmarks, scale);
        console.log("Update poses", poses);
    }, [sceneRef, poses, scale]);

    return <canvas style={PoseViewStyle} id="poseView" ref={canvasRef}></canvas>
}

export default PoseView;