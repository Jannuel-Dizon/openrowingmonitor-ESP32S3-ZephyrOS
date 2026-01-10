"use strict";
/*
  Open Rowing Monitor, https://github.com/laberning/openrowingmonitor

  Component that renders a metric of the dashboard
*/

import { AppElement, html, css } from "./AppElement.js";
import { customElement, property } from "lit";

export class DashboardMetric extends AppElement {
  static styles = css`
    .label,
    .content {
      padding: 0.1em 0;
    }

    .icon {
      height: 1.8em;
    }

    .metric-value {
      font-size: 150%;
    }

    .metric-unit {
      font-size: 80%;
    }

    ::slotted(*) {
      right: 0.2em;
      bottom: 0;
      position: absolute;
    }
  `;

  static get properties() {
    return {
      icon: { type: Object },
    };
  }

  static get properties() {
    return {
      unit: { type: String },
    };
  }

  static get properties() {
    return {
      value: { type: String },
    };
  }

  render() {
    return html`
      <div class="label">${this.icon}</div>
      <div class="content">
        <span class="metric-value"
          >${this.value !== undefined ? this.value : "--"}</span
        >
        <span class="metric-unit">${this.unit}</span>
      </div>
      <slot></slot>
    `;
  }
}
customElements.define("dashboard-metric", DashboardMetric);
