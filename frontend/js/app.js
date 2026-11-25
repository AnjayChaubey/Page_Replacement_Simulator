/**
 * Main Application Orchestrator for Page Replacement Simulator
 */
document.addEventListener('DOMContentLoaded', () => {
  // Navigation elements
  const navLinks = document.querySelectorAll('.nav-link');
  const sections = document.querySelectorAll('.view-section');
  const statusDot = document.getElementById('backendStatusDot');
  const statusText = document.getElementById('backendStatusText');

  // Input elements
  const inputFrames = document.getElementById('inputFrames');
  const inputRefString = document.getElementById('inputRefString');
  const selectAlgo = document.getElementById('selectAlgo');
  const selectPreset = document.getElementById('selectPreset');

  // Stored state
  let lastComparisonData = null;

  // 1. Navigation switching
  function switchView(targetViewId) {
    sections.forEach(sec => sec.classList.remove('active'));
    navLinks.forEach(link => link.classList.remove('active'));

    const activeSec = document.getElementById(targetViewId);
    if (activeSec) activeSec.classList.add('active');

    const activeLink = document.querySelector(`.nav-link[data-target="${targetViewId}"]`);
    if (activeLink) activeLink.classList.add('active');

    // Section-specific refresh triggers
    if (targetViewId === 'view-dashboard') {
      updateDashboardStats();
    } else if (targetViewId === 'view-history') {
      renderHistoryTable();
    }

    // Scroll to top of content
    window.scrollTo({ top: 0, behavior: 'smooth' });
  }

  navLinks.forEach(link => {
    link.addEventListener('click', (e) => {
      e.preventDefault();
      const target = link.dataset.target;
      if (target) switchView(target);

      // Close mobile sidebar if open
      const sidebar = document.querySelector('.sidebar');
      if (sidebar) sidebar.classList.remove('open');
    });
  });

  // Mobile menu toggle
  const mobileToggle = document.getElementById('mobileMenuToggle');
  if (mobileToggle) {
    mobileToggle.addEventListener('click', () => {
      const sidebar = document.querySelector('.sidebar');
      if (sidebar) sidebar.classList.toggle('open');
    });
  }

  // Quick Action Buttons on Dashboard
  const btnQuickNewSim = document.getElementById('btnQuickNewSim');
  if (btnQuickNewSim) btnQuickNewSim.addEventListener('click', () => switchView('view-simulator'));

  const btnQuickCompare = document.getElementById('btnQuickCompare');
  if (btnQuickCompare) btnQuickCompare.addEventListener('click', () => {
    switchView('view-comparison');
    runComparisonFromInputs();
  });

  const btnQuickExplore = document.getElementById('btnQuickExplore');
  if (btnQuickExplore) btnQuickExplore.addEventListener('click', () => switchView('view-learning'));

  // 2. Reference string parsing and validation
  function parseReferenceString(rawStr) {
    if (!rawStr || typeof rawStr !== 'string') return [];
    // Split on commas, whitespace, or semicolons
    const tokens = rawStr.split(/[\s,;]+/).filter(t => t.trim().length > 0);
    const result = [];

    for (const t of tokens) {
      const num = Number(t);
      if (isNaN(num) || !Number.isInteger(num)) {
        throw new Error(`Invalid page reference: "${t}". Must be an integer.`);
      }
      if (num < 0) {
        throw new Error(`Negative page ID not allowed: ${num}. Page IDs must be >= 0.`);
      }
      result.push(num);
    }

    return result;
  }

  // 3. Preset references loader
  const PRESETS = {
    'standard': [7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2],
    'belady_classic': [1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5],
    'sequential': [0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5],
    'repeated': [2, 3, 2, 1, 5, 2, 4, 5, 3, 2, 5, 2],
    'locality': [1, 2, 1, 3, 2, 1, 2, 8, 9, 8, 10, 8, 9, 1, 2, 3]
  };

  if (selectPreset) {
    selectPreset.addEventListener('change', async (e) => {
      const val = e.target.value;
      if (PRESETS[val]) {
        inputRefString.value = PRESETS[val].join(', ');
        showToast('Preset loaded successfully', 'info');
      } else if (val === 'random' || val === 'high_diversity') {
        try {
          const count = 20;
          const maxId = val === 'high_diversity' ? 18 : 7;
          const gen = await API.generateWorkload(val, count, maxId);
          inputRefString.value = gen.referenceString.join(', ');
          showToast(`Generated ${val} workload (${count} references)`, 'info');
        } catch (err) {
          showToast('Failed to generate workload: ' + err.message, 'error');
        }
      }
    });
  }

  // Quick preset chips buttons
  document.querySelectorAll('.preset-chip').forEach(chip => {
    chip.addEventListener('click', () => {
      const presetKey = chip.dataset.preset;
      if (PRESETS[presetKey]) {
        inputRefString.value = PRESETS[presetKey].join(', ');
        if (presetKey === 'standard') inputFrames.value = 3;
        showToast(`Loaded ${chip.textContent.trim()}`, 'info');
      }
    });
  });

  // Random workload generator button
  const btnRandomGen = document.getElementById('btnGenerateRandom');
  if (btnRandomGen) {
    btnRandomGen.addEventListener('click', async () => {
      try {
        const count = parseInt(document.getElementById('genCount')?.value || '20', 10);
        const maxId = parseInt(document.getElementById('genMaxPage')?.value || '7', 10);
        const seedVal = document.getElementById('genSeed')?.value;
        const seed = seedVal ? parseInt(seedVal, 10) : null;
        const type = document.getElementById('genWorkloadType')?.value || 'random';

        const gen = await API.generateWorkload(type, count, maxId, seed);
        inputRefString.value = gen.referenceString.join(', ');
        showToast(`Generated ${gen.referenceString.length} page references!`, 'success');
      } catch (err) {
        showToast('Workload generation failed: ' + err.message, 'error');
      }
    });
  }

  // 4. Run Simulation
  const btnStartSim = document.getElementById('btnStartSimulation');
  if (btnStartSim) {
    btnStartSim.addEventListener('click', async () => {
      await executeSimulation();
    });
  }

  async function executeSimulation() {
    let frames = parseInt(inputFrames.value, 10);
    if (isNaN(frames) || frames < 1 || frames > 64) {
      showToast('Frame count must be an integer between 1 and 64', 'error');
      inputFrames.focus();
      return;
    }

    let refString = [];
    try {
      refString = parseReferenceString(inputRefString.value);
    } catch (err) {
      showToast(err.message, 'error');
      inputRefString.focus();
      return;
    }

    if (refString.length === 0) {
      showToast('Please enter at least one page reference', 'error');
      inputRefString.focus();
      return;
    }

    const algo = selectAlgo.value;

    btnStartSim.disabled = true;
    btnStartSim.innerHTML = `<i data-lucide="loader-2" class="spin"></i> Simulating in C++...`;
    if (window.lucide) window.lucide.createIcons();

    try {
      const result = await API.runSimulation(algo, frames, refString);

      // Save to history
      HistoryManager.save(result);

      // Load into live simulation stage
      SimulationController.loadSimulation(result);

      // Update active algorithm badge
      const activeAlgoBadge = document.getElementById('currentSimAlgoBadge');
      if (activeAlgoBadge) {
        activeAlgoBadge.textContent = `${result.algorithmFullName} (${result.frameCount} Frames)`;
      }

      showToast(`Simulation complete: ${result.pageFaults} faults, ${result.pageHits} hits (${result.hitRatio.toFixed(1)}% hit ratio)`, 'success');
      updateDashboardStats();
    } catch (err) {
      showToast(`Simulation failed: ${err.message}`, 'error');
    } finally {
      btnStartSim.disabled = false;
      btnStartSim.innerHTML = `<i data-lucide="play"></i> Run Simulation`;
      if (window.lucide) window.lucide.createIcons();
    }
  }

  // 5. Stepper playback buttons
  document.getElementById('btnPlayPause')?.addEventListener('click', () => SimulationController.togglePlay());
  document.getElementById('btnNextStep')?.addEventListener('click', () => SimulationController.nextStep());
  document.getElementById('btnPrevStep')?.addEventListener('click', () => SimulationController.prevStep());
  document.getElementById('btnResetSim')?.addEventListener('click', () => SimulationController.reset());
  document.getElementById('btnRunAll')?.addEventListener('click', () => SimulationController.runAll());

  // Speed buttons
  document.querySelectorAll('.speed-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      const spd = parseFloat(btn.dataset.speed);
      SimulationController.setSpeed(spd);
    });
  });

  // Export current simulation CSV button
  document.getElementById('btnExportSimCSV')?.addEventListener('click', () => {
    const cur = SimulationController.getCurrentResult();
    if (!cur) {
      showToast('No active simulation to export', 'error');
      return;
    }
    HistoryManager.exportSimulationCSV(cur);
    showToast('Simulation CSV downloaded!', 'success');
  });

  // 6. Algorithm Comparison Module
  const btnRunComparison = document.getElementById('btnRunComparison');
  if (btnRunComparison) {
    btnRunComparison.addEventListener('click', () => runComparisonFromInputs());
  }

  async function runComparisonFromInputs() {
    const compFramesInput = document.getElementById('compFrames') || inputFrames;
    const compRefInput = document.getElementById('compRefString') || inputRefString;

    let frames = parseInt(compFramesInput.value, 10);
    if (isNaN(frames) || frames < 1) frames = 3;

    let refString = [];
    try {
      refString = parseReferenceString(compRefInput.value || inputRefString.value);
    } catch (err) {
      showToast(err.message, 'error');
      return;
    }

    if (refString.length === 0) {
      refString = PRESETS.standard;
      compRefInput.value = refString.join(', ');
    }

    if (btnRunComparison) {
      btnRunComparison.disabled = true;
      btnRunComparison.innerHTML = `<i data-lucide="loader-2" class="spin"></i> Computing 5 Algorithms in C++...`;
      if (window.lucide) window.lucide.createIcons();
    }

    try {
      const comp = await API.runComparison(frames, refString);
      lastComparisonData = comp;
      renderComparisonView(comp);
      showToast(`Compared all 5 algorithms on ${frames} frames!`, 'success');
    } catch (err) {
      showToast('Comparison failed: ' + err.message, 'error');
    } finally {
      if (btnRunComparison) {
        btnRunComparison.disabled = false;
        btnRunComparison.innerHTML = `<i data-lucide="bar-chart-2"></i> Compare All Algorithms`;
        if (window.lucide) window.lucide.createIcons();
      }
    }
  }

  function renderComparisonView(comp) {
    const tbody = document.getElementById('comparisonTableBody');
    if (tbody) {
      tbody.innerHTML = '';
      comp.results.forEach(r => {
        const tr = document.createElement('tr');
        const isBest = (r.algorithm === comp.bestAlgorithm);
        if (isBest) tr.classList.add('best-row');

        tr.innerHTML = `
          <td>
            <strong>${r.algorithm}</strong>
            <div style="font-size: 0.75rem; color: var(--text-muted);">${r.algorithmFullName}</div>
            ${isBest ? `<span class="best-badge"><i data-lucide="award"></i> Best Online</span>` : ''}
          </td>
          <td><span style="color: var(--color-success); font-weight: 700;">${r.pageHits}</span></td>
          <td><span style="color: var(--color-error); font-weight: 700;">${r.pageFaults}</span></td>
          <td><strong>${r.hitRatio.toFixed(1)}%</strong></td>
          <td>${r.faultRatio.toFixed(1)}%</td>
          <td>${r.replacements}</td>
          <td>${r.freeAllocations}</td>
          <td style="font-size: 0.78rem; color: var(--text-secondary); max-width: 240px;">${r.tieBreakerPolicy}</td>
        `;
        tbody.appendChild(tr);
      });
    }

    // Theoretical Note Banner
    const noteEl = document.getElementById('compTheoreticalNote');
    if (noteEl) {
      noteEl.innerHTML = `
        <div style="display: flex; gap: 12px; align-items: flex-start;">
          <i data-lucide="info" style="color: var(--accent-secondary); margin-top: 2px;"></i>
          <div>
            <strong>Theoretical Benchmark Note:</strong> ${comp.theoreticalNote}
          </div>
        </div>
      `;
    }

    // Render 4 Comparison Charts
    ChartManager.renderComparisonCharts(comp.results);

    if (window.lucide) window.lucide.createIcons();
  }

  // Export Comparison CSV Button
  document.getElementById('btnExportCompCSV')?.addEventListener('click', () => {
    if (!lastComparisonData) {
      showToast('No comparison data to export. Run a comparison first.', 'error');
      return;
    }
    HistoryManager.exportComparisonCSV(lastComparisonData);
    showToast('Comparison report CSV downloaded!', 'success');
  });

  // 7. Workload Analysis Runner
  const btnRunWorkload = document.getElementById('btnRunWorkloadAnalysis');
  if (btnRunWorkload) {
    btnRunWorkload.addEventListener('click', async () => {
      const type = document.getElementById('selectAnalysisWorkload')?.value || 'locality';
      const frameCounts = [2, 3, 4, 5, 6, 7];

      btnRunWorkload.disabled = true;
      btnRunWorkload.innerHTML = `<i data-lucide="loader-2" class="spin"></i> Analyzing Workload in C++...`;
      if (window.lucide) window.lucide.createIcons();

      try {
        const analysis = await API.runWorkloadAnalysis(type, [], frameCounts);

        // Render charts
        ChartManager.renderWorkloadCharts(analysis);

        // Update summary text
        const summaryEl = document.getElementById('workloadAnalysisSummary');
        if (summaryEl) {
          summaryEl.textContent = analysis.analysisSummary;
        }

        showToast(`Workload analysis complete for "${type}"!`, 'success');
      } catch (err) {
        showToast('Workload analysis failed: ' + err.message, 'error');
      } finally {
        btnRunWorkload.disabled = false;
        btnRunWorkload.innerHTML = `<i data-lucide="activity"></i> Run Workload Experiment`;
        if (window.lucide) window.lucide.createIcons();
      }
    });
  }

  // 8. Belady's Anomaly Runner
  const btnRunBelady = document.getElementById('btnRunBelady');
  if (btnRunBelady) {
    btnRunBelady.addEventListener('click', async () => {
      const f1 = parseInt(document.getElementById('beladyFrame1')?.value || '3', 10);
      const f2 = parseInt(document.getElementById('beladyFrame2')?.value || '4', 10);
      const customRef = document.getElementById('beladyCustomString')?.value;

      let refString = [];
      if (customRef && customRef.trim()) {
        try {
          refString = parseReferenceString(customRef);
        } catch (e) {
          showToast(e.message, 'error');
          return;
        }
      } else {
        refString = PRESETS.belady_classic;
      }

      btnRunBelady.disabled = true;
      btnRunBelady.innerHTML = `<i data-lucide="loader-2" class="spin"></i> Running Experiment in C++...`;
      if (window.lucide) window.lucide.createIcons();

      try {
        const belady = await API.runBeladyExperiment(refString, f1, f2);

        // Update banner
        const banner = document.getElementById('beladyAlertBanner');
        if (banner) {
          banner.className = `anomaly-alert-banner ${belady.anomalyOccurred ? 'anomaly-true' : 'anomaly-false'}`;
          banner.innerHTML = `
            <i data-lucide="${belady.anomalyOccurred ? 'alert-triangle' : 'check-circle-2'}"></i>
            <div>${belady.explanation}</div>
          `;
        }

        // Render chart
        ChartManager.renderBeladyChart(belady);

        // Render summary cards
        document.getElementById('beladyFifo1Val').textContent = `${belady.fifoResult1.pageFaults} faults (${belady.frame1} frames)`;
        document.getElementById('beladyFifo2Val').textContent = `${belady.fifoResult2.pageFaults} faults (${belady.frame2} frames)`;
        document.getElementById('beladyLru1Val').textContent = `${belady.lruResult1.pageFaults} faults`;
        document.getElementById('beladyLru2Val').textContent = `${belady.lruResult2.pageFaults} faults`;

        showToast(belady.anomalyOccurred ? 'Belady Anomaly Confirmed!' : 'Experiment evaluated successfully.', 'info');
      } catch (err) {
        showToast('Belady experiment failed: ' + err.message, 'error');
      } finally {
        btnRunBelady.disabled = false;
        btnRunBelady.innerHTML = `<i data-lucide="flask-conical"></i> Run Anomaly Test`;
        if (window.lucide) window.lucide.createIcons();
      }
    });
  }

  // 9. Dashboard statistics updater
  function updateDashboardStats() {
    const stats = HistoryManager.getOverallStats();
    const setText = (id, val) => {
      const el = document.getElementById(id);
      if (el) el.textContent = val;
    };

    setText('dashTotalSims', stats.totalSimulations);
    setText('dashTotalRefs', stats.totalReferences);
    setText('dashBestAlgo', stats.bestAlgorithm);
    setText('dashAvgFaultRate', `${stats.avgFaultRate}%`);

    // Render recent 5 simulations on dashboard
    const recentTable = document.getElementById('dashboardRecentTableBody');
    if (recentTable) {
      const history = HistoryManager.getHistory().slice(0, 5);
      recentTable.innerHTML = '';
      if (history.length === 0) {
        recentTable.innerHTML = `<tr><td colspan="6" style="text-align: center; color: var(--text-muted); padding: 24px;">No simulations run yet. Click "Start New Simulation" above!</td></tr>`;
      } else {
        history.forEach(item => {
          const tr = document.createElement('tr');
          tr.innerHTML = `
            <td>${item.date}</td>
            <td><strong>${item.algorithm}</strong></td>
            <td>${item.frameCount}</td>
            <td>${item.totalReferences}</td>
            <td><span style="color: var(--color-error); font-weight: 700;">${item.pageFaults}</span></td>
            <td>
              <button class="btn btn-secondary btn-sm" onclick="appReopenSimulation('${item.id}')">
                <i data-lucide="eye"></i> View
              </button>
            </td>
          `;
          recentTable.appendChild(tr);
        });
      }
    }
    if (window.lucide) window.lucide.createIcons();
  }

  // 10. History Table Renderer
  function renderHistoryTable() {
    const tbody = document.getElementById('historyTableBody');
    if (!tbody) return;

    const history = HistoryManager.getHistory();
    tbody.innerHTML = '';

    if (history.length === 0) {
      tbody.innerHTML = `<tr><td colspan="8" style="text-align: center; color: var(--text-muted); padding: 32px;">Simulation history is empty.</td></tr>`;
      return;
    }

    history.forEach(item => {
      const tr = document.createElement('tr');
      tr.innerHTML = `
        <td>${item.date}</td>
        <td><strong>${item.algorithm}</strong></td>
        <td>${item.frameCount}</td>
        <td><div style="max-width: 180px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;" title="${item.referenceString.join(', ')}">${item.referenceString.join(', ')}</div></td>
        <td><span style="color: var(--color-success); font-weight: 700;">${item.pageHits}</span></td>
        <td><span style="color: var(--color-error); font-weight: 700;">${item.pageFaults}</span></td>
        <td><strong>${item.hitRatio.toFixed(1)}%</strong></td>
        <td>
          <div style="display: flex; gap: 6px;">
            <button class="btn btn-secondary btn-sm" title="Reopen Simulation" onclick="appReopenSimulation('${item.id}')">
              <i data-lucide="play-circle"></i>
            </button>
            <button class="btn btn-secondary btn-sm" title="Download CSV" onclick="appExportHistoryItem('${item.id}')">
              <i data-lucide="download"></i>
            </button>
            <button class="btn btn-secondary btn-sm" style="color: var(--color-error);" title="Delete" onclick="appDeleteHistoryItem('${item.id}')">
              <i data-lucide="trash-2"></i>
            </button>
          </div>
        </td>
      `;
      tbody.appendChild(tr);
    });

    if (window.lucide) window.lucide.createIcons();
  }

  // Clear all history
  document.getElementById('btnClearHistory')?.addEventListener('click', () => {
    if (confirm('Are you sure you want to clear all simulation history?')) {
      HistoryManager.clearHistory();
      renderHistoryTable();
      updateDashboardStats();
      showToast('Simulation history cleared', 'info');
    }
  });

  // Global helper functions for inline button callbacks
  window.appReopenSimulation = (id) => {
    const record = HistoryManager.getHistory().find(r => r.id === id);
    if (!record) return;

    inputFrames.value = record.frameCount;
    inputRefString.value = record.referenceString.join(', ');
    selectAlgo.value = record.algorithm;

    SimulationController.loadSimulation(record);
    switchView('view-simulator');
    showToast(`Reopened ${record.algorithm} simulation (${record.frameCount} frames)`, 'info');
  };

  window.appExportHistoryItem = (id) => {
    const record = HistoryManager.getHistory().find(r => r.id === id);
    if (!record) return;
    HistoryManager.exportSimulationCSV(record);
    showToast('Simulation CSV exported!', 'success');
  };

  window.appDeleteHistoryItem = (id) => {
    HistoryManager.deleteRecord(id);
    renderHistoryTable();
    updateDashboardStats();
    showToast('Simulation record deleted', 'info');
  };

  // Print button
  document.getElementById('btnPrintSummary')?.addEventListener('click', () => {
    window.print();
  });

  // 11. Toast Notifications
  function showToast(message, type = 'info') {
    const container = document.getElementById('toastContainer');
    if (!container) return;

    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    const iconName = type === 'success' ? 'check-circle' : (type === 'error' ? 'alert-octagon' : 'info');
    toast.innerHTML = `<i data-lucide="${iconName}"></i> <span>${message}</span>`;
    container.appendChild(toast);

    if (window.lucide) window.lucide.createIcons();

    setTimeout(() => {
      toast.style.opacity = '0';
      toast.style.transform = 'translateX(100%)';
      toast.style.transition = 'all 0.3s ease';
      setTimeout(() => container.removeChild(toast), 300);
    }, 3500);
  }
  window.showToast = showToast;

  // 12. Backend Health Polling
  async function pollBackendHealth() {
    try {
      const health = await API.checkHealth();
      if (statusDot) {
        statusDot.className = 'status-dot online';
      }
      if (statusText) {
        statusText.textContent = `Backend C++17 (v${health.version})`;
      }
    } catch (_) {
      if (statusDot) {
        statusDot.className = 'status-dot offline';
      }
      if (statusText) {
        statusText.textContent = 'Backend Offline';
      }
    }
  }

  pollBackendHealth();
  setInterval(pollBackendHealth, 10000);

  // Initialize Lucide icons
  if (window.lucide) {
    window.lucide.createIcons();
  }

  // Load standard initial simulation on startup so simulator is immediately interactive
  executeSimulation();
  updateDashboardStats();
});
