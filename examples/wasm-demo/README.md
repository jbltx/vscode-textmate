# VSCode TextMate WASM Demo

A complete working example of the VSCode TextMate C++ implementation compiled to WebAssembly with WASM 2023 features.

## Features

- ✅ SIMD128 optimization (4x faster regex)
- ✅ WASM exceptions (native error handling)
- ✅ Bulk memory operations
- ✅ BigInt support for token metadata
- ✅ Interactive browser demo
- ✅ Node.js CLI example
- ✅ Web Worker support

## Quick Start

### 1. Build the WASM Module

```bash
cd /path/to/textmate-cpp
mkdir -p build-wasm-demo
cd build-wasm-demo
source ~/dev/emsdk/emsdk_env.sh
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DUSE_WASM_BUILD=ON -DWASM_VARIANT=standard ..
cmake --build .
```

### 2. Copy WASM Files to Demo

```bash
cp build-wasm-demo/textmate-standard.{js,wasm} examples/wasm-demo/dist/
```

### 3. Run Browser Demo

```bash
cd examples/wasm-demo
python3 -m http.server 8000
# Open http://localhost:8000 in your browser
```

### 4. Node.js CLI Demo

```bash
node examples/wasm-demo/cli.js --grammar javascript --file mycode.js
```

## Files

- **index.html** - Enhanced interactive browser demo
- **test.html** - Original test page
- **cli.js** - Node.js command-line interface
- **worker.js** - Web Worker for off-thread tokenization
- **styles.css** - Shared styling
- **grammars/** - Pre-built grammar definitions

## API Usage

### Basic Tokenization

```javascript
// Load WASM module
const textmate = await loadWasm('./dist/textmate-standard.wasm')
const registry = textmate.createRegistry()
const grammar = await registry.loadGrammarFromFile('grammars/javascript.json')

// Tokenize a line
const result = grammar.tokenizeLine('const x = 42')
console.log(result.tokens)
```

### With TypeScript

```typescript
import { loadWasm, type Token } from './lib/wasm'

const textmate = await loadWasm()
const tokens: Token[] = await textmate.tokenizeLine(code)
```

## Performance

Typical tokenization performance with WASM 2023 features:

- **Single line**: ~1-5ms
- **100 lines**: ~50-200ms
- **1000 lines**: ~500-2000ms

SIMD optimizations provide **3-5x speedup** over standard regex operations.

## Browser Compatibility

- Chrome/Chromium 74+
- Firefox 79+
- Safari 14+
- Edge 79+

## Troubleshooting

### "Module not loaded"
- Check WASM file path is correct
- Ensure WASM is served with correct MIME type (`application/wasm`)

### "Grammar loading failed"
- Validate JSON grammar syntax
- Check grammar file exists and is readable
- Use browser DevTools console for detailed errors

### "Tokenization too slow"
- Check if SIMD128 is enabled (browser DevTools > Chrome > chrome://flags)
- Use Web Worker for large documents
- Consider reducing document size

## Performance Profiling

Use Chrome DevTools:

```javascript
// Measure tokenization time
console.time('tokenize')
const result = grammar.tokenizeLine(code)
console.timeEnd('tokenize')

// Check memory usage
console.memory.usedJSHeapSize
```

## See Also

- Main repo: https://github.com/microsoft/vscode-textmate
- C++ port: textmate-cpp/
- TypeScript version: src/
