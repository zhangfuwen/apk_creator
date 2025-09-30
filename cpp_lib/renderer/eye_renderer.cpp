#include "eye_renderer.h"
#include <GLES2/gl2.h>
#include <stdlib.h>

#include "ndk_utils/log.h"

// Random float in [-r, r]
static float jitter(float r) {
    return r * (2.0f * ((float)(rand() / RAND_MAX)) - 1.0f);
}

// Evaluate animation at time t
float evaluateAnimation(const Animation& anim, float t) {
    if (anim.count == 0)
        return 0.0f;
    if (t <= anim.keys[0].time)
        return anim.keys[0].value;
    if (t >= anim.keys[anim.count - 1].time) {
        return anim.looping ? evaluateAnimation(anim, fmodf(t, anim.keys[anim.count - 1].time))
                            : anim.keys[anim.count - 1].value;
    }

    for (int i = 0; i < anim.count - 1; ++i) {
        if (t >= anim.keys[i].time && t <= anim.keys[i + 1].time) {
            float t0 = anim.keys[i].time;
            float t1 = anim.keys[i + 1].time;
            float v0 = anim.keys[i].value;
            float v1 = anim.keys[i + 1].value;
            float ratio = (t - t0) / (t1 - t0);
            return v0 * (1.0f - ratio) + v1 * ratio;
        }
    }
    return anim.keys[anim.count - 1].value;
}

// ==================== Keyframe Animations ====================

// Blink: Fast open-close-open
const Keyframe blinkKeys[] = {
        {0.00f, 1.0f},  // Open
        {0.05f, 0.7f},  // Closing
        {0.15f, 0.5f},  // Closed
        {0.20f, 0.3f},  // Opening
        {0.30f, 0.0f}   // Open
};
const Keyframe sleepyKeys[] = {
        {0.00f, 1.0f},  // Open
        {0.50f, 0.8f},  // Closing
        {0.80f, 0.7f},  // Closed
        {1.00f, 0.6f},  // Opening
        {3.00f, 0.6f},  // Open
        {4.40f, 1.0f}   // Open
};

const Animation ANIM_BLINK = {.keys = blinkKeys, .count = 5, .looping = false};
const Animation ANIM_SLEEPY = {.keys = sleepyKeys, .count = 6, .looping = false};

// Wrinkle: Appears during blink
const Keyframe wrinkleKeys[] = {{0.00f, 0.0f}, {0.08f, 1.0f}, {0.18f, 1.0f}, {0.25f, 0.0f}, {0.30f, 0.0f}};

const Animation ANIM_WRINKLE = {.keys = wrinkleKeys, .count = 5, .looping = false};

// Keyframes for mouth open/close
const Keyframe mouthKeys[] = {{0.0f, 0.0f},  // Closed
                              {0.1f, 1.0f},  // Open
                              {2.2f, 1.0f},  // Open
                              {3.3f, 1.0f},  // Close
                              {3.4f, 0.0f}};

const Animation ANIM_MOUTH = {.keys = mouthKeys, .count = 5, .looping = false};

// ==================== EyeRenderer Implementation ====================

EyeRenderer::EyeRenderer() {
    //    srand(time(nullptr));

    // Pre-build unit circle
    for (int i = 0; i < SEGMENTS; ++i) {
        float theta = 2.0f * M_PI * i / SEGMENTS;
        circleVertices[2 * i] = cos(theta);
        circleVertices[2 * i + 1] = sin(theta);
    }

    compileShader();
}

EyeRenderer::~EyeRenderer() {
    if (program)
        glDeleteProgram(program);
}

void EyeRenderer::resize(int width, int height) {
    //    glViewport(0, 0, width, height);
    aspectRatio = (float)width / (height ? height : 1);
}

void EyeRenderer::playMouth() {
    LOGI("playMouth");
    if (mouthPlayer.state != PLAYING) {
        mouthPlayer.state = PLAYING;
        mouthPlayer.playTime = 0.0f;
    }
}

