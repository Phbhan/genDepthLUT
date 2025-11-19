const char BOWL_VERT_SHADER[] =
R"glsl(#version 320 es
precision mediump float;
layout (location = 0) in vec3 verticePos;
layout (location = 1) in vec2 vertexUV;
uniform float meshW;
uniform float meshH;
out vec2 UV;
out vec2 UVx;
void main() {
    gl_Position =  vec4(vertexUV.x*2.0-1.0, vertexUV.y*2.0-1.0, 0.0, 1.0);
    UV = vec2(verticePos.x/meshH + 0.5, verticePos.y/meshW + 0.5);
    UVx = vertexUV;
}
)glsl";


const char BOWL_FRAG_SHADER[] =
R"glsl(#version 320 es
precision mediump float;
in vec2 UV;
in vec2 UVx;
out vec4 fragmentColor;
void main() {
    if ( UVx.x > 0.0 &&  UVx.y > 0.0) {
        fragmentColor = vec4(UV.x, UV.y, 1.0, 1.0);
    } else {
        discard;
    }
}

)glsl";