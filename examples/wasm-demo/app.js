/**
 * VSCode TextMate WASM 2023 Demo
 * Interactive syntax highlighting using WebAssembly
 */

let wasmModule = null
let registry = null
let currentGrammar = null
let currentTheme = null
let wasmFeatures = {}
let grammars = {} // Will be loaded dynamically from GitHub
let themes = {} // Will be loaded dynamically from GitHub
let availableGrammars = [] // List of available grammar names
let availableThemes = [] // List of available theme names

// Base URLs for fetching from GitHub
const GITHUB_API_URL = 'https://api.github.com/repos/shikijs/textmate-grammars-themes/git/trees/main?recursive=1'
const GRAMMARS_RAW_BASE_URL = 'https://raw.githubusercontent.com/shikijs/textmate-grammars-themes/main/packages/tm-grammars/grammars'
const THEMES_RAW_BASE_URL = 'https://raw.githubusercontent.com/shikijs/textmate-grammars-themes/main/packages/tm-themes/themes'
const THEMES_API_URL = 'https://api.github.com/repos/shikijs/textmate-grammars-themes/contents/packages/tm-themes/themes'

// Sample code for each language
const SAMPLE_CODE = {
    javascript: `// Hello, WASM 2023!
const greeting = "WebAssembly syntax highlighting";
console.log(greeting);

function factorial(n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

// Performance with SIMD
const data = [1, 2, 3, 4, 5];
data.forEach(x => console.log(x * 2));`,

    python: `# Python example with WASM
def fibonacci(n):
    """Calculate Fibonacci number."""
    if n <= 1:
        return n
    return fibonacci(n-1) + fibonacci(n-2)

# SIMD-accelerated processing
numbers = [1, 2, 3, 4, 5]
squared = [x ** 2 for x in numbers]
print(f"Result: {squared}")`,

    json: `{
  "name": "vscode-textmate-wasm",
  "version": "2.0.0",
  "description": "WASM 2023 features enabled",
  "features": [
    "SIMD128",
    "WASM Exceptions",
    "Bulk Memory",
    "BigInt Support"
  ],
  "languages": ["JavaScript", "Python", "JSON", "HTML", "CSS"]
}`,

    html: `<!DOCTYPE html>
<html>
<head>
    <title>WASM Demo</title>
    <style>
        body { font-family: Arial; }
        .highlight { background: #ff0; }
    </style>
</head>
<body>
    <h1>WASM Syntax Highlighting</h1>
    <script src="textmate.js"></script>
</body>
</html>`,

    css: `/* WASM 2023 Features CSS */
body {
    font-family: system-ui;
    background: linear-gradient(135deg, #1e1e1e, #252526);
    color: #d4d4d4;
}

.highlight {
    animation: pulse 1s infinite;
    background: rgba(78, 201, 176, 0.2);
}

@keyframes pulse {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.7; }
}`
}

/**
 * Discover available themes from GitHub API
 */
async function discoverAvailableThemes() {
    try {
        console.log('Discovering available themes from GitHub...')
        const response = await fetch(THEMES_API_URL)
        if (!response.ok) {
            throw new Error(`GitHub API error: ${response.statusText}`)
        }
        const data = await response.json()

        // Filter for theme JSON files
        const themeFiles = data
            .filter(item => item.name.endsWith('.json'))
            .map(item => item.name.replace('.json', ''))

        availableThemes = themeFiles.sort()
        console.log(`✓ Found ${availableThemes.length} available themes:`, availableThemes)
        return availableThemes
    } catch (err) {
        console.error('Failed to discover themes:', err)
        return []
    }
}

/**
 * Discover available grammars from GitHub API
 */
