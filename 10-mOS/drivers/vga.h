
#ifndef __MYOS__DRIVERS__VGA_H
#define __MYOS__DRIVERS__VGA_H

namespace myos
{
    namespace drivers
    {

        class VideoGraphicsArray
        {
        protected:
            virtual uint8_t GetColorIndex(uint8_t r, uint8_t g, uint8_t b);


        public:
            VideoGraphicsArray();
            ~VideoGraphicsArray();

            virtual bool SupportsMode(uint32_t width, uint32_t height, uint32_t colordepth);
            virtual bool SetMode(uint32_t width, uint32_t height, uint32_t colordepth);
            virtual void PutPixel(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b);
            virtual void PutPixel(int32_t x, int32_t y, uint8_t colorIndex);

            virtual void FillRectangle(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t r, uint8_t g, uint8_t b);

        };

    }
}

#endif