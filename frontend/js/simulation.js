/**
 * Live Memory Simulation Controller & Frame Visualization
 */
const SimulationController = (() => {
  let currentResult = null;
  let currentStepIndex = 0;
  let isPlaying = false;
  let playTimer = null;
  let speedMultiplier = 1.0;

  const SPEED_DELAYS = {
    0.5: 1600,
    1.0: 800,
    2.0: 400,
    4.0: 200
  };

  function loadSimulation(simResult) {
    pause();
    currentResult = simResult;
    currentStepIndex = 0;

    // Render tape pills
    renderTape();

    // Render frames at step 0
    renderCurrentStep();

    // Render donut chart
    ChartManager.renderDonutChart('chartHitFaultDonut', simResult.pageHits, simResult.pageFaults);
  }

  function renderTape() {
    const tapeContainer = document.getElementById('referenceTape');
    if (!tapeContainer || !currentResult) return;

    tapeContainer.innerHTML = '';
    currentResult.referenceString.forEach((page, idx) => {
      const chip = document.createElement('div');
      chip.className = 'tape-item';
      chip.dataset.step = idx;
      chip.textContent = page;
      chip.title = `Step ${idx + 1}: Page ${page}`;
      chip.addEventListener('click', () => {
        goToStep(idx);
      });
      tapeContainer.appendChild(chip);
    });
  }

  function updateTapeActive() {
    const chips = document.querySelectorAll('.tape-item');
    chips.forEach((chip, idx) => {
      chip.classList.remove('current', 'past-hit', 'past-fault');
      if (idx === currentStepIndex) {
        chip.classList.add('current');
        chip.scrollIntoView({ behavior: 'smooth', block: 'nearest', inline: 'center' });
      } else if (idx < currentStepIndex && currentResult && currentResult.steps[idx]) {
        if (currentResult.steps[idx].isHit) {
          chip.classList.add('past-hit');
        } else {
          chip.classList.add('past-fault');
        }
      }
    });
  }

  function renderCurrentStep() {
    if (!currentResult || !currentResult.steps || currentResult.steps.length === 0) return;

    const step = currentResult.steps[currentStepIndex];
    const total = currentResult.totalReferences;

    // Update Step Counter Text and progress bar
    const counterEl = document.getElementById('stepCounterDisplay');
    if (counterEl) {
      counterEl.innerHTML = `Step <span>${currentStepIndex + 1}</span> of <span>${total}</span>`;
    }
    const progressBar = document.getElementById('stepProgressBar');
    if (progressBar) {
      const pct = ((currentStepIndex + 1) / total) * 100;
      progressBar.style.width = `${pct}%`;
    }

    // Update Status Pill
    const statusPill = document.getElementById('stepStatusPill');
    if (statusPill) {
      statusPill.className = 'status-badge-pill';
      if (step.isHit) {
        statusPill.classList.add('hit');
        statusPill.innerHTML = `<i data-lucide="check-circle-2"></i> PAGE HIT`;
      } else if (step.status === 'FAULT_ALLOCATE') {
        statusPill.classList.add('fault');
        statusPill.innerHTML = `<i data-lucide="alert-circle"></i> PAGE FAULT (Free Allocation)`;
      } else {
        statusPill.classList.add('fault');
        statusPill.innerHTML = `<i data-lucide="refresh-cw"></i> PAGE FAULT (Replaced ${step.victimPage})`;
      }
    }

    // Update Explanation Box
    const explBox = document.getElementById('stepExplanation');
    if (explBox) {
      explBox.textContent = step.explanation;
    }

    // Render Memory Frames
    const framesGrid = document.getElementById('memoryFramesGrid');
    if (framesGrid) {
      framesGrid.innerHTML = '';
      const algo = currentResult.algorithm;
      const meta = step.algorithmState || {};

      step.frames.forEach((page, fIdx) => {
        const frameEl = document.createElement('div');
        frameEl.className = 'frame-block';

        if (page === -1) {
          frameEl.classList.add('empty');
        } else if (fIdx === step.allocatedFrameIndex) {
          if (step.isHit) {
            frameEl.classList.add('highlight-hit');
          } else if (step.status === 'FAULT_ALLOCATE') {
            frameEl.classList.add('highlight-allocate');
          } else {
            frameEl.classList.add('highlight-replace');
          }
        }

        // Frame Label
        const label = document.createElement('div');
        label.className = 'frame-label';
        label.textContent = `Frame ${fIdx}`;
        frameEl.appendChild(label);

        // Page Value Circle/Box
        const pageVal = document.createElement('div');
        pageVal.className = 'page-value';
        pageVal.textContent = page === -1 ? '—' : page;
        frameEl.appendChild(pageVal);

        // Algorithm-specific Badges & Indicators
        if (algo === 'SecondChance' && meta.referenceBits) {
          const bitVal = meta.referenceBits[fIdx] !== undefined ? meta.referenceBits[fIdx] : 0;
          const bitBadge = document.createElement('div');
          bitBadge.className = 'frame-metadata-badge';
          bitBadge.innerHTML = `Ref Bit: <strong>${bitVal}</strong>`;
          frameEl.appendChild(bitBadge);

          if (meta.clockHand === fIdx) {
            const handPointer = document.createElement('div');
            handPointer.className = 'clock-hand-pointer';
            handPointer.innerHTML = `<span>👉 CLOCK</span>`;
            frameEl.appendChild(handPointer);
          }
        } else if (algo === 'LRU' && meta.frameStats && meta.frameStats[fIdx]) {
          const fStat = meta.frameStats[fIdx];
          if (fStat.page !== -1) {
            const ageBadge = document.createElement('div');
            ageBadge.className = 'frame-metadata-badge';
            ageBadge.textContent = `Idle: ${fStat.idleSteps} steps`;
            frameEl.appendChild(ageBadge);
          }
        } else if (algo === 'LFU' && meta.frameStats && meta.frameStats[fIdx]) {
          const fStat = meta.frameStats[fIdx];
          if (fStat.page !== -1) {
            const freqBadge = document.createElement('div');
            freqBadge.className = 'frame-metadata-badge';
            freqBadge.textContent = `Freq: ${fStat.frequency}×`;
            frameEl.appendChild(freqBadge);
          }
        } else if (algo === 'Optimal' && meta.frameStats && meta.frameStats[fIdx]) {
          const fStat = meta.frameStats[fIdx];
          if (fStat.page !== -1) {
            const optBadge = document.createElement('div');
            optBadge.className = 'frame-metadata-badge';
            optBadge.textContent = fStat.neverUsedAgain ? 'Next: Never' : `Next: #${fStat.nextUseStep + 1}`;
            frameEl.appendChild(optBadge);
          }
        } else if (algo === 'FIFO' && meta.queueOrder) {
          const pos = meta.queueOrder.indexOf(fIdx);
          if (pos !== -1) {
            const fifoBadge = document.createElement('div');
            fifoBadge.className = 'frame-metadata-badge';
            fifoBadge.textContent = pos === 0 ? 'Next Victim' : `Queue #${pos + 1}`;
            frameEl.appendChild(fifoBadge);
          }
        }

        framesGrid.appendChild(frameEl);
      });
    }

    // Cumulative Statistics up to this step
    updateCumulativeStats();

    // Update Active Chip in Tape
    updateTapeActive();

    // Re-render Lucide icons if available
    if (window.lucide) {
      window.lucide.createIcons();
    }
  }

  function updateCumulativeStats() {
    if (!currentResult) return;

    let hitsSoFar = 0;
    let faultsSoFar = 0;
    let replacementsSoFar = 0;
    let allocationsSoFar = 0;

    for (let i = 0; i <= currentStepIndex; ++i) {
      const s = currentResult.steps[i];
      if (s.isHit) {
        hitsSoFar++;
      } else {
        faultsSoFar++;
        if (s.status === 'FAULT_ALLOCATE') {
          allocationsSoFar++;
        } else {
          replacementsSoFar++;
        }
      }
    }

    const stepsCount = currentStepIndex + 1;
    const hitRatio = stepsCount > 0 ? ((hitsSoFar / stepsCount) * 100).toFixed(1) : '0';
    const faultRatio = stepsCount > 0 ? ((faultsSoFar / stepsCount) * 100).toFixed(1) : '0';

    const setText = (id, val) => {
      const el = document.getElementById(id);
      if (el) el.textContent = val;
    };

    setText('statHitsLive', hitsSoFar);
    setText('statFaultsLive', faultsSoFar);
    setText('statHitRatioLive', `${hitRatio}%`);
    setText('statFaultRatioLive', `${faultRatio}%`);
    setText('statReplacementsLive', replacementsSoFar);
    setText('statAllocationsLive', allocationsSoFar);

    const hitProgress = document.getElementById('statHitProgressBar');
    if (hitProgress) hitProgress.style.width = `${hitRatio}%`;
    const faultProgress = document.getElementById('statFaultProgressBar');
    if (faultProgress) faultProgress.style.width = `${faultRatio}%`;
  }

  function play() {
    if (!currentResult || isPlaying) return;
    if (currentStepIndex >= currentResult.totalReferences - 1) {
      currentStepIndex = 0;
    }

    isPlaying = true;
    updatePlayButton();

    const delay = SPEED_DELAYS[speedMultiplier] || 800;
    playTimer = setInterval(() => {
      if (currentStepIndex < currentResult.totalReferences - 1) {
        currentStepIndex++;
        renderCurrentStep();
      } else {
        pause();
      }
    }, delay);
  }

  function pause() {
    isPlaying = false;
    if (playTimer) {
      clearInterval(playTimer);
      playTimer = null;
    }
    updatePlayButton();
  }

  function togglePlay() {
    if (isPlaying) {
      pause();
    } else {
      play();
    }
  }

  function updatePlayButton() {
    const playBtn = document.getElementById('btnPlayPause');
    if (!playBtn) return;
    if (isPlaying) {
      playBtn.innerHTML = `<i data-lucide="pause"></i> Pause`;
      playBtn.classList.remove('btn-primary');
      playBtn.classList.add('btn-secondary');
    } else {
      playBtn.innerHTML = `<i data-lucide="play"></i> Resume`;
      playBtn.classList.remove('btn-secondary');
      playBtn.classList.add('btn-primary');
    }
    if (window.lucide) window.lucide.createIcons();
  }

  function nextStep() {
    if (!currentResult) return;
    pause();
    if (currentStepIndex < currentResult.totalReferences - 1) {
      currentStepIndex++;
      renderCurrentStep();
    }
  }

  function prevStep() {
    if (!currentResult) return;
    pause();
    if (currentStepIndex > 0) {
      currentStepIndex--;
      renderCurrentStep();
    }
  }

  function goToStep(idx) {
    if (!currentResult || idx < 0 || idx >= currentResult.totalReferences) return;
    pause();
    currentStepIndex = idx;
    renderCurrentStep();
  }

  function reset() {
    pause();
    currentStepIndex = 0;
    renderCurrentStep();
  }

  function runAll() {
    if (!currentResult) return;
    pause();
    currentStepIndex = currentResult.totalReferences - 1;
    renderCurrentStep();
  }

  function setSpeed(mult) {
    speedMultiplier = mult;
    document.querySelectorAll('.speed-btn').forEach(btn => {
      btn.classList.toggle('active', parseFloat(btn.dataset.speed) === mult);
    });

    if (isPlaying) {
      pause();
      play();
    }
  }

  return {
    loadSimulation,
    play,
    pause,
    togglePlay,
    nextStep,
    prevStep,
    goToStep,
    reset,
    runAll,
    setSpeed,
    getCurrentResult() { return currentResult; }
  };
})();