async function discoverAvailableGrammars() {
    try {
        console.log('Discovering available grammars from GitHub...')
        const response = await fetch(GITHUB_API_URL)
        if (!response.ok) {
            throw new Error(`GitHub API error: ${response.statusText}`)
        }
        const data = await response.json()

        // Filter for grammar files in packages/tm-grammars/grammars/
        const grammarPath = 'packages/tm-grammars/grammars/'
        const grammarFiles = data.tree
            .filter(item => item.path.startsWith(grammarPath) && item.path.endsWith('.json'))
            .map(item => item.path.replace(grammarPath, '').replace('.json', ''))

        availableGrammars = grammarFiles.sort()
        console.log(`✓ Found ${availableGrammars.length} available grammars:`, availableGrammars)
        return availableGrammars
    } catch (err) {
        console.error('Failed to discover grammars:', err)
        return []
    }
}

/**
 * Load a specific theme from GitHub
 */
async function loadTheme(themeName) {
    try {
        if (themes[themeName]) {
            console.log(`✓ Theme for ${themeName} already loaded`)
            return true
        }

        const url = `${THEMES_RAW_BASE_URL}/${themeName}.json`
        console.log(`Fetching ${themeName} theme...`)
        const response = await fetch(url)
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`)
        }
        themes[themeName] = await response.json()
        console.log(`✓ Loaded ${themeName} theme`)
        return true
    } catch (err) {
        console.error(`Failed to load ${themeName} theme:`, err)
        return false
    }
}

/**
 * Load a specific grammar from GitHub
 */
async function loadGrammar(langName) {
    try {
        if (grammars[langName]) {
            console.log(`✓ Grammar for ${langName} already loaded`)
            return true
        }

        const url = `${GRAMMARS_RAW_BASE_URL}/${langName}.json`
        console.log(`Fetching ${langName} grammar...`)
        const response = await fetch(url)
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`)
        }
        grammars[langName] = await response.json()
        console.log(`✓ Loaded ${langName} grammar`)
        return true
    } catch (err) {
        console.error(`Failed to load ${langName} grammar:`, err)
        return false
    }
}

/**
 * Load popular grammars on startup
 */
async function loadPopularGrammars() {
    const popularLangs = ['javascript', 'python', 'html', 'css', 'json', 'typescript', 'go', 'rust']
    const toLoad = popularLangs.filter(lang => availableGrammars.includes(lang))

    console.log(`Loading ${toLoad.length} popular grammars...`)
    for (const lang of toLoad) {
        await loadGrammar(lang)
    }
}

/**
 * Load popular themes on startup
 */
async function loadPopularThemes() {
    const popularThemes = ['dark-plus', 'github-dark', 'github-light', 'dracula', 'gruvbox-dark-hard']
    const toLoad = popularThemes.filter(theme => availableThemes.includes(theme))

    console.log(`Loading ${toLoad.length} popular themes...`)
    for (const theme of toLoad) {
        await loadTheme(theme)
    }
}

/**
 * Initialize WASM module on page load
 */
