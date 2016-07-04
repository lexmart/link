#include "platform_game_shared.h"
#include "link_intrinsics.h"
#include "link_math.cpp"
#include "link_random.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct memory_arena
{
    void *Base;
    int Used;
    int Size;
};

inline memory_arena
InitMemoryArena(void *Base, int Size)
{
    memory_arena Result = {};
    Result.Base = Base;
    Result.Size = Size;
    return Result;
}

struct bitmap
{
    u32 *Memory;
    i32 Width;
    i32 Height;
};

internal void *
PushMemory(memory_arena *Arena, int Bytes)
{
    void *Result = 0;
    
    if((Arena->Used + Bytes) < Arena->Size)
    {
        Result = (u8 *)Arena->Base + Arena->Used;
        Arena->Used += Bytes;
    }
    else
    {
        // NOTE(lex): Not enough space left
        InvalidCodePath;
    }
    
    return Result;
}
#define PushStruct(type, Arena) (type *)PushMemory(Arena, sizeof(type))
#define PushArray(type, Count, Arena) (type *)PushMemory(Arena, sizeof(type)*Count)

inline b32
MemoryBlocksEqual(u8 *A, u8 *B, int Bytes)
{
    b32 Result = true;
    
    for(int ByteIndex = 0;
        ByteIndex < Bytes;
        ByteIndex++, A++, B++)
    {
        if(*A != *B)
        {
            Result = false;
        }
    }
    
    return Result;
}

#pragma pack(push, 1)
struct bitmap_header
{
    u16 FileType;
    u32 FileSize;
    u16 Reserved1;
    u16 Reserved2;
    u32 BitmapOffset;
    u32 Size;
    i32 Width;
    i32 Height;
    u16 Planes;
    u16 BitsPerPixel;
    u32 Compression;
    u32 SizeOfBitmap;
    i32 HorzRes;
    i32 VertRes;
    u32 ColorsUsed;
    u32 ColorImp;

    u32 RedMask;
    u32 GreenMask;
    u32 BlueMask;
};
#pragma pack(pop)

struct linked_bitmaps
{
    bitmap R, U, L, D;
    bitmap R_U, R_L, R_D;
    bitmap U_L, U_D;
    bitmap L_D;
    bitmap R_U_L, U_L_D, R_L_D, R_U_D;
    bitmap R_U_L_D;
};

struct tile_bitmaps
{
    bitmap Empty;
    bitmap Red;
    bitmap Green;
    bitmap Blue;
    bitmap LinkedLeftSide;
    bitmap LinkedRightSide;
    // NOTE: [right][up][left][down] where 1 = true and 0 = false
    bitmap Linked[2][2][2][2];
};

struct level
{
    int *Tiles;
    int Width;
    int Height;
    file_contents *FileHandle;
};

struct timer
{
    b32 Set;
    r32 TimeLeft;
};

inline void
SetTimer(timer *Timer, r32 Seconds)
{
    Timer->TimeLeft = Seconds;
    Timer->Set = true;
}

inline void 
TickTimer(timer *Timer, r32 TimePassed)
{
    if(Timer->TimeLeft > 0)
    {
        Timer->TimeLeft -= TimePassed;
    }
}

inline b32
IsTimerDone(timer *Timer)
{
    b32 Result = (Timer->TimeLeft <= 0);
    if(Result)
    {
        Timer->Set = false;
    }
    return Result;
}
    
struct game_state
{
    b32 Initialized;
    memory_arena Arena, LevelArena;
    
    bitmap SpriteSheet;
    tile_bitmaps TileBitmaps;
    
    r32 TileSideInPixels;
    
    timer NextLevelTimer;
    
    v2 MouseDownP;
    
    level Level;
    int SelectedTileType;

    int CurrentLevelId;
    int MaxLevelId;
    
    level LevelHistory[4096]; // NOTE(lex): This must be a pow2
    level LevelAtFrameStart;
    int HistoryLastInsertedIndex;
    int LevelHistoryCount;
    int UndoOffset;
};


enum tile_type
{
    TileType_Empty,
    TileType_Linked,
    
    // NOTE(lex): These must be the first two tiles
    
    TileType_Red,
    TileType_Green,
    
    TileType_Count
};