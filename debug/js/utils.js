/**
 * @file utils.js
 * @brief This contains the necessary functionalities for AR and debugging in 3D view
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */

import * as THREE from 'three';
import { TrackballControls } from '/js/TrackballControls.js';

const getRotation = (deviceOrientation) => {
  const zee = new THREE.Vector3( 0, 0, 1 );
  const euler = new THREE.Euler();
  const q0 = new THREE.Quaternion();
  const q1 = new THREE.Quaternion( - Math.sqrt( 0.5 ), 0, 0, Math.sqrt( 0.5 ) ); // - PI/2 around the x-axis
  const obj = new THREE.PerspectiveCamera(70, 480 / 640, 0.1, 1000);
  // const obj = new THREE.Object3D();
  obj.rotation.reorder( "YXZ" );
  const alpha = deviceOrientation.alpha ? THREE.Math.degToRad( deviceOrientation.alpha ) + Math.PI/2 : 0; // Z
  const beta = deviceOrientation.beta ? THREE.Math.degToRad( deviceOrientation.beta ) : 0; // X'
  const gamma = deviceOrientation.gamma ? THREE.Math.degToRad( deviceOrientation.gamma ) : 0; // Y''
  const screenOrientation = window.orientation || 0;
  const orient = screenOrientation ? THREE.Math.degToRad( screenOrientation ) : 0; // O

  euler.set(beta, -alpha, gamma, 'YXZ' ); // 'ZXY' for the device, but 'YXZ' for us
  obj.quaternion.setFromEuler( euler ); // orient the device
  obj.quaternion.multiply( q1 ); // camera looks out the back of the device, not the top
  obj.quaternion.multiply( q0.setFromAxisAngle( zee, - orient ) ); // adjust for screen orientation
  obj.rotateOnWorldAxis(new THREE.Vector3(0, 1, 0), 22/7);
  const {x, y, z, order} = obj.rotation;
  return {x, y:y, z:z, order};
};
  
const syncDataWithServer = () => {
  let enable = false;
  let syncData = [];

  const sync = () => {
    if (syncData.length == 0) {
      // console.log("No sync data");
      setTimeout(sync, 500);
      return;
    }
    console.log("sync data present");
    while(syncData.length > 0) {
      const ele = syncData.shift();
      console.log("Syncing ", ele.filename);

      if (null == ele.blob) {
        console.log("sync Blob is null");
        continue;
      }
      const formData = new FormData();
      formData.append('filetoupload', ele.blob, ele.filename);
      axios.post('/uploadForm', formData);
      console.log("Sent sync data", ele.filename);
    }
    setTimeout(sync, 10);
  }
  sync();

  const queueData = (blob, filename) => {
    syncData.push({blob, filename});
  };

  return {
    toggleSync: () => {
      if (!enable) {
        enable = true;
        syncData = [];
        axios.get('/clearUploadDir');
      } else {
        enable = false;
      }
    },
    queueData,
    queueImageDetails : ({id, blob, x, y, z, order}) => {
      if (enable) {
        console.log("Adding to sync data");
        if (blob == null) {
          console.log("sync imgdata blob is null");
          return;
        }
        queueData(blob, "image"+id+".png");
        const orientText = x+","+y+","+z+","+order;
        queueData(new Blob([orientText]), "orient"+id+".txt");
      }
    },
    isEnabled: () => {
      return enable;
    },
  }
}

const trim = (value) => Math.round(value*100)/100;

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

const getMarkerCubes = () => {
  const cubes = [];
  //RED for X
  cubes.push(Cube(0xff0000, [1,1,1], [10, 0, 0]));
  cubes.push(Cube(0x00ff00, [0.5,0.5,0.5], [10, 1, 0]));
  cubes.push(Cube(0x0000ff, [0.5,0.5,0.5], [10, 0, 1]));
  //GREEN for Y
  cubes.push(Cube(0x00ff00, [1,1,1], [0, 10, 0]));
  cubes.push(Cube(0x0000ff, [0.5,0.5,0.5], [0, 10, 1]));
  cubes.push(Cube(0xff0000, [0.5,0.5,0.5], [1, 10, 0]));
  //BLUE for Z
  cubes.push(Cube(0x0000ff, [1,1,1], [0, 0, 10]));
  cubes.push(Cube(0x00ff00, [0.5,0.5,0.5], [0, 1, 10]));
  cubes.push(Cube(0xff0000, [0.5,0.5,0.5], [1, 0, 10]));

  cubes.push(Cube(0xff9999, [2,2,2], [0,6,12]));
  return cubes;
}