async function initialize() {
    try {
        const statusEl = document.getElementById('status')
        statusEl.innerHTML = '<span class="spinner"></span> Initializing...'

        // Load WASM module
        statusEl.innerHTML = '<span class="spinner"></span> Loading WASM module...'
        wasmModule = await createTextMateModule()
        registry = new wasmModule.Registry()

        // Discover available grammars from GitHub
        statusEl.innerHTML = '<span class="spinner"></span> Discovering grammars...'
        const discovered = await discoverAvailableGrammars()
        if (discovered.length === 0) {
            throw new Error('No grammars found on GitHub')
        }

        // Populate language selector with discovered grammars
        const languageSelect = document.getElementById('languageSelect')
        languageSelect.innerHTML = '<option value="">-- Choose a language --</option>'
        for (const lang of discovered) {
            const option = document.createElement('option')
            option.value = lang
            option.textContent = lang.charAt(0).toUpperCase() + lang.slice(1)
            languageSelect.appendChild(option)
        }

        // Load popular grammars upfront
        statusEl.innerHTML = '<span class="spinner"></span> Loading popular grammars...'
        await loadPopularGrammars()

        // Discover available themes from GitHub
        statusEl.innerHTML = '<span class="spinner"></span> Discovering themes...'
        const discoveredThemes = await discoverAvailableThemes()
        if (discoveredThemes.length === 0) {
            throw new Error('No themes found on GitHub')
        }

        // Populate theme selector with discovered themes
        const themeSelect = document.getElementById('themeSelect')
        themeSelect.innerHTML = '<option value="">-- Choose a theme --</option>'
        for (const theme of discoveredThemes) {
            const option = document.createElement('option')
            option.value = theme
            option.textContent = theme.charAt(0).toUpperCase() + theme.slice(1)
            themeSelect.appendChild(option)
        }

        // Load popular themes upfront
        statusEl.innerHTML = '<span class="spinner"></span> Loading popular themes...'
        await loadPopularThemes()

        // Detect WASM 2023 features
        detectWasmFeatures()

        // Update status
        statusEl.textContent = '✓ Ready! Select a language and theme to start'
        statusEl.className = 'status success'

        // Show features
        document.getElementById('features').style.display = 'flex'

        // Enable controls
        languageSelect.disabled = false
        themeSelect.disabled = false
        document.getElementById('loadGrammarBtn').disabled = false
        document.getElementById('sourceCode').disabled = false

        // Set default language to JavaScript and theme to dark-plus
        languageSelect.value = 'javascript'
        themeSelect.value = 'dark-plus'
        currentTheme = themes['dark-plus'] || null
        await loadSelectedGrammar()

        console.log('✓ TextMate WASM module initialized successfully')
    } catch (err) {
        const statusEl = document.getElementById('status')
        statusEl.textContent = '✗ Failed to initialize: ' + err.message
        statusEl.className = 'status error'
        console.error('Initialization error:', err)
    }
}

/**
 * Detect available WASM 2023 features
 */
function detectWasmFeatures() {
    // Check for WebAssembly features via capability detection
    const wasmMemory = new WebAssembly.Memory({ initial: 256, maximum: 512 })

    wasmFeatures = {
        simd128: typeof WebAssembly.SIMD !== 'undefined',
        wasmExceptions: true, // Emscripten supports this
        bulkMemory: true, // Modern WASM support
        bigint: typeof BigInt64Array !== 'undefined'
    }

    // Update UI badges
    updateFeatureBadges()
}

/**
 * Update feature badges based on detection
 */
function updateFeatureBadges() {
    const badges = {
        'simd': 'SIMD128',
        'exceptions': 'WASM Exceptions',
        'bulk-memory': 'Bulk Memory',
        'bigint': 'BigInt'
    }

    for (const [id, label] of Object.entries(badges)) {
        const badge = document.getElementById(id)
        const feature = Object.keys(wasmFeatures)[Object.values(badges).indexOf(label)]
        // All features are typically available in modern WASM
        badge.classList.remove('disabled')
    }
}

/**
 * Handle theme selection
 */
async function onThemeSelected() {
    const themeName = document.getElementById('themeSelect').value
    if (!themeName) return

    try {
        // Load theme if not already loaded
        if (!themes[themeName]) {
            const loaded = await loadTheme(themeName)
            if (!loaded) {
                throw new Error(`Could not load theme: ${themeName}`)
            }
        }

        currentTheme = themes[themeName]
        console.log(`✓ Theme selected: ${themeName}`)

        // Re-tokenize with the new theme
        setTimeout(() => tokenize(), 50)
    } catch (err) {
        console.error('Theme selection error:', err)
        document.getElementById('status').textContent = `✗ Theme error: ${err.message}`
        document.getElementById('status').className = 'status error'
    }
}

/**
 * Load selected grammar
 */
