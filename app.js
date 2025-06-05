/**
 * Bitcoin Puzzle #73 Kangaroo Solver - Frontend Application
 * Real-time monitoring and control interface
 */

class KangarooSolverApp {
    constructor() {
        this.isRunning = false;
        this.updateInterval = null;
        this.chart = null;
        this.chartData = {
            labels: [],
            jumpData: [],
            dpData: []
        };
        
        this.elements = {
            statusIndicator: document.getElementById('status-indicator'),
            statusText: document.getElementById('status-text'),
            runtime: document.getElementById('runtime'),
            threads: document.getElementById('threads'),
            totalJumps: document.getElementById('total-jumps'),
            jumpRate: document.getElementById('jump-rate'),
            distinguishedPoints: document.getElementById('distinguished-points'),
            collisions: document.getElementById('collisions'),
            dpRate: document.getElementById('dp-rate'),
            eta: document.getElementById('eta'),
            startBtn: document.getElementById('start-btn'),
            stopBtn: document.getElementById('stop-btn'),
            saveCheckpointBtn: document.getElementById('save-checkpoint-btn'),
            loadCheckpointBtn: document.getElementById('load-checkpoint-btn'),
            threadsInput: document.getElementById('threads-input'),
            distinguishedBitsInput: document.getElementById('distinguished-bits-input'),
            logOutput: document.getElementById('log-output'),
            clearLogBtn: document.getElementById('clear-log-btn'),
            solutionModal: document.getElementById('solution-modal'),
            solutionPrivateKey: document.getElementById('solution-private-key'),
            solutionJumps: document.getElementById('solution-jumps'),
            solutionTime: document.getElementById('solution-time'),
            solutionRate: document.getElementById('solution-rate'),
            copyKeyBtn: document.getElementById('copy-key-btn'),
            closeModalBtn: document.getElementById('close-modal-btn')
        };
        
        this.init();
    }
    
    init() {
        this.setupEventListeners();
        this.initializeChart();
        this.loadInitialState();
        this.startStatusUpdates();
        this.logMessage('INFO', 'Web interface initialized');
    }
    
    setupEventListeners() {
        // Control buttons
        this.elements.startBtn.addEventListener('click', () => this.startSolver());
        this.elements.stopBtn.addEventListener('click', () => this.stopSolver());
        this.elements.saveCheckpointBtn.addEventListener('click', () => this.saveCheckpoint());
        this.elements.loadCheckpointBtn.addEventListener('click', () => this.loadCheckpoint());
        
        // Log controls
        this.elements.clearLogBtn.addEventListener('click', () => this.clearLog());
        
        // Modal controls
        this.elements.copyKeyBtn.addEventListener('click', () => this.copyPrivateKey());
        this.elements.closeModalBtn.addEventListener('click', () => this.closeSolutionModal());
        
        // Settings changes
        this.elements.threadsInput.addEventListener('change', () => this.updateSettings());
        this.elements.distinguishedBitsInput.addEventListener('change', () => this.updateSettings());
        
        // Keyboard shortcuts
        document.addEventListener('keydown', (e) => {
            if (e.ctrlKey || e.metaKey) {
                switch(e.key) {
                    case 's':
                        e.preventDefault();
                        this.saveCheckpoint();
                        break;
                    case 'o':
                        e.preventDefault();
                        this.loadCheckpoint();
                        break;
                    case 'l':
                        e.preventDefault();
                        this.clearLog();
                        break;
                }
            }
            
            if (e.key === 'Escape') {
                this.closeSolutionModal();
            }
        });
        
        // Auto-save settings
        window.addEventListener('beforeunload', () => {
            this.saveSettings();
        });
    }
    
