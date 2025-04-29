// Configurazione
const config = {
    apiUrl: 'http://localhost:8080',
    refreshInterval: 1000,
    memoryPageSize: 256,
    assemblyPageSize: 64
};

// Stato globale
let state = {
    processInfo: null,
    memoryData: null,
    threads: [],
    modules: [],
    hooks: [],
    calls: [],
    currentMemoryAddress: 0,
    currentAssemblyAddress: 0
};

// Inizializzazione
document.addEventListener('DOMContentLoaded', () => {
    initializeCharts();
    initializeNetwork();
    setupEventListeners();
    startRefreshLoop();
});

// Inizializzazione grafici
function initializeCharts() {
    const ctx = document.getElementById('memory-chart').getContext('2d');
    window.memoryChart = new Chart(ctx, {
        type: 'doughnut',
        data: {
            labels: ['Working Set', 'Private Usage'],
            datasets: [{
                data: [0, 0],
                backgroundColor: ['#0d6efd', '#6c757d']
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false
        }
    });
}

// Inizializzazione network
function initializeNetwork() {
    const container = document.getElementById('calls-network');
    const data = {
        nodes: new vis.DataSet([]),
        edges: new vis.DataSet([])
    };
    const options = {
        nodes: {
            shape: 'dot',
            size: 16
        },
        edges: {
            arrows: 'to',
            smooth: {
                type: 'continuous'
            }
        },
        physics: {
            stabilization: false,
            barnesHut: {
                gravitationalConstant: -80000,
                springConstant: 0.001,
                springLength: 200
            }
        }
    };
    window.callsNetwork = new vis.Network(container, data, options);
}

// Setup event listeners
function setupEventListeners() {
    // Refresh button
    document.getElementById('refreshBtn').addEventListener('click', refreshAll);

    // Export button
    document.getElementById('exportBtn').addEventListener('click', exportData);

    // Memory search
    document.getElementById('memory-scan').addEventListener('click', () => {
        const pattern = document.getElementById('memory-search').value;
        scanMemory(pattern);
    });

    // Assembly disassemble
    document.getElementById('assembly-disassemble').addEventListener('click', () => {
        const address = document.getElementById('assembly-address').value;
        disassemble(address);
    });

    // Tab changes
    document.querySelectorAll('[data-bs-toggle="list"]').forEach(tab => {
        tab.addEventListener('shown.bs.tab', (e) => {
            const target = e.target.getAttribute('href').substring(1);
            refreshTab(target);
        });
    });
}

// Loop di aggiornamento
function startRefreshLoop() {
    setInterval(refreshAll, config.refreshInterval);
}

// Aggiorna tutti i dati
async function refreshAll() {
    try {
        await Promise.all([
            refreshProcessInfo(),
            refreshMemoryData(),
            refreshThreads(),
            refreshModules(),
            refreshHooks(),
            refreshCalls()
        ]);
    } catch (error) {
        console.error('Error refreshing data:', error);
    }
}

// Aggiorna tab specifico
async function refreshTab(tab) {
    try {
        switch (tab) {
            case 'process':
                await refreshProcessInfo();
                break;
            case 'memory':
                await refreshMemoryData();
                break;
            case 'threads':
                await refreshThreads();
                break;
            case 'modules':
                await refreshModules();
                break;
            case 'hooks':
                await refreshHooks();
                break;
            case 'assembly':
                await refreshAssembly();
                break;
            case 'calls':
                await refreshCalls();
                break;
        }
    } catch (error) {
        console.error(`Error refreshing ${tab}:`, error);
    }
}

// Aggiorna informazioni processo
async function refreshProcessInfo() {
    const response = await fetch(`${config.apiUrl}/process`);
    const data = await response.json();
    state.processInfo = data.data;

    // Aggiorna UI
    document.getElementById('process-pid').textContent = state.processInfo.pid;
    document.getElementById('process-name').textContent = state.processInfo.name;
    document.getElementById('process-path').textContent = state.processInfo.path;
    document.getElementById('process-arch').textContent = state.processInfo.architecture;

    // Aggiorna grafico memoria
    window.memoryChart.data.datasets[0].data = [
        state.processInfo.workingSetSize,
        state.processInfo.privateUsage
    ];
    window.memoryChart.update();
}

// Aggiorna dati memoria
async function refreshMemoryData() {
    const response = await fetch(`${config.apiUrl}/memory`);
    const data = await response.json();
    state.memoryData = data.data;

    // Aggiorna visualizzazione memoria
    updateMemoryView();
}

// Aggiorna thread
async function refreshThreads() {
    const response = await fetch(`${config.apiUrl}/threads`);
    const data = await response.json();
    state.threads = data.data;

    // Aggiorna tabella thread
    const tbody = document.getElementById('threads-list');
    tbody.innerHTML = state.threads.map(thread => `
        <tr>
            <td>${thread.id}</td>
            <td>${thread.priority}</td>
            <td>${thread.state}</td>
            <td>0x${thread.rip.toString(16)}</td>
            <td>0x${thread.rsp.toString(16)}</td>
            <td>0x${thread.rbp.toString(16)}</td>
            <td>
                <button class="btn btn-sm btn-primary" onclick="viewThreadStack(${thread.id})">
                    Stack
                </button>
            </td>
        </tr>
    `).join('');
}

// Aggiorna moduli
async function refreshModules() {
    const response = await fetch(`${config.apiUrl}/modules`);
    const data = await response.json();
    state.modules = data.data;

    // Aggiorna tabella moduli
    const tbody = document.getElementById('modules-list');
    tbody.innerHTML = state.modules.map(module => `
        <tr>
            <td>${module.name}</td>
            <td>0x${module.base.toString(16)}</td>
            <td>${module.size}</td>
            <td>0x${module.entryPoint.toString(16)}</td>
            <td>
                <button class="btn btn-sm btn-primary" onclick="viewModuleMemory(${module.base})">
                    Memoria
                </button>
            </td>
        </tr>
    `).join('');
}

// Aggiorna hook
async function refreshHooks() {
    const response = await fetch(`${config.apiUrl}/hooks`);
    const data = await response.json();
    state.hooks = data.data;

    // Aggiorna tabella hook
    const tbody = document.getElementById('hooks-list');
    tbody.innerHTML = state.hooks.map(hook => `
        <tr>
            <td>0x${hook.target.toString(16)}</td>
            <td>0x${hook.detour.toString(16)}</td>
            <td>0x${hook.original.toString(16)}</td>
            <td>${hook.name}</td>
            <td>
                <button class="btn btn-sm btn-primary" onclick="viewHookCode(${hook.target})">
                    Codice
                </button>
            </td>
        </tr>
    `).join('');
}

// Aggiorna assembly
async function refreshAssembly() {
    if (state.currentAssemblyAddress) {
        await disassemble(state.currentAssemblyAddress);
    }
}

// Aggiorna chiamate
async function refreshCalls() {
    const response = await fetch(`${config.apiUrl}/calls`);
    const data = await response.json();
    state.calls = data.data;

    // Aggiorna network
    const nodes = state.calls.map(call => ({
        id: call.id,
        label: call.name,
        title: `0x${call.address.toString(16)}`
    }));

    const edges = state.calls.flatMap(call => 
        call.calls.map(target => ({
            from: call.id,
            to: target
        }))
    );

    window.callsNetwork.setData({ nodes, edges });
}

// Aggiorna visualizzazione memoria
function updateMemoryView() {
    const hexView = document.getElementById('memory-hex');
    const asciiView = document.getElementById('memory-ascii');
    
    let hexContent = '';
    let asciiContent = '';
    
    for (let i = 0; i < state.memoryData.length; i += 16) {
        // Indirizzo
        hexContent += `0x${(state.currentMemoryAddress + i).toString(16).padStart(16, '0')}: `;
        
        // Bytes esadecimali
        for (let j = 0; j < 16; j++) {
            if (i + j < state.memoryData.length) {
                hexContent += state.memoryData[i + j].toString(16).padStart(2, '0') + ' ';
            } else {
                hexContent += '   ';
            }
        }
        
        // Caratteri ASCII
        asciiContent += '  ';
        for (let j = 0; j < 16; j++) {
            if (i + j < state.memoryData.length) {
                const byte = state.memoryData[i + j];
                asciiContent += (byte >= 32 && byte <= 126) ? String.fromCharCode(byte) : '.';
            }
        }
        
        hexContent += '\n';
        asciiContent += '\n';
    }
    
    hexView.textContent = hexContent;
    asciiView.textContent = asciiContent;
}

// Scansiona memoria
async function scanMemory(pattern) {
    const response = await fetch(`${config.apiUrl}/memory/scan`, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ pattern })
    });
    const data = await response.json();
    
    if (data.status === 'ok') {
        state.currentMemoryAddress = data.data.address;
        await refreshMemoryData();
    }
}

// Disassembla
async function disassemble(address) {
    const response = await fetch(`${config.apiUrl}/assembly`, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ address })
    });
    const data = await response.json();
    
    if (data.status === 'ok') {
        state.currentAssemblyAddress = address;
        document.getElementById('assembly-output').textContent = data.data.assembly;
    }
}

// Visualizza stack thread
async function viewThreadStack(threadId) {
    const response = await fetch(`${config.apiUrl}/thread/${threadId}/stack`);
    const data = await response.json();
    
    if (data.status === 'ok') {
        state.currentMemoryAddress = data.data.address;
        await refreshMemoryData();
    }
}

// Visualizza memoria modulo
async function viewModuleMemory(moduleBase) {
    state.currentMemoryAddress = moduleBase;
    await refreshMemoryData();
}

// Visualizza codice hook
async function viewHookCode(hookAddress) {
    state.currentAssemblyAddress = hookAddress;
    await refreshAssembly();
}

// Esporta dati
function exportData() {
    const data = {
        processInfo: state.processInfo,
        memoryData: state.memoryData,
        threads: state.threads,
        modules: state.modules,
        hooks: state.hooks,
        calls: state.calls
    };
    
    const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `dymain-export-${new Date().toISOString()}.json`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
} 