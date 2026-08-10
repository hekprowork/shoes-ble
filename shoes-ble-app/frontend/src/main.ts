import './style.css';
import Chart from 'chart.js/auto';
import * as THREE from 'three';
// @ts-ignore
import { StartBLE } from '../wailsjs/go/main/App';
// @ts-ignore
import { EventsOn } from '../wailsjs/runtime/runtime';

// 1. Setup UI
const connectBtn = document.getElementById('connectBtn') as HTMLButtonElement;
const statusDiv = document.getElementById('status') as HTMLDivElement;

connectBtn.onclick = () => {
    StartBLE();
};

EventsOn('ble_status', (status: string) => {
    statusDiv.innerText = `Status: ${status}`;
});

// 2. Setup Chart.js
const fsrCtx = (document.getElementById('fsrChart') as HTMLCanvasElement).getContext('2d');
const imuCtx = (document.getElementById('imuChart') as HTMLCanvasElement).getContext('2d');

const maxDataPoints = 100;
const labels = Array.from({ length: maxDataPoints }, (_, i) => i);

const fsrChart = new Chart(fsrCtx!, {
    type: 'line',
    data: {
        labels: labels,
        datasets: [
            { label: 'FSR 1', data: Array(maxDataPoints).fill(0), borderColor: 'red', tension: 0.1 },
            { label: 'FSR 2', data: Array(maxDataPoints).fill(0), borderColor: 'blue', tension: 0.1 }
        ]
    },
    options: { animation: false, responsive: true, maintainAspectRatio: false }
});

const imuChart = new Chart(imuCtx!, {
    type: 'line',
    data: {
        labels: labels,
        datasets: [
            { label: 'Acc X', data: Array(maxDataPoints).fill(0), borderColor: 'red', tension: 0.1 },
            { label: 'Acc Y', data: Array(maxDataPoints).fill(0), borderColor: 'green', tension: 0.1 },
            { label: 'Acc Z', data: Array(maxDataPoints).fill(0), borderColor: 'blue', tension: 0.1 },
            { label: 'Gyro X', data: Array(maxDataPoints).fill(0), borderColor: 'yellow', tension: 0.1 },
            { label: 'Gyro Y', data: Array(maxDataPoints).fill(0), borderColor: 'cyan', tension: 0.1 },
            { label: 'Gyro Z', data: Array(maxDataPoints).fill(0), borderColor: 'magenta', tension: 0.1 }
        ]
    },
    options: { animation: false, responsive: true, maintainAspectRatio: false }
});

EventsOn('sensor_data', (data: any) => {
    // Update FSR
    fsrChart.data.datasets[0].data.shift();
    fsrChart.data.datasets[0].data.push(data.fsr1);
    fsrChart.data.datasets[1].data.shift();
    fsrChart.data.datasets[1].data.push(data.fsr2);
    fsrChart.update();

    // Update IMU
    imuChart.data.datasets[0].data.shift(); imuChart.data.datasets[0].data.push(data.accX);
    imuChart.data.datasets[1].data.shift(); imuChart.data.datasets[1].data.push(data.accY);
    imuChart.data.datasets[2].data.shift(); imuChart.data.datasets[2].data.push(data.accZ);
    imuChart.data.datasets[3].data.shift(); imuChart.data.datasets[3].data.push(data.gyroX);
    imuChart.data.datasets[4].data.shift(); imuChart.data.datasets[4].data.push(data.gyroY);
    imuChart.data.datasets[5].data.shift(); imuChart.data.datasets[5].data.push(data.gyroZ);
    imuChart.update();
});

// 3. Setup Three.js
const threeContainer = document.getElementById('three-container')!;
const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(75, threeContainer.clientWidth / threeContainer.clientHeight, 0.1, 1000);
const renderer = new THREE.WebGLRenderer({ alpha: true });
renderer.setSize(threeContainer.clientWidth, threeContainer.clientHeight);
threeContainer.appendChild(renderer.domElement);

const geometry = new THREE.BoxGeometry(2, 0.5, 4);
const material = new THREE.MeshBasicMaterial({ color: 0x00ff00, wireframe: true });
const box = new THREE.Mesh(geometry, material);
scene.add(box);

const axesHelper = new THREE.AxesHelper( 5 );
scene.add( axesHelper );

camera.position.z = 6;
camera.position.y = 3;
camera.lookAt(0, 0, 0);

function animate() {
    requestAnimationFrame(animate);
    renderer.render(scene, camera);
}
animate();

EventsOn('quaternion_data', (data: any) => {
    // data.w, data.x, data.y, data.z
    const quaternion = new THREE.Quaternion(data.x, data.y, data.z, data.w);
    box.setRotationFromQuaternion(quaternion);
});