void EyeRenderer::update(float deltaTime) {
    time += deltaTime;

    // Update blink player
    if (blinkPlayer.state == PLAYING) {
        blinkPlayer.playTime += deltaTime;
        float duration = ANIM_BLINK.keys[ANIM_BLINK.count - 1].time;
        if (blinkPlayer.playTime / blinkPlayer.timeScale >= duration) {
            blinkPlayer.state = FINISHED;
            blinkPlayer.playTime = 0.0f;
        }
    }
    if (sleepyPlayer.state == PLAYING) {
        sleepyPlayer.playTime += deltaTime;
        float duration = ANIM_SLEEPY.keys[ANIM_SLEEPY.count - 1].time;
        if (sleepyPlayer.playTime / sleepyPlayer.timeScale >= duration) {
            sleepyPlayer.state = FINISHED;
            sleepyPlayer.playTime = 0.0f;
        }
    }

    // Update mouth player (optional: auto-play)
    if (mouthPlayer.state == PLAYING) {
        mouthPlayer.playTime += deltaTime;
        float duration = ANIM_MOUTH.keys[ANIM_MOUTH.count - 1].time;
        if (mouthPlayer.playTime >= duration) {
            LOGI("mouthPlayer.playTime >= duration, playTime = %f, duration = %f", mouthPlayer.playTime, duration);
            mouthPlayer.state = FINISHED;
            mouthPlayer.playTime = 0.0f;
        }
    }
}