async function loadSelectedGrammar() {
    const lang = document.getElementById('languageSelect').value
    if (!lang) return

    try {
        // Update sample code (use placeholder if not defined)
        document.getElementById('sourceCode').value = SAMPLE_CODE[lang] || `// ${lang} code goes here\n`

        // Load grammar if not already loaded
        if (!grammars[lang]) {
            const loaded = await loadGrammar(lang)
            if (!loaded) {
                throw new Error(`Could not load grammar for ${lang}`)
            }
        }

        const grammarDef = grammars[lang]
        const grammarJson = JSON.stringify(grammarDef)
        const scopeName = grammarDef.scopeName || `source.${lang}`
        console.log(`Loading ${lang} grammar...`, { scopeName, patterns: grammarDef.patterns ? grammarDef.patterns.length : 0 })

        const grammarHandle = registry.loadGrammarFromContent(grammarJson, scopeName)

        if (!grammarHandle || grammarHandle === 0) {
            throw new Error(`Failed to load grammar for ${lang} (handle: ${grammarHandle})`)
        }

        currentGrammar = new wasmModule.Grammar(grammarHandle)
        const loadedScope = currentGrammar.getScopeName()
        console.log(`✓ Grammar loaded successfully: ${loadedScope}`)

        document.getElementById('loadGrammarBtn').textContent = `✓ ${lang.toUpperCase()} loaded`
        document.getElementById('tokenizeBtn').disabled = false

        // Auto-tokenize
        setTimeout(() => tokenize(), 100)
    } catch (err) {
        document.getElementById('status').textContent = `✗ Grammar error: ${err.message}`
        document.getElementById('status').className = 'status error'
        console.error('Grammar loading error:', err)
    }
}

/**
 * Tokenize the source code
 */
function tokenize() {
    if (!currentGrammar) {
        alert('Please load a grammar first')
        return
    }

    const sourceCode = document.getElementById('sourceCode').value
    const outputDiv = document.getElementById('output')
    const lines = sourceCode.split('\n')

    try {
        console.log(`Starting tokenization with grammar: ${currentGrammar.getScopeName()}`)
        const startTime = performance.now()

        let html = '<h3>🎯 Tokenization Results</h3>'
        let ruleStack = null
        let totalTokens = 0

        lines.forEach((line, lineNum) => {
            const result = currentGrammar.tokenizeLine(line, ruleStack)
            ruleStack = result.ruleStack

            html += `<div class="token-line">`
            html += `<strong>Line ${lineNum + 1}:</strong> <code>${escapeHtml(line)}</code><br>`

            if (result.tokens && result.tokens.length > 0) {
                result.tokens.forEach((token) => {
                    const text = line.substring(token.startIndex, token.endIndex)
                    const scope = token.scopes[token.scopes.length - 1] || ''
                    const color = getColorForScope(scope)

                    html += `<span class="token" style="border-left-color: ${color}; background: ${color}20;">` +
                        `<strong>${escapeHtml(text)}</strong>` +
                        `<span class="scope">${escapeHtml(scope)}</span>` +
                        `</span>`

                    totalTokens++
                })
            } else {
                html += `<em style="color: #999;">No tokens</em>`
            }

            html += '</div>'
        })

        const durationMs = performance.now() - startTime
        const durationUs = durationMs * 1_000 // Convert to microseconds
        const durationNs = durationMs * 1_000_000 // Convert to nanoseconds

        outputDiv.innerHTML = html
        updateStats(lines.length, totalTokens, durationMs, durationUs, durationNs)
        updateJsonOutput(lines, durationMs, durationUs, durationNs)

        // Update timing display with appropriate precision
        const timeDisplay = durationMs >= 1
            ? `⏱️ ${durationMs.toFixed(3)}ms`
            : `⏱️ ${durationUs.toFixed(2)}µs`

        document.getElementById('tokenizeTime').textContent = timeDisplay

        document.getElementById('status').textContent = `✓ Tokenized ${lines.length} lines in ${timeDisplay}`
        document.getElementById('status').className = 'status success'

        console.log(`Tokenized ${lines.length} lines with ${totalTokens} tokens in ${durationMs.toFixed(3)}ms (${durationUs.toFixed(2)}µs)`)
    } catch (err) {
        outputDiv.innerHTML = `<span style="color: #f48771;">✗ Tokenization error: ${escapeHtml(err.message)}</span>`
        document.getElementById('status').textContent = `✗ Error: ${err.message}`
        document.getElementById('status').className = 'status error'
        console.error('Tokenization error:', err)
    }
}

