# WASM Demo Guide - Complete Walkthrough

This guide walks you through the complete VSCode TextMate WASM 2023 demo with all features enabled.

## 📦 What's Included

### Pre-built WASM Files
- **textmate-standard.wasm** (69 KB) - The compiled C++ TextMate library
- **textmate-standard.js** (22 KB) - Emscripten runtime wrapper

### Demo Applications
- **index.html** - Modern interactive browser demo
- **test.html** - Original test page
- **app.js** - Main application logic
- **styles.css** - Professional styling
- **cli.js** - Node.js command-line interface
- **build.sh** - Automated build script

## 🚀 Quick Start (3 Steps)

### Step 1: Navigate to Demo Directory
```bash
cd examples/wasm-demo
```

### Step 2: Start Web Server
```bash
python3 -m http.server 8000
```

### Step 3: Open Browser
```
http://localhost:8000
```

Done! You should see the interactive demo with:
- Language selection (JavaScript, Python, JSON, HTML, CSS)
- Live code editor
- Real-time tokenization
- Performance statistics
- WASM 2023 features display

## 🎯 Using the Browser Demo

### 1. Select a Language
- Click the language dropdown
- Choose from: JavaScript, Python, JSON, HTML, CSS
- Grammar loads automatically

### 2. Edit Code
- Modify the sample code in the editor
- Or paste your own code

### 3. Tokenize
- Click "⚡ Tokenize" button
- Results appear instantly
- Performance metrics shown

### 4. Inspect Results
- View token list with scopes
- See JSON representation
- Copy results to clipboard

## 💻 Using the CLI

### Basic Usage
```bash
node cli.js --grammar javascript --file mycode.js
```

### With Stdin
```bash
echo "const x = 42" | node cli.js --grammar javascript
```

### Show Statistics
```bash
node cli.js --grammar python --stats < script.py
```

### JSON Output
```bash
node cli.js --grammar javascript --file code.js --json
```

### Get Help
```bash
node cli.js --help
```

## 🏗️ Building from Source

If you need to rebuild the WASM module:

```bash
# Navigate to C++ implementation
cd ../../textmate-cpp

# Create build directory
mkdir -p build-wasm-demo
cd build-wasm-demo

# Activate Emscripten
source ~/dev/emsdk/emsdk_env.sh

# Configure with WASM 2023 features
emcmake cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DUSE_WASM_BUILD=ON \
  -DWASM_VARIANT=standard \
  ..

# Build
cmake --build .

# Copy to demo
cp textmate-standard.* ../examples/wasm-demo/dist/
```

Or use the automated script:
```bash
cd examples/wasm-demo
./build.sh
```

## 📊 WASM 2023 Features in Action

The demo showcases all enabled WASM 2023 features:

### 1. SIMD128 (4x faster regex)
- Enabled with `-msimd128` flag
- Accelerates pattern matching
- Faster token boundary detection

### 2. WASM Exceptions
- Native C++ exception handling
- No JavaScript interop overhead
- Enabled with `-fwasm-exceptions`

### 3. Bulk Memory
- Efficient data copying
- `memory.copy` operations
- Enabled with `-mbulk-memory`

### 4. BigInt Support
- 64-bit token metadata
- `BigInt64Array` compatibility
- Enabled with `-sWASM_BIGINT`

## 📈 Performance Characteristics

### Typical Performance
- Single line: 1-5ms
- 100 lines: 50-200ms
- 1000 lines: 500-2000ms

### Optimization Tips
1. **Use Web Workers** for large documents
2. **Enable SIMD** in browser flags
3. **Batch tokenization** for multiple lines
4. **Cache grammars** to avoid reloading

## 🔍 Debugging

### Browser DevTools
```javascript
// Measure performance
console.time('tokenize')
grammar.tokenizeLine(code)
console.timeEnd('tokenize')

// Check memory
console.log(performance.memory)

// View WASM module
console.log(window.createTextMateModule)
```

### Common Issues

**"Module not loaded"**
- Check if WASM files exist in `dist/`
- Ensure web server is running
- Check browser console for errors

**"Grammar loading failed"**
- Validate JSON grammar syntax
- Check grammar is properly formatted
- Use JSON validator tool

**"Slow tokenization"**
- Check if SIMD is available
- Try smaller file sizes
- Use Web Worker for large files

## 🎨 Customization

### Add a New Language

1. Define grammar in `app.js`:
```javascript
const GRAMMARS = {
    rust: {
        scopeName: "source.rust",
        patterns: [
            // Define patterns...
        ]
    }
}
```

2. Add sample code:
```javascript
const SAMPLE_CODE = {
    rust: `fn main() {
        println!("Hello, WASM!");
    }`
}
```

3. Add to language selector in `index.html`:
```html
<option value="rust">Rust</option>
```

### Customize Colors

Edit `app.js` `getColorForScope()` function:
```javascript
function getColorForScope(scope) {
    if (scope.includes('keyword')) return '#c586c0'  // Change this
    // ...
}
```

Or edit `styles.css` directly for predefined classes.

## 📚 API Examples

### Load Grammar
```javascript
const grammarJson = JSON.stringify(GRAMMARS.javascript)
const handle = registry.loadGrammarFromContent(grammarJson, 'source.js')
const grammar = new wasmModule.Grammar(handle)
```

### Tokenize Line
```javascript
const result = grammar.tokenizeLine('const x = 42', ruleStack)
console.log(result.tokens)  // Token array
console.log(result.ruleStack)  // State for next line
```

### Process Multiple Lines
```javascript
let ruleStack = null
const allTokens = []

for (const line of code.split('\n')) {
    const result = grammar.tokenizeLine(line, ruleStack)
    ruleStack = result.ruleStack
    allTokens.push(result.tokens)
}
```

## 🔗 Integration Examples

### React Component
```jsx
import { useEffect, useState } from 'react'

export function CodeHighlighter({ code, language }) {
    const [tokens, setTokens] = useState([])

    useEffect(() => {
        if (grammar) {
            const result = grammar.tokenizeLine(code, null)
            setTokens(result.tokens)
        }
    }, [code, grammar])

    return (
        <pre>
            {tokens.map(token => (
                <span key={token.startIndex} style={{ color: getColor(token) }}>
                    {code.substring(token.startIndex, token.endIndex)}
                </span>
            ))}
        </pre>
    )
}
```

### Vue Component
```vue
<template>
    <pre><span v-for="token in tokens" :key="token.startIndex" :style="{ color: getColor(token) }">
        {{ code.substring(token.startIndex, token.endIndex) }}
    </span></pre>
</template>

<script setup>
import { ref, watch } from 'vue'

const props = defineProps(['code', 'grammar'])
const tokens = ref([])

watch(() => props.code, (newCode) => {
    if (props.grammar) {
        const result = props.grammar.tokenizeLine(newCode, null)
        tokens.value = result.tokens
    }
})
</script>
```

## 📋 Checklist

Before using in production:

- [ ] Test in target browsers (Chrome, Firefox, Safari, Edge)
- [ ] Verify WASM file loads correctly
- [ ] Test with sample grammars
- [ ] Measure performance in your environment
- [ ] Set up error handling
- [ ] Configure MIME types correctly
- [ ] Implement caching strategy
- [ ] Add offline fallback (if needed)
- [ ] Test with large documents
- [ ] Profile memory usage

## 🐛 Troubleshooting

### WASM Module Not Found
```
Solution: Ensure dist/textmate-standard.wasm exists and is served correctly
```

### Grammar Loading Fails
```javascript
// Add debugging
const handle = registry.loadGrammarFromContent(json, scope)
if (!handle || handle === 0) {
    console.error('Grammar loading failed')
}
```

### Memory Issues
```javascript
// Monitor memory usage
if (performance.memory) {
    console.log(performance.memory.usedJSHeapSize / 1048576, 'MB')
}
```

## 📖 Additional Resources

- **Main Repository**: https://github.com/microsoft/vscode-textmate
- **Emscripten Docs**: https://emscripten.org/
- **WebAssembly Docs**: https://webassembly.org/
- **WASM 2023 Features**: https://github.com/WebAssembly/proposals

## 🎓 Learning Path

1. **Start here**: Run `index.html` and explore
2. **Try CLI**: Use `cli.js` for command-line interface
3. **Modify code**: Edit app.js to customize behavior
4. **Add grammar**: Create a new language grammar
5. **Integrate**: Use in your project
6. **Deploy**: Set up for production

## ✅ Success Indicators

Your setup is working correctly when:

- ✓ Browser demo loads without errors
- ✓ Language selector works
- ✓ Tokenization completes in <100ms
- ✓ Token results display correctly
- ✓ Performance stats show reasonable throughput
- ✓ WASM 2023 features are detected

Congratulations! You now have a working VSCode TextMate WASM implementation with WASM 2023 features enabled! 🎉