const arRender = (arDomId, fov) => {
  const threejsContainer = document.getElementById(arDomId);
  const threejsWidth = threejsContainer.offsetWidth;
  const threejsHeight = threejsContainer.offsetHeight;
  console.log(threejsWidth, threejsHeight);
  // scene
  const scene = new THREE.Scene();
  // camera
  const cameraAR = new THREE.PerspectiveCamera(fov, threejsWidth / threejsHeight, 0.1, 1000);
  cameraAR.position.set(0, 0, 0);
  cameraAR.up.set(0, -1, 0);
  cameraAR.lookAt(0, 0, 27);
	cameraAR.rotation.reorder( "YXZ" );
  const axesHelper = new THREE.AxesHelper( 5 );
  cameraAR.add( axesHelper );

  // Light
  const ambientLight = new THREE.AmbientLight(0xffffff, 1);
  scene.add(ambientLight);
  const pointLight = new THREE.PointLight(0xffffff, 0.2);
  pointLight.position.x = 2;
  pointLight.position.y = 3;
  pointLight.position.z = 4;
  scene.add(pointLight);

  //Cube
  addMarkerCubes(scene);
  
  // responsiveness
  window.addEventListener('resize', () => {
    const threejsWidth = threejsContainer.offsetWidth;
    const threejsHeight = threejsContainer.offsetHeight;
    cameraAR.aspect = threejsWidth / threejsHeight;
    cameraAR.updateProjectionMatrix();
    renderer.setSize(threejsWidth, threejsHeight);
    renderer.render(scene, cameraAR);
  })
  // renderer
  const renderer = new THREE.WebGL1Renderer( { alpha: true } );
  // const renderer = new THREE.WebGL1Renderer();
  renderer.setClearColor( 0x000000, 0 );
  renderer.setSize(threejsWidth, threejsHeight);
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  // animation
  function animate() {
    renderer.render(scene, cameraAR);
    requestAnimationFrame(animate);
  }
  // rendering the scene
  threejsContainer.append(renderer.domElement);
  renderer.render(scene, cameraAR);
  requestAnimationFrame(animate);
  
  return {
    update: (position, rotation) => {
      // console.log("Position ", position);
      cameraAR.position.set(position[0], position[1], position[2]);
      cameraAR.rotation.set(0, Math.PI, Math.PI);
      cameraAR.rotateY(-rotation.y);
      cameraAR.rotateX(rotation.x);
      cameraAR.rotateZ(-rotation.z);
    }
  };
}


