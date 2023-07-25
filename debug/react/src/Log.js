/**
 * @file Log.js
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
import React, {useEffect, useState} from 'react';
import {fetchLogs} from './Utils.js';

const ContainerStyle = {
    display: 'flex',
    flexDirection: 'column',
    width: '100%',
    height: '100%',
}

const RowStyle = {
  display: 'flex',
  flexDirection: 'row',
}

const ButtonStyle = {
  margin: '3px',
  borderRadius: '9px',
  height: '30px',
  width: '250px',
  fontSize: '18px'
}

const LogContainerStyle = {
    display: 'flex',
    flexDirection: 'column',
    width: '100%',
    height: '100%',
    overflowY: 'scroll',
}

const LogStyle = {
    display: 'flex',
    flexDirection: 'row',
    justifyContent: 'flexStart'
}


function Log({init, frameId, stageId, rId}) {
    const [logs, setLogs] = useState([""]);
    const [useStage, setUseStage] = useState(true);
    const [useRANSAC, setUseRANSAC] = useState(true);

    useEffect(() => {
        const call = async () => {
            const logsResp = await fetchLogs(frameId, (useStage? stageId : null), (useRANSAC? rId: null));
            setLogs(logsResp);
        };
        call();
    }, [init, frameId, stageId, rId, useStage, useRANSAC]);

    useEffect(() => {
        const logTitle = document.getElementById("logTitle");
        
        if (useStage && useRANSAC) 
            logTitle.innerText = "Logs By Frame, Stage & RANSAC iter";
        else if (useStage)
            logTitle.innerText = "Logs By Frame & Stage";
        else
            logTitle.innerText = "Logs By Frame";

        document.getElementById("logDisableStage").innerText = (useStage? `Disable Stage Search`: `Enable Stage Search`);
        document.getElementById("logDisableRANSAC").innerText = (useRANSAC? `Disable RANSAC Search`: `Enable RANSAC Search`);
    }, [init, useStage, useRANSAC]);


    const onStageClick = () => {
        const newValue = !useStage;
        setUseStage(newValue);
        if (!newValue) {
            setUseRANSAC(false);
        }
    }

    const onRANSACClick = () => {
        const newValue = !useRANSAC;
        setUseRANSAC(newValue);
    }

    return <div style={ContainerStyle}>
        <h2 id="logTitle">Logs By Frame, Stage, RANSAC</h2>
        <div style={RowStyle}>
            <button id="logDisableStage" style={ButtonStyle} onClick={onStageClick}>Disable Stage Search</button>
            <button id="logDisableRANSAC" style={ButtonStyle} onClick={onRANSACClick}>Disable RANSAC Search</button>
        </div>
        <div style={LogContainerStyle}>
            {logs.map((log, index) => <div style={LogStyle} key={index}>{log}</div>)}
        </div>
    </div>;
}

export default Log;