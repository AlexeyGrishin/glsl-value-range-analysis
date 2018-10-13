export class NullPrinter {
    printTree(tree) {}
    printVars(vars) {}
    printAfterPass1(out) {}
    printAfterPass2(out) {}
    printAfterPass3(out) {}
    printAfterPass4(out) {}
    printCommand(cmd) {}
    printCommandResult(cmd) {}
    printResult(response) {}
    beforeCommandLoop() {}
    afterCommandLoop() {}
}

export class DebugPrinter {

    constructor(variablesMap) {
        const idToStr = function(varPtr) {
            if (varPtr.id === 0) return '_';
            let variable = variablesMap.getById(varPtr.id);
            if (!variable) return `?${varPtr.id}`;
            return `${variable.type} ${variable.name}${varPtr.part ? ("." + varPtr.part) : ""}`
        }
        this.op2string = function(op) {
            return `${op.line ? (op.line + ': ') : ''}${op.out.map(idToStr).join(",")} = ${op.op}(${op.args.map(idToStr).join()}${op.range ? (',' + op.range) : ''})`;
        }
    }



    printTree(tree) {
        console.group("tree");
        printNode(tree);
        console.groupEnd();
    }
    printVars(vars) {
        console.log(vars);        
    }
    printAfterPass1(out) {
        console.groupCollapsed("pass1");
        console.log(out.map(this.op2string).join("\n"));
        console.groupEnd();
    }
    printAfterPass2(out) {
        console.groupCollapsed("pass2");
        console.log(out.map(this.op2string).join("\n"));
        console.groupEnd();       
    }
    printAfterPass3(out) {
        console.groupCollapsed("pass3");
        console.log(out.map(this.op2string).join("\n"));
        console.groupEnd();
    }
    printAfterPass4(out) {
        console.groupCollapsed("pass4");
        let o = [];
        for (let op of out) {
            o.push([op.cmdId, op.opCode, '(' + op.op + ')', op.args.toString(), op.range.left, op.range.right, op.range.flag].join(','));
        }
        console.log(o.join('\n'));
        console.groupEnd();   
    }
    beforeCommandLoop() {
        console.groupCollapsed("process");
    }
    printCommand(op) {
        console.log('process', op.cmdId);
    }

    afterCommandLoop() {
        console.groupEnd();
    }
    printCommandResult(ret, errorMessage) {
        console.log(' = ', ret, errorMessage);
    }
    printResult(report) {
        let filter = () => true;
        //let filter = (r) => r.varId == 31;
        //let filter = (r) => r.varId == map.getVariable("k.x").id
        console.groupCollapsed("response");
        console.log(JSON.stringify(report.filter(filter), null, 4));
        console.groupEnd();
    }


}


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

