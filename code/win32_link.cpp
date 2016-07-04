
/* TODO

-Sound

*/

#include "platform_game_shared.h"
#include <windows.h>
#include <Windowsx.h>

struct win32_input_recording
{
    b32 Recording, Playing;
    HANDLE FileHandle;
    void *PlaybackMemory;
    int PlaybackIndex;
    int PlaybackCount;
    void *PlaybackGameState;
};

global_variable struct win32_state
{
    b32 Running;
    LARGE_INTEGER PerformanceFrequency;
    win32_input_recording InputRecording;
} Win32State;

struct win32_screen_buffer
{
    int Width, Height;
    void *Memory;
    BITMAPINFO Info;
};

struct win32_game_code
{
    FILETIME LastLoadTime;
    char EXEFilePath[1024];
    char GameCodePath[1024];
    char TempGameCodePath[1024];
    HMODULE Library;
    game_update_and_render *GameUpdateAndRender;
};

PLATFORM_FREE_FILE(PlatformFreeFile)
{
    if(Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

PLATFORM_READ_FILE(PlatformReadFile)
{
    HANDLE FileHandle = CreateFile(Filename,
                                   GENERIC_READ,
                                   0,
                                   0,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   0);
    
    file_contents Result = {};
    
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD FileSize = GetFileSize(FileHandle, 0);
        Result.Size = FileSize;
        
        Result.Memory = VirtualAlloc(0, Result.Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE); 
        if(Result.Memory)
        {
            DWORD BytesRead;
            if(!ReadFile(FileHandle, Result.Memory, FileSize, &BytesRead, 0) ||
               BytesRead != FileSize)
            {
                VirtualFree(Result.Memory, 0, MEM_RELEASE);
                Result = {};
            }
            
            CloseHandle(FileHandle);
        }        
    }
    
    return Result;
}

PLATFORM_WRITE_FILE(PlatformWriteFile)
{
    b32 Result = false;
    
    HANDLE FileHandle = CreateFile(Filename,
                                   GENERIC_WRITE,
                                   0,
                                   0,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   0);
    
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        if(WriteFile(FileHandle, Memory, BytesToWrite, &BytesWritten, 0))
        {
            if(BytesWritten == BytesToWrite)
            {
                Result = true;
            }
            else
            {
                // NOTE: Failure
            }
        }
        else
        {
            // NOTE: Failure
        }
        CloseHandle(FileHandle);
    }
    
    return Result;
}

internal void
Win32LoadGameCodePaths(win32_game_code *GameCode)
{
    GetModuleFileName(0, GameCode->EXEFilePath, ArrayCount(GameCode->EXEFilePath));
    
    int LastSlashIndex = 0;
    for(int CharIndex = 0;
        CharIndex < ArrayCount(GameCode->EXEFilePath);
        CharIndex++)
    {        
        GameCode->GameCodePath[CharIndex] = GameCode->EXEFilePath[CharIndex];
        GameCode->TempGameCodePath[CharIndex] = GameCode->EXEFilePath[CharIndex];
        
        if(GameCode->EXEFilePath[CharIndex] == '\\')
        {
            LastSlashIndex = CharIndex;
        }
        
        if(!GameCode->EXEFilePath[CharIndex])
        {
            break;
        }
    }
    
    GameCode->GameCodePath[LastSlashIndex + 1] = 0;
    GameCode->TempGameCodePath[LastSlashIndex + 1] = 0;
    
    char *GameCodeFileName = "link.dll";
    char *CurChar = (GameCode->GameCodePath + LastSlashIndex + 1);
    while(*GameCodeFileName)
    {
        *CurChar++ = *GameCodeFileName++;
    }
    *CurChar = 0;
    
    char *TempGameCodeFileName = "link_temp.dll";
    CurChar = (GameCode->TempGameCodePath + LastSlashIndex + 1);
    while(*TempGameCodeFileName)
    {
        *CurChar++ = *TempGameCodeFileName++;
    }
    *CurChar = 0;
}