void EyeRenderer::render() {
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUniform2f(uniformResolution, 2.0f * aspectRatio, 2.0f);
    glEnableVertexAttribArray(attribPosition);

    if (blinkPlayer.timeScale < 3.0f) {
        LOGV("blinkPlayer.timeScale: %f", blinkPlayer.timeScale);
    }
    float blinkValue = 0.0f;
    if (blinkPlayer.state == PLAYING) {
        blinkValue = evaluateAnimation(ANIM_BLINK, blinkPlayer.playTime / blinkPlayer.timeScale);
        wrinkleLevel = evaluateAnimation(ANIM_WRINKLE, blinkPlayer.playTime / blinkPlayer.timeScale);
    } else {
        wrinkleLevel = 0.0f;
    }
    if (sleepyPlayer.state == PLAYING) {
        blinkValue = evaluateAnimation(ANIM_SLEEPY, sleepyPlayer.playTime / sleepyPlayer.timeScale);
        wrinkleLevel = evaluateAnimation(ANIM_WRINKLE, sleepyPlayer.playTime / sleepyPlayer.timeScale);
    } else {
        wrinkleLevel = 0.0f;
    }


    float idleOffset = idleEnabled ? jitter(0.02f) : 0.0f;

    drawEye(leftEye, blinkValue, wrinkleLevel, idleOffset);
    drawEye(rightEye, blinkValue, wrinkleLevel, idleOffset);

    float mouthValue = 0.0f;
    if (mouthPlayer.state == PLAYING) {
        mouthValue = evaluateAnimation(ANIM_MOUTH, mouthPlayer.playTime);
    }

    // Draw mouth
    drawMouth(time, mouthValue, 0.0f);  // Smile whe

    glDisableVertexAttribArray(attribPosition);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

void EyeRenderer::playSleepy(float scale) {
    LOGI("playSleepy %f", scale);
    if (sleepyPlayer.state != PLAYING) {
        sleepyPlayer.state = PLAYING;
        sleepyPlayer.playTime = 0.0f;
        sleepyPlayer.timeScale = scale;
    }
}

void EyeRenderer::playBlink(float scale) {
    LOGI("playBlink %f", scale);
    if (blinkPlayer.state != PLAYING) {
        blinkPlayer.state = PLAYING;
        blinkPlayer.playTime = 0.0f;
        blinkPlayer.timeScale = scale;
    }
}

void EyeRenderer::setWrinkle(float strength) {
    wrinkleLevel = (strength < 0.0f) ? 0.0f : (strength > 1.0f) ? 1.0f : strength;
}

void EyeRenderer::enableIdle(bool enabled) {
    idleEnabled = enabled;
}

void EyeRenderer::drawEye(const Eye& eye, float blink, float wrinkle, float idleOffset) {
    glEnableVertexAttribArray(attribPosition);
    Vec2 center(eye.cx + idleOffset, eye.cy + idleOffset * 0.5f);

    drawSclera(eye);
    drawIris(eye, idleOffset);
    drawPupil(eye, idleOffset);
    drawEyelid(eye, blink);
    if (wrinkle > 0.01f) {
        //    drawWrinkles(eye, wrinkle);
    }
}

void EyeRenderer::drawSclera(const Eye& eye) {
    float rx = eye.radius * aspectRatio;
    float ry = eye.radius;
    buildCircle(eye.cx, eye.cy, rx, ry, circleVertices);

    glVertexAttribPointer(attribPosition, 2, GL_FLOAT, GL_FALSE, 0, circleVertices);
    glUniform4f(uniformColor, 1.0f, 1.0f, 1.0f, 1.0f);  // White
    glDrawArrays(GL_TRIANGLE_FAN, 0, SEGMENTS);
}

void EyeRenderer::drawIris(const Eye& eye, float idleOffset) {
    float irx = eye.irisRadius * aspectRatio;
    float iry = eye.irisRadius;
    float cx = eye.cx + idleOffset * 0.3f;
    float cy = eye.cy + idleOffset * 0.3f;
    buildCircle(cx, cy, irx, iry, circleVertices);

    glVertexAttribPointer(attribPosition, 2, GL_FLOAT, GL_FALSE, 0, circleVertices);
    glUniform4f(uniformColor, 0.2f, 0.4f, 0.8f, 1.0f);  // Blue
    glDrawArrays(GL_TRIANGLE_FAN, 0, SEGMENTS);
}

void EyeRenderer::drawPupil(const Eye& eye, float idleOffset) {
    float pr = eye.irisRadius * eye.pupilScale;
    float prx = pr * aspectRatio;
    float pry = pr;
    float cx = eye.cx + idleOffset * 0.5f;
    float cy = eye.cy + idleOffset * 0.5f;
    buildCircle(cx, cy, prx, pry, circleVertices);

    glVertexAttribPointer(attribPosition, 2, GL_FLOAT, GL_FALSE, 0, circleVertices);
    glUniform4f(uniformColor, 0.0f, 0.0f, 0.0f, 1.0f);  // Black
    glDrawArrays(GL_TRIANGLE_FAN, 0, SEGMENTS);
}

void EyeRenderer::drawEyelid(const Eye& eye, float blink) {
    if (blink <= 0.0f)
        return;

    float rx = eye.radius * aspectRatio;
    float lidHeight = eye.radius * (1.0f - blink * blink) * 2;

    float vertices[] = {eye.cx - rx, eye.cy + eye.radius - lidHeight,
                        eye.cx + rx, eye.cy + eye.radius - lidHeight,
                        eye.cx + rx, eye.cy + eye.radius,
                        eye.cx - rx, eye.cy + eye.radius};

    glVertexAttribPointer(attribPosition, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glUniform4f(uniformColor, 0.0f, 0.0f, 0.0f, 1.0f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void EyeRenderer::drawWrinkles(const Eye& eye, float wrinkle) {
    float rx = eye.radius * aspectRatio;
    float baseLen = rx * 0.5f;

    float startX = eye.cx + rx;
    float startYU = eye.cy + eye.radius * 0.4f;
    float startYL = eye.cy - eye.radius * 0.4f;

    float endX = startX + baseLen * (1.0f + wrinkle * 0.8f);
    float endYU = startYU + baseLen * (0.6f + wrinkle * 0.6f);
    float endYL = startYL - baseLen * (0.6f + wrinkle * 0.6f);

    // float startX = 0.1f;
    // float endX = 0.8f;
    //
    // float startYU = 0.5f;
    // float startYL = 0.2f;
    // float endYU = 0.8f;
    // float endYL = 0.2f;

    float lines[] = {startX, startYU, endX, endYU, startX, startYL, endX, endYL};

    glVertexAttribPointer(attribPosition, 2, GL_FLOAT, GL_FALSE, 0, lines);
    glUniform4f(uniformColor, 1.0f, 0.0f, 0.0f, 1.0f);
    glLineWidth(1.5f);
    glDrawArrays(GL_LINES, 0, 4);
    glLineWidth(1.0f);
}

void EyeRenderer::buildCircle(float cx, float cy, float rx, float ry, float* outVertices) {
    for (int i = 0; i < SEGMENTS; ++i) {
        float theta = 2.0f * M_PI * i / SEGMENTS;
        outVertices[2 * i] = cx + cos(theta) * rx;
        outVertices[2 * i + 1] = cy + sin(theta) * ry;
    }
}

void EyeRenderer::drawMouth(float t, float smileFactor, float openFactor) {
    (void)t;
    float cx = 0.0f;                                   // Center of face
    float cy = -0.3f;                                  // Below eyes
    float width = 0.4f;                                // Mouth width
    float height = 0.1f * (1.0f + openFactor * 2.0f);  // Taller when open

    // Apply smile curve: y = a * x^2
    float vertices[SEGMENTS * 2];
    float halfWidth = width * 0.5f;
    float smileCurve = 0.3f * smileFactor;  // How much it curves up

    for (int i = 0; i < SEGMENTS; ++i) {
        float xNorm = (float)i / (SEGMENTS - 1);  // 0 to 1
        float x = cx - halfWidth + xNorm * width;
        float y = cy + smileCurve * (x - cx) * (x - cx) / (width * 0.5f);
        vertices[2 * i] = x;
        vertices[2 * i + 1] = y;
    }

    glVertexAttribPointer(attribPosition, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glUniform4f(uniformColor, 0.8f, 0.2f, 0.2f, 1.0f);  // Red/pink mouth
    glLineWidth(3.0f);
    glDrawArrays(GL_LINE_STRIP, 0, SEGMENTS);
    glLineWidth(1.0f);

    // Optional: draw open mouth as ellipse (like a talking oval)
    if (openFactor > 0.1f) {
        float openHeight = height * 0.8f;
        buildCircle(cx, cy - openHeight * 0.5f, width * 0.4f, openHeight, circleVertices);
        glVertexAttribPointer(attribPosition, 2, GL_FLOAT, GL_FALSE, 0, circleVertices);
        glUniform4f(uniformColor, 0.5f, 0.1f, 0.1f, 1.0f);  // Darker inside
        glDrawArrays(GL_TRIANGLE_FAN, 0, SEGMENTS);
    }
}

void EyeRenderer::compileShader() {
    const char* vertexShaderSource = R"(
        attribute vec2 a_position;
        uniform vec2 u_resolution;
        void main() {
            vec2 clipSpace = (a_position / u_resolution) * 2.0;// - 1.0;
            gl_Position = vec4(clipSpace, 0, 1);
            //gl_Position = vec4(a_position, 0, 1);
        }
    )";

    const char* fragmentShaderSource = R"(
        precision mediump float;
        uniform vec4 u_color;
        void main() {
            gl_FragColor = u_color;
        }
    )";

    auto compile = [](GLenum type, const char* source) -> GLuint {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint len = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
            if (len > 1) {
                char* log = new char[len];
                glGetShaderInfoLog(shader, len, nullptr, log);
                // In production: log error via your system
                LOGI("Shader compilation failed: %s", log);
                delete[] log;
            }
        }
        return shader;
    };

    GLuint vs = compile(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentShaderSource);

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    //glBindAttribLocation(program, 0, "a_position");
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    attribPosition = glGetAttribLocation(program, "a_position");
    uniformColor = glGetUniformLocation(program, "u_color");
    uniformResolution = glGetUniformLocation(program, "u_resolution");
    LOGI("Program: %d, attribPosition: %d, uniformColor: %d, uniformResolution: %d", program, attribPosition,
         uniformColor, uniformResolution);
}

void EyeRenderer::handleButton([[maybe_unused]] float x, [[maybe_unused]] float y, [[maybe_unused]] int button, [[maybe_unused]] int bDown) { 
    if (x > 0.0f) {
        playBlink();
    } else {
        playMouth();
    }
}

void EyeRenderer::handleMotion(float x, float y, int mask) {
    LOGD("Motion: %f,%f (%d)\n", x, y, mask);
    if ( x > 0.0f) {
        playBlink();
    } else {
        playMouth();
    }
}
