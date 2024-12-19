//  shaders/sm4/light.vert
//

#version 420

//  input
layout(location = 0) in vec2 vtxPosition;
//layout(location = 1) in vec3 vtxTexCoord;

//  output
out Vertex {
    vec4    Position;
} Output;

void main( void ) {

    //  output position
    gl_Position     = vec4(vtxPosition,0,1);
    Output.Position = vec4(vtxPosition,0,1);
}