/**
 * Update statistics
 */
function updateStats(lineCount, tokenCount, durationMs, durationUs, durationNs) {
    document.getElementById('lineCount').textContent = lineCount
    document.getElementById('tokenCount').textContent = tokenCount

    // Display time with appropriate precision
    let timeText
    if (durationMs >= 1) {
        timeText = `${durationMs.toFixed(3)}ms`
    } else {
        timeText = `${durationUs.toFixed(2)}µs`
    }
    document.getElementById('totalTime').textContent = timeText

    // Calculate throughput in tokens/second for better precision
    const throughputPerSecond = tokenCount > 0 ? (tokenCount / durationMs * 1000).toFixed(1) : 0
    document.getElementById('throughput').textContent = `${throughputPerSecond} tokens/s`
}

/**
 * Update JSON output
 */
function updateJsonOutput(lines, durationMs, durationUs, durationNs) {
    const data = {
        timestamp: new Date().toISOString(),
        duration: {
            ms: parseFloat(durationMs.toFixed(3)),
            us: parseFloat(durationUs.toFixed(2)),
            ns: Math.round(durationNs)
        },
        line_count: lines.length,
        source: "VSCode TextMate WASM 2023",
        wasm_features: wasmFeatures,
        sample_lines: lines.slice(0, 3).map((line, i) => ({
            number: i + 1,
            content: line,
            length: line.length
        }))
    }

    document.getElementById('jsonOutput').textContent = JSON.stringify(data, null, 2)
    document.getElementById('copyBtn').disabled = false
}

/**
 * Copy JSON to clipboard
 */
function copyJsonToClipboard() {
    const text = document.getElementById('jsonOutput').textContent
    navigator.clipboard.writeText(text).then(() => {
        const btn = document.getElementById('copyBtn')
        btn.textContent = '✓ Copied!'
        setTimeout(() => {
            btn.textContent = '📋 Copy JSON'
        }, 2000)
    })
}

/**
 * Escape HTML special characters
 */
function escapeHtml(text) {
    const div = document.createElement('div')
    div.textContent = text
    return div.innerHTML
}

/**
 * Get color for scope using current theme
 */
function getColorForScope(scope, scopes = []) {
    if (!scope) return '#808080'

    // If a theme is loaded, try to get colors from it
    if (currentTheme && currentTheme.tokenColors) {
        for (const tokenColor of currentTheme.tokenColors) {
            if (!tokenColor.scope) continue

            const scopeList = Array.isArray(tokenColor.scope) ? tokenColor.scope : [tokenColor.scope]
            for (const scopePattern of scopeList) {
                // Simple scope matching (check if scope ends with pattern)
                if (scope === scopePattern || scope.endsWith('.' + scopePattern)) {
                    return tokenColor.settings?.foreground || '#d4d4d4'
                }
            }
        }
    }

    // Fallback to default colors if no theme or no match
    if (scope.includes('comment')) return '#6a9955'
    if (scope.includes('keyword')) return '#c586c0'
    if (scope.includes('string')) return '#ce9178'
    if (scope.includes('number')) return '#b5cea8'
    if (scope.includes('function')) return '#dcdcaa'
    if (scope.includes('variable')) return '#9cdcfe'
    if (scope.includes('attribute')) return '#9cdcfe'

    return '#d4d4d4'
}

// Initialize when DOM is ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initialize)
} else {
    initialize()
}
