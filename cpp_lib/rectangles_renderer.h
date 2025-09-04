#pragma once

#include <GLES3/gl32.h>
#include <vector>

#include "render.h"

namespace renderer_2d {

class RectanglesRenderer : Renderer {
private:
    GLuint program;
    GLuint vbo_quad;        // Unit quad geometry
    GLuint vbo_instances;   // Instance data buffer
    GLuint ebo;             // Index buffer

    GLint posLoc;
    GLint offsetScaleLoc;
    GLint colorLoc;

    struct InstanceData {
        float x, y, w, h;
        float r, g, b, a;
    };

    std::vector<InstanceData> instances;

    GLuint compileShader(GLenum type, const char* source);
    GLuint createProgram();

public:
    RectanglesRenderer();
    ~RectanglesRenderer();

    // Add a rectangle: x, y, width, height, color (r,g,b,a)
    void addRectangle(float x, float y, float w, float h,
                      float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);

    // Update GPU buffer and draw all rectangles
    void render();

    // Clear all rectangles
    void clear();
};
};
