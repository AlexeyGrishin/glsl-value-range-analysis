import TokenString from 'glsl-tokenizer/string'
import ParseTokens from 'glsl-parser/direct'
import {VariablesMap, convert, devectorize, addVarsInOut, prepareForAnalysis, varPtr, Components} from './ast2dataflow'
import Range from './range'
import OpCode from "./opcodes"
import UIController from "./ui"

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
uniform sampler2D sampler;
uniform float a;
uniform float b/*0,1*/; 
uniform vec3 c/*-1,1*/;

void main() {
    vec4 color = texture2D(sampler, vec2(0.5));
    vec2 k;
    k.xy = sin(color+a).rg;
    if (k.x > 0.5) {
        gl_FragColor.r = k.x * 2.;
    } else {
        gl_FragColor.r = k.x / 2.;
    }
    gl_FragColor.a = 2.;
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

const DEBUG = false;
function process(src) {

    let tree = ParseTokens(TokenString(src));
    let map = new VariablesMap();
    map.addVariable("gl_FragColor", "vec4");

    function idToStr(varPtr) {
        if (varPtr.id === 0) return '_';
        let variable = map.getById(varPtr.id);
        if (!variable) return `?${varPtr.id}`;
        return `${variable.type} ${variable.name}${varPtr.part ? ("." + varPtr.part) : ""}`
    }

    function op2string(op) {
        return `${op.line ? (op.line + ': ') : ''}${op.out.map(idToStr).join(",")} = ${op.op}(${op.args.map(idToStr).join()}${op.range ? (',' + op.range) : ''})`
    }
    if (DEBUG) {
        console.groupCollapsed("tree");
        printNode(tree);
        console.groupEnd();
    }
    let pass1 = convert(tree, map);
    pass1.push({op: "_output", out: [], args: [varPtr(map.getVariable("gl_FragColor"))]});
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
        console.group("pass3");
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
    for (let op of pass4) {
        //console.log('process', op);
        let ret = flow.addCommand(op.cmdId, op.opCode, op.args, op.range.left, op.range.right, op.range.flag);
        //if (ret) console.log("  ERROR: ", ret);
    }
    let report = [];
    flow.getReport(report);
    if (DEBUG) {
        //let filter = () => true;
        let filter = (r) => r.varId == 1;
        console.groupCollapsed("response");
        console.log(JSON.stringify(report.filter(filter), null, 4));
        console.groupEnd();
    }
    //console.log('returned', report.length);
    flow.fini();

    return new HumanReport(fixReport(report), map, pass4, src);//{map, report};
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

class HumanReport
{
    constructor(report, map, ops, src) {

        function getRealVarName(varId) {
            let variable = map.getById(varId);
            if (!variable) return {print: "?unknown?", highlight: null};
            let name = variable.name;
            if (name.indexOf(".") !== -1) {
                let [main, suffix] = name.split(".");
                let foundMain = Components.main.split('').some(ms => src.indexOf(`${main}.${ms}`) != -1);
                let foundColor = Components.color.split('').some(ms => src.indexOf(`${main}.${ms}`) != -1);
                if (foundColor && !foundMain) {
                    suffix = Components.color.charAt(Components.main.indexOf(suffix));
                }
                return {print: `${main}.${suffix}`, highlight: "main."}
            }
            return {print: name, highlight: name}
        }

        function formatVarChange(varChange) {
            let varName = getRealVarName(varChange.varId);
            let newRange = varChange.range;
            return `${varName.print} ∈ ${newRange.toString()}`;
        }
 
        function getOriginalOp(cmdId) { return ops.find(op => op.cmdId == cmdId)}
        //1. branches
        let branches = report.filter(b => b.type === "branch");
        let branchesMap = new Map();
        for (let b of branches) {
            b.op = getOriginalOp(b.cmdId);
            let varChange = report.find(c => c.branchId == b.branchId && c.type == "change" && c.cmdId == b.cmdId && c.varId == b.varId);
            if (b.branchId != 0 && varChange) {
                b.reason = formatVarChange(varChange);
            }
            b.line = b.op ? b.op.line : -1;
            branchesMap.set(+b.branchId, b);
        }

        for (let b of branches) {
            if (b.branchId != 0) {
                b.parent = branchesMap.get(+b.parentId);
            }
        }

        function getFullBranchConditions(branchId) {
            let branch = branchesMap.get(+branchId);
            let conds = [];
            while (branch != null) {
                if (branch.reason) conds.unshift(branch.reason);
                branch = branch.parent;
            }
            return conds.join(" && ");
        }

        let activeBranches = branches.filter(b => b.active);

        function findLastChangeForVar(varId, branchId) {
            let b = branchesMap.get(branchId);
            while (b) {
                let lastChange = report.filter(r => r.type == "change" && r.varId == varId && r.branchId == branchId).pop();
                if (lastChange) return lastChange;
                b = b.parent;
            }
            return null;
        }

        //2. warnings
        let dupWarnings = report.filter(r => r.type === "warning").map(w => {
            let op = getOriginalOp(w.cmdId);
            let text = `Call to ${op.op}: ${getRealVarName(w.varId).print} expected to fit ${w.expectedRange}, but it is ${w.actualRange}`;
            if (op.opCode === OpCode._output) {
                text = `${getRealVarName(w.varId).print} expected to fit ${w.expectedRange}, but it is ${w.actualRange}`;
            }
            let line = op.line;
            if (!line || line == -1) {
                let lastChange = findLastChangeForVar(w.varId, w.branchId);
                if (lastChange) {
                    line = getOriginalOp(lastChange.cmdId).line;
                }
            }
            return {
                text: text,
                line: line,
                branchId: w.branchId
            }
        });
        //  look for duplicates, unite
        this.warnings = [];
        for (let w of dupWarnings) {
            if (w.isDuplicate) continue;
            let duplicates = dupWarnings.filter(dw => dw.text == w.text && dw != w);
            let branches = [w.branchId];
            if (duplicates.length) {
                for (let dup of duplicates) {
                    branches.push(dup.branchId);
                    dup.isDuplicate = true;
                }
            }
            let conditions = branches.map(getFullBranchConditions)
                .filter(c => c.trim().length);

            let str = conditions.length === 1 ? conditions[0] : conditions.map(c => "(" + c + ")").join(" || ");
            if (conditions.length == activeBranches.length) {
                str = "";
            }
            if (str.length > 0) {
                w.condition = str;
                //w.text = "When " + str + ":\n " + w.text;
            }
            this.warnings.push(w);
        }
    }    
}




UIController.init(src, (src) => process(src));