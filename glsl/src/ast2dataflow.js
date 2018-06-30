import OpCode from "./opcodes"
import TypeRange from "./range"

function op(opCode, outTypeFn, devectorize) {
    let getOutType = typeof outTypeFn === "function" ? outTypeFn : () => outTypeFn;
    return {opCode, getOutType, devectorize}
}

function firstArg(inTypes) { return inTypes[0]; }
function secondArg(inTypes) { return inTypes[1]; }

const OpsMap = {
    "+": op(OpCode.plus, firstArg, devecMath),
    "-": op(OpCode.minus, firstArg, devecMath),
    "*": op(OpCode.mul, firstArg, devecMath),
    "/": op(OpCode.div, firstArg, devecMath),
    "sin": op(OpCode.sin, firstArg, devecMath),
    "cos": op(OpCode.cos, firstArg, devecMath),
    "<": op(OpCode.lt, "boolean", devecCompare),
    ">": op(OpCode.gt, "boolean", devecCompare),
    "<=": op(OpCode.lte, "boolean", devecCompare),
    ">=": op(OpCode.gte, "boolean", devecCompare),
    "==": op(OpCode.eq, "boolean", devecCompare),
    "texture2D": op(OpCode.texture2D, "vec4", devecCommon),
    "length": op(OpCode.length, "float", devecCommon),
    "step": op(OpCode.step, secondArg, devecMath),

    "vec4": op(undefined, "vec4", devecVecCtor),
    "vec3": op(undefined, "vec3", devecVecCtor),
    "vec2": op(undefined, "vec2", devecVecCtor),

    "=": op(OpCode.assign, secondArg, devecMath),

    "_define": op(OpCode._define),
    "_ifeq": op(OpCode._ifeq, undefined, devecCommon),
    "_endif": op(OpCode._endif, undefined, devecCommon),
    "_forget": op(OpCode._forget),
    "_output": op(OpCode._output, undefined, devecCommon)
};

//todo: type enums?
function predictOutType(operator, inTypes) {
    let knownOp = OpsMap[operator];
    if (knownOp) return knownOp.getOutType(inTypes);
    return undefined;
}


export class VariablesMap {
    constructor() {
        this.nextVarId = 1;
        this.vars = [null];
        this.nameToVar = new Map();
        this.constToVar = new Map();
        this.block = 1;
    }

    addVariable(name, type, initialRange) {
        let variable = {name, type, range: initialRange, id: this.nextVarId++};
        this.nameToVar.set(name, variable);
        this.vars.push(variable);
        return variable;   
    }

    getConst(value, type) {
        return this.constToVar.get(value);
    }

    addConst(value, type) {
        let id = this.nextVarId++;
        let variable = {name: `const(${value})`, id, type, range: value, value};
        this.nameToVar.set(name, variable);
        this.vars.push(variable);
        this.constToVar.set(value, variable);
        return variable; 
    }

    isConst(id) {
        let variable = this.getById(id);
        return variable.value !== undefined;
    }

    getVariables() {
        return this.vars.slice(1);
    }

    isTemp(id) {
        return this.getById(id).isTemp;
    }

    addTempVariable(type) {
        let id = this.nextVarId++;
        let variable = {name: `tmp${id}`, id, type};
        variable.isTemp = true;
        this.nameToVar.set(name, variable);
        this.vars.push(variable);
        return variable;   
    }

    rename(variable, newName) {
        this.nameToVar.delete(variable.name);
        variable.name = newName;
        this.nameToVar.set(newName, variable);
      
    } 

    getVariable(name) {
        return this.nameToVar.get(name);
    }

    getById(id) {
        if (typeof id === "object" && id.id !== undefined) {
            id = id.id;
        }
        return this.vars[id];
    }

    enterBlock() {
        this.block++;
    }

    exitBlock() {
        this.block--;
    }

}


const FullOpStruct = {
    op: "string",
    args: [{id: "varId", part: "xy"}, "..."],
    out: [{id: "varId", part: "xy"}, "..."],
    range: "range-as-str"
};

const SmallOpStruct = {
    cmdId: "number",
    opCode: "number",
    args: ["varId", "..."],
    range: "range-as-obj"
};

function noVarPtr() { return varPtr(0); }
export function varPtr(varId, part) {
    if (typeof varId === "object") {
        if (varId.part !== undefined) {
            //here shall be some merge, for cases like (((point.yzw).y).x)
            //but not now
            throw new Error("Not supported, sorry");
        }
        return varPtr(varId.id, part);
    }
    let v = {id: varId};
    if (part) v.part = part;
    return v;
}

//todo: define Op struct format (maybe with a class)
export function convert(ast, map = new VariablesMap()) {
    let out = [];

    let currentFn = null;
    function insideFunction() {
        return currentFn != null;
    }

    //returns variable id
    function call(node) {
        let functionName = node.children[0].token.data;
        let args = node.children.slice(1);
        let argVars = args.map(process);
        let variable = map.addTempVariable(predictOutType(functionName, argVars.map(vptr => map.getById(vptr).type)));
        out.push({op: functionName, out: [varPtr(variable)], args: argVars, line: node.token.line});
        return varPtr(variable);
    }

    //returns variable id
    function operator(node) {
        let operator = node.token.data;
        if (operator === '.') {
            let argLeftPtr = process(node.children[0]);
            let part = node.children[1].token.data;
            let varType = ["float", "vec2", "vec3", "vec4"][part.length-1];
            let variable = map.addTempVariable(varType);
            //todo: optimize, just add part to last op
            out.push({op: "=", out: [varPtr(variable)], args: [varPtr(argLeftPtr, part)], line: node.token.line});
            return varPtr(variable);
        }
        let argVars = node.children.map(process);
        let variable = map.addTempVariable(predictOutType(operator, argVars.map(vptr => map.getById(vptr).type)));
        out.push({op: operator, out: [varPtr(variable)], args: argVars, line: node.token.line});
        return varPtr(variable);
    }

    function declvar(node) {
        let c = node.children;
        let decllist = c[c.length-1];
        let name = decllist.children[0].token.data;
        let type = c[c.length-2].token.data;
        //nothing else matters
        let possibleRange = undefined;
        if (decllist.token.preceding) {
            let cmt = decllist.token.preceding.find(t => t.type === "block-comment");
            if (cmt) {
                possibleRange = cmt.data.substring(2, cmt.data.length-2);
            }
        }
        if (!possibleRange && insideFunction()) {
            //so this is not uniform, assume default value 
            possibleRange = "0"; 
        }
        let variable = map.addVariable(name, type, possibleRange);
        if (decllist.token.data === ";") {
            //just declaration
        } else {
            let expr = decllist.children[1];
            let inId = process(expr);
            //special case
            if (map.isConst(inId)) {
                //no previous op
                out.push({op: "=", args: [inId], out: [varPtr(variable)]});
            } else {
                let prevOp = out.pop();
                if (prevOp.out[0].id !== inId.id) throw new Error("wrong");
                prevOp.out = [varPtr(variable.id)];
                out.push(prevOp);
            }
        }
        return varPtr(variable);
    }

    function declfn(node) {
        let fn = node.children[node.children.length-1];
        let fnName = fn.children[0].token.data;
        currentFn = fnName;
        if (fnName === 'main') {
            let stmtList = fn.children[fn.children.length-1];
            process(stmtList);
        }
        currentFn = null;
    }

    function decl(node) {
        if (node.children.some(c => c.type === "decllist")) {
            return declvar(node);
        }
        if (node.children.some(c => c.type === "function")) {
            return declfn(node);
        }
    }

    function assign(node) {
        let left = node.children[0];
        let right = node.children[1];
        let argId = process(right);

        let variable, part;
        switch (left.type) {
            case "operator":
                if (left.token.data === ".") {
                    variable = map.getVariable(left.children[0].token.data);
                    part = left.children[1].token.data;
                    //console.log('get id for ', left.children[0].token.data, '=>', varId)
                }
                break;
            case "ident":
                variable = map.getVariable(left.token.data);
                break;
        }
        if (variable) {
            //todo: same optimization as in declaration: if left part is "whole", then do not introduce temp var
            out.push({op: "=", out: [varPtr(variable.id, part)], args: [argId], line: node.token.line});
        } else {
            console.warn("Don't know how to assign", left, " = ", right, "(" + argId + ")");
        }
    }

    function ifcondition(node) {
        let [cond, thenblock, elseblock] = node.children;
        let condVarId = process(cond);
        out.push({op: "_ifeq", args: [condVarId], out: [], range: TypeRange.create("float", 1), line: thenblock.line});
        process(thenblock);
        out.push({op: "_endif", args: [condVarId], out: []});
        if (elseblock) {
            out.push({op: "_ifeq", args: [condVarId], out: [], range: TypeRange.create("float", 0), line: elseblock.line});
            process(elseblock);
            out.push({op: "_endif", args: [condVarId], out: []});      
        }
    }

    function process(node) {
        let variable;
        switch (node.type) {
            case "expr":
                return process(node.children[0]);
            case "literal":
                variable = map.getConst(node.token.data);
                if (!variable) {
                    variable = map.addConst(node.token.data, node.token.type);
                }
                return varPtr(variable);
            case "ident":
                variable = map.getVariable(node.token.data);
                if (variable) return varPtr(variable);
                break;
            case "decl":
                return decl(node);
            case "call":
                return call(node);
            case "assign":
                return assign(node);
            case "binary":
            case "operator":
                return operator(node);
            case "stmtlist":
            case "stmt":
                return processChildren(node);
            case "group":
                if (node.children.length === 1) {
                    return process(node.children[0]);
                }
                break;
            case "if":
                return ifcondition(node); 
        }
        console.warn("unknown node type: " + node.type);
        return processChildren(node);
    }

    function processChildren(node) {
        if (!node.children) return;
        
        for (let ch of node.children) {
            process(ch);
        }
    }

    process(ast);
    return out;
}

export const Components = {
    main: "xyzw",
    color: "rgba",

    x:0, r:0,
    y:1, g:1,
    z:2, b:2,
    w:3, a:3
};

function getPtrComponents(ptr, map) {
    let variable = map.getById(ptr);
    let allComponents = variable._devectorized;
    if (!allComponents) {
        if (ptr.part) throw new Error("part for non-vector?");
        return [ptr];
    }
    return ptr.part ? (ptr.part.split('').map(c => Components[c])).map(idx => allComponents[idx]) : allComponents;
}

function devecVecCtor(op, map) {
    let expectedSize = {"vec4": 4, "vec3": 3, "vec2": 2}[op.op];
    //1. devectorize output
    let outArgs = getPtrComponents(op.out[0], map);
    //2. devectorize all args, one by one
    let inArgs = [];
    for (let arg of op.args) {
        inArgs.push(...getPtrComponents(arg, map));
    }
    //3. add args if not done
    while (inArgs.length < expectedSize) {
        //repeat last arg
        inArgs.push(inArgs[inArgs.length-1]);
    }
    while (outArgs.length < 4) {
        outArgs.push(noVarPtr());
        inArgs.push(noVarPtr());
    }
    return {
        op: "=",
        out: outArgs,
        args: inArgs,
        range: op.range,
        line: op.line
    }
}

function devecCompare(op, map) {
    let ret = devecMath(op, map);
    ret.out = [ret.out[0]];
    return ret;
}

function devecMath(op, map) {
    //case 1 - vecX vs vecX
    //case 2 - vecX vs float
    let outArgs = getPtrComponents(op.out[0], map);
    let arg1 = getPtrComponents(op.args[0], map);
    let arg1isVector = arg1.length > 1;
    let arg2 = [];
    if (op.args.length > 1) {
        arg2 = getPtrComponents(op.args[1], map);
    }
    while (outArgs.length < 4) outArgs.push(noVarPtr());
    while (arg1.length < 4) arg1.push(noVarPtr());
    if (arg2.length > 0) {
        let isSingle = arg2.length === 1 && arg1isVector;
        while (arg2.length < 4) arg2.push(isSingle ? arg2[0] : noVarPtr());
        arg1 = arg1.concat(arg2);
    }
    return {op: op.op, out: outArgs, args: arg1, range: op.range, line: op.line};
}

function devecCommon(op, map) {
    let outArgs = op.out.length ? getPtrComponents(op.out[0], map) : [];
    //2. devectorize all args, one by one
    let inArgs = [];
    for (let arg of op.args) {
        inArgs.push(...getPtrComponents(arg, map));
    }
    return {
        op: op.op,
        out: outArgs,
        args: inArgs,
        range: op.range,
        line: op.line
    }    
}

function ensureNoVectors(op, map) {
    for (let vptr of op.args.concat(op.out)) {
        let variable = map.getById(vptr);
        if (variable._devectorized) {
            console.error("vector arg still here", op);
            throw new Error("vector arg still here");
        }
        if (vptr.part) {
            console.error("vector arg still here", op);
            throw new Error("vector arg still here");
        }
    }
}

export function devectorize(ops, map) {
    let out = [];
    for (let variable of map.getVariables()) {
        if (variable.type.startsWith("vec")) {
            let vecSize = parseInt(variable.type.substring(3));
            let components = [variable];
            for (let i = 1; i < vecSize; i++) {
                let newVariable = map.addVariable(variable.name + "." + Components.main[i], "float", variable.range);
                components.push(newVariable);
                newVariable.isTemp = variable.isTemp;
            }
            map.rename(variable, variable.name + "." + Components.main[0]);
            variable.type = "float";
            variable._devectorized = components.map(v => varPtr(v));
        }
    }

    for (let op of ops) {
        if (OpsMap[op.op]) {
            let devectorize = OpsMap[op.op].devectorize;
            out.push(devectorize(op, map));
        } else {
            ensureNoVectors(op, map);
            out.push(op);
        }
    }
    return out;
}


export function addVarsInOut(ops, map) {
    let out = [];
    let lastSeenPerId = new Map();
    for (let op of ops) {
        let vars = op.out.concat(op.args);
        let varsToMark = [];
        for (let varptr of vars) {
            if (varptr.id === noVarPtr().id) continue;
            let variable = map.getById(varptr);
            if (!lastSeenPerId.has(variable.id) && varsToMark.indexOf(variable.id) === -1) {
                out.push({op: "_define", out: [varPtr(variable)], args: [], range: TypeRange.create(variable.type, variable.range)});
            }
            varsToMark.push(variable.id);
        }
        out.push(op);
        for (let id of varsToMark) {
            lastSeenPerId.set(id, out.length);
        }
    }

    let list = Array.from(lastSeenPerId, ([id,line]) => ({id, line})).sort((a,b) => b.line - a.line);
    for (let {id, line} of list) {
        out.splice(line, 0, {op: "_forget", out: [], args: [varPtr(id)]});
    }

    return out;
}

export function prepareForAnalysis(ops, map) {
    let out = [];
    let cmdId = 1;
    for (let op of ops) {
        let knownOp = OpsMap[op.op];
        if (!knownOp) throw new Error("unknown op: " + op.op);
        let newOp = {
            cmdId,
            opCode: knownOp.opCode,
            args: op.out.concat(op.args).map(v => v.id),
            line: op.line || -1,
            range: op.range || TypeRange.none,

            op: op.op
        }
        if (newOp.opCode === undefined) throw new Error("no op!" + op.op);
        out.push(newOp);
        cmdId++;
    }
    return out;
}


/*
todo: pipe it, like
    op -> convert -> converted_op -> devectorize -> [devectorized_ops] -> ...

*/