#include <algorithm>
#include <cstdlib>
#include <memory>
#include <vector>
#include <GLES3/gl32.h>

#include "rectangles_renderer.h"
#include "ndk_utils/log.h"

namespace renderer_2d {
    struct Point {
        float x, y;
    };

    struct Size {
        float width, height;
    };

    struct Color {
        float r, g, b, a;
    };

    class Object {
      public:
        Object(Point start) { position = start; }
        virtual ~Object() {}

        Point position;
        Size  scale = {1.0f, 1.0f};
        Color color = {0.5f, 0.0f, 0.0f, 1.0f};

        virtual void draw() { }
        virtual void update() {}

        void addChild(std::shared_ptr<Object> o) { children.push_back(o); }

      protected:
        std::vector<std::shared_ptr<Object>> children;
    };

    class Rectangle : public Object {
      public:
        Rectangle(float x, float y, float w, float h) : Object({x, y}) { size_ = {w, h}; }

        Rectangle(Point start, Size sz) : Object(start) { size_ = sz; }
        void update() override {
            srand(time(nullptr));
            auto delta_x = (rand() %100) * 0.01 - 0.5;
            auto delta_y = (rand() %100) * 0.01 - 0.5;
            position.x += delta_x;
            position.y += delta_y;
            position.x = std::clamp(position.x, 0.0f, 1.0f);
            position.y = std::clamp(position.y, 0.0f, 1.0f);

            LOGV("new position, x:%f, y:%f", position.x, position.y);
        }

      private:
        friend class RectanglesRenderer;
        friend class Scene;
        Size size_;
    };

    class Triangle : Object {
        Triangle(Point a, Point b, Point c) : Object(a) {
            points[0] = a;
            points[1] = b;
            points[2] = c;
        }

        void draw() override { }

      private:
        Point points[3];
    };

    class Line : Object {
        Line(Point a, Point b) : Object(a) {
            points[0] = a;
            points[1] = b;
        }

        void draw() override { }

      private:
        Point points[2];
    };

    class Circle : Object {
        Circle(Point center, float radius) : Object(center) { radius_ = radius; }

        void draw() override { }

      private:
        float radius_;
    };

    class Scene {
        public:
        RectanglesRenderer rectangles_renderer;

        void addRectangle(std::shared_ptr<Rectangle> rect) { rectangles.push_back(rect); }

        void update() {
            LOGV("update");
            for(auto &rect : rectangles) {
                rect->update();
            }
        }

        void draw() {
            rectangles_renderer.clear();
            for (auto o : rectangles) {
                rectangles_renderer.addRectangle(o->position.x, o->position.y, o->size_.width, o->size_.height);
            }
            rectangles_renderer.render();
        }

      private:
        std::vector<std::shared_ptr<Rectangle>> rectangles;
    };
}  // namespace renderer_2d
