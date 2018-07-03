let fs = require('fs')
let path = require('path')

const OpCodes = `
    plus minus mul div
    sin cos atan
    texture2D
    step
    length
    lt gt lte gte eq
    assign
    max min
    fract power mix dot clamp normalize smoothstep floor cross unary_minus or and

    _define
    _forget
    _ifeq
    _endif
    _output

    _function
    _return
`.split(/\s+/).map(s => s.trim()).filter(s => s.length);

fs.writeFileSync(path.join(".", "glsl", "src", "opcodes.js"), `
export default OpCode = {
    ${OpCodes.map((opcode, idx) => `${opcode}: ${idx+1}`).join(",\n    ")}
}
`, {encoding: 'utf8'});

fs.writeFileSync(path.join(".", "dataflow", "src", "model", "opcodes.h"), `
#pragma once
enum OpCode {
    ${OpCodes.map((opcode, idx) => `${opcode}_op = ${idx+1}`).join(",\n    ")}
};
#define MAX_OPS ${OpCodes.length+1}
`)

