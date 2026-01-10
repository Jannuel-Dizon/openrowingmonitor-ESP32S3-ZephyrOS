/* client/lib/app.js */
"use strict";
// import { default as NoSleep } from "nosleep.js";
import * as NoSleep from "nosleep.js";
import { BleClient } from "./BleClient.js";
import { filterObjectByKeys } from "./helper.js";

export function createApp(app) {
  // 1. Detect App Mode
  const urlParameters = new URLSearchParams(window.location.search);
  const mode = urlParameters.get("mode");
  const appMode = mode === "standalone" ? "STANDALONE" : "BROWSER";

  app.updateState({ ...app.getState(), appMode });

  // 2. Initialize BLE Client
  const bleClient = new BleClient((newState) => {
    // Merge BLE state updates (connected status, metrics) into App State
    const currentState = app.getState();
    const metrics = { ...currentState.metrics, ...newState.metrics };
    app.updateState({ ...currentState, ...newState, metrics });
  });

  requestWakeLock();

  // 3. New Action Handler (Connect is the main one)
  function handleAction(action) {
    switch (action.command) {
      case "connect":
        bleClient.connect();
        break;

      case "reset":
        // Since we are serverless, we just reset the local UI
        const blankMetrics = {
          strokesTotal: 0,
          distanceTotal: 0,
          caloriesTotal: 0,
          power: 0,
          strokesPerMinute: 0,
          splitFormatted: 0,
        };
        app.updateState({ ...app.getState(), metrics: blankMetrics });
        break;

      default:
        console.warn("Unknown action:", action);
    }
  }

  async function requestWakeLock() {
    const noSleep = new NoSleep();
    document.addEventListener(
      "click",
      function enableNoSleep() {
        document.removeEventListener("click", enableNoSleep, false);
        noSleep.enable();
      },
      false,
    );
  }

  return {
    handleAction,
  };
}
