
#ifndef __MYOS__GUI__WIDGET_H
#define __MYOS__GUI__WIDGET_H


#include <common/graphicscontext.h>
//#include <drivers/keyboard.h>

namespace myos
{
    namespace gui
    {

        class Widget //: public myos::drivers::KeyboardEventHandler
        {
        protected:
            Widget* parent;
            int32_t x;
            int32_t y;
            int32_t w;
            int32_t h;

            uint8_t r;
            uint8_t g;
            uint8_t b;
            bool Focussable;

        public:

            Widget(Widget* parent,
                   int32_t x, int32_t y, int32_t w, int32_t h,
                   uint8_t r, uint8_t g, uint8_t b);
            ~Widget();

            virtual void GetFocus(Widget* widget);
            virtual void ModelToScreen(int32_t x, int32_t y);
            virtual bool ContainsCoordinate(int32_t x, int32_t y);

            virtual void Draw(common::GraphicsContext* gc);
            virtual void OnMouseDown(int32_t x, int32_t y, uint8_t button);
            virtual void OnMouseUp(int32_t x, int32_t y, uint8_t button);
            virtual void OnMouseMove(int32_t oldx, int32_t oldy, int32_t newx, int32_t newy);
        };


        class CompositeWidget : public Widget
        {
        private:
            Widget* children[100];
            int numChildren;
            Widget* focussedChild;

        public:
            CompositeWidget(Widget* parent,
                   int32_t x, int32_t y, int32_t w, int32_t h,
                   uint8_t r, uint8_t g, uint8_t b);
            ~CompositeWidget();

            virtual void GetFocus(Widget* widget);
            virtual bool AddChild(Widget* child);

            virtual void Draw(common::GraphicsContext* gc);
            virtual void OnMouseDown(int32_t x, int32_t y, uint8_t button);
            virtual void OnMouseUp(int32_t x, int32_t y, uint8_t button);
            virtual void OnMouseMove(int32_t oldx, int32_t oldy, int32_t newx, int32_t newy);

            virtual void OnKeyDown(char);
            virtual void OnKeyUp(char);
        };
    }
}


#endif
