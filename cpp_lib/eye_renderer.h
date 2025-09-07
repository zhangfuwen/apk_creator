#ifndef EYE_RENDERER_H
#define EYE_RENDERER_H

#include <math.h>
#include <GLES3/gl32.h>

// 2D vector helper
struct Vec2 {
    float x, y;
    Vec2(float x = 0, float y = 0) : x(x), y(y) {}
};

// --- Keyframe Animation System ---
struct Keyframe {
    float time;   // Time in seconds
    float value;  // Value at this time (e.g., blink = 1.0)
};

struct Animation {
    const Keyframe* keys;  // Array of keyframes
    int count;             // Number of keyframes
    bool looping;          // Whether to loop
};

// Helper: Evaluate animation at time t
float evaluateAnimation(const Animation& anim, float t);

// --- Eye Renderer Class ---
class EyeRenderer {
public:
    EyeRenderer();
    ~EyeRenderer();

    void resize(int width, int height);
    void update(float deltaTime);
    void render();

    // Animation control
    //void playBlink(float duration = 0.3f);
    void playBlink(float duration = 2.0f);
    void setWrinkle(float strength);
    void enableIdle(bool enabled);

    // Access current time
    float getTime() const { return time; }

private:
    enum AnimState { STOPPED, PLAYING, FINISHED };
    struct AnimPlayer {
        const Animation* anim = nullptr;
        float playTime = 0.0f;
        float timeScale = 1.0f;
        AnimState state = STOPPED;
    };

    AnimPlayer blinkPlayer;
    float wrinkleLevel = 0.0f;
    bool idleEnabled = true;

    // Eye geometry
    struct Eye {
        float cx, cy;
        float radius;
        float irisRadius;
        float pupilScale;
    };

    Eye leftEye  = { -0.4f,  0.2f, 0.3f, 0.12f, 0.6f };
    Eye rightEye = {  0.4f,  0.2f, 0.3f, 0.12f, 0.6f };

    float time = 0.0f;
    float aspectRatio = 1.0f;

    // Rendering
    GLuint program = 0;
    GLint attribPosition = 0;
    GLint uniformColor = 0;
    GLint uniformResolution = 0;

    static const int SEGMENTS = 64;
    float circleVertices[SEGMENTS * 2];
    float lineVertices[4];

    void compileShader();
    void buildCircle(float cx, float cy, float rx, float ry, float* outVertices);
    void drawEye(const Eye& eye, float blink, float wrinkle, float idleOffset);

    void drawSclera(const Eye& eye);
    void drawIris(const Eye& eye, float idleOffset);
    void drawPupil(const Eye& eye, float idleOffset);
    void drawEyelid(const Eye& eye, float blink);
    void drawWrinkles(const Eye& eye, float wrinkle);
};

#endif // EYE_RENDERER_H
