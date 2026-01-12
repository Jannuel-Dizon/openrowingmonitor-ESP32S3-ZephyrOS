// const { act } = require("react");
import { act } from "https://esm.sh/react@18";

const FTMS_SERVICE_UUID = 0x1826;
const ROWER_DATA_CHAR_UUID = 0x2ad1;

const connectBtn = document.getElementById("connectBtn");
const statusText = document.getElementById("status");

// UI Elements
const ui = {
  instPower: document.getElementById("instPower"),
  spm: document.getElementById("spm"),
  pace: document.getElementById("pace"),
  distance: document.getElementById("distance"),
  time: document.getElementById("time"),
  strokeCount: document.getElementById("strokeCount"),
};

connectBtn.addEventListener("click", async () => {
  try {
    statusText.innerText = "Requesting Bluetooth Device...";
    const device = await navigator.bluetooth.requestDevice({
      filters: [{ services: [FTMS_SERVICE_UUID] }],
    });

    statusText.innerText = "Connecting to GATT Server...";
    const server = await device.gatt.connect();

    statusText.innerText = "Getting Service...";
    const service = await server.getPrimaryService(FTMS_SERVICE_UUID);

    statusText.innerText = "Getting Characteristic...";
    const characteristic =
      await service.getCharacteristic(ROWER_DATA_CHAR_UUID);

    await characteristic.startNotifications();
    characteristic.addEventListener("characteristicvaluechanged", handleData);

    statusText.innerText = "Connected & Receiving Data";
    connectBtn.style.display = "none";

    device.addEventListener("gattserverdisconnected", () => {
      statusText.innerText = "Disconnected";
      connectBtn.style.display = "block";
    });
  } catch (error) {
    console.error(error);
    statusText.innerText = "Error: " + error.message;
  }
});

/*
let isRecording = false;
let recordingStartTime = 0;
let csvData = "Time_sec,Angular_Accel_RadS/2\n";

const recordBtn = document.getElementById("recordBtn");
const csvOutput = document.getElementById("csvOutput");
const copyBtn = document.getElementById("copyBtn");

recordBtn.addEventListener("click", () => {
  isRecording = !isRecording;
  if (isRecording) {
    recordBtn.innerText = "STOP RECORDING";
    recordBtn.style.backgroundColor = "#f44336";
    csvData = "Time_Sec,Angular_Accel_RadS2\n"; // Reset header
    recordingStartTime = Date.now();
  } else {
    recordBtn.innerText = "START RECORDING";
    recordBtn.style.backgroundColor = "#ff9800";
  }
});
copyBtn.addEventListener("click", () => {
  csvOutput.select();
  document.execCommand("copy");
  alert("Copied to clipboard!");
});
*/

function handleData(event) {
  const data = event.target.value; // DataView
  let cursor = 0;
  // 1. Flags (UINT16)
  const flags = data.getUint16(cursor, true);
  cursor += 2;

  // 2. Stroke Rate (UINT8) & Stroke Count (UINT16)
  // Note: Bit 0 is NOT set in your code, so these are present
  const instSpm = data.getUint8(cursor) / 2.0;
  cursor += 1;
  const strokeCount = data.getUint16(cursor, true);
  cursor += 2;

  // 3. Avg Stroke Rate (UINT8) - Bit 1
  const avgSpm = data.getUint8(cursor) / 2.0;
  cursor += 1;

  // 4. Total Distance (UINT24) - Bit 2
  // DataView doesn't have getUint24, so we manually parse
  const distance =
    data.getUint8(cursor) |
    (data.getUint8(cursor + 1) << 8) |
    (data.getUint8(cursor + 2) << 16);
  cursor += 3;

  // 5. Inst Pace (UINT16) - Bit 3
  const instPaceSecs = data.getUint16(cursor, true);
  cursor += 2;

  // 6. Avg Pace (UINT16) - Bit 4
  const avgPaceSecs = data.getUint16(cursor, true);
  cursor += 2;

  // 7. Inst Power (SINT16) - Bit 5
  const instPower = data.getInt16(cursor, true);
  cursor += 2;

  // 8. Avg Power (SINT16) - Bit 6
  const avgPower = data.getInt16(cursor, true);
  cursor += 2;

  // 9. Elapsed Time (UINT16) - Bit 11
  const elapsedTime = data.getUint16(cursor, true);
  cursor += 2;

  // Update UI
  ui.instPower.innerText = instPower;
  ui.spm.innerText = instSpm.toFixed(1);
  ui.distance.innerText = distance;
  ui.strokeCount.innerText = strokeCount;
  ui.time.innerText = formatTime(elapsedTime);
  ui.pace.innerText = formatTime(instPaceSecs);
  /*
  // Recording Logic
  if (isRecording) {
    const timeOffset = ((Date.now() - recordingStartTime) / 1000).toFixed(3);
    csvData += `${timeOffset},${actualAccel.toFixed(3)}\n`;
    csvOutput.value = csvData; // Real-time update
  }
  */
}

function formatTime(seconds) {
  if (seconds === 65535 || seconds === 0) return "0:00";
  const mins = Math.floor(seconds / 60);
  const secs = seconds % 60;
  return `${mins}:${secs.toString().padStart(2, "0")}`;
}
