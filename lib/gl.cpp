#include <memory>
#include <stdint.h>
#include <vector>
#include <GLES3/gl32.h>

#include "rectangles_renderer.h"

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

    struct Object {
        Object(Point start) { position = start; }

        Point position;
        Size  scale = {1.0f, 1.0f};
        Color color = {0.5f, 0.0f, 0.0f, 1.0f};

        virtual void draw() { }

        void addChild(Object o) { children.push_back(o); }

      private:
        std::vector<Object> children;
    };

    class Rectangle : Object {
        Rectangle(Point start, Size sz) : Object(start) { size_ = sz; }

      private:
        friend class RectanglesRenderer;
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

    struct Scene {
        RectanglesRenderer rectangles_renderer;

        void addRectangle(Rectangle* rect) { rectangles.push_back(std::shared_ptr<Rectangle>(rect)); }

        void addObejct(Object o) { objects.push_back(o); }

        void draw() {
            for (auto o : rectangles) {
                rectangles_renderer.addRectangle(o->position.x, o.position.y, o.size_.width, o.size_.height);
            }
        }

      private:
        std::vector<std::shared_ptr<Rectangle>> rectangles;
        std::vector<Object>                     objects;
    };
}  // namespace renderer_2d
