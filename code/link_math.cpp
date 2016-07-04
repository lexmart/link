#define Square(A) ((A)*(A))

struct v2
{
    union
    {
        r32 E[2];
        struct
        {
            r32 X, Y;
        };
    };

    inline v2 &operator*=(r32 A);
    inline v2 &operator+=(v2 A);
};

inline v2
V2(r32 X, r32 Y)
{
    v2 Result;
    Result.X = X;
    Result.Y = Y;
    return Result;
}

inline v2 operator*(r32 C, v2 V)
{
    v2 Result;
    
    Result.X = C*V.X;
    Result.Y = C*V.Y;

    return Result;
}

inline v2 operator-(v2 V)
{
    v2 Result;

    Result.X = -V.X;
    Result.Y = -V.Y;

    return Result;
}

inline v2 operator+(v2 A, v2 B)
{
    v2 Result;

    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;

    return Result;
}

inline v2 operator-(v2 A, v2 B)
{
    v2 Result;

    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;

    return Result;
}

inline v2 &
v2::operator*=(r32 V)
{
    *this = V * *this;
    return *this;
}

inline v2 &
v2::operator+=(v2 V)
{
    *this = V + *this;
    return *this;
}

inline v2
GetMostSignfiicantCardinalDirection(v2 V)
{
    v2 Result = {};
    
    if(AbsoluteValue(V.X) > AbsoluteValue(V.Y))
    {
        if(V.X > 0)
        {
            Result.X = 1;
        }
        else
        {
            Result.X = -1;
        }
    }
    else
    {
        if(V.Y > 0)
        {
            Result.Y = 1;
        }
        else
        {
            Result.Y = -1;
        }
    }
    
    return Result;
}

inline r32
Inner(v2 A, v2 B)
{
    r32 Result;
    Result = (A.X*B.X) + (A.Y*B.Y);
    return Result;
}

inline r32
LengthSq(v2 V)
{
    r32 Result;
    Result = (Square(V.X) + Square(V.Y));
    return Result;
}

struct rect2
{
    v2 Min;
    v2 Max;
};

internal rect2
Rect2(v2 Min, v2 Max)
{
    rect2 Result;
    Result.Min = Min;
    Result.Max = Max;
    return Result;
}

internal b32
IsPointInRect(v2 Point, rect2 Rect)
{
    b32 Result = ((Point.X >= Rect.Min.X) && (Point.Y >= Rect.Min.Y) &&
                  (Point.X <= Rect.Max.X) && (Point.Y <= Rect.Max.Y));
    return Result;
}