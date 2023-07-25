/**
 * @file test.js
 * @brief This contains the functionalities for generating test images.
 * @author Parikshit Basu
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
import * as THREE from 'three';
import { TrackballControls } from '/js/TrackballControls.js';

let phoneSimulator = null;
let deviceOrientationControls = "";
let orientText = "";
const trim = (value) => Math.round(value*100)/100;
const DeviceOrientationControls = function( object ) {

	var scope = this;

	this.object = object;
	this.object.rotation.reorder( "YXZ" );

	this.enabled = true;

	this.deviceOrientation = {};
	this.screenOrientation = 0;

	this.alpha = 0;
	this.alphaOffsetAngle = Math.PI/2;
	this.betaOffsetAngle = 0;
	this.gammaOffsetAngle = 0;

	var onDeviceOrientationChangeEvent = function( event ) {

		scope.deviceOrientation = event;
	};

	var onScreenOrientationChangeEvent = function() {

		scope.screenOrientation = window.orientation || 0;

	};

	// The angles alpha, beta and gamma form a set of intrinsic Tait-Bryan angles of type Z-X'-Y''

	var setObjectQuaternion = function() {

		var zee = new THREE.Vector3( 0, 0, 1 );

		var euler = new THREE.Euler();

		var q0 = new THREE.Quaternion();

		var q1 = new THREE.Quaternion( - Math.sqrt( 0.5 ), 0, 0, Math.sqrt( 0.5 ) ); // - PI/2 around the x-axis

		return function( quaternion, alpha, beta, gamma, orient ) {

			euler.set( beta, -alpha, gamma, 'YXZ' ); // 'ZXY' for the device, but 'YXZ' for us

			quaternion.setFromEuler( euler ); // orient the device

			quaternion.multiply( q1 ); // camera looks out the back of the device, not the top

			quaternion.multiply( q0.setFromAxisAngle( zee, - orient ) ); // adjust for screen orientation

		};

	}();

	this.connect = function() {

		onScreenOrientationChangeEvent(); // run once on load

		window.addEventListener( 'orientationchange', onScreenOrientationChangeEvent, false );
		window.addEventListener( 'deviceorientation', onDeviceOrientationChangeEvent, true );

		scope.enabled = true;

	};

	this.disconnect = function() {

		window.removeEventListener( 'orientationchange', onScreenOrientationChangeEvent, false );
		window.removeEventListener( 'deviceorientation', onDeviceOrientationChangeEvent, false );

		scope.enabled = false;

	};

	this.update = function() {

		if ( scope.enabled === false ) return;

		var alpha = scope.deviceOrientation.alpha ? THREE.Math.degToRad( scope.deviceOrientation.alpha ) + this.alphaOffsetAngle : 0; // Z
		var beta = scope.deviceOrientation.beta ? THREE.Math.degToRad( scope.deviceOrientation.beta ) + this.betaOffsetAngle : 0; // X'
		var gamma = scope.deviceOrientation.gamma ? THREE.Math.degToRad( scope.deviceOrientation.gamma ) + this.gammaOffsetAngle : 0; // Y''
		var orient = scope.screenOrientation ? THREE.Math.degToRad( scope.screenOrientation ) : 0; // O

		setObjectQuaternion( scope.object.quaternion, alpha, beta, gamma, orient );
    scope.object.rotateOnWorldAxis(new THREE.Vector3(0, 1, 0), 22/7);
    const rot = scope.object.rotation;
    console.log("Rotation noAR ", trim(rot.x*180/Math.PI), trim(rot.y*180/Math.PI), trim(rot.z*180/Math.PI));
    orientText = rot.x+","+rot.y+","+rot.z+","+rot.order;
    document.getElementById("orient").innerText = trim(rot.x)+", "+trim(rot.y)+", "+trim(rot.z)+", "+rot.order;
		this.alpha = alpha;

	};

	this.updateAlphaOffsetAngle = function( angle ) {

		this.alphaOffsetAngle = angle;
		this.update();

	};

	this.updateBetaOffsetAngle = function( angle ) {

		this.betaOffsetAngle = angle;
		this.update();

	};

	this.updateGammaOffsetAngle = function( angle ) {

		this.gammaOffsetAngle = angle;
		this.update();

	};

	this.dispose = function() {

		this.disconnect();

	};

	this.connect();

};

const cameraObj = (current, origin) => {
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
  group.rotateY(Math.PI);
  group.rotateZ(Math.PI);

  result.add(group);
  return result;
}

const createScene = () => {
  const floorDepth = 250;
  const gridSize = floorDepth * 2;
  const canvas = document.getElementById("canvas3D");
  let scene = new THREE.Scene();

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
  floor.position.set(...[0, -floorDepth, 0]);

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

  const cameraScene = new THREE.PerspectiveCamera(75, canvas.offsetWidth / canvas.offsetHeight, 0.1, 100000);
  cameraScene.position.set(0, 0, -10);
  cameraScene.lookAt(0, 0, 0);
  cameraScene.up.set(0, -1, 0);
  scene.add(cameraScene);

  const controls = new TrackballControls(cameraScene, renderer.domElement);

  controls.rotateSpeed = 1.0;
  controls.zoomSpeed = 1.2;
  controls.panSpeed = 0.8;

  controls.keys = ['KeyA', 'KeyS', 'KeyD'];

  window.addEventListener('resize', () => {
      cameraScene.aspect = canvas.offsetWidth / canvas.offsetHeight;
      cameraScene.updateProjectionMatrix();
      renderer.setSize(canvas.offsetWidth, canvas.offsetHeight);

      controls.handleResize();
  });

  console.log("Height and Width", canvas.offsetWidth, canvas.offsetHeight);

  phoneSimulator = cameraObj(true, true);
  phoneSimulator.position.set(0, 0, 0);
  scene.add(phoneSimulator);
  deviceOrientationControls = new DeviceOrientationControls(phoneSimulator);
  deviceOrientationControls.update();

  function animate() {
    controls.update();
    renderer.render(scene, cameraScene);
    requestAnimationFrame(animate);
  }
  animate();
}

createScene();



const getOrientation = () => {
  const orientation = window.orientation;
  {
    switch(orientation) {
      case 0: return 0;
      case 90: return 90;
      case 180: return 180;
      case -90: return -90;
      default: return 0;
    }
  }

  if (screen.orientation) {
    const type = screen.orientation.type;
    switch (type) {
      case "portrait-primary":
        console.log("Portrait Primary");
        return 0;
      case "landscape-primary":
        console.log("Landscape Primary.");
        return 90;
      case "portrait-secondary":
        console.log("Portrait Secondary");
        return 180;
      case "landscape-secondary":
        console.log("Landscape Secondary");
        return -90;
      default:
        console.log("The orientation API isn't supported in this browser :(");
        return 0;
    }
  }
  return 0;
}

let run = false;
let count = 0;
let camera;
const startVideo = (mediaStream) => {
  console.log("Got the web cam stream");
  const srcVideo = document.querySelector("#srcVideo");
  srcVideo.srcObject = mediaStream;
  const canvas = document.querySelector("#canvasVideo");
  const ctx = canvas.getContext("2d", { willReadFrequently: true });

  const sendImage = () => {
    const orientation = getOrientation();
    // console.log("Orientation is ", orientation);
    if (orientation == 90 || orientation == -90) {
      canvas.width = 640;
      canvas.height = 480;
    } else {
      canvas.width = 480;
      canvas.height = 640;
    }
    ctx.translate(canvas.width/2,canvas.height/2);
    ctx.rotate(orientation*Math.PI/180);
    ctx.drawImage(srcVideo, -canvas.width/2, -canvas.height/2, canvas.width, canvas.height);
    // const photo = canvas.toDataURL('image/png');

    canvas.toBlob(function(blob) {
      const formData = new FormData();
      formData.append('filetoupload', blob, 'image'+count+'.png');
      axios.post('/uploadForm', formData);

      const orientData = new FormData();
      const textblob = new Blob([orientText], {type : 'text/plain'})
      orientData.append('filetoupload', textblob, 'orient'+count+'.txt');
      axios.post('/uploadForm', orientData).then(() => {
        if (run) setTimeout(sendImage, 0);
      });
      count++;
    }, 'image/png', 0.7);
  };

  const stopVideo = () => {
    console.log("Stopping");
    if (run == false) return;
    run = false;
  };

  document.querySelector("#start").onclick = () => {
    console.log("Starting");
    if (run == true) return;
    run = true;
    sendImage();
    // setTimeout(stopVideo, 20000);
  };

  document.querySelector("#stop").onclick = stopVideo;
};

 const onReady = () => {
  // navigator.mediaDevices.getUserMedia({ video: true }).then((mediaStream) => {
  navigator.mediaDevices.getUserMedia(
    {audio: false, video: {facingMode:{exact: "environment"}}}
  ).then(startVideo)
  .catch((error) => {
    console.error("ERROR, could not start back facing camera. Trying front facing");
    navigator.mediaDevices.getUserMedia({ video: true }).then(startVideo);
  });
};

onReady();
// camera = new THREE.PerspectiveCamera(30, 480 / 640, 0.1, 1000);
// deviceOrientationControls = new DeviceOrientationControls(camera);

const handleOrientation = (event) => {
  if (deviceOrientationControls != "") {
    deviceOrientationControls.update();
  //   if (run) {
  //     // orient = trim(event.beta)+","+trim(event.gamma)+","+trim(event.alpha);
  //     // const quat = camera.quaternion;
  //     // orient = quat.w+","+quat.x+","+quat.y+","+quat.z;
  //     // console.log(orient);
  //   }
  }
}
window.addEventListener("deviceorientation", handleOrientation);
// window.addEventListener("devicemotion", handleAcceleration);