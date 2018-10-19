## What is it

  Prototype for GLSL static value range analysis, which shall prevent cases like that:

  ```
  gl_FragColor.r = sin(something)+1.; //WARN: color shall be between 0 and 1
  ```
## Status

  Very-very alpha. It supports ~5% of GLSL functionality. But at least it covers the case from "What is it" section :)
  
## Where to try

  [Here](https://alexeygrishin.github.io/glsl-value-range-analysis/html/)

## How to build  

### Preparation

1. Install `SConstruct` (required for unit tests only)
2. Install `node` + `npm`
3. Install `google test` in your environment
  1. How I did it in MAC, and that shall work for Linux too:
  ```
  git clone https://github.com/google/googletest
  cd googletest
  mkdir build
  cd build
  cmake ..
  make
  make install
  ```
4. Install [https://leaningtech.com/cheerp/](cheerp)
5. Run `npm install` in root folder 
6. Depending on your OS:
  1. MacOS - nothing else required
  2. Windows/Linux - please replace path to `cheerp/bin/clang++` with valid one for your OS

### Build

* `npm run wasm` - to build dataflow analyzer from C to WebAssembly using cheerp. WASM is added to `html` folder
* `npm run js` - to build js parser and UI
* `npm run server` - to run web server on port 3355. Go to http://localhost:3355 and try editor 
* `npm run dataflow-tests` - to run some unit-tests for dataflow analyzer


## About code

 * `glsl` - contains test UI, and also converter for GLSL to Command Sequence which goes to data flow analyzer
  * `ast2dataflow.js` contains most of conversion logic
  * `index.js` calls `ast2dataflow.js` and passes result to data flow analyzer
 * `dataflow` - contains data flow analyzer, which goes through command sequence and tries to predict variables ranges
  * `analyzer.h` - main facade interface
  * `analisys_context.h` - common processing logic, all about variables and branches management
  * `ops/buildin_ops.cpp` - list of supported GLSL operations and functions