    initializeChart() {
        const ctx = document.getElementById('progress-chart').getContext('2d');
        
        this.chart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [
                    {
                        label: 'Jump Rate (K/s)',
                        data: [],
                        borderColor: 'hsl(220 100% 55%)',
                        backgroundColor: 'hsla(220 100% 55% / 0.1)',
                        fill: false,
                        tension: 0.4,
                        yAxisID: 'y'
                    },
                    {
                        label: 'Distinguished Points',
                        data: [],
                        borderColor: 'hsl(45 100% 50%)',
                        backgroundColor: 'hsla(45 100% 50% / 0.1)',
                        fill: false,
                        tension: 0.4,
                        yAxisID: 'y1'
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                interaction: {
                    mode: 'index',
                    intersect: false,
                },
                scales: {
                    x: {
                        display: true,
                        title: {
                            display: true,
                            text: 'Time'
                        }
                    },
                    y: {
                        type: 'linear',
                        display: true,
                        position: 'left',
                        title: {
                            display: true,
                            text: 'Jump Rate (K/s)'
                        }
                    },
                    y1: {
                        type: 'linear',
                        display: true,
                        position: 'right',
                        title: {
                            display: true,
                            text: 'Distinguished Points'
                        },
                        grid: {
                            drawOnChartArea: false,
                        },
                    }
                },
                plugins: {
                    legend: {
                        display: true,
                        position: 'top'
                    },
                    tooltip: {
                        mode: 'index',
                        intersect: false
                    }
                }
            }
        });
    }
    
    async loadInitialState() {
        try {
            // Load saved settings
            this.loadSettings();
            
            // Check if solver is already running
            const response = await this.makeRequest('/api/status');
            if (response.success) {
                this.updateUI(response.data);
            }
        } catch (error) {
            this.logMessage('ERROR', `Failed to load initial state: ${error.message}`);
        }
    }
    
    startStatusUpdates() {
        // Update every second
        this.updateInterval = setInterval(() => {
            this.updateStatus();
        }, 1000);
    }
    
    async updateStatus() {
        try {
            const response = await this.makeRequest('/api/status');
            if (response.success) {
                this.updateUI(response.data);
                
                // Check if puzzle is solved
                if (response.data.is_solved) {
                    this.showSolutionModal(response.data);
                }
            }
        } catch (error) {
            // Don't spam log with connection errors
            if (this.shouldLogError(error)) {
                this.logMessage('ERROR', `Status update failed: ${error.message}`);
            }
        }
    }
    
    shouldLogError(error) {
        // Only log errors every 10 seconds to avoid spam
        const now = Date.now();
        if (!this.lastErrorLog || (now - this.lastErrorLog) > 10000) {
            this.lastErrorLog = now;
            return true;
        }
        return false;
    }
    
    updateUI(data) {
        // Update status indicator
        const statusDot = this.elements.statusIndicator.querySelector('.status-dot');
        if (data.threads_active > 0) {
            this.elements.statusText.textContent = 'Running';
            statusDot.className = 'status-dot running';
            this.isRunning = true;
            this.elements.startBtn.disabled = true;
            this.elements.stopBtn.disabled = false;
        } else {
            this.elements.statusText.textContent = 'Stopped';
            statusDot.className = 'status-dot stopped';
            this.isRunning = false;
            this.elements.startBtn.disabled = false;
            this.elements.stopBtn.disabled = true;
        }
        
        // Update statistics
        this.elements.runtime.textContent = this.formatTime(data.elapsed_time);
        this.elements.threads.textContent = data.threads_active;
        this.elements.totalJumps.textContent = this.formatNumber(data.total_jumps);
        this.elements.jumpRate.textContent = this.formatRate(data.total_jumps / Math.max(data.elapsed_time, 1));
        this.elements.distinguishedPoints.textContent = this.formatNumber(data.distinguished_points);
        this.elements.collisions.textContent = data.collisions_found;
        this.elements.dpRate.textContent = this.formatRate(data.distinguished_points / Math.max(data.elapsed_time, 1), '/s');
        this.elements.eta.textContent = this.estimateETA(data);
        
        // Update chart
        this.updateChart(data);
    }
    
    updateChart(data) {
        const now = new Date().toLocaleTimeString();
        const jumpRate = data.total_jumps / Math.max(data.elapsed_time, 1) / 1000; // K/s
        
        // Add new data point
        this.chartData.labels.push(now);
        this.chartData.jumpData.push(jumpRate);
        this.chartData.dpData.push(data.distinguished_points);
        
        // Keep only last 50 points
        if (this.chartData.labels.length > 50) {
            this.chartData.labels.shift();
            this.chartData.jumpData.shift();
            this.chartData.dpData.shift();
        }
        
        // Update chart
        this.chart.data.labels = this.chartData.labels;
        this.chart.data.datasets[0].data = this.chartData.jumpData;
        this.chart.data.datasets[1].data = this.chartData.dpData;
        this.chart.update('none'); // No animation for real-time updates
    }
    
    async startSolver() {
        try {
            this.logMessage('INFO', 'Starting Kangaroo solver...');
            
            const config = {
                threads: parseInt(this.elements.threadsInput.value),
                distinguished_bits: parseInt(this.elements.distinguishedBitsInput.value)
            };
            
            const response = await this.makeRequest('/api/start', 'POST', config);
            
            if (response.success) {
                this.logMessage('INFO', 'Solver started successfully');
                this.elements.startBtn.disabled = true;
                this.elements.stopBtn.disabled = false;
            } else {
                this.logMessage('ERROR', `Failed to start solver: ${response.error}`);
            }
        } catch (error) {
            this.logMessage('ERROR', `Start request failed: ${error.message}`);
        }
    }
    
    async stopSolver() {
        try {
            this.logMessage('INFO', 'Stopping Kangaroo solver...');
            
            const response = await this.makeRequest('/api/stop', 'POST');
            
            if (response.success) {
                this.logMessage('INFO', 'Solver stopped successfully');
                this.elements.startBtn.disabled = false;
                this.elements.stopBtn.disabled = true;
            } else {
                this.logMessage('ERROR', `Failed to stop solver: ${response.error}`);
            }
        } catch (error) {
            this.logMessage('ERROR', `Stop request failed: ${error.message}`);
        }
    }
    
    async saveCheckpoint() {
        try {
            this.logMessage('INFO', 'Saving checkpoint...');
            
            const response = await this.makeRequest('/api/checkpoint/save', 'POST');
            
            if (response.success) {
                this.logMessage('INFO', 'Checkpoint saved successfully');
            } else {
                this.logMessage('ERROR', `Failed to save checkpoint: ${response.error}`);
            }
        } catch (error) {
            this.logMessage('ERROR', `Save checkpoint failed: ${error.message}`);
        }
    }
    
    async loadCheckpoint() {
        try {
            this.logMessage('INFO', 'Loading checkpoint...');
            
            const response = await this.makeRequest('/api/checkpoint/load', 'POST');
            
            if (response.success) {
                this.logMessage('INFO', 'Checkpoint loaded successfully');
            } else {
                this.logMessage('ERROR', `Failed to load checkpoint: ${response.error}`);
            }
        } catch (error) {
            this.logMessage('ERROR', `Load checkpoint failed: ${error.message}`);
        }
    }
    
    async updateSettings() {
        try {
            const config = {
                threads: parseInt(this.elements.threadsInput.value),
                distinguished_bits: parseInt(this.elements.distinguishedBitsInput.value)
            };
            
            const response = await this.makeRequest('/api/config', 'POST', config);
            
            if (response.success) {
                this.logMessage('INFO', 'Settings updated');
                this.saveSettings();
            } else {
                this.logMessage('ERROR', `Failed to update settings: ${response.error}`);
            }
        } catch (error) {
            this.logMessage('ERROR', `Update settings failed: ${error.message}`);
        }
    }
    
    saveSettings() {
        const settings = {
            threads: this.elements.threadsInput.value,
            distinguished_bits: this.elements.distinguishedBitsInput.value
        };
        localStorage.setItem('kangaroo_settings', JSON.stringify(settings));
    }
    
    loadSettings() {
        try {
            const settings = JSON.parse(localStorage.getItem('kangaroo_settings') || '{}');
            
            if (settings.threads) {
                this.elements.threadsInput.value = settings.threads;
            }
            
            if (settings.distinguished_bits) {
                this.elements.distinguishedBitsInput.value = settings.distinguished_bits;
            }
        } catch (error) {
            this.logMessage('WARNING', 'Failed to load saved settings');
        }
    }
    
    logMessage(level, message) {
        const timestamp = new Date().toLocaleTimeString();
        const logEntry = `[${timestamp}] ${level}: ${message}`;
        
        const logElement = document.createElement('div');
        logElement.textContent = logEntry;
        logElement.className = `log-entry log-${level.toLowerCase()}`;
        
        this.elements.logOutput.appendChild(logElement);
        
        // Auto-scroll to bottom
        this.elements.logOutput.scrollTop = this.elements.logOutput.scrollHeight;
        
        // Keep only last 1000 entries
        const entries = this.elements.logOutput.children;
        if (entries.length > 1000) {
            this.elements.logOutput.removeChild(entries[0]);
        }
    }
    
    clearLog() {
        this.elements.logOutput.innerHTML = '';
        this.logMessage('INFO', 'Log cleared');
    }
    
    showSolutionModal(data) {
        this.elements.solutionPrivateKey.textContent = data.found_key;
        this.elements.solutionJumps.textContent = this.formatNumber(data.total_jumps);
        this.elements.solutionTime.textContent = this.formatTime(data.elapsed_time);
        this.elements.solutionRate.textContent = this.formatRate(data.total_jumps / Math.max(data.elapsed_time, 1));
        
        this.elements.solutionModal.style.display = 'block';
        
        this.logMessage('SUCCESS', `🎉 PUZZLE SOLVED! Private key: ${data.found_key}`);
        
        // Stop updates since puzzle is solved
        if (this.updateInterval) {
            clearInterval(this.updateInterval);
            this.updateInterval = null;
        }
    }
    
    closeSolutionModal() {
        this.elements.solutionModal.style.display = 'none';
    }
    
    async copyPrivateKey() {
        try {
            const privateKey = this.elements.solutionPrivateKey.textContent;
            await navigator.clipboard.writeText(privateKey);
            
            // Show feedback
            const originalText = this.elements.copyKeyBtn.innerHTML;
            this.elements.copyKeyBtn.innerHTML = '<i class="fas fa-check"></i> Copied!';
            this.elements.copyKeyBtn.style.background = 'hsl(120 60% 50%)';
            
            setTimeout(() => {
                this.elements.copyKeyBtn.innerHTML = originalText;
                this.elements.copyKeyBtn.style.background = '';
            }, 2000);
            
            this.logMessage('INFO', 'Private key copied to clipboard');
        } catch (error) {
            this.logMessage('ERROR', 'Failed to copy private key to clipboard');
        }
    }
    
    async makeRequest(url, method = 'GET', data = null) {
        const options = {
            method,
            headers: {
                'Content-Type': 'application/json'
            }
        };
        
        if (data) {
            options.body = JSON.stringify(data);
        }
        
        const response = await fetch(url, options);
        
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        
        return await response.json();
    }
    
    formatNumber(num) {
        if (num >= 1e15) {
            return (num / 1e15).toFixed(2) + 'P';
        } else if (num >= 1e12) {
            return (num / 1e12).toFixed(2) + 'T';
        } else if (num >= 1e9) {
            return (num / 1e9).toFixed(2) + 'G';
        } else if (num >= 1e6) {
            return (num / 1e6).toFixed(2) + 'M';
        } else if (num >= 1e3) {
            return (num / 1e3).toFixed(2) + 'K';
        } else {
            return num.toString();
        }
    }
    
    formatRate(rate, suffix = '/s') {
        return this.formatNumber(rate) + suffix;
    }
    
    formatTime(seconds) {
        if (seconds < 60) {
            return `${seconds}s`;
        } else if (seconds < 3600) {
            const minutes = Math.floor(seconds / 60);
            const remainingSeconds = seconds % 60;
            return `${minutes}m ${remainingSeconds}s`;
        } else if (seconds < 86400) {
            const hours = Math.floor(seconds / 3600);
            const minutes = Math.floor((seconds % 3600) / 60);
            return `${hours}h ${minutes}m`;
        } else {
            const days = Math.floor(seconds / 86400);
            const hours = Math.floor((seconds % 86400) / 3600);
            return `${days}d ${hours}h`;
        }
    }
    
    estimateETA(data) {
        if (data.total_jumps === 0 || data.elapsed_time === 0) {
            return 'Unknown';
        }
        
        const jumpRate = data.total_jumps / data.elapsed_time;
        const searchSpace = Math.pow(2, 73) - Math.pow(2, 72); // Bitcoin Puzzle #73 range
        const expectedOperations = Math.sqrt(searchSpace) * 1.25; // Kangaroo efficiency factor
        const remainingOperations = Math.max(0, expectedOperations - data.total_jumps);
        
        if (remainingOperations <= 0) {
            return 'Soon';
        }
        
        const etaSeconds = Math.floor(remainingOperations / jumpRate);
        return this.formatTime(etaSeconds);
    }
}

// Initialize the application when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    new KangarooSolverApp();
});

// Handle modal clicks outside content
window.addEventListener('click', (event) => {
    const modal = document.getElementById('solution-modal');
    if (event.target === modal) {
        modal.style.display = 'none';
    }
});
