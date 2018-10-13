export const Examples = [
    {
        name: "Hello World",
        src: `
uniform sampler2D sampler;

void main() {
    vec4 color = texture2D(sampler, vec2(0.5));  
    gl_FragColor.rgb = (color.rba/2. + vec3(0.1, 0.5, 0.6)).zyx;
    gl_FragColor.a = min(gl_FragColor.r, gl_FragColor.g);
}
        `
    },
    {
        name: "Built-ins",
        src: `
void main() {
    float a = 1.0;
    vec2 b = vec2(2.0, 3.0);
    vec3 c = vec3(b, 4.0);

    vec2 normalizedB = normalize(b);
    vec2 sqrtB = sqrt(b);
    vec3 sinC = sin(c);

    float powered = pow(b.x, b.y);
    float mixed = mix(a, c.z, 0.2);

    vec3 clapmedC = clamp(c, 2.2, 2.3);

    gl_FragColor = vec4(c / 4.0, b.x * 0.1);
}        
        
        `
    },
    {
        name: "Conditions",
        src: `
uniform sampler2D uSampler;
uniform vec2 vTextureCoord /*0,1*/;  

void main() {
    vec4 color = texture2D(uSampler, vTextureCoord);
    float flag = color.a - 0.5;
    if (flag > 0.) {
        color.a -= 0.5;
    } else {
        color.a /= 2.;
    }
    gl_FragColor = color;
}
        `
    },
    {
        name: "Complex Conditions",
        src: `
uniform sampler2D uSampler;
uniform sampler2D tempMap;
uniform vec2 vTextureCoord /*0,1*/;

const float R1 = 0.45;
const float M = 0.5;
const float R2 = 0.55;        
        
void main() {
    vec4 origColor = texture2D(uSampler, vTextureCoord);
    vec4 tMap = texture2D(tempMap, vTextureCoord);
    vec4 realColor = origColor;
    float a = origColor.a;
    float t = tMap.a;
    vec4 middleColor = vec4(0.4, 0.4, 0.4, 1.0);

    if (a == 240) {  //apply fully
        if (t < R1) {
            //cold
            realColor = mix(middleColor, vec4(0.4,0.8,1.0,1.0), 1. - t / R1);
        } else if (t < R2) {
            //middle
            realColor = middleColor;
        } else {
            //hot
            realColor = mix(middleColor, realColor*2., (t-R2)/(1.-R2));
        }
    }else if (a == 128) { //snow
        if (t < R1) {
            realColor = mix(vec4(1,1,1,0), vec4(0.5,1.0,1.0,1.0), 1. - t / R1);
        }
    } else if (a == 32) { //ignore
        realColor.a = 1.;
    } else if (a == 0) { //bg
        if (t < M) {
            realColor = mix(middleColor, vec4(0.8,0.8,1.,1.), pow(1. - t / M, 0.5));
        } else {
            realColor = mix(middleColor, clamp(realColor, 0., 1.), pow((t-M)/(1.-M), 0.5));
        }  
    }
    gl_FragColor = realColor;
}
        
        `
    },
    {
        name: "Functions",
        src: `
varying float white /*0,1*/;

float dithering2(vec2 xy) {
    return floor(mod(xy.x+xy.y, 2.));
}

float dithering4(vec2 xy) {
    return floor(mod(xy.x*2.+xy.y, 4.));
}

float dithering42(vec2 xy, float edge) {
    float a1 = step(fract(xy.x / edge), 1./edge);
    float a2 = step(fract(xy.y / edge), 1./edge);
    return a2*a1;
}
float dithering52(vec2 xy, float edge) {
    float a1 = step(fract(xy.x / edge), 1./edge);
    float a2 = step(fract(xy.y / edge), 1./edge);
    float a3 = step(fract((xy.x+0.5*edge) / edge), 1./edge);
    float a4 = step(fract((xy.y+0.5*edge) / edge), 1./edge);
    return max(a2*a1, a3*a4);
}         

float dither(vec2 xy, float white) {
    if (white < 0.1) return 0.;
    if (white < 0.2) return dithering42(xy, 8.); 
    if (white < 0.3) return dithering52(xy, 8.); 
    if (white < 0.4) return dithering52(xy, 4.); 
    if (white < 0.6) return dithering2(xy); 
    if (white < 0.7) return 1. - dithering52(xy, 4.); 
    if (white < 0.8) return 1. - dithering52(xy, 8.); 
    if (white < 0.9) return 1. - dithering42(xy, 8.); 
    return 1.;

}  

void main() {
    gl_FragColor = vec4(1.) * dither(gl_FragCoord, white);
}
        `
    },
    {
        name: "Loops",
        src: `
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
    gl_FragColor = mix(vec4(0.5), vec4(0.6), a);

}       
        
        `
    },
    {
        name: "Dependent bug",
        src: `

uniform float value /*9,10*/;     
void main() {

    float val2 = value - 1.;

    //Here gl_FragColor shall be vec4(1), but it does not - due to bug in analyzer
    gl_FragColor = vec4(value-val2);

}
        `
    },
    {
        name: "ShaderToy - new",
        src: `
///// Source: https://www.shadertoy.com/new

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

    // Time varying pixel color
    vec3 col = 0.5 + 0.5*cos(iTime+vec3(0,2,4));
    
    // Output to screen
    fragColor = vec4(col,1.0);
}       
        `
    },
    {
        name: "ShaderToy - circles",
        src: `
///// Source: https://www.shadertoy.com/view/XsjGDt        
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

}
        `
    },
    {
        name: "ShaderToy - Rotating Sphere. Dot bug",
        src: `
////// Source: https://www.shadertoy.com/view/4sj3zy        
//If you're new to GLSL or programming in general,
//I encourage you to play with these variables and values to see what they do! 

//Variable declarations

//Sets background colour(red, green, blue)
vec3 bgCol = vec3(0.6, 0.5, 0.6);

//Sets size of the sphere and brightness of the shine
float sphereScale = 0.7;
float sphereShine = 0.5;

//Sets diffuse colour(red, green, blue), specular colour(red, green, blue), 
//and initial specular point position(x, y)
vec3 sphereDiff = vec3(0.5, 0.0, 0.5);
vec3 sphereSpec = vec3(1.0, 1.0, 1.0);
vec2 specPoint = vec2(0.2, -0.1);

//Main method/function
void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	//Creates shader pixel coordinates
	vec2 uv = fragCoord.xy / iResolution.xy;
	
	//Sets the position of the camera
	vec2 p = uv * 2.3 - 1.0;
	p.x *= iResolution.x / iResolution.y;
	
	//Rotates the sphere in a circle
	p.x += cos(-iTime) * 0.35;
	p.y += sin(-iTime) * 0.35;
	
	//Rotates the specular point with the sphere
	specPoint.x += cos(-iTime) * 0.35;
	specPoint.y += sin(-iTime) * 0.35;
	
    //Sets the radius of the sphere to the middle of the screen
    
    //// Here you may see interesting issue with analyzer. 
    //// It calculated that p.x and p.y have range [-1.35,1.65], so it is executed following expression like:
    ////   dot({x: [-1.35,1.65], y: [-1.35,1.65]}, {x: [-1.35,1.65], y: [-1.35,1.65]})
    //// and for analyzer's logic it is possible that in first argument x is -1.35, and in second - it is
    //// 1.65.
    //// But it is wrong because arguments are the same, so arguments always identical
    //// TODO: fix this issue 
	float radius = sqrt(dot(p, p));
	
	vec3 col = bgCol;
	
	//Sets the initial dark shadow around the edge of the sphere
	float f = smoothstep(sphereScale * 0.9, sphereScale, length(p + specPoint));
	col -= mix(col, vec3(0.0), f) * 0.2;
	
	//Only carries out the logic if the radius of the sphere is less than the scale
	if(radius < sphereScale) {
		vec3 bg = col;
		
		//Sets the diffuse colour of the sphere (solid colour)
		col = sphereDiff;
		
		//Adds smooth dark borders to help achieve 3D look
		f = smoothstep(sphereScale * 0.7, sphereScale, radius);
		col = mix(col, sphereDiff * 0.45, f);
		
		//Adds specular glow to help achive 3D look
		f = 1.0 - smoothstep(-0.2, 0.6, length(p - specPoint));
		col += f * sphereShine * sphereSpec;
		
		//Smoothes the edge of the sphere
		f = smoothstep(sphereScale - 0.01, sphereScale, radius);
		col = mix(col, bg, f);
	}	
	
	//The final output of the shader logic above
	//fragColor is a vector with 4 paramaters(red, green, blue, alpha)
	//Only 2 need to be used here, as "col" is a vector that already carries r, g, and b values
	fragColor = vec4(col, 1);
}

        `
    }
];