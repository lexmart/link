#include <intrin.h>
#include <math.h>

// TODO(lex): Replace these all with intrinsics
// https://software.intel.com/sites/landingpage/IntrinsicsGuide/

struct bit_scan_result
{
    b32 Found;
    int Index;
};

inline bit_scan_result
FindLeastSignificantSetBit(u32 Value)
{
    bit_scan_result Result = {};

#if 1
    Result.Found = _BitScanForward((unsigned long *)&Result.Index, Value);
#else
    for(int TestBit = 0;
        TestBit < 32;
        TestBit++)
    {
        if(Value & (1 << TestBit))
        {
            Result.Index = TestBit;
            Result.Found = true;
            break;
        }
    }
#endif

    return Result;
}

inline i32
RoundReal32ToInt32(r32 Number)
{
    return (i32)roundf(Number);
}


inline r32
SquareRoot(r32 Number)
{
    r32 Result = sqrtf(Number);
    return Result;
}

inline r32
AbsoluteValue(r32 Number)
{
    r32 Result = fabs(Number);
    return Result;
}

inline int
AbsoluteValue(int Number)
{
    if(Number < 0)
    {
        Number = -Number;
    }
    return Number;
}

inline int
StringLength(char *String)
{
    int Result = 0;
    
    while(*String++)
    {
        Result++;
    }
    
    return Result;
}

inline void 
IntToString(int Int, char *Output)
{
    char IntString[64] = {};
    int IntStringIndex = 0;
    
    while(Int)
    {
        IntString[IntStringIndex++] = (Int % 10) + '0';
        Int /= 10;
    }
    
    int RightIndex = IntStringIndex - 1;
    for(int LeftIndex = 0;
        LeftIndex < RightIndex;
        LeftIndex++, RightIndex--)
    {
        char Temp = IntString[LeftIndex];
        IntString[LeftIndex] = IntString[RightIndex];
        IntString[RightIndex] = Temp;
    }
    
    // NOTE: Assumes there is enough space
    char *Char = &IntString[0];
    while(*Char)
    {
        *Output++ = *Char++;
    }
    *Output = 0;
}

inline int
StringToInt(char *String)
{
    int Result = 0;
    
    while(*String)
    {
        Result *= 10;
        
        int Digit = *String - '0';
        Result += Digit;
        
        String++;
    }
    
    return Result;
}

inline void
CopySubString(char *Base, char *Substring)
{
    while(*Substring)
    {
        *Base++ = *Substring++;
    }
}