import TokenString from 'glsl-tokenizer/string'
import ParseTokens from 'glsl-parser/direct'
import {VariablesMap, convert, devectorize, addVarsInOut, prepareForAnalysis, varPtr, Components} from './ast2dataflow'
import Range from './range'
import UIController from "./ui"
import HumanReport from "./humanreport"

window.TypeRange = Range;

var src1 = `
    void main() {
        float a = 3.;
        gl_FragColor.r = a;
    }
`;

var src3 = `
uniform sampler2D sampler;

void main() {
    vec4 color = texture2D(sampler, vec2(0.5));  
    gl_FragColor.rgb = (color*10.).zyx;
}
`;

var src = `
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

    // Time varying pixel color
    //vec3 col = 0.5 + 0.5*cos(iTime+vec3(0,2,4));
    vec3 col = 0.5 + vec3(0,2,4);

    // Output to screen
    fragColor = vec4(col,1.0);
    //fragColor = vec4(1.0);
}
`;

var src4 = `
uniform sampler2D sampler;
uniform vec2 textureCoord /*0,1*/;

void main() {

    const float b = 0.2;
    float a = b;
    float c = 0.;
    for (int i = 0; i < 3; i++) {
        a += 0.1;
        c += float(i);
    }
    for (int i = 4; i > -5; i-=2) {
        a += b;
        if (a > 0.7) break;
    }
    a = power(a, -(-2.));

    gl_FragColor = mix(vec4(0.5), vec4(0.6), a);

}
`;

var src3 = `
uniform sampler2D sampler;
uniform vec2 textureCoord /*0,1*/;
uniform float a;
uniform float b/*0,1*/; 
uniform vec3 c/*-1,1*/;

vec2 mrfn(vec2 v) {
    if (v.x > 0.5 || v.y > 0.5) {
        return v/2.;
    }
    return clamp(v, 0, 0.5);
}

void main() {
    vec4 color = texture2D(sampler, vec2(0.5));
    vec2 k;
    k.xy = mrfn(sin(color+a).rg);
    if (k.x > 0.5) {
        gl_FragColor.r = k.x * 2. - b;
    } else {
        gl_FragColor.r = k.x / 2.;
    }
    gl_FragColor.a = step(0., k.x)*k.x;  //k.x < 0 ? 0 : 1
    gl_FragColor.gb = vec2(1.);
}

`
function printNode(node, indent = 0) {
    let type = node.type;
    let line = ""; let data = node.data;
    let preceding = "";
    if (node.token) {
        if (node.type !== node.token.type) type += ":" + node.token.type;
        if (!node.data) data = node.token.data;
        if (data !== node.token.data) data = node.data + ":" + node.token.data;
        if (node.token.preceding && node.token.preceding.some(t => t.type === "block-comment")) {
            preceding = "//" + node.token.preceding.find(t => t.type === "block-comment").data;
        }
    }
    if (!node.type.startsWith("placeholder")) console.log(`${"".padStart(indent)} ${line}: ${type} = ${data}${preceding}`);
    for (let c of node.children || []) printNode(c, indent+1);
}

function addCommonVars(map) {
    map.addVariable("gl_FragColor", "vec4");
}

function addShadertoyVars(map) {
    map.addVariable("fragColor", "vec4");
    map.addVariable("iTime", "float", "[0,Infinity)");
    map.addVariable("iResolution", "vec3", "[0,4000]");
    map.addVariable("fragCoord", "vec2", "[0,4000]");
    map.addVariable("iMouse", "vec4", "[0,4000]");
    map.addVariable("iChannel0", "sampler2D");
    map.addVariable("iChannel1", "sampler2D");
    map.addVariable("iChannel2", "sampler2D");
    map.addVariable("iChannel3", "sampler2D");
}

function getOutVarId(map) {
    let outVar = map.getVariable("fragColor"); //shadertoy
    if (!outVar) {
        outVar = map.getVariable("gl_FragColor");
    }
    return outVar ? varPtr(outVar) : null;
}

const DEBUG = true;
function process(src) {

    let tree = ParseTokens(TokenString(src));
    let map = new VariablesMap();
    addCommonVars(map);
    if (src.indexOf("mainImage(") !== -1 && src.indexOf("gl_FragColor") === -1) {
        addShadertoyVars(map);
    }

    function idToStr(varPtr) {
        if (varPtr.id === 0) return '_';
        let variable = map.getById(varPtr.id);
        if (!variable) return `?${varPtr.id}`;
        return `${variable.type} ${variable.name}${varPtr.part ? ("." + varPtr.part) : ""}`
    }

    function op2string(op) {
        return `${op.line ? (op.line + ': ') : ''}${op.out.map(idToStr).join(",")} = ${op.op}(${op.args.map(idToStr).join()}${op.range ? (',' + op.range) : ''})`
    }
    let pass1 = convert(tree, map);
    let outVarId = getOutVarId(map);
    if (outVarId) {
        pass1.push({op: "_output", out: [], args: [outVarId]});
    } //todo: else?
    if (DEBUG) {
        console.group("tree");
        printNode(tree);
        console.log(map);
        console.groupEnd();
    }
    if (DEBUG) {
        console.groupCollapsed("pass1");
        console.log(pass1.map(op2string).join("\n"));
        console.groupEnd();
    }
    let pass2 = devectorize(pass1, map);
    if (DEBUG) {
        console.groupCollapsed("pass2");
        console.log(pass2.map(op2string).join("\n"));
        console.groupEnd();
    }
    let pass3 = addVarsInOut(pass2, map);
    if (DEBUG) {
        console.groupCollapsed("pass3");
        console.log(pass3.map(op2string).join("\n"));
        console.groupEnd();
    }    
    let pass4 = prepareForAnalysis(pass3, map);
    if (DEBUG) {
        console.groupCollapsed("pass4");
        let o = [];
        for (let op of pass4) {
            o.push([op.cmdId, op.opCode, '(' + op.op + ')', op.args.toString(), op.range.left, op.range.right, op.range.flag].join(','));
        }
        console.log(o.join('\n'));
        console.groupEnd();
    }
    
    //console.log(pass4.map(JSON.stringify).join("\n"));
    let flow = new DataFlowApi();
    flow.init();
    //return;//todo temp
    if (DEBUG) {
        console.group("process");
    }
    for (let op of pass4) {
        //console.log('process', op);
        console.log('process', op.cmdId);
        let ret = flow.addCommand(op.cmdId, op.opCode, op.args, op.range.left, op.range.right, op.range.flag);
        console.log(' = ', ret, getErrorMessage(ret));
        //todo: show error in UI (if any);
        //if (ret) console.log("  ERROR: ", ret);
    }
    if (DEBUG) {
        console.groupEnd();
    }
    let report = [];
    flow.getReport(report);
    if (DEBUG) {
        let filter = () => true;
        //let filter = (r) => r.varId == 31;
        //let filter = (r) => r.varId == map.getVariable("k.x").id
        console.groupCollapsed("response");
        console.log(JSON.stringify(report.filter(filter), null, 4));
        console.groupEnd();
    }
    //console.log('returned', report.length);
    flow.fini();

    return new HumanReport(fixReport(report), map, pass4, src);//{map, report};
}

function getErrorMessage(ret) {
    return [
        "OK",
        "Unknown error",
        "Non-existent opCode",
        "Unknown variable",
        "Argument absent"
    ][ret] || "Unknown error";
}

function fixReport(report) {
    for (let item of report) {
        for (key in item) {
            if (item.hasOwnProperty(key)) {
                if (item[key] instanceof Number) {
                    item[key] = +item[key];
                }
            }
        }
    }
    return report;
}

window.DataFlowApi.promise.then(() => {
    UIController.enableButton();
    UIController.process();
});

UIController.init(src, (src) => process(src));

UIController.showTemp = true;