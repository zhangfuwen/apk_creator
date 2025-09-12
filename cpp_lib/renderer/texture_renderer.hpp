#include <GLES2/gl2.h>
#include "ndk_utils/log.h"

class TextureRenderer {
  private:
    GLuint mProgram = 0;
    GLuint mVertexPosAttr = 0;
    GLuint mTexCoordAttr = 0;
    GLuint mTextureUniform = 0;

    GLuint mVBO = 0;
    GLuint mTEX = 0;
    GLuint mTextureID = 0;

    float mVertices[8 * 2] = {
            // Position (x, y)      // TexCoord (u, v)
            -0.5f, -0.5f, 0.0f, 1.0f, 0.5f, -0.5f, 1.0f, 1.0f, 0.5f, 0.5f, 1.0f, 0.0f, -0.5f, 0.5f, 0.0f, 0.0f};

    GLushort mIndices[6] = {0, 1, 2, 0, 2, 3};  // Two triangles

  public:
    bool init() {
        const char* vertexShaderSource = "attribute vec2 a_Position;            \n"
                                         "attribute vec2 a_TexCoord;            \n"
                                         "varying vec2 v_TexCoord;              \n"
                                         "void main() {                         \n"
                                         "    gl_Position = vec4(a_Position, 0.0, 1.0); \n"
                                         "    v_TexCoord = a_TexCoord;          \n"
                                         "}";

        const char* fragmentShaderSource = "precision mediump float;              \n"
                                           "uniform sampler2D u_Texture;          \n"
                                           "varying vec2 v_TexCoord;              \n"
                                           "void main() {                         \n"
                                           "    gl_FragColor = texture2D(u_Texture, v_TexCoord);\n"
                                           "}";

        // Compile shaders
        GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexShaderSource);
        GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

        if (!vertexShader || !fragmentShader) {
            LOGE("Failed to compile shaders");
            return false;
        }

        // Create program
        mProgram = glCreateProgram();
        glAttachShader(mProgram, vertexShader);
        glAttachShader(mProgram, fragmentShader);
        glLinkProgram(mProgram);

        // Check link status
        GLint linked;
        glGetProgramiv(mProgram, GL_LINK_STATUS, &linked);
        if (!linked) {
            GLint infoLen = 0;
            glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen > 1) {
                char* infoLog = new char[infoLen];
                glGetProgramInfoLog(mProgram, infoLen, nullptr, infoLog);
                LOGE("Error linking program: %s", infoLog);
                delete[] infoLog;
            }
            glDeleteProgram(mProgram);
            mProgram = 0;
            return false;
        }

        // Get attribute/uniform locations
        mVertexPosAttr = glGetAttribLocation(mProgram, "a_Position");
        mTexCoordAttr = glGetAttribLocation(mProgram, "a_TexCoord");
        mTextureUniform = glGetUniformLocation(mProgram, "u_Texture");

        // Generate VBO and upload vertices
        glGenBuffers(1, &mVBO);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(mVertices), mVertices, GL_STATIC_DRAW);

        // Generate EBO for indices
        glGenBuffers(1, &mTEX);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mTEX);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(mIndices), mIndices, GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        LOGI("Renderer initialized successfully");
        return true;
    }

    bool loadTexture(const void* pixels, int width, int height) {
        if (!mProgram)
            return false;
        if (!pixels || width <= 0 || height <= 0) {
            LOGE("Invalid texture data: pixels=%p, w=%d, h=%d", pixels, width, height);
            return false;
        } else {
            LOGE("texture data: pixels=%p, w=%d, h=%d", pixels, width, height);
        }

        if (mTextureID == 0) {
            glGenTextures(1, &mTextureID);
            glBindTexture(GL_TEXTURE_2D, mTextureID);

            // Set sampling parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            
        } else {
            glBindTexture(GL_TEXTURE_2D, mTextureID);
        }

        // 👉 Fix 1: Ensure correct unpack alignment
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);  // Default is 4-byte align

        // 👉 Fix 2: Calculate row size and ensure no padding issues
        int stride = width * 4;  // RGBA = 4 bytes per pixel
        // int expectedSize = stride * height;

        // Optional: Log some debug info
        LOGV("Uploading texture: %dx%d, stride=%d", width, height, stride);

        // 👉 Main texture upload
        glTexImage2D(GL_TEXTURE_2D,
                     0,        // level
                     GL_RGBA,  // internal format
                     width, height,
                     0,                 // border - must be 0 in GLES!
                     GL_RGBA,           // format of incoming data
                     GL_UNSIGNED_BYTE,  // type of incoming data
                     pixels);

        // 👉 Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            LOGE("glTexImage2D failed with error: %d (0x%x)", error, error);
            return false;
        }

        LOGV("Texture uploaded successfully");
        return true;
    }

    void render() {
        if (!mProgram || !mTextureID)
            return;

        glUseProgram(mProgram);

        // Bind texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mTextureID);
        glUniform1i(mTextureUniform, 0);  // Use texture unit 0

        // Bind vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mTEX);

        glEnableVertexAttribArray(mVertexPosAttr);
        glVertexAttribPointer(mVertexPosAttr, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(mTexCoordAttr);
        glVertexAttribPointer(mTexCoordAttr, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        // Draw quad
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

        // Cleanup
        glDisableVertexAttribArray(mVertexPosAttr);
        glDisableVertexAttribArray(mTexCoordAttr);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }

    void destroy() {
        if (mProgram) {
            glDeleteProgram(mProgram);
            mProgram = 0;
        }
        if (mVBO) {
            glDeleteBuffers(1, &mVBO);
            mVBO = 0;
        }
        if (mTEX) {
            glDeleteBuffers(1, &mTEX);
            mTEX = 0;
        }
        if (mTextureID) {
            glDeleteTextures(1, &mTextureID);
            mTextureID = 0;
        }
    }

  private:
    GLuint loadShader(GLenum type, const char* source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen > 1) {
                char* infoLog = new char[infoLen];
                glGetShaderInfoLog(shader, infoLen, nullptr, infoLog);
                LOGE("Error compiling shader: %s", infoLog);
                delete[] infoLog;
            }
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }
};
