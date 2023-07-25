/**
 * @file Details.js.
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

const DetailsStyle = {
    margin: '5px'
}

function Details(props) {
    const {title, details} = props;
    return <div style={ColStyle}>
        <h2>{title}</h2>
        {details.map((detail, index) => 
            <div key={index} style={DetailsStyle}>{detail}</div>
        )}
    </div>;
}

export default Details;