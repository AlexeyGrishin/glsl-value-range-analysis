import TokenString from 'glsl-tokenizer/string'
import ParseTokens from 'glsl-parser/direct'

var src = `
uniform sampler2D sampler;

void main() {
    vec4 color = texture2D(sampler, vec2(0.5, 0.5));
    vec2 k;
    k.xy = sin(color).rg;
    if (k.x > 0.5) {
        gl_FragColor.r = k.x * 2.;
    } else {
        gl_FragColor.r = k.x / 2.;
    }
}

`
var tokens = TokenString(src)
var tree = ParseTokens(tokens)

function removeParents(node) {
    delete node.parent;
    delete node.stage;
    delete node.mode;
    if (node.token) {
    //node.data = node.token.data;
    delete node.token.position;
    delete node.token.line;
    delete node.token.column;
    delete node.token.preceding;
    //delete node.token;
    }
    for (let c of node.children || []) removeParents(c);
    if (node.children && node.children.length === 0) delete node.children;
}

removeParents(tree);
  //console.log(tree);
  
function printNode(node, indent = 0) {
    console.log(`${"".padStart(indent)} ${node.type} = ${node.data}`);
    for (let c of node.children || []) printNode(c, indent+1);
}
  
printNode(tree);
