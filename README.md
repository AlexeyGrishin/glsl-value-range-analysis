## What is it

  Prototype for GLSL static value range analysis, which shall prevent cases like that:

  ```
  gl_FragColor.r = sin(something)+1.; //WARN: color shall be between 0 and 1
  ```
## Status

  Very-very alpha. It supports ~5% of GLSL functionality. But at least it covers the case from "What is it" section :)
  
## Where to try

  TBD gh-pages

## How to build  

### Preparation

1. Install `SConstruct`
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

### Build

* `npm run wasm` - to build dataflow analyzer from C to WebAssembly
* `npm run js` - to build js parser and UI
* `npm run server` - to run web server on port 3355. Go to http://localhost:3355 and try editor 

* `npm run dataflow-tests` to run some unit-tests for dataflow analyzer

## TODO

* Automate watch. It shall
  * watch for changes in dataflow/*. if changed:
    * run tests
    * run wasm build
  * watch for changes in glsl/*. if changed:
    * run js build
    
      