#!/usr/bin/env node

/**
 * VSCode TextMate WASM CLI
 * Command-line interface for syntax highlighting
 */

const fs = require('fs')
const path = require('path')

// Color codes for terminal output
const colors = {
    reset: '\x1b[0m',
    bright: '\x1b[1m',
    dim: '\x1b[2m',
    red: '\x1b[31m',
    green: '\x1b[32m',
    yellow: '\x1b[33m',
    blue: '\x1b[34m',
    cyan: '\x1b[36m',
    white: '\x1b[37m',
}

// Simple grammars
const GRAMMARS = {
    javascript: {
        scopeName: "source.js",
        patterns: [
            { name: "comment.line", match: "//.*$" },
            { name: "keyword", match: "\\b(const|let|var|function|return|if|else)\\b" },
            { name: "string", match: '"([^"\\\\]|\\\\.)*"' },
            { name: "number", match: "\\b\\d+\\.?\\d*\\b" }
        ]
    },
    python: {
        scopeName: "source.python",
        patterns: [
            { name: "comment.line", match: "#.*$" },
            { name: "keyword", match: "\\b(def|class|return|if|else)\\b" },
            { name: "string", match: '"([^"\\\\]|\\\\.)*"' },
            { name: "number", match: "\\b\\d+\\.?\\d*\\b" }
        ]
    }
}

/**
 * Print help message
 */
function printHelp() {
    console.log(`
${colors.bright}VSCode TextMate WASM CLI${colors.reset}

${colors.blue}Usage:${colors.reset}
  node cli.js --grammar <language> --file <path>
  node cli.js --grammar <language> < input.txt

${colors.blue}Options:${colors.reset}
  --grammar <lang>   Language grammar (${Object.keys(GRAMMARS).join(', ')})
  --file <path>      Input file path
  --json             Output as JSON
  --stats            Show statistics

${colors.blue}Examples:${colors.reset}
  node cli.js --grammar javascript --file main.js
  node cli.js --grammar python --stats < script.py
  echo "const x = 42" | node cli.js --grammar javascript
    `)
}

/**
 * Parse command line arguments
 */
function parseArgs() {
    const args = process.argv.slice(2)
    const options = {
        grammar: null,
        file: null,
        json: false,
        stats: false
    }

    for (let i = 0; i < args.length; i++) {
        switch (args[i]) {
            case '--help':
            case '-h':
                return null
            case '--grammar':
                options.grammar = args[++i]
                break
            case '--file':
                options.file = args[++i]
                break
            case '--json':
                options.json = true
                break
            case '--stats':
                options.stats = true
                break
        }
    }

    return options
}

/**
 * Main CLI function
 */
async function main() {
    const options = parseArgs()

    if (!options) {
        printHelp()
        process.exit(0)
    }

    if (!options.grammar || !GRAMMARS[options.grammar]) {
        console.error(`${colors.red}Error: Grammar '${options.grammar}' not supported${colors.reset}`)
        console.error(`Supported grammars: ${Object.keys(GRAMMARS).join(', ')}`)
        process.exit(1)
    }

    try {
        // Try to load WASM module
        let wasmModule
        try {
            wasmModule = require('../dist/textmate-standard.js')
        } catch (e) {
            console.error(`${colors.red}Error: Cannot load WASM module${colors.reset}`)
            console.error('Make sure to build the WASM module first:')
            console.error('  cd ../../textmate-cpp')
            console.error('  mkdir build-wasm-demo && cd build-wasm-demo')
            console.error('  source ~/dev/emsdk/emsdk_env.sh')
            console.error('  emcmake cmake -DCMAKE_BUILD_TYPE=Release -DUSE_WASM_BUILD=ON ..')
            console.error('  cmake --build .')
            process.exit(1)
        }

        // Note: Full WASM functionality requires Node.js support via node-loader or similar
        console.log(`${colors.yellow}Note: Full WASM support in Node.js requires additional setup${colors.reset}`)
        console.log(`${colors.yellow}Using JavaScript fallback for demonstration${colors.reset}`)

        // Read input
        let input
        if (options.file) {
            input = fs.readFileSync(options.file, 'utf-8')
        } else {
            input = fs.readFileSync(0, 'utf-8')
        }

        const lines = input.split('\n')
        const grammar = GRAMMARS[options.grammar]

        if (options.json) {
            outputJson(lines, grammar, options)
        } else {
            outputTerminal(lines, grammar, options)
        }

    } catch (err) {
        console.error(`${colors.red}Error: ${err.message}${colors.reset}`)
        process.exit(1)
    }
}

/**
 * Output results as JSON
 */
function outputJson(lines, grammar, options) {
    const result = {
        timestamp: new Date().toISOString(),
        grammar: grammar.scopeName,
        line_count: lines.length,
        stats: options.stats ? {} : undefined,
        lines: lines.map((line, i) => ({
            number: i + 1,
            content: line,
            length: line.length
        }))
    }

    if (options.stats) {
        result.stats = {
            total_lines: lines.length,
            total_chars: lines.reduce((sum, line) => sum + line.length, 0),
            avg_line_length: (lines.reduce((sum, line) => sum + line.length, 0) / lines.length).toFixed(2)
        }
    }

    console.log(JSON.stringify(result, null, 2))
}

/**
 * Output results in terminal format
 */
function outputTerminal(lines, grammar, options) {
    console.log(`${colors.cyan}${colors.bright}Grammar: ${grammar.scopeName}${colors.reset}`)
    console.log(`${colors.dim}Lines: ${lines.length}${colors.reset}\n`)

    lines.forEach((line, lineNum) => {
        console.log(`${colors.dim}${String(lineNum + 1).padStart(3, ' ')} ${colors.reset}${line}`)
    })

    if (options.stats) {
        const totalChars = lines.reduce((sum, line) => sum + line.length, 0)
        const avgLineLength = (totalChars / lines.length).toFixed(2)

        console.log(`\n${colors.bright}Statistics:${colors.reset}`)
        console.log(`  Total lines:     ${lines.length}`)
        console.log(`  Total characters: ${totalChars}`)
        console.log(`  Average length:  ${avgLineLength}`)
    }
}

// Run main function
if (require.main === module) {
    main().catch(err => {
        console.error(`${colors.red}Unexpected error: ${err.message}${colors.reset}`)
        process.exit(1)
    })
}

module.exports = { GRAMMARS }
