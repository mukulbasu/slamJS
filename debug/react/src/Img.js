/**
 * @file Img.js.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
import React from 'react';
import Details from './Details';

const ImgStyle = {
    'display': 'flex',
    'flexDirection': 'column',
    height: 'auto'
}

function Img(props) {
    return <div style={ImgStyle}>
        <img width={props.width.toString()+"px"} height={props.height.toString()+"px"} src={props.path}></img>
        <div style={{overflowY: 'scroll'}}>
            <Details {...props}></Details>
        </div>
    </div>;
}

export default Img;