const debugPoseRender = (domElementId) => {
  const floorDepth = 250;
  const gridSize = floorDepth * 2;
  const floorPositions = [0, -floorDepth, 0];
  const canvas = document.getElementById(domElementId);

  const dirLight1 = new THREE.DirectionalLight(0xffffff);
  dirLight1.position.set(1000, 1000, -1000);
  const dirLight2 = new THREE.DirectionalLight(0xffffff);
  dirLight2.position.set(-1000, -1000, -1000);
  const dirLight3 = new THREE.DirectionalLight(0xffffff);
  dirLight3.position.set(0, 0, 1000);
  const ambientLight = new THREE.AmbientLight(0x222222);

  const pos = [
    [0, floorDepth, 0],
    [0, -floorDepth, 0],
    [floorDepth, 0, 0],
    [-floorDepth, 0, 0],
    [0, 0, floorDepth],
    [0, 0, -floorDepth],
  ]
  const floors = [];
  for (let i = 0; i < 6; i++) {
      const floor = new THREE.GridHelper(gridSize, 5, 0x808080, 0x808080);
      floor.position.set(...pos[i]);
      if (i > 3) {
          floor.rotation.set(Math.PI / 2, 0, 0);
      } else if (i > 1) {
          floor.rotation.set(0, 0, Math.PI / 2);
      }
      floors.push(floor);
  }

  const markerCubes = getMarkerCubes();
  let scene = null, camera = null, controls = null, renderer = null, started = false;

  const start = () => {
    scene = new THREE.Scene();
    renderer = new THREE.WebGLRenderer({ canvas: canvas, alpha: true });
    renderer.setSize(canvas.offsetWidth, canvas.offsetHeight);

    scene.add(dirLight1);
    scene.add(dirLight2);
    scene.add(dirLight3);
    scene.add(ambientLight);
    floors.forEach(floor => scene.add(floor));
    markerCubes.forEach(markerCube => scene.add(markerCube));

    // scene.background = new THREE.Color(0xffffff);
    // scene.fog = new THREE.FogExp2(0xcccccc, 0.002);

    camera = new THREE.PerspectiveCamera(75, canvas.offsetWidth / canvas.offsetHeight, 0.1, 100000);
    camera.position.set(9, -12, -27);
    camera.up.set(0, -1, 0);
    camera.rotation.reorder( "YXZ" );
    scene.add(camera);
    
    // const axesHelper = new THREE.AxesHelper( 5 );
    // scene.add( axesHelper );

    controls = new TrackballControls(camera, renderer.domElement);

    controls.rotateSpeed = 1.0;
    controls.zoomSpeed = 1.2;
    controls.panSpeed = 0.8;

    controls.keys = ['KeyA', 'KeyS', 'KeyD'];
    started = true;

    window.addEventListener('resize', () => {
        camera.aspect = canvas.offsetWidth / canvas.offsetHeight;
        camera.updateProjectionMatrix();
        renderer.setSize(canvas.offsetWidth, canvas.offsetHeight);

        controls.handleResize();
    });
    function animate() {
      requestAnimationFrame(animate);
      if (started) {
        controls.update();
        renderer.render(scene, camera);
      }
    }
    animate();
  }

  const stop = () => {
    started = false;
  }

  let keyframeTrans = [], keyframeRots = [], latestTrans, latestRots;

  const cameraObj = ({current, origin}) => {
    const result = new THREE.Group();
    const group = new THREE.Group();
    //Origin is Red, current is Blue
    const color = current ? 0x0000FF : origin ? 0xFF0000 : 0x00FF00;

    const geoDims = [ [2, 1, 1], [1, 0.5, 2.5], [2, 1, 1], [1, 0.5, 2.5] ]
    const geometries = geoDims.map(dim => new THREE.BoxGeometry(...dim));

    const positions = [[0, 0.5, 0], [0, 0.25, 1], [0, -0.5, 0], [0, -0.25, 1]];

    const matTop = new THREE.MeshStandardMaterial({color: color, emissive: 0x072534, side: THREE.DoubleSide, flatShading: true});
    const matBottom = new THREE.MeshStandardMaterial({color: 0x8B8000, emissive: 0x072534, side: THREE.DoubleSide, flatShading: true});

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

  const keyFrameObjs = [];
  let originObj = null, currentObj = null;
  const CameraObjTypes = ['currentObj', 'originObj', 'keyFrameObj'];
  const update = () => {
    scene.children.forEach(child => {
      if (CameraObjTypes.includes(child.name)) scene.remove(child);
    });
    const setPose = (obj, trans, rots) => {
      obj.position.set(...trans);
      obj.rotation.set(0, 0, 0);
      obj.rotateY(rots.y);
      obj.rotateX(rots.x);
      obj.rotateZ(rots.z);
    }
  
    const allTrans = [...keyframeTrans, latestTrans];
    const allRots = [...keyframeRots, latestRots];
    for (const index in allTrans) {
      const trans = allTrans[index];
      const rots = allRots[index];
      if (trans) {
        let obj;
        if (index == allTrans.length-1) {
          if (currentObj == null) {
            obj = cameraObj({current: true, origin: false});
            obj.name = "currentObj";
            currentObj = obj;
          } else {
            obj = currentObj;
          }
        } else if (index == 0) {
          if (originObj == null) {
            obj = cameraObj({current: false, origin: true});
            obj.name = "originObj";
            originObj = obj;
          } else {
            obj = originObj;
          }
        } else {
          if (keyFrameObjs.length > 0) {
            obj = keyFrameObjs.pop();
          } else {
            obj = cameraObj({current: false, origin: false});
            obj.name = "keyFrameObj";
            keyFrameObjs.push(obj);
          }
        }
        scene.add(obj);
        setPose(obj, trans, rots);
      }
    }
  }

  return {
    updateKeyframes: (keyframeTransArg, keyframeRotsArg) => {
      keyframeTrans = keyframeTransArg;
      keyframeRots = keyframeRotsArg;
      update();
    },
    updateCurrentFrame: (trans, rots) => {
      latestTrans = trans;
      latestRots = rots;
      update();
    },
    start, 
    stop
  }
}

export { getRotation, syncDataWithServer, trim, arRender, debugPoseRender };