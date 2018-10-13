import OpCode from "./opcodes"
import {Components} from "./ast2dataflow"

export default class HumanReport
{
    constructor(report, map, ops, src) {

        function getRealVarName(varId) {
            let variable = map.getById(varId);
            if (!variable) return {print: "?unknown?", highlight: null};
            let name = variable.name;
            if (name.indexOf(":") !== -1 && name.indexOf("_") !== -1) {
                name = name.split("_").slice(1).join("_");
            }
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
            b.endCmdId = ops[ops.length-1].cmdId;
            b.children = [];
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
                b.parent.children.push(b);
                b.parent.endCmdId = b.cmdId;
            }
        }
        this.branches = branches;


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
                let lastChange = report.filter(r => r.type == "change" && r.varId == varId && r.branchId == b.branchId).pop();
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

        let flow = this.flow = [];

        let lines = src.split("\n");
        function getSrcLine(line) {
            return lines[line - 1];
        }

        function addSourceCodeRef(item, cmdId) {
            let op = getOriginalOp(cmdId);
            if (op && op.line !== -1) {
                item.line = op.line;
                item.src = getSrcLine(op.line);
            }
        }

        //flow item = {indent, text, src, line, linecomment}
        
        function fillBranch(branch, indent = 0) {
            let item = {
                branch,
                indent,
                text: (branch.branchId === 0 ? "[MAIN]" : ("[Branch#" + branch.branchId + "]: " + branch.reason))
            };
            addSourceCodeRef(item, branch.cmdId);
            flow.push(item);
            for (let cmdId = branch.cmdId; cmdId < branch.endCmdId; cmdId++) {
                for (let c of report.filter(c => c.type == "change" && c.cmdId == cmdId && c.branchId == branch.branchId)) {
                    if (getOriginalOp(c.cmdId).opCode == OpCode._define) continue;
                    item = {
                        indent: indent+2,
                        varId: c.varId,
                        isTemp: map.isTemp(c.varId),
                        text: getRealVarName(c.varId).print + " = " + c.range.toString()
                    }
                    //todo: add comment about function called
                    let comments = [];
                    if (c.revertable) {
                        comments.push("revertable");
                    }
                    if (c.reason !== "operation") {
                        comments.push(c.reason);
                    }
                    if (comments.length > 0) {
                        item.linecomment = comments.join(", ");
                    }
                    addSourceCodeRef(item, c.cmdId);

                    flow.push(item);
                }
            }
            for (let cb of branch.children) {
                fillBranch(cb, indent + 2);
            }
        }

        fillBranch(branchesMap.get(0));
    }    
}
