/* date = July 29th 2025 1:21 am */

#ifndef BASE_MATH_H
#define BASE_MATH_H

typedef struct V2F V2F;
struct V2F
{
    f32 x, y;
};

typedef struct V2S V2S;
struct V2S
{
    s32 x, y;
};

typedef struct V4F V4F;
struct V4F
{
    f32 x, y, z, w;
};

typedef struct RectF RectF;
struct RectF
{
    V2F pos;
    V2F size;
};

function RectF make_rectf(f32 pos_x, f32 pos_y, f32 size_x, f32 size_y)
{
    return (RectF){(V2F){pos_x, pos_y}, (V2F){size_x, size_y}};
}

#endif //BASE_MATH_H
