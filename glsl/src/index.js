import TokenString from 'glsl-tokenizer/string'
import ParseTokens from 'glsl-parser/direct'
import {VariablesMap, convert, devectorize, addVarsInOut, prepareForAnalysis, varPtr, Components} from './ast2dataflow'
import Range from './range'
import UIController from "./ui"
import HumanReport from "./humanreport"
import {Examples} from './examples'
import {NullPrinter, DebugPrinter} from './debug';

window.TypeRange = Range;

var src = Examples[0];


function addCommonVars(map) {
    map.addVariable("gl_FragColor", "vec4");
    map.addVariable("gl_FragCoord", "vec2", "[0,4000]")
}

function addShadertoyVars(map) {
    map.addVariable("fragColor", "vec4");
    map.addVariable("iTime", "float", "[0,Infinity)");
    map.addVariable("iResolution", "vec3", "[4000,4000]");
    map.addVariable("fragCoord", "vec2", "[0,4000]");
    map.addVariable("iMouse", "vec4", "[0,4000]");
    map.addVariable("iChannel0", "sampler2D");
    map.addVariable("iChannel1", "sampler2D");
    map.addVariable("iChannel2", "sampler2D");
    map.addVariable("iChannel3", "sampler2D");
    map.addVariable("iChannelResolution[0]", "vec3", "[4000,4000]");
}

function getOutVarId(map) {
    let outVar = map.getVariable("fragColor"); //shadertoy
    if (!outVar) {
        outVar = map.getVariable("gl_FragColor");
    }
    return outVar ? varPtr(outVar) : null;
}

const DEBUG = true;
function process(src, needDebug = DEBUG) {

    let map = new VariablesMap();
    const logger = needDebug ? new DebugPrinter(map) : new NullPrinter();

    let tree = ParseTokens(TokenString(src));
    addCommonVars(map);
    if (src.indexOf("mainImage(") !== -1 && src.indexOf("gl_FragColor") === -1) {
        addShadertoyVars(map);
    }

    let pass1 = convert(tree, map);
    let outVarId = getOutVarId(map);
    if (outVarId) {
        pass1.push({op: "_output", out: [], args: [outVarId]});
    } //todo: else?
    logger.printTree(tree);
    logger.printVars(map);
    logger.printAfterPass1(pass1);
    let pass2 = devectorize(pass1, map);
    logger.printAfterPass2(pass2);
    let pass3 = addVarsInOut(pass2, map);
    logger.printAfterPass3(pass3);
    let pass4 = prepareForAnalysis(pass3, map);
    logger.printAfterPass4(pass4);
    
    let flow = new DataFlowApi();
    flow.init();
    logger.beforeCommandLoop();
    for (let op of pass4) {
        //console.log('process', op);
        logger.printCommand(op);
        let ret = flow.addCommand(op.cmdId, op.opCode, op.args, op.range.left, op.range.right, op.range.flag);
        logger.printCommandResult(op, getErrorMessage(ret));
        if (ret !== 0) throw new Error("Error during processing command#" + op.cmdId + "(" +  op.op + "): " + getErrorMessage(ret));
    }
    logger.afterCommandLoop();
    let report = [];
    flow.getReport(report);
    logger.printResult(report);
    flow.fini();

    return new HumanReport(fixReport(report), map, pass4, src);
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

class UISettings {
    load() {
        let settingsStr = window.localStorage.getItem("settings");
        this.settings = settingsStr ? JSON.parse(settingsStr) : {
            showTemp: false, 
            needDebug: false,
            src: Examples[0].src
        }
        return this;
    }

    save(update) {
        this.settings = Object.assign({}, this.settings, update);
        //console.log("save settnings", this.settings);
        window.localStorage.setItem("settings", JSON.stringify(this.settings));
    }

    get showTemp() { return this.settings.showTemp; }
    get needDebug() { return this.settings.needDebug; }
    get src() { return this.settings.src; }
}

const settings = new UISettings().load();
//local storage - keeps src and selected example idx

UIController.init(settings, (src, needDebug) => process(src, needDebug));
for (const ex of Examples) {
    UIController.addExampleButton(ex);
}