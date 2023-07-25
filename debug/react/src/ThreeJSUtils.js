/**
 * @file ThreeJSUtils.js.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
import * as THREE from 'three';
// import TrackballControls from 'three-trackballcontrols';
import { TrackballControls } from 'three/examples/jsm/controls/TrackballControls.js';

const Cube = (color, size, position) => {
    const cube = new THREE.Mesh(new THREE.BoxGeometry(...size), 
        new THREE.MeshBasicMaterial({color})); 
    cube.position.set(...position);
    return cube;
}


const addMarkerCubes = (scene) => {
    //RED for X
    scene.add(Cube(0xff0000, [1,1,1], [10, 0, 0]));
    scene.add(Cube(0x00ff00, [0.5,0.5,0.5], [10, 1, 0]));
    scene.add(Cube(0x0000ff, [0.5,0.5,0.5], [10, 0, 1]));
    //GREEN for Y
    scene.add(Cube(0x00ff00, [1,1,1], [0, 10, 0]));
    scene.add(Cube(0x0000ff, [0.5,0.5,0.5], [0, 10, 1]));
    scene.add(Cube(0xff0000, [0.5,0.5,0.5], [1, 10, 0]));
    //BLUE for Z
    scene.add(Cube(0x0000ff, [1,1,1], [0, 0, 10]));
    scene.add(Cube(0x00ff00, [0.5,0.5,0.5], [0, 1, 10]));
    scene.add(Cube(0xff0000, [0.5,0.5,0.5], [1, 0, 10]));

    scene.add(Cube(0xff9999, [2,2,2], [0,6,12]));
}

const cameraObj = ({current, origin}) => {
    const result = new THREE.Group();
    const group = new THREE.Group();
    //Origin is Red, current is Blue
    const color = current ? 0x0000FF : origin ? 0xFF0000 : 0x00FF00;

    const geoDims = [ [2, 1, 1], [1, 0.5, 2.5], [2, 1, 1], [1, 0.5, 2.5] ]
    const geometries = geoDims.map(dim => new THREE.BoxGeometry(...dim));

    const positions = [[0, 0.5, 0], [0, 0.25, 1], [0, -0.5, 0], [0, -0.25, 1]];

    const matTop = new THREE.MeshStandardMaterial({color: color, emissive: 0x072534, side: THREE.DoubleSide, flatShading: true, transparent: (current||origin? false: true), opacity: 0.2});
    const matBottom = new THREE.MeshStandardMaterial({color: 0x8B8000, emissive: 0x072534, side: THREE.DoubleSide, flatShading: true, transparent: (current||origin? false: true), opacity: 0.2});

    for (let index in geometries) {
        const mesh = new THREE.Mesh(geometries[index], (index < 2)? matTop : matBottom);
        mesh.position.set(...positions[index]);
        group.add(mesh);
    }
    group.rotateX(Math.PI);
    group.scale.set(0.5, 0.5, 0.5);

    result.add(group);
    if (current || origin) {
      const axesHelper = new THREE.AxesHelper( 5 );
      result.add( axesHelper );
    }
    return result;
}

const landmarkObj = (current, result) => {
  const getCube = (width, height, depth, color) => {
      const geometry = new THREE.BoxGeometry(width, height, depth);
      const material = new THREE.MeshBasicMaterial({ color });
      return new THREE.Mesh(geometry, material);
  }
  const group = new THREE.Group();
  const dim = 1;//(current? 9 : 3);

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
  group.add(getCube(dim, dim, dim, getColor(result)));
  if (current) {
      group.add(getCube(dim*2, dim*0.7, dim*0.7, 0x800080));
  // } else {
  //     group.add(getCube(dim*0.7, dim*1.3, dim*0.7, 0x808080))
  }
  return group;
}

export const initialize = (canvasRef) => {
    const floorDepth = 250;
    const gridSize = floorDepth * 2;
    const floorPositions = [0, -floorDepth, 0];
    const scene = new THREE.Scene();
    const canvas = canvasRef.current;
    const renderer = new THREE.WebGLRenderer({ canvas: canvas });
    renderer.setSize(canvas.offsetWidth, canvas.offsetHeight);
  
    const dirLight1 = new THREE.DirectionalLight(0xffffff);
    dirLight1.position.set(1000, 1000, -1000);
    scene.add(dirLight1);
  
    const dirLight2 = new THREE.DirectionalLight(0xffffff);
    dirLight2.position.set(-1000, -1000, -1000);
    scene.add(dirLight2);
  
    const dirLight3 = new THREE.DirectionalLight(0xffffff);
    dirLight3.position.set(0, 0, 1000);
    scene.add(dirLight3);
  
    const ambientLight = new THREE.AmbientLight(0x222222);
    scene.add(ambientLight);
  
    scene.background = new THREE.Color(0xffffff);
    scene.fog = new THREE.FogExp2(0xcccccc, 0.002);
  
    const floor = new THREE.GridHelper(gridSize, 15, 0x808080, 0x808080);
    floor.position.set(...floorPositions);
    // scene.add( floor );
  
    const pos = [
        [0, floorDepth, 0],
        [0, -floorDepth, 0],
        [floorDepth, 0, 0],
        [-floorDepth, 0, 0],
        [0, 0, floorDepth],
        [0, 0, -floorDepth],
    ]
    for (let i = 0; i < 6; i++) {
        const floors = new THREE.GridHelper(gridSize, 5, 0x808080, 0x808080);
        floors.position.set(...pos[i]);
        if (i > 3) {
            floors.rotation.set(Math.PI / 2, 0, 0);
        } else if (i > 1) {
            floors.rotation.set(0, 0, Math.PI / 2);
        }
        scene.add(floors);
    }
  
    addMarkerCubes(scene);
  
    const camera = new THREE.PerspectiveCamera(75, canvas.offsetWidth / canvas.offsetHeight, 0.1, 100000);
    camera.position.set(9, -12, -27);
    camera.up.set(0, -1, 0);
      camera.rotation.reorder( "YXZ" );
    scene.add(camera);
    
    // const axesHelper = new THREE.AxesHelper( 5 );
    // scene.add( axesHelper );
  
    const controls = new TrackballControls(camera, renderer.domElement);
  
    controls.rotateSpeed = 1.0;
    controls.zoomSpeed = 1.2;
    controls.panSpeed = 0.8;
  
    controls.keys = ['KeyA', 'KeyS', 'KeyD'];
  
    function animate() {
      requestAnimationFrame(animate);
  
      controls.update();
      renderer.render(scene, camera);
    }
    animate();
  
    return {scene, camera, renderer, controls};
}


export const resize = (sceneRef, dims) => {
  const {camera, renderer, controls} = sceneRef;
  camera.aspect = dims.width / dims.height;
  camera.updateProjectionMatrix();
  renderer.setSize(dims.width, dims.height);
  controls.handleResize();
};


export const updatePoses = (sceneRef, poses, landmarks, scale) => {
  const scene = sceneRef.scene;
  const types = ["keyFrameObj", "frameObj", "originObj", "currentObj"];
  const existingObjMap = {};
  types.forEach(type => existingObjMap[type] = []);
  let originObj = null, currentObj = null;
  scene.children.forEach(child => {
    if (!(child.name in existingObjMap)) existingObjMap[child.name] = [];
    if (types.includes(child.name)) {
      existingObjMap[child.name].push(child);
    }
    if (child.name.startsWith("landmark")) {
      if (!(child.name in existingObjMap)) existingObjMap[child.name] = [];
      existingObjMap[child.name].push(child);
    }
  });

  const setPose = (obj, trans, rots, scale) => {
    obj.position.set(trans[0]*scale, trans[1]*scale, trans[2]*scale);
    obj.rotation.set(0, 0, 0);
    obj.rotateY(rots.y*Math.PI/180);
    obj.rotateX(rots.x*Math.PI/180);
    obj.rotateZ(rots.z*Math.PI/180);
  }

  const getCameraObj = (type) => {
    let obj;
    if (existingObjMap[type].length > 0) {
      obj = existingObjMap[type].pop();
    } else {
      const current = type === "currentObj";
      const origin = type === "originObj";
      const keyframe = type === "keyFrameObj";
      obj = cameraObj({current, origin, keyframe});
      obj.name = type;
      scene.add(obj);
    }
    return obj;
  }

  poses.forEach((pose, index) => {
      const { trans, rots} = pose;
      if (trans) {
        let obj;
        if (pose.isCurrent) {
          obj = getCameraObj("currentObj");
        } else if (index == 0) {
          obj = getCameraObj("originObj");
        } else if (pose.isKeyframe) {
          obj = getCameraObj("keyFrameObj");
        } else {
          obj = getCameraObj("frameObj");
        }
        setPose(obj, trans, rots, scale);
      }
  });

  console.log("Landmarks ", landmarks);
  Object.values(landmarks).forEach(landmark => {
    const type = `landmark_${landmark.selected}_${landmark.result}`;
    let obj;
    if (type in existingObjMap && existingObjMap[type].length > 0) {
      obj = existingObjMap[type].pop();
    } else {
      obj = landmarkObj(landmark.selected, landmark.result);
      obj.name = type;
      scene.add(obj);
    }
    const trans = landmark.trans;
    obj.position.set(trans[0]*scale, trans[1]*scale, trans[2]*scale);
  });

  for (let type in existingObjMap) {
    existingObjMap[type].forEach(child => scene.remove(child));
  }
}