/**
 * Chart.js Manager for Page Replacement Simulator
 */
const ChartManager = (() => {
  const chartInstances = {};

  const ALGO_COLORS = {
    'FIFO': { main: '#6366F1', light: 'rgba(99, 102, 241, 0.2)' },
    'LRU': { main: '#06B6D4', light: 'rgba(6, 182, 212, 0.2)' },
    'Optimal': { main: '#F59E0B', light: 'rgba(245, 158, 11, 0.2)' },
    'LFU': { main: '#A855F7', light: 'rgba(168, 85, 247, 0.2)' },
    'SecondChance': { main: '#22C55E', light: 'rgba(34, 197, 94, 0.2)' }
  };

  const DEFAULT_OPTIONS = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: {
        labels: {
          color: '#94A3B8',
          font: { family: '-apple-system, BlinkMacSystemFont, Segoe UI', size: 12 }
        }
      },
      tooltip: {
        backgroundColor: '#172033',
        titleColor: '#F8FAFC',
        bodyColor: '#CBD5E1',
        borderColor: 'rgba(99, 102, 241, 0.3)',
        borderWidth: 1,
        padding: 10
      }
    },
    scales: {
      x: {
        grid: { color: 'rgba(255, 255, 255, 0.05)' },
        ticks: { color: '#94A3B8', font: { size: 11 } }
      },
      y: {
        grid: { color: 'rgba(255, 255, 255, 0.05)' },
        ticks: { color: '#94A3B8', font: { size: 11 } },
        beginAtZero: true
      }
    }
  };

  function destroyChart(canvasId) {
    if (chartInstances[canvasId]) {
      chartInstances[canvasId].destroy();
      delete chartInstances[canvasId];
    }
  }

  return {
    renderDonutChart(canvasId, hits, faults) {
      destroyChart(canvasId);
      const canvas = document.getElementById(canvasId);
      if (!canvas) return;

      const total = hits + faults;
      const hitPercent = total > 0 ? ((hits / total) * 100).toFixed(1) : '0';
      const faultPercent = total > 0 ? ((faults / total) * 100).toFixed(1) : '0';

      chartInstances[canvasId] = new Chart(canvas, {
        type: 'doughnut',
        data: {
          labels: [`Hits (${hitPercent}%)`, `Faults (${faultPercent}%)`],
          datasets: [{
            data: [hits, faults],
            backgroundColor: ['#22C55E', '#EF4444'],
            borderColor: '#172033',
            borderWidth: 3,
            hoverOffset: 6
          }]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          cutout: '70%',
          plugins: {
            legend: {
              position: 'bottom',
              labels: { color: '#94A3B8', font: { size: 12 }, padding: 16 }
            },
            tooltip: {
              backgroundColor: '#172033',
              titleColor: '#F8FAFC',
              bodyColor: '#CBD5E1',
              borderColor: 'rgba(255, 255, 255, 0.1)',
              borderWidth: 1
            }
          }
        }
      });
    },

    renderComparisonCharts(results) {
      const labels = results.map(r => r.algorithm);
      const faultsData = results.map(r => r.pageFaults);
      const hitsData = results.map(r => r.pageHits);
      const faultRateData = results.map(r => Number(r.faultRatio.toFixed(2)));
      const hitRatioData = results.map(r => Number(r.hitRatio.toFixed(2)));
      const bgColors = labels.map(l => (ALGO_COLORS[l] ? ALGO_COLORS[l].main : '#6366F1'));

      // 1. Page Fault Comparison Bar Chart
      destroyChart('chartFaultsCompare');
      const ctxFaults = document.getElementById('chartFaultsCompare');
      if (ctxFaults) {
        chartInstances['chartFaultsCompare'] = new Chart(ctxFaults, {
          type: 'bar',
          data: {
            labels,
            datasets: [{
              label: 'Page Faults (Lower is Better)',
              data: faultsData,
              backgroundColor: bgColors,
              borderRadius: 6
            }]
          },
          options: {
            ...DEFAULT_OPTIONS,
            plugins: { ...DEFAULT_OPTIONS.plugins, legend: { display: false } }
          }
        });
      }

      // 2. Page Hit Comparison Bar Chart
      destroyChart('chartHitsCompare');
      const ctxHits = document.getElementById('chartHitsCompare');
      if (ctxHits) {
        chartInstances['chartHitsCompare'] = new Chart(ctxHits, {
          type: 'bar',
          data: {
            labels,
            datasets: [{
              label: 'Page Hits (Higher is Better)',
              data: hitsData,
              backgroundColor: bgColors,
              borderRadius: 6
            }]
          },
          options: {
            ...DEFAULT_OPTIONS,
            plugins: { ...DEFAULT_OPTIONS.plugins, legend: { display: false } }
          }
        });
      }

      // 3. Page Fault Rate (%)
      destroyChart('chartFaultRateCompare');
      const ctxFaultRate = document.getElementById('chartFaultRateCompare');
      if (ctxFaultRate) {
        chartInstances['chartFaultRateCompare'] = new Chart(ctxFaultRate, {
          type: 'bar',
          data: {
            labels,
            datasets: [{
              label: 'Fault Rate %',
              data: faultRateData,
              backgroundColor: '#EF4444',
              borderRadius: 6
            }]
          },
          options: {
            ...DEFAULT_OPTIONS,
            plugins: { ...DEFAULT_OPTIONS.plugins, legend: { display: false } },
            scales: {
              ...DEFAULT_OPTIONS.scales,
              y: { ...DEFAULT_OPTIONS.scales.y, max: 100 }
            }
          }
        });
      }

      // 4. Hit Ratio (%)
      destroyChart('chartHitRatioCompare');
      const ctxHitRatio = document.getElementById('chartHitRatioCompare');
      if (ctxHitRatio) {
        chartInstances['chartHitRatioCompare'] = new Chart(ctxHitRatio, {
          type: 'bar',
          data: {
            labels,
            datasets: [{
              label: 'Hit Ratio %',
              data: hitRatioData,
              backgroundColor: '#22C55E',
              borderRadius: 6
            }]
          },
          options: {
            ...DEFAULT_OPTIONS,
            plugins: { ...DEFAULT_OPTIONS.plugins, legend: { display: false } },
            scales: {
              ...DEFAULT_OPTIONS.scales,
              y: { ...DEFAULT_OPTIONS.scales.y, max: 100 }
            }
          }
        });
      }
    },

    renderWorkloadCharts(analysisData) {
      const frameCounts = analysisData.frameCountsTested;
      const algos = ['FIFO', 'LRU', 'Optimal', 'LFU', 'SecondChance'];

      // Datasets for Faults vs Frame Count
      const faultDatasets = algos.map(algo => ({
        label: algo,
        data: analysisData.dataPoints.map(pt => pt.algorithmFaults[algo]),
        borderColor: ALGO_COLORS[algo] ? ALGO_COLORS[algo].main : '#6366F1',
        backgroundColor: ALGO_COLORS[algo] ? ALGO_COLORS[algo].light : 'rgba(99, 102, 241, 0.1)',
        tension: 0.3,
        fill: false,
        pointRadius: 4,
        pointHoverRadius: 6
      }));

      destroyChart('chartWorkloadFaults');
      const ctxFaults = document.getElementById('chartWorkloadFaults');
      if (ctxFaults) {
        chartInstances['chartWorkloadFaults'] = new Chart(ctxFaults, {
          type: 'line',
          data: {
            labels: frameCounts.map(f => `${f} Frames`),
            datasets: faultDatasets
          },
          options: {
            ...DEFAULT_OPTIONS,
            scales: {
              ...DEFAULT_OPTIONS.scales,
              x: { ...DEFAULT_OPTIONS.scales.x, title: { display: true, text: 'Memory Frames', color: '#94A3B8' } },
              y: { ...DEFAULT_OPTIONS.scales.y, title: { display: true, text: 'Page Faults', color: '#94A3B8' } }
            }
          }
        });
      }

      // Datasets for Hit Ratio vs Frame Count
      const hitDatasets = algos.map(algo => ({
        label: algo,
        data: analysisData.dataPoints.map(pt => pt.algorithmHitRatios[algo]),
        borderColor: ALGO_COLORS[algo] ? ALGO_COLORS[algo].main : '#6366F1',
        backgroundColor: ALGO_COLORS[algo] ? ALGO_COLORS[algo].light : 'rgba(99, 102, 241, 0.1)',
        tension: 0.3,
        fill: false,
        pointRadius: 4,
        pointHoverRadius: 6
      }));

      destroyChart('chartWorkloadHits');
      const ctxHits = document.getElementById('chartWorkloadHits');
      if (ctxHits) {
        chartInstances['chartWorkloadHits'] = new Chart(ctxHits, {
          type: 'line',
          data: {
            labels: frameCounts.map(f => `${f} Frames`),
            datasets: hitDatasets
          },
          options: {
            ...DEFAULT_OPTIONS,
            scales: {
              ...DEFAULT_OPTIONS.scales,
              x: { ...DEFAULT_OPTIONS.scales.x, title: { display: true, text: 'Memory Frames', color: '#94A3B8' } },
              y: { ...DEFAULT_OPTIONS.scales.y, title: { display: true, text: 'Hit Ratio (%)', color: '#94A3B8' }, max: 100 }
            }
          }
        });
      }
    },

    renderBeladyChart(beladyData) {
      destroyChart('chartBeladyComparison');
      const ctx = document.getElementById('chartBeladyComparison');
      if (!ctx) return;

      const f1 = beladyData.frame1;
      const f2 = beladyData.frame2;

      chartInstances['chartBeladyComparison'] = new Chart(ctx, {
        type: 'bar',
        data: {
          labels: ['FIFO', 'LRU (Stack)', 'Optimal (Stack)'],
          datasets: [
            {
              label: `${f1} Frames`,
              data: [
                beladyData.fifoResult1.pageFaults,
                beladyData.lruResult1.pageFaults,
                beladyData.optimalResult1.pageFaults
              ],
              backgroundColor: '#6366F1',
              borderRadius: 6
            },
            {
              label: `${f2} Frames`,
              data: [
                beladyData.fifoResult2.pageFaults,
                beladyData.lruResult2.pageFaults,
                beladyData.optimalResult2.pageFaults
              ],
              backgroundColor: '#06B6D4',
              borderRadius: 6
            }
          ]
        },
        options: {
          ...DEFAULT_OPTIONS,
          scales: {
            ...DEFAULT_OPTIONS.scales,
            y: { ...DEFAULT_OPTIONS.scales.y, title: { display: true, text: 'Page Faults', color: '#94A3B8' } }
          }
        }
      });
    }
  };
})();