inline FILETIME
Win32GetLastWriteTime(char *Filename)
{
    WIN32_FIND_DATA FileData;
    HANDLE FileHandle = FindFirstFile(Filename, &FileData);
    FindClose(FileHandle);
    return FileData.ftLastWriteTime;
}

internal void 
Win32LoadGameCode(win32_game_code *GameCode)
{
    FILETIME LastWriteTime = Win32GetLastWriteTime(GameCode->GameCodePath);
    if(CompareFileTime(&LastWriteTime, &GameCode->LastLoadTime) != 0)
    {
        GameCode->LastLoadTime = LastWriteTime;
        
        if(GameCode->Library)
        {
            FreeLibrary(GameCode->Library);
            GameCode->Library = 0;
            GameCode->GameUpdateAndRender = GameUpdateAndRenderStub;
        }
        
        if(CopyFile(GameCode->GameCodePath,
                    GameCode->TempGameCodePath,
                    FALSE))
        {
            GameCode->Library = LoadLibrary(GameCode->TempGameCodePath);
            
            if(GameCode->Library)
            {
                GameCode->GameUpdateAndRender = (game_update_and_render *)
                    GetProcAddress(GameCode->Library, "GameUpdateAndRender");
            }
        }
    }
}

internal void
Win32ResizeScreenBuffer(win32_screen_buffer *Buffer, int Width, int Height)
{
    if(Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }
    
    Buffer->Info = {};
    Buffer->Info.bmiHeader.biSize = sizeof(BITMAPINFO);
    Buffer->Info.bmiHeader.biWidth = Width;
    Buffer->Info.bmiHeader.biHeight = -Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    
    Buffer->Width = Width;
    Buffer->Height = Height;
    
    int BytesPerPixel = 4;
    int MemorySize = BytesPerPixel * Width * Height;
    Buffer->Memory = VirtualAlloc(0, MemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

internal void
Win32RenderScreenBuffer(win32_screen_buffer *Buffer, HWND Window)
{
    RECT ClientRect;
    GetWindowRect(Window, &ClientRect);
    int WindowWidth = ClientRect.right - ClientRect.left;
    int WindowHeight = ClientRect.bottom - ClientRect.top;
    
    HDC DeviceContext = GetDC(Window);
    
    PatBlt(DeviceContext,
           Buffer->Width, 0, WindowWidth - Buffer->Width, WindowHeight,
           BLACKNESS);
    PatBlt(DeviceContext,
           0, Buffer->Height, WindowWidth, WindowHeight - Buffer->Height,
           BLACKNESS);
    
    StretchDIBits(DeviceContext,
                  0, 0, Buffer->Width, Buffer->Height,
                  0, 0, Buffer->Width, Buffer->Height,
                  Buffer->Memory, &Buffer->Info,
                  DIB_RGB_COLORS,
                  SRCCOPY);
    ReleaseDC(Window, DeviceContext);
}

LRESULT CALLBACK 
Win32MessageCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch(Message)
    {
        case WM_QUIT:
        case WM_CLOSE:
        case WM_DESTROY:
        {
            Win32State.Running = false;
        } break;
        default:
        {
            return DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}

internal void
Win32ProcessKeyboard(char Key, b32 IsDown, game_controller *Controller)
{    
    switch(Key)
    {
        case 'W':
        {
            //Controller->MoveUp.WasDown = Controller->MoveUp.IsDown;
            Controller->MoveUp.IsDown = IsDown;
        } break;
        case 'A':
        {
            //Controller->MoveLeft.WasDown = Controller->MoveUp.IsDown;
            Controller->MoveLeft.IsDown = IsDown;
        } break;
        case 'S':
        {
            //Controller->MoveDown.WasDown = Controller->MoveUp.IsDown;
            Controller->MoveDown.IsDown = IsDown;   
        } break;
        case 'D':
        {
            //Controller->MoveRight.WasDown = Controller->MoveUp.IsDown;
            Controller->MoveRight.IsDown = IsDown;
        } break;
        case VK_RIGHT:
        {
            //Controller->ActionRight.WasDown = Controller->MoveUp.IsDown;
            Controller->ActionRight.IsDown = IsDown;
        } break;
        case VK_UP:
        {
            //Controller->ActionUp.WasDown = Controller->MoveUp.IsDown;
            Controller->ActionUp.IsDown = IsDown;
        } break;
        case VK_LEFT:
        {
            //Controller->ActionLeft.WasDown = Controller->MoveUp.IsDown;
            Controller->ActionLeft.IsDown = IsDown;
        } break;
        case VK_DOWN:
        {
            //Controller->ActionDown.WasDown = Controller->MoveUp.IsDown;
            Controller->ActionDown.IsDown = IsDown;
        } break;
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x37:
        case 0x38:
        case 0x39:
        {
            int Number = (Key - 0x30);
            Controller->Numbers[Number].IsDown = IsDown;
        } break;
        case VK_ESCAPE:
        {
            Win32State.Running = false;
        } break;
    }
}

inline LARGE_INTEGER
Win32GetTime()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

inline r32
Win32SecondsBetween(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    return (r32)(End.QuadPart - Start.QuadPart) / (r32)Win32State.PerformanceFrequency.QuadPart;
}

internal void
Win32StartRecording(game_memory *GameMemory)
{
    Win32State.InputRecording.Recording = true;
    Win32State.InputRecording.Playing = false;
    
    Win32State.InputRecording.FileHandle = CreateFile("input.out",
                                                      GENERIC_READ | GENERIC_WRITE,
                                                      0,
                                                      0,
                                                      CREATE_ALWAYS,
                                                      FILE_ATTRIBUTE_NORMAL,
                                                      0);
    Win32State.InputRecording.PlaybackCount = 0;
    
    u32 TotalMemorySize = GameMemory->PermanentMemorySize + GameMemory->TempMemorySize;
    CopyMemory(Win32State.InputRecording.PlaybackGameState, GameMemory->PermanentMemory, TotalMemorySize);
}

internal void
Win32RecordInput(game_controller *Input)
{
    DWORD BytesWritten;
    b32 Success = WriteFile(Win32State.InputRecording.FileHandle, (void *)Input, sizeof(game_controller),
                            &BytesWritten, 0);
    Win32State.InputRecording.PlaybackCount++;
    Assert(BytesWritten == sizeof(game_controller));
    Assert(Success);
}

internal game_controller
Win32GetInput(game_memory *GameMemory)
{
    if(Win32State.InputRecording.PlaybackIndex == 0)
    {
        u32 TotalMemorySize = GameMemory->PermanentMemorySize + GameMemory->TempMemorySize;
        CopyMemory(GameMemory->PermanentMemory, Win32State.InputRecording.PlaybackGameState, TotalMemorySize);
    }
    
    game_controller Result = {};
    game_controller *Input = ((game_controller *)Win32State.InputRecording.PlaybackMemory +
                              Win32State.InputRecording.PlaybackIndex);
    if(++Win32State.InputRecording.PlaybackIndex >= Win32State.InputRecording.PlaybackCount)
    {
        Win32State.InputRecording.PlaybackIndex = 0;
    }
    
    Result = *Input;
    return Result;
}

#include <stdio.h>

internal void
Win32StopRecording()
{
    Win32State.InputRecording.Recording = false;
    
    if(Win32State.InputRecording.PlaybackMemory)
    {
        PlatformFreeFile(Win32State.InputRecording.PlaybackMemory);
    }
    
    CloseHandle(Win32State.InputRecording.FileHandle);
    file_contents InputRecordingFile = PlatformReadFile("input.out");
    Win32State.InputRecording.PlaybackMemory = InputRecordingFile.Memory;
    Win32State.InputRecording.PlaybackIndex = 0;
}

int CALLBACK 
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int CommandShow)
{    
    WNDCLASSEX WindowClass = {};
    WindowClass.cbSize = sizeof(WNDCLASSEX);
    WindowClass.style = CS_VREDRAW|CS_HREDRAW|CS_CLASSDC;
    WindowClass.lpfnWndProc = &Win32MessageCallback;
    WindowClass.hInstance = Instance;
    //WindowClass.hIcon = ???; // TODO(lex): Set window icon
    //WindowClass.hIconSm = ???;
    //WindowClass.hCursor = ???; // TODO(lex): Set cursor
    WindowClass.lpszClassName = "LinkClassName";
    
    
    if(RegisterClassEx(&WindowClass))
    {
        HWND Window = CreateWindowEx(0,
                                     WindowClass.lpszClassName,
                                     "Link Demo",
                                     WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     1000,
                                     600,
                                     0,
                                     0,
                                     Instance,
                                     0);        
        
        game_memory GameMemory = {};
        GameMemory.PermanentMemorySize = MegaBytes(64);
        GameMemory.TempMemorySize = MegaBytes(256);
        
        u32 TotalMemorySize = GameMemory.PermanentMemorySize + GameMemory.TempMemorySize;
        
        GameMemory.PermanentMemory = 
            VirtualAlloc(0, TotalMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        GameMemory.TempMemory = (u32 *)GameMemory.PermanentMemory + GameMemory.PermanentMemorySize;
        
        Win32State.InputRecording.PlaybackGameState = 
            VirtualAlloc(0, TotalMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        
        timeBeginPeriod(1);
        
        if(Window && GameMemory.PermanentMemory && GameMemory.TempMemory)
        {
            win32_screen_buffer Win32Screen = {};
            Win32ResizeScreenBuffer(&Win32Screen, 960, 540);
            
            game_controller GameKeyboard = {};
            
            win32_game_code GameCode = {};
            Win32LoadGameCodePaths(&GameCode);
            
            r32 FramesPerSecond = 30.0f;
            r32 SecondsPerFrame = (1.0f / FramesPerSecond);
            QueryPerformanceFrequency(&Win32State.PerformanceFrequency);

            platform_services PlatformServices = {};
            PlatformServices.FreeFile = &PlatformFreeFile;
            PlatformServices.ReadFile = &PlatformReadFile;
            PlatformServices.WriteFile = &PlatformWriteFile;
            
            Win32State.Running = true;
            while(Win32State.Running)
            {
                LARGE_INTEGER FrameStartTime = Win32GetTime();
                
                Win32LoadGameCode(&GameCode);
                
                GameKeyboard.MouseLeftDownEvent = false;
                GameKeyboard.MouseLeftUpEvent = false;
                GameKeyboard.MouseRightDownEvent = false;
                GameKeyboard.MouseRightUpEvent = false;

                for(int GameButtonIndex = 0;
                    GameButtonIndex < ArrayCount(GameKeyboard.Buttons);
                    GameButtonIndex++)
                {
                    game_button *Button = (GameKeyboard.Buttons + GameButtonIndex);
                    Button->WasDown = Button->IsDown;
                    Button->IsDown = false; // TODO: Fix keyboard code, it seems to skip release events!
                }
                
                for(int GameButtonIndex = 0;
                    GameButtonIndex < ArrayCount(GameKeyboard.Numbers);
                    GameButtonIndex++)
                {
                    game_button *Button = (GameKeyboard.Numbers + GameButtonIndex);
                    Button->WasDown = Button->IsDown;
                    Button->IsDown = false;
                }
                
                MSG Message;
                while(PeekMessage(&Message, Window, 0, 0, PM_REMOVE))
                {
                    if(Message.message == WM_LBUTTONDOWN)
                    {
                        GameKeyboard.MouseLeftDownEvent = true;
                        GameKeyboard.MouseX = GET_X_LPARAM(Message.lParam); 
                        GameKeyboard.MouseY = GET_Y_LPARAM(Message.lParam);
                    }
                    else if(Message.message == WM_LBUTTONUP)
                    {
                        GameKeyboard.MouseLeftUpEvent = true;
                        GameKeyboard.MouseX = GET_X_LPARAM(Message.lParam); 
                        GameKeyboard.MouseY = GET_Y_LPARAM(Message.lParam);
                    }
                    if(Message.message == WM_RBUTTONDOWN)
                    {
                        GameKeyboard.MouseRightDownEvent = true;
                        GameKeyboard.MouseX = GET_X_LPARAM(Message.lParam); 
                        GameKeyboard.MouseY = GET_Y_LPARAM(Message.lParam);
                    }
                    else if(Message.message == WM_RBUTTONUP)
                    {
                        GameKeyboard.MouseRightUpEvent = true;
                        GameKeyboard.MouseX = GET_X_LPARAM(Message.lParam); 
                        GameKeyboard.MouseY = GET_Y_LPARAM(Message.lParam);
                    }
                    if(Message.message == WM_KEYUP ||
                       Message.message == WM_KEYDOWN ||
                       Message.message == WM_SYSKEYDOWN ||
                       Message.message == WM_SYSKEYUP)
                    {
                        char Key = (char)Message.wParam;
                        b32 IsDown = ((Message.lParam & (1 << 31)) == 0);
                        
                        if(IsDown && (char)Message.wParam == 'L')
                        {
                            if(!Win32State.InputRecording.Recording &&
                               !Win32State.InputRecording.Playing)
                            {
                                SetWindowText(Window, "Link Demo - Recording");
                                Win32StartRecording(&GameMemory);
                            }
                            else if(Win32State.InputRecording.Recording)
                            {
                                SetWindowText(Window, "Link Demo - Playing");
                                Win32StopRecording();
                                Win32State.InputRecording.Playing = true;
                            }
                            else
                            {
                                SetWindowText(Window, "Link Demo");
                                Win32State.InputRecording.Playing = false;
                            }
                        }
                        else Win32ProcessKeyboard(Key, IsDown, &GameKeyboard);
                    }
                    else
                    {
                        // NOTE(lex): To process keyboard input
                        TranslateMessage(&Message);
                        // NOTE(lex): To send messages to Win32MessageCallback
                        DispatchMessage(&Message);
                    }
                }
                
                game_screen GameScreen;
                GameScreen.Memory = Win32Screen.Memory;
                GameScreen.Width = Win32Screen.Width;
                GameScreen.Height = Win32Screen.Height;
                
                if(!Win32State.InputRecording.Playing && !Win32State.InputRecording.Recording)
                {
                    GameCode.GameUpdateAndRender(&PlatformServices, &GameScreen, &GameMemory, &GameKeyboard, SecondsPerFrame);
                }
                else if(Win32State.InputRecording.Recording)
                {
                    Win32RecordInput(&GameKeyboard);
                    GameCode.GameUpdateAndRender(&PlatformServices, &GameScreen, &GameMemory, &GameKeyboard, SecondsPerFrame);
                }
                else if(Win32State.InputRecording.Playing)
                {
                    game_controller PlaybackKeyboard = Win32GetInput(&GameMemory);
                    GameCode.GameUpdateAndRender(&PlatformServices, &GameScreen, &GameMemory, &PlaybackKeyboard, SecondsPerFrame);
                }
                else
                {
                    InvalidCodePath;
                }
                                
                Win32RenderScreenBuffer(&Win32Screen, Window);
                
                LARGE_INTEGER FrameEndTime = Win32GetTime();
                                
                if(Win32SecondsBetween(FrameStartTime, FrameEndTime) <= SecondsPerFrame)
                {
                    r32 SecondsLeft = SecondsPerFrame - Win32SecondsBetween(FrameStartTime, Win32GetTime());
                    Sleep((int)(1000*SecondsLeft));
#if 0
                    while(Win32SecondsBetween(FrameStartTime, Win32GetTime()) < SecondsPerFrame);
#endif
                }
                else
                {
                    OutputDebugStringA("MISSED A FRAME\n");
                }
            }
        }
    }
}