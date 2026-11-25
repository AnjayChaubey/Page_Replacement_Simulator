/**
 * API Client for Page Replacement Simulator C++ Backend
 */
const API = (() => {
  // Determine backend base URL: if loaded from the C++ server, use relative path; otherwise default to localhost:8080
  const isDirectFile = window.location.protocol === 'file:';
  const isDifferentOrigin = !window.location.origin.includes(':8080') && window.location.hostname !== '';
  const BASE_URL = (isDirectFile || isDifferentOrigin) ? 'http://127.0.0.1:8080' : '';

  async function request(endpoint, options = {}) {
    const url = `${BASE_URL}${endpoint}`;
    const defaultHeaders = {
      'Content-Type': 'application/json',
      'Accept': 'application/json'
    };

    try {
      const response = await fetch(url, {
        ...options,
        headers: { ...defaultHeaders, ...options.headers }
      });

      if (!response.ok) {
        let errorMsg = `Server error (${response.status})`;
        try {
          const errData = await response.json();
          if (errData.error) errorMsg = errData.error;
        } catch (_) {}
        throw new Error(errorMsg);
      }

      return await response.json();
    } catch (err) {
      console.error(`[API Error] ${endpoint}:`, err);
      throw err;
    }
  }

  return {
    async checkHealth() {
      return await request('/api/health');
    },

    async runSimulation(algorithm, frames, referenceString) {
      return await request('/api/simulate', {
        method: 'POST',
        body: JSON.stringify({ algorithm, frames, referenceString })
      });
    },

    async runComparison(frames, referenceString) {
      return await request('/api/compare', {
        method: 'POST',
        body: JSON.stringify({ frames, referenceString })
      });
    },

    async generateWorkload(workloadType, count, maxPageId, seed = null) {
      return await request('/api/generate-workload', {
        method: 'POST',
        body: JSON.stringify({ workloadType, count, maxPageId, seed })
      });
    },

    async runWorkloadAnalysis(workloadType, referenceString, frameCounts) {
      return await request('/api/workload-analysis', {
        method: 'POST',
        body: JSON.stringify({ workloadType, referenceString, frameCounts })
      });
    },

    async runBeladyExperiment(referenceString, frame1 = 3, frame2 = 4) {
      return await request('/api/belady-experiment', {
        method: 'POST',
        body: JSON.stringify({ referenceString, frame1, frame2 })
      });
    }
  };
})();
