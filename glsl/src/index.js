import TokenString from 'glsl-tokenizer/string'
import ParseTokens from 'glsl-parser/direct'
import {VariablesMap, convert, devectorize, addVarsInOut, prepareForAnalysis, varPtr, Components} from './ast2dataflow'
import Range from './range'
import UIController from "./ui"
import HumanReport from "./humanreport"

window.TypeRange = Range;

//todo: move all that to "examples" and add selector
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


//https://www.shadertoy.com/view/4dsGzH
var srcShaderToy_Waves = `
vec3 COLOR1 = vec3(0.0, 0.0, 0.3);
vec3 COLOR2 = vec3(0.5, 0.0, 0.0);
float BLOCK_WIDTH = 0.01;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
	
	// To create the BG pattern
	vec3 final_color = vec3(1.0);
	vec3 bg_color = vec3(0.0);
	vec3 wave_color = vec3(0.0);
	
	float c1 = mod(uv.x, 2.0 * BLOCK_WIDTH);
	c1 = step(BLOCK_WIDTH, c1);
	
	float c2 = mod(uv.y, 2.0 * BLOCK_WIDTH);
	c2 = step(BLOCK_WIDTH, c2);
	
	bg_color = mix(uv.x * COLOR1, uv.y * COLOR2, c1 * c2);
	
	
	// To create the waves
	float wave_width = 0.01;
	uv  = -1.0 + 2.0 * uv;
	uv.y += 0.1;
	for(float i = 0.0; i < 10.0; i++) {
		
		uv.y += (0.07 * sin(uv.x + i/7.0 + iTime ));
		wave_width = abs(1.0 / (150.0 * uv.y));
		wave_color += vec3(wave_width * 1.9, wave_width, wave_width * 1.5);
	}
	
	final_color = bg_color + wave_color;
	
	
	fragColor = vec4(final_color, 1.0);
}
`;

//https://www.shadertoy.com/view/XsjGDt
var srcShaderToy_Circle = `
/**
 * @author jonobr1 / http://jonobr1.com/
 */

/**
 * Convert r, g, b to normalized vec3
 */
vec3 rgb(float r, float g, float b) {
	return vec3(r / 255.0, g / 255.0, b / 255.0);
}

/**
 * Draw a circle at vec2 'pos' with radius 'rad' and
 * color 'color'.
 */
vec4 circle(vec2 uv, vec2 pos, float rad, vec3 color) {
	float d = length(pos - uv) - rad;
	float t = clamp(d, 0.0, 1.0);
	return vec4(color, 1.0 - t);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {

	vec2 uv = fragCoord.xy;
	vec2 center = iResolution.xy * 0.5;
	float radius = 0.25 * iResolution.y;

    // Background layer
	vec4 layer1 = vec4(rgb(210.0, 222.0, 228.0), 1.0);
	
	// Circle
	vec3 red = rgb(225.0, 95.0, 60.0);
	vec4 layer2 = circle(uv, center, radius, red);
	
	// Blend the two
	fragColor = mix(layer1, layer2, layer2.a);

}`;

//https://www.shadertoy.com/view/MdX3zr
//todo: mod
var srcShaderToy_Flame = `
float noise(vec3 p) //Thx to Las^Mercury
{
	vec3 i = floor(p);
	vec4 a = dot(i, vec3(1., 57., 21.)) + vec4(0., 57., 21., 78.);
	vec3 f = cos((p-i)*acos(-1.))*(-.5)+.5;
	a = mix(sin(cos(a)*a),sin(cos(1.+a)*(1.+a)), f.x);
	a.xy = mix(a.xz, a.yw, f.y);
	return mix(a.x, a.y, f.z);
}

float sphere(vec3 p, vec4 spr)
{
	return length(spr.xyz-p) - spr.w;
}

float flame(vec3 p)
{
	float d = sphere(p*vec3(1.,.5,1.), vec4(.0,-1.,.0,1.));
	return d + (noise(p+vec3(.0,iTime*2.,.0)) + noise(p*3.)*.5)*.25*(p.y) ;
}

float scene(vec3 p)
{
	return min(100.-length(p) , abs(flame(p)) );
}

vec4 raymarch(vec3 org, vec3 dir)
{
    float d = 0.0;
    float glow = 0.0;
    float eps = 0.02;
	vec3  p = org;
	bool glowed = false;
	
	for(int i=0; i<64; i++)
	{
		d = scene(p) + eps;
		p += d * dir;
		if( d>eps )
		{
			if(flame(p) < .0)
				glowed=true;
			if(glowed)
       			glow = float(i)/64.;
		}
	}
	return vec4(p,glow);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 v = -1.0 + 2.0 * fragCoord.xy / iResolution.xy;
	v.x *= iResolution.x/iResolution.y;
	
	vec3 org = vec3(0., -2., 4.);
	vec3 dir = normalize(vec3(v.x*1.6, -v.y, -1.5));
	
	vec4 p = raymarch(org, dir);
	float glow = p.w;
	
	vec4 col = mix(vec4(1.,.5,.1,1.), vec4(0.1,.5,1.,1.), p.y*.02+.4);
	
	fragColor = mix(vec4(0.), col, pow(glow*2.,4.));
	//fragColor = mix(vec4(1.), mix(vec4(1.,.5,.1,1.),vec4(0.1,.5,1.,1.),p.y*.02+.4), pow(glow*2.,4.));

}


`;


var src = srcShaderToy_Circle;
var srcShaderToyNew = `
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

    // Time varying pixel color
    vec3 col = 0.5 + 0.5*cos(iTime+vec3(0,2,4));
    
    // Output to screen
    fragColor = vec4(col,1.0);
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
        console.groupCollapsed("process");
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