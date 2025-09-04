// RectanglesRenderer.cpp
#include "RectanglesRenderer.h"
#include <cstdio>
#include <cstdlib>

// Vertex Shader
const char* vertexShaderSource = R"glsl(
#version 300 es
precision mediump float;

in vec2 a_position;
in vec4 a_offset_scale; // x, y, w, h
in vec4 a_color;        // r, g, b, a

out vec4 v_color;

void main() {
    vec2 pos = a_position * a_offset_scale.zw + a_offset_scale.xy;
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0); // [0,1] => [-1,1]
    v_color = a_color;
}
)glsl";

// Fragment Shader
const char* fragmentShaderSource = R"glsl(
#version 300 es
precision mediump float;

in vec4 v_color;
out vec4 fragColor;

void main() {
    fragColor = v_color;
}
)glsl";

RectanglesRenderer::RectanglesRenderer()
    : program(0), vbo_quad(0), vbo_instances(0), ebo(0),
      posLoc(-1), offsetScaleLoc(-1), colorLoc(-1) {

    program = createProgram();
    if (!program) {
        fprintf(stderr, "Failed to create shader program!\n");
        return;
    }

    glUseProgram(program);

    posLoc = glGetAttribLocation(program, "a_position");
    offsetScaleLoc = glGetAttribLocation(program, "a_offset_scale");
    colorLoc = glGetAttribLocation(program, "a_color");

    // Quad: unit square [0,0] to [1,1]
    float quadVertices[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    GLushort quadIndices[] = {0, 1, 2, 0, 2, 3};

    // Create buffers
    glGenBuffers(1, &vbo_quad);
    glGenBuffers(1, &vbo_instances);
    glGenBuffers(1, &ebo);

    // Upload quad geometry
    glBindBuffer(GL_ARRAY_BUFFER, vbo_quad);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Upload index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);
}

RectanglesRenderer::~RectanglesRenderer() {
    if (vbo_quad) glDeleteBuffers(1, &vbo_quad);
    if (vbo_instances) glDeleteBuffers(1, &vbo_instances);
    if (ebo) glDeleteBuffers(1, &ebo);
    if (program) glDeleteProgram(program);
}

GLuint RectanglesRenderer::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        char* log = (char*)malloc(logLength);
        glGetShaderInfoLog(shader, logLength, nullptr, log);
        fprintf(stderr, "Shader compilation failed:\n%s\n", log);
        free(log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint RectanglesRenderer::createProgram() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    if (!vertexShader || !fragmentShader) {
        if (vertexShader) glDeleteShader(vertexShader);
        if (fragmentShader) glDeleteShader(fragmentShader);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        char* log = (char*)malloc(logLength);
        glGetProgramInfoLog(program, logLength, nullptr, log);
        fprintf(stderr, "Program linking failed:\n%s\n", log);
        free(log);
        glDeleteProgram(program);
        program = 0;
    }

    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}

void RectanglesRenderer::addRectangle(float x, float y, float w, float h,
                                      float r, float g, float b, float a) {
    instances.push_back({x, y, w, h, r, g, b, a});
}

void RectanglesRenderer::render() {
    if (instances.empty() || !program) return;

    glUseProgram(program);

    // Bind instance VBO and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instances);
    glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(InstanceData), instances.data(), GL_DYNAMIC_DRAW);

    // Bind quad VBO for base geometry
    glBindBuffer(GL_ARRAY_BUFFER, vbo_quad);
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);

    // Bind instance VBO for per-rectangle data
    glBindBuffer(GL_ARRAY_BUFFER, vbo_instances);
    glEnableVertexAttribArray(offsetScaleLoc);
    glEnableVertexAttribArray(colorLoc);

    // a_offset_scale: x,y,w,h
    glVertexAttribPointer(offsetScaleLoc, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)0);
    glVertexAttribDivisor(offsetScaleLoc, 1); // Advance per instance

    // a_color: r,g,b,a
    glVertexAttribPointer(colorLoc, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, r));
    glVertexAttribDivisor(colorLoc, 1);

    // Bind element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    // Draw all rectangles in one call
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0, (GLsizei)instances.size());
}

void RectanglesRenderer::clear() {
    instances.clear();
}
