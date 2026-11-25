/**
 * Simulation History & Export Manager (LocalStorage + CSV Export)
 */
const HistoryManager = (() => {
  const STORAGE_KEY = 'page_replacement_sim_history_v1';

  function getAllRecords() {
    try {
      const raw = localStorage.getItem(STORAGE_KEY);
      return raw ? JSON.parse(raw) : [];
    } catch (e) {
      console.warn('LocalStorage error:', e);
      return [];
    }
  }

  function saveRecords(records) {
    try {
      // Keep up to 50 most recent simulations
      const capped = records.slice(0, 50);
      localStorage.setItem(STORAGE_KEY, JSON.stringify(capped));
    } catch (e) {
      console.warn('LocalStorage save failed:', e);
    }
  }

  return {
    save(simulationResult) {
      const records = getAllRecords();
      const newEntry = {
        id: 'sim_' + Date.now() + '_' + Math.random().toString(36).substr(2, 4),
        date: new Date().toLocaleString(),
        algorithm: simulationResult.algorithm,
        frameCount: simulationResult.frameCount,
        referenceString: simulationResult.referenceString,
        totalReferences: simulationResult.totalReferences,
        pageHits: simulationResult.pageHits,
        pageFaults: simulationResult.pageFaults,
        hitRatio: simulationResult.hitRatio,
        faultRatio: simulationResult.faultRatio,
        replacements: simulationResult.replacements,
        freeAllocations: simulationResult.freeAllocations,
        steps: simulationResult.steps
      };

      records.unshift(newEntry);
      saveRecords(records);
      return newEntry;
    },

    getHistory() {
      return getAllRecords();
    },

    deleteRecord(id) {
      const records = getAllRecords().filter(r => r.id !== id);
      saveRecords(records);
    },

    clearHistory() {
      localStorage.removeItem(STORAGE_KEY);
    },

    getOverallStats() {
      const records = getAllRecords();
      if (records.length === 0) {
        return {
          totalSimulations: 0,
          totalReferences: 0,
          bestAlgorithm: 'N/A',
          avgFaultRate: 0.0
        };
      }

      let totalRefs = 0;
      let totalFaultRateSum = 0;
      const algoWins = {};

      records.forEach(r => {
        totalRefs += r.totalReferences;
        totalFaultRateSum += r.faultRatio;
        algoWins[r.algorithm] = (algoWins[r.algorithm] || 0) + 1;
      });

      let bestAlgo = 'LRU';
      let maxWins = -1;
      for (const [algo, count] of Object.entries(algoWins)) {
        if (count > maxWins) {
          maxWins = count;
          bestAlgo = algo;
        }
      }

      return {
        totalSimulations: records.length,
        totalReferences: totalRefs,
        bestAlgorithm: bestAlgo,
        avgFaultRate: (totalFaultRateSum / records.length).toFixed(1)
      };
    },

    exportSimulationCSV(result) {
      let csv = 'PAGE REPLACEMENT SIMULATOR - SIMULATION REPORT\n';
      csv += `Algorithm,${result.algorithmFullName || result.algorithm}\n`;
      csv += `Frame Count,${result.frameCount}\n`;
      csv += `Total References,${result.totalReferences}\n`;
      csv += `Page Hits,${result.pageHits}\n`;
      csv += `Page Faults,${result.pageFaults}\n`;
      csv += `Hit Ratio %,${result.hitRatio.toFixed(2)}\n`;
      csv += `Page Fault Ratio %,${result.faultRatio.toFixed(2)}\n`;
      csv += `Replacements,${result.replacements}\n`;
      csv += `Free Allocations,${result.freeAllocations}\n`;
      csv += `Reference String,"${result.referenceString.join(', ')}"\n\n`;

      csv += 'Step,Page,Status,Victim Page,Allocated Frame,Memory Frames State,Explanation\n';
      result.steps.forEach(s => {
        const frameStr = s.frames.map(f => (f === -1 ? '[_]' : `[${f}]`)).join(' ');
        const sanitizedExpl = `"${s.explanation.replace(/"/g, '""')}"`;
        csv += `${s.stepIndex + 1},${s.page},${s.status},${s.victimPage === -1 ? '-' : s.victimPage},Frame ${s.allocatedFrameIndex},"${frameStr}",${sanitizedExpl}\n`;
      });

      downloadFile(csv, `simulation_${result.algorithm}_${result.frameCount}frames.csv`, 'text/csv;charset=utf-8;');
    },

    exportComparisonCSV(compResult) {
      let csv = 'PAGE REPLACEMENT SIMULATOR - ALGORITHM COMPARISON REPORT\n';
      csv += `Frame Count,${compResult.frameCount}\n`;
      csv += `Reference String,"${compResult.referenceString.join(', ')}"\n`;
      csv += `Best Performing Online Algorithm,${compResult.bestAlgorithm} (${compResult.lowestFaults} faults)\n`;
      csv += `Theoretical Note,"${compResult.theoreticalNote.replace(/"/g, '""')}"\n\n`;

      csv += 'Algorithm,Full Name,Total References,Page Hits,Page Faults,Hit Ratio %,Fault Ratio %,Replacements,Free Allocations,Tie Breaker Policy\n';
      compResult.results.forEach(r => {
        csv += `"${r.algorithm}","${r.algorithmFullName}",${r.totalReferences},${r.pageHits},${r.pageFaults},${r.hitRatio.toFixed(2)},${r.faultRatio.toFixed(2)},${r.replacements},${r.freeAllocations},"${r.tieBreakerPolicy}"\n`;
      });

      downloadFile(csv, `algorithm_comparison_${compResult.frameCount}frames.csv`, 'text/csv;charset=utf-8;');
    }
  };

  function downloadFile(content, fileName, mimeType) {
    const blob = new Blob([content], { type: mimeType });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = fileName;
    document.body.appendChild(a);
    a.click();
    setTimeout(() => {
      document.body.removeChild(a);
      window.URL.revokeObjectURL(url);
    }, 100);
  }
})();
