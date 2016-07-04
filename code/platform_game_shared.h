#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float r32;
typedef double r64;

typedef int b32;

#define global_variable static
#define local_persist static
#define internal static

#define KiloBytes(Num) ((Num)*1024)
#define MegaBytes(Num) (KiloBytes((Num))*1024)
#define GigaBytes(Num) (MegaBytes((Num))*1024)

#define ArrayCount(Array) (sizeof(Array)/(sizeof((Array)[0])))

#define Assert(Expression) if(!(Expression)) { *((int *)0) = 0; }

#define InvalidCodePath Assert(!"Invalid code path");

struct game_memory
{
    void *PermanentMemory;
    int PermanentMemorySize;
    void *TempMemory;
    int TempMemorySize;
};

struct game_button
{
    b32 WasDown;
    b32 IsDown;
};

struct game_controller
{
    union
    {
        struct
        {
            game_button MoveRight, MoveUp, MoveLeft, MoveDown;
            game_button ActionRight, ActionUp, ActionLeft, ActionDown;
        };
        game_button Buttons[8];
    };
    
    game_button Numbers[10];
        
    b32 MouseLeftDownEvent, MouseLeftUpEvent;
    b32 MouseRightDownEvent, MouseRightUpEvent;
    int MouseX;
    int MouseY;
};

#define PressedOnce(Button) ((Button).IsDown && (!(Button).WasDown))

struct game_screen
{
    void *Memory;
    int Width, Height;
};

struct file_contents
{
    u64 Size;
    void *Memory;
};

#define PLATFORM_FREE_FILE(name) void name(void *Memory)
typedef PLATFORM_FREE_FILE(platform_free_file);

#define PLATFORM_READ_FILE(name) file_contents name(char *Filename)
typedef PLATFORM_READ_FILE(platform_read_file);

#define PLATFORM_WRITE_FILE(name) b32 name(char *Filename, void *Memory, u32 BytesToWrite)
typedef PLATFORM_WRITE_FILE(platform_write_file);

struct platform_services
{
    platform_free_file *FreeFile;
    platform_read_file *ReadFile;
    platform_write_file *WriteFile;
};

#define GAME_UPDATE_AND_RENDER(name) void name(platform_services *Platform, game_screen *Screen, game_memory *Memory, game_controller *Controller, r32 dT)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub)
{
}