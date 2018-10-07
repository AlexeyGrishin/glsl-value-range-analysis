import OpCode from "./opcodes"
import TypeRange from "./range"

function op(opCode, outTypeFn, devectorize) {
    let getOutType = typeof outTypeFn === "function" ? outTypeFn : () => outTypeFn;
    return {opCode, getOutType, devectorize}
}

function firstArg(inTypes) { return inTypes[0]; }
function secondArg(inTypes) { return inTypes[1]; }

//todo[grishin]: add all new functions, already supported
const OpsMap = {
    "+": op(OpCode.plus, firstArg, devecMath),
    "-": op(OpCode.minus, firstArg, devecMath),
    "*": op(OpCode.mul, firstArg, devecMath),
    "/": op(OpCode.div, firstArg, devecMath),
    "sin": op(OpCode.sin, firstArg, devecMath),
    "cos": op(OpCode.cos, firstArg, devecMath),
    "atan": op(OpCode.atan, firstArg, devecMath),
    "min": op(OpCode.min, firstArg, devecMath),
    "max": op(OpCode.max, firstArg, devecMath),
    "fract": op(OpCode.fract, firstArg, devecMath),
    "floor": op(OpCode.floor, firstArg, devecMath),
    "power": op(OpCode.power, firstArg, devecMath),
    "mix": op(OpCode.mix, firstArg, devecMath),
    "normalize": op(OpCode.normalize, firstArg, devecMath),
    //todo: dot
    //todo: cross
    "unary-": op(OpCode.unary_minus, firstArg, devecMath),
    "<": op(OpCode.lt, "boolean", devecCompare),
    ">": op(OpCode.gt, "boolean", devecCompare),
    "<=": op(OpCode.lte, "boolean", devecCompare),
    ">=": op(OpCode.gte, "boolean", devecCompare),
    "==": op(OpCode.eq, "boolean", devecCompare),
    "||": op(OpCode.or, "boolean", devecCompare),
    "&&": op(OpCode.and, "boolean", devecCompare),
    "texture2D": op(OpCode.texture2D, "vec4", devecCommon),
    "length": op(OpCode.length, "float", devecCommon),
    "step": op(OpCode.step, secondArg, devecMath),
    "smoothstep": op(OpCode.smoothstep, secondArg, devecMath),
    "clamp": op(OpCode.clamp, firstArg, devecMath),

    "vec4": op(undefined, "vec4", devecVecCtor),
    "vec3": op(undefined, "vec3", devecVecCtor),
    "vec2": op(undefined, "vec2", devecVecCtor),

    "=": op(OpCode.assign, secondArg, devecMath),

    "_define": op(OpCode._define),
    "_ifeq": op(OpCode._ifeq, undefined, devecCommon),
    "_endif": op(OpCode._endif, undefined, devecCommon),
    "_forget": op(OpCode._forget),
    "_output": op(OpCode._output, undefined, devecCommon),

    "_watch": op(OpCode._watch, undefined, devecNothing),
    "_endwatch": op(OpCode._endwatch, undefined, devecNothing),
    "_ignore": op(OpCode._ignore, undefined, devecNothing),


    "float": op(OpCode._copy, "float", devecNothing),
    "int": op(OpCode._copy, "float", devecNothing)
};

//todo: type enums?
function predictOutBuiltinType(operator, inTypes) {
    let knownOp = OpsMap[operator];
    if (knownOp) return knownOp.getOutType(inTypes);
    return undefined;
}

function isCopyFn(operator) {
    return OpsMap[operator] && OpsMap[operator].opCode === OpCode._copy;
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
        value = value + ""; //expected to be string
        return this.constToVar.get(value);
    }

    copyRange(toId, fromId) {
        this.getById(toId).range = this.getById(fromId).range;
    }

    addConst(value, type) {
        let id = this.nextVarId++;
        value = value + "";
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

    //todo: create class
    let customFunctions = new Map();
    let customFunctionsByName = new Map(); 
    let functionCallsCounter = 9600;
    let functionCallStack = [];

    let loopCallStack = [];

    function insideLoop() {
        return loopCallStack.length > 0;
    }

    function insideFunction() {
        return functionCallStack.length > 0;
    }

    function functionId(name, types) {
        return name + "(" + types.join(",") + ")";
    }

    function isCustomFunction(functionName) {
        return customFunctionsByName.has(functionName);
    }

    function predictOutType(operator, inTypes) {
        if (isCustomFunction(operator)) {
            return customFunctions.get(functionId(operator, inTypes)).outType;
        } else {
            return predictOutBuiltinType(operator, inTypes);
        }
    }

    function getActualVariableName(name, isDeclaration) {
        if (!insideFunction()) return name;

        if (isDeclaration) {
            let currentCall = functionCallStack[functionCallStack.length-1];
            let newName = currentCall.namePrefix + "_" + name;
            currentCall.renamedVars.set(name, newName);
            return newName;
        }

        for (let call of functionCallStack.reverse()) {
            if (call.renamedVars.has(name)) {
                return call.renamedVars.get(name);
            }
        }
        return name;
    }

    function callCustom(name, outVar, argVars) {
        const fid = functionId(name, argVars.map(vptr => map.getById(vptr).type));
        const callId = functionCallsCounter++;

        //todo: check recrusive calls here

        functionCallStack.push({
            callId, 
            functionId: fid, 
            namePrefix: fid+":"+callId, 
            renamedVars: new Map(),
            returnVar: outVar
        });
        let customFunction = customFunctions.get(fid);
        out.push({op: "_watch", out: [varPtr(outVar)], args: [], line: customFunction.node.token.line})

        let i = 0;
        for (let fnArgName of customFunction.inArgNames) {
            let newName = getActualVariableName(fnArgName, true);
            let variable = map.addVariable(newName, customFunction.inTypes[i], undefined);
            //todo: check
            out.push({op: "=", out: [varPtr(variable)], args: [varPtr(argVars[i])], line: customFunction.node.token.line});    
            i++;
        }

        process(customFunction.body);
        
        out.push({op: "_endwatch", out: [varPtr(outVar)], args: [], line: customFunction.node.token.line})

        functionCallStack.pop();

    }

    //returns variable id
    function call(node) {
        let functionName = node.children[0].token.data;
        let args = node.children.slice(1);
        let argVars = args.map(process);
        let outType = predictOutType(functionName, argVars.map(vptr => map.getById(vptr).type));
        if (outType === undefined) {
            console.warn("undefined type for call result", functionName, " assume float");
            outType = "float";
        }
        if (isCopyFn(functionName)) {
            return varPtr(argVars[0]);
        }
        let variable = map.addTempVariable(outType);
        if (isCustomFunction(functionName)) {
            callCustom(functionName, variable, argVars);
        } else {
            out.push({op: functionName, out: [varPtr(variable)], args: argVars, line: node.token.line});
        }
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
        let name = getActualVariableName(decllist.children[0].token.data, true);
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
                map.copyRange(variable.id, inId);
            } else {
                let prevOp = out.pop();
                if (prevOp.out[0].id !== inId.id) throw new Error("wrong");
                prevOp.out = [varPtr(variable.id)];
                out.push(prevOp);
            }
        }
        return varPtr(variable);
    }

    //todo: needed?
    function renameVariables(node, fnName) {
        if (node.type === "ident") {
            node.token.data = fnName + "$" + node.token.data;
            if (node.data) {
                node.data = node.token.data;
            }
        }
        if (node.type === "call") {
            node.children.slice(1).forEach(c => renameVariables(c, fnName));
            return;
        }
        if (node.children) node.children.forEach(c => renameVariables(c, fnName));
    }

    //todo: in, out, inout modifiers
    function declfn(node) {
        let outType = node.children[node.children.length-2].token.data;
        let fn = node.children[node.children.length-1];
        let fnName = fn.children[0].token.data;
        let stmtList = fn.children[fn.children.length-1];
        if (fnName === 'main') {
            process(stmtList);
        } else {
            let declArgs = fn.children[1].children;
            let inTypes = declArgs.map(decl => decl.token.data);
            let customFn = {
                node,
                outType,
                inTypes,
                name: fnName,
                //todo: bad here is that I parse all manually, not using "process"
                inArgNames: declArgs.map(decl => decl.children[decl.children.length-1].children[0].token.data),
                body: stmtList
            };
            customFunctionsByName.set(fnName, customFn);
            customFunctions.set(functionId(fnName, inTypes), customFn);
            console.log(customFunctions);
        }
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

        switch (node.token.data) {
            case "=":
                return assignDirect(node);
            case "+=":
            case "-=":
            case "*=":
            case "/=":
                let modifyOp = node.token.data[0]; //+,-, or something else
                let fakeNode = {
                    ...node,
                    token: {
                        ...node.token,
                        data: modifyOp
                    }
                }
                let tmpVarPtr = operator(fakeNode); 
                return assignDirectToVariable(node.children[0], tmpVarPtr);
        }
    }

    function assignDirectToVariable(left, argId) {
        let variable, part;
        switch (left.type) {
            case "operator":
                if (left.token.data === ".") {
                    //todo: introduce helper for getVaraible that always calls to getActualVariableName
                    variable = map.getVariable(getActualVariableName(left.children[0].token.data));
                    part = left.children[1].token.data;
                    //console.log('get id for ', left.children[0].token.data, '=>', varId)
                }
                break;
            case "ident":
            case "builtin": //gl_FragColor
                variable = map.getVariable(getActualVariableName(left.token.data));
                break;
        }
        if (variable) {
            //todo: same optimization as in declaration: if left part is "whole", then do not introduce temp var
            out.push({op: "=", out: [varPtr(variable.id, part)], args: [argId], line: left.token.line});
            return variable;
        } 
    }

    function assignDirect(node) {
        let left = node.children[0];
        let right = node.children[1];
        let argId = process(right);

        let variable = assignDirectToVariable(left, argId);
        if (!variable) {
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

    function returnexpr(node) {
        if (!insideFunction()) return console.warn("Return outside function? huh")
        let currentCall = functionCallStack[functionCallStack.length-1];
        let varId = process(node.children[0]); //expression
        out.push({op: "=", out: [varPtr(currentCall.returnVar)], args: [varId], line: node.token.line});
        out.push({op: "_ignore", out: [varPtr(currentCall.returnVar)], args: [], line: node.token.line})
    }

    function breakexpr(node) {
        if (!insideLoop()) return console.warn("Break outside loop? heh")
        let currentLoop = loopCallStack[loopCallStack.length-1];
        out.push({op: "_ignore", out: [varPtr(currentLoop.iteratorVar)], args: [], line: node.token.line})
    }

    function getConst(node) {
        switch (node.type) {
            case "literal": return parseFloat(node.token.data);
            case "unary":
                if (node.token.data === "-" && node.children[0]) {
                    let out = getConst(node.children[0]);
                    if (out !== undefined) return -out;
                }
        }
        return undefined;
    }

    function simpleloop(node) {
        let initexpr = node.children[0];
        let condexpr = node.children[1];
        let changeexpr = node.children[2];
        let body = node.children[3];
        
        //console.log(initexpr, condexpr, changeexpr);

        if (initexpr.type !== "decl") return false;
        let iteratorVar = map.getById(declvar(initexpr));
        let initRange = TypeRange.create(iteratorVar.type, iteratorVar.range);
        if (!initRange.isSingle) return false;
        let initValue = initRange.left;


        if (condexpr.type !== "expr" && condexpr.children[0].type !== "binary") return false;
        condexpr = condexpr.children[0];
        if (condexpr.token.data !== "<" && condexpr.token.data !== ">") return false;
        if (condexpr.children[0].type !== "ident") return false;
        //todo: check variable name
        //if (condexpr.children[0].token.data !== )
        let limit = getConst(condexpr.children[1]);
        if (limit === undefined) return false;

        let step;
        if (changeexpr.type !== "expr") return false;
        changeexpr = changeexpr.children[0];
        switch (changeexpr.token.data) {
            case "++":
                step = +1; break;
            case "--":
                step = -1; break;
            case "+=":
                step = getConst(changeexpr.children[1]);
                break;
            case "-=":
                step = -getConst(changeexpr.children[1]);
                break;
            default:
                return false;
        }
        //console.log("loop", iteratorVar, initValue, limit, step);

        let loop = {iteratorVar};
        loopCallStack.push(loop);
        out.push({op: "_watch", out: [varPtr(iteratorVar)], args: [], line: node.token.line});
        
        let condition = limit > initValue ? i => i < limit : i => i > limit;
        for (let i = initValue; condition(i); i += step ) {

            //todo: copy-paste from process
            let constValue = map.getConst(i) || map.addConst(i, iteratorVar.type);

            out.push({op: "=", out: [varPtr(iteratorVar)], args: [varPtr(constValue)], line: node.token.line});
            process(body);
        }


        out.push({op: "_endwatch", out: [varPtr(iteratorVar)], args: [], line: node.token.line});
        loopCallStack.pop();
        return true;
    }

    function forloop(node) {
        if (!simpleloop(node)) {
            throw new Error("Sorry, non-trivial loops not supported")
        }
    }

    function unary(node) {
        let fakeNode = {...node, token: {
            ...node.token,
            data: "unary"+node.token.data
        }}
        return operator(fakeNode);
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
                variable = map.getVariable(getActualVariableName(node.token.data));
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
            case "unary":
                return unary(node);
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
            case "return":
                return returnexpr(node);
            case "forloop":
                return forloop(node);
            case "break":
                return breakexpr(node);

        }
        console.warn("unknown node type: " + node.type, node);
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

function devecNothing(op, map) {
    return op;
}

/*function devecMix(op, map) {
    //vecX = mix(vecX, vecX, float)
    //float = mix(float, float, float);
    let out = devecMath(op, map);
    out.args.push(op.args[2]);
}*/

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
    if (op.args.length > 2) { //clamp/mix case
        arg2 = arg2.concat(getPtrComponents(op.args[2], map));
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
        if (variable.type === undefined) {
            console.warn("Variable ", variable.name, " has no type", variable);
        }
        else if (variable.type.startsWith("vec")) {
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
        if (newOp.opCode === undefined) {
            throw new Error("no op!" + op.op);
        }
        out.push(newOp);
        cmdId++;
    }
    return out;
}


/*
todo: pipe it, like
    op -> convert -> converted_op -> devectorize -> [devectorized_ops] -> ...

*/