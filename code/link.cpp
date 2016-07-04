#include "link.h"

// TODO(lex): Custom file format. Could write a converter that run into platform layer on startup and bake 
// stb_image into it.
internal bitmap
LoadImage(char *FileName)
{
    bitmap Result = {};
    int Components;
    Result.Memory = (u32 *)stbi_load(FileName, &Result.Width, &Result.Height, &Components, 0);
    return Result;
}

internal bitmap
SubBitmap(memory_arena *Arena, bitmap *ParentBitmap, int MinX, int MinY, int Width, int Height)
{
    bitmap Result = {};
    
    int NumPixels = Width*Height;
    Result.Memory = PushArray(u32, NumPixels, Arena);
    Result.Width = Width;
    Result.Height = Height;
    
    for(int DestY = 0;
        DestY < Height;
        DestY++)
    {
        for(int DestX = 0;
            DestX < Width;
            DestX++)
        {
            int SourceX = MinX + DestX;
            int SourceY = MinY + DestY;
            
            u32 SourceValue = *(ParentBitmap->Memory + (ParentBitmap->Width*SourceY) + SourceX);
            *(Result.Memory + (Result.Width*DestY) + DestX) = SourceValue;
        }
    }
    
    return Result;
}

internal bitmap
LoadBitmap_(platform_read_file *ReadFile, char *FileName)
{
    // NOTE: AA RR GG BB
    bitmap Result = {};
    file_contents ReadResult = ReadFile(FileName);
    if(ReadResult.Size != 0)
    {
        bitmap_header *Header = (bitmap_header *)ReadResult.Memory;
        u32 *Pixels = (u32 *)((u8 *)ReadResult.Memory + Header->BitmapOffset);
        
        u32 RedMask = Header->RedMask;
        u32 GreenMask = Header->GreenMask;
        u32 BlueMask = Header->BlueMask;
        u32 AlphaMask = ~(RedMask | GreenMask | BlueMask);
        
        bit_scan_result RedShift = FindLeastSignificantSetBit(RedMask);
        bit_scan_result GreenShift = FindLeastSignificantSetBit(GreenMask);
        bit_scan_result BlueShift = FindLeastSignificantSetBit(BlueMask);
        bit_scan_result AlphaShift = FindLeastSignificantSetBit(AlphaMask);
        
        Assert(RedShift.Found);
        Assert(GreenShift.Found);
        Assert(BlueShift.Found);
        Assert(AlphaShift.Found);
        
        u32 *Pixel = Pixels;
        for(int Y = 0; Y < Header->Height; Y++)
        {
            for(int X = 0; X < Header->Width; X++)
            {
                u32 Color = *Pixel;
                *Pixel++ = ((((Color >> AlphaShift.Index) & 0xFF) << 24) |
                            (((Color >> RedShift.Index) & 0xFF) << 16) |
                            (((Color >> GreenShift.Index) & 0xFF) << 8) |
                            (((Color >> BlueShift.Index) & 0xFF) << 0));
            }
        }
        
        // NOTE(lex): Flip pixels
        
        u32 *TopRow = Pixels;
        u32 *BottomRow = Pixels + (Header->Height - 1)*Header->Width;
        
        while(TopRow < BottomRow)
        {
            u32 *TopPixel = TopRow;
            u32 *BottomPixel = BottomRow;
            for(int X = 0; X < Header->Width; X++)
            {
                u32 Temp = *TopPixel;
                *TopPixel = *BottomPixel;
                *BottomPixel = Temp;
                TopPixel++;
                BottomPixel++;
            }
            TopRow += Header->Width;
            BottomRow -= Header->Width;
        }
        
        Result.Width = Header->Width;
        Result.Height = Header->Height;
        Result.Memory = Pixels;
    }
    return Result;
}
#define LoadBitmap(FileName) LoadBitmap_(Platform->ReadFile, FileName)

internal void
DrawBitmap(game_screen *Screen, bitmap *Bitmap, v2 P, v2 HotSpot = V2(0, 0))
{
    P.Y = (Screen->Height - P.Y);
    
    int MinX = RoundReal32ToInt32(P.X - HotSpot.X);
    int MinY = RoundReal32ToInt32(P.Y - HotSpot.Y);
    int MaxX = MinX + Bitmap->Width;
    int MaxY = MinY + Bitmap->Height;
    
    int OutOfBoundsOffsetX = 0;
    int OutOfBoundsOffsetY = 0;
    
    if(MinX < 0)
    {
        OutOfBoundsOffsetX = -MinX;
        MinX = 0;
    }
    if(MinY < 0)
    {
        OutOfBoundsOffsetY = -MinY;
        MinY = 0;
    }
    if(MaxX >= Screen->Width)
    {
        MaxX = (Screen->Width - 1);
    }
    if(MaxY >= Screen->Height)
    {
        MaxY = (Screen->Height - 1);
    }
    
    u32 *ScreenRowPixel = (u32 *)Screen->Memory + (Screen->Width*MinY) + MinX;
    u32 *BitmapRowPixel = (u32 *)Bitmap->Memory + (Bitmap->Width*OutOfBoundsOffsetY) + OutOfBoundsOffsetX;
    
    int DrawHeight = (MaxY - MinY);
    int DrawWidth = (MaxX - MinX);
    
    for(int Y = 0; Y < DrawHeight; Y++)
    {
        u32 *ScreenPixel = ScreenRowPixel;
        u32 *BitmapPixel = BitmapRowPixel;
        for(int X = 0; X < DrawWidth; X++)
        {
            u32 RedMask = 0x00FF0000;
            u32 GreenMask = 0x0000FF00;
            u32 BlueMask = 0x000000FF;
            u32 AlphaMask = 0xFF000000;
            
            u8 BitmapA = ((*BitmapPixel & AlphaMask) >> 24);            
            r32 Alpha = (r32)BitmapA / 255.0f;
            
            u8 ScreenR = (u8)(*ScreenPixel & 0x000000FF);
            u8 ScreenG = (u8)((*ScreenPixel & 0x0000FF00) >> 8);
            u8 ScreenB = (u8)((*ScreenPixel & 0x00FF0000) >> 16);
            
            u8 BitmapR = (u8)(*BitmapPixel & 0x000000FF);
            u8 BitmapG = (u8)((*BitmapPixel & 0x0000FF00) >> 8);
            u8 BitmapB = (u8)((*BitmapPixel & 0x00FF0000) >> 16);
            
            u8 ResultR = (u8)(Alpha*BitmapR + (1.0f - Alpha)*ScreenR);
            u8 ResultG = (u8)(Alpha*BitmapG + (1.0f - Alpha)*ScreenG);
            u8 ResultB = (u8)(Alpha*BitmapB + (1.0f - Alpha)*ScreenB);
            
            u32 Result = ((ResultB << 16) | ((ResultG << 8) | ResultR));
            
            *ScreenPixel++ = Result;
            BitmapPixel++;
        }
        ScreenRowPixel += Screen->Width;
        BitmapRowPixel += Bitmap->Width;
    }
}

inline u32
GetPixel(bitmap *Bitmap, int X, int Y)
{
    u32 Result = *(Bitmap->Memory + (Y * Bitmap->Width) + X);
    return Result;
}

inline void
SetPixel(bitmap *Bitmap, int X, int Y, u32 Value)
{
    u32 *Pixel = (Bitmap->Memory + (Y * Bitmap->Width) + X);
    *Pixel = Value;
}

// NOTE: In place rotation, but only works for square bitmaps 
// TODO: Allow non-square bitmap rotation?
internal void
RotateBitmapClockwise(bitmap *Bitmap)
{
    Assert(Bitmap->Width == Bitmap->Height);
    
    int SideDim = Bitmap->Width;
    
    for(int Layer = 0;
        Layer < SideDim/2;
        Layer++)
    {
        int LayerSideDim = SideDim - (2*Layer);
        
        int MinIndex = Layer;
        int MaxIndex = MinIndex + LayerSideDim - 1;
        
        for(int Offset = 0;
            Offset < LayerSideDim - 1;
            Offset++)
        {
            u32 Top = GetPixel(Bitmap, MinIndex + Offset, MinIndex);
            u32 Right = GetPixel(Bitmap, MaxIndex, MinIndex + Offset);
            u32 Down = GetPixel(Bitmap, MaxIndex - Offset, MaxIndex);
            u32 Left = GetPixel(Bitmap, MinIndex, MaxIndex - Offset);
            
            SetPixel(Bitmap, MinIndex + Offset, MinIndex, Left);
            SetPixel(Bitmap, MaxIndex, MinIndex + Offset, Top);
            SetPixel(Bitmap, MaxIndex - Offset, MaxIndex, Right);
            SetPixel(Bitmap, MinIndex, MaxIndex - Offset, Down);
        }
    }
}

inline void 
ClearScreen(game_screen *Screen, u32 Color)
{
    for(i32 Y = 0; Y < Screen->Height; Y++)
    {
        for(i32 X = 0; X < Screen->Width; X++)
        {
            u32 *Pixel = (u32 *)Screen->Memory + (Y * Screen->Width) + X;
            *Pixel = Color;
        }
    }
}

inline b32
IsTileInBounds(level *Level, int X, int Y)
{
    b32 Result = ((X >= 0) && (Y >= 0) && (X < Level->Width) && (Y < Level->Height));
    return Result;
}

internal int
GetTile(level *Level, int X, int Y)
{
    // TODO: Do we want the map to actually be indexed from the bottom right corner?
    Y = (Level->Height - Y - 1);
     
    int Result;
    int Index = (Level->Width*Y) + X;
    if(IsTileInBounds(Level, X, Y))
    {
        Result = *(Level->Tiles + Index);
    }
    else
    {
        Result = TileType_Linked;
    }
    return Result;
}

inline b32
IsTileLinked(level *Level, int X, int Y)
{
    b32 Result = false;
    
    if((X >= 0) && (Y >= 0) && (X < Level->Width) && (Y < Level->Height))
    {
        Result = (GetTile(Level, X, Y) == TileType_Linked);
    }
    
    return Result;
}

internal void
SetTile(level *Level, int X, int Y, int Value)
{
    Y = (Level->Height - Y - 1);
    
    int Index = (Level->Width*Y) + X;
    Assert((Index >= 0) && (Index < (Level->Width*Level->Height)));
    *(Level->Tiles + Index) = Value;
}

internal v2
GetMouseP(game_screen *Screen, game_controller *Controller)
{
    v2 Result;
    
    Result.X = (r32)Controller->MouseX;
    Result.Y = (r32)(Screen->Height - Controller->MouseY - 1);
    
    return Result;
}

internal void
LinkConnected(level *Level, int TileX, int TileY, b32 InitialCall = 1)
{
    int CenteredTileType = GetTile(Level, TileX, TileY);
    
    int RightLinked = (GetTile(Level, TileX+1, TileY) == CenteredTileType);
    if(RightLinked)
    {
        SetTile(Level, TileX, TileY, TileType_Linked);
        LinkConnected(Level, TileX+1, TileY, 0);
    }
    
    int UpLinked = (GetTile(Level, TileX, TileY+1) == CenteredTileType);
    if(UpLinked)
    {
        SetTile(Level, TileX, TileY, TileType_Linked);
        LinkConnected(Level, TileX, TileY+1, 0);
    }
    
    int LeftLinked = (GetTile(Level, TileX-1, TileY) == CenteredTileType);
    if(LeftLinked)
    {
        SetTile(Level, TileX, TileY, TileType_Linked);
        LinkConnected(Level, TileX-1, TileY, 0);
    }
    
    int DownLinked = (GetTile(Level, TileX, TileY-1) == CenteredTileType);
    if(DownLinked)
    {
        SetTile(Level, TileX, TileY, TileType_Linked);
        LinkConnected(Level, TileX, TileY-1, 0);
    }
    
    if(!RightLinked && !UpLinked && !LeftLinked && ! DownLinked)
    {
        if(!InitialCall)
        {
            SetTile(Level, TileX, TileY, TileType_Linked);
        }
    }
}

inline b32
LevelCompleted(level *Level)
{
    for(int TileY = 0;
        TileY < Level->Height;
        TileY++)
    {
        for(int TileX = 0;
            TileX < Level->Width;
            TileX++)
        {
            int TileValue = *(Level->Tiles + (Level->Width * TileY) + TileX);
            if((TileValue != TileType_Empty) && (TileValue != TileType_Linked))
            {
                return false;
            }
        }
    }
    
    return true;
}

internal void
PushTile(level *Level, int TileX, int TileY, v2 Direction)
{
    if(GetTile(Level, TileX, TileY) > TileType_Linked)
    {
        int dTileX = RoundReal32ToInt32(Direction.X);
        int dTileY = RoundReal32ToInt32(Direction.Y);
        Assert((AbsoluteValue(dTileX) + AbsoluteValue(dTileY)) == 1);
        
        int NewTileX = TileX;
        int NewTileY = TileY;
        
        while(GetTile(Level, NewTileX + dTileX, NewTileY + dTileY) == TileType_Empty)
        {
            NewTileX += dTileX;
            NewTileY += dTileY;
        }

        if((NewTileX != TileX) || (NewTileY != TileY))
        {
            int TileType = GetTile(Level, TileX, TileY);
            SetTile(Level, TileX, TileY, TileType_Empty);
            SetTile(Level, NewTileX, NewTileY, TileType);
            
            LinkConnected(Level, NewTileX, NewTileY);
        }
    }
}

internal level
BuildLevelFromArray(memory_arena *LevelArena, int *LevelData, int Width, int Height)
{
    level Result = {};
    int NumTiles = (Width*Height);
    Result.Tiles = PushArray(int, NumTiles, LevelArena);
    
    int *SourceTile = LevelData;
    int *DestTile = Result.Tiles;
    for(int TileIndex = 0;
        TileIndex < NumTiles;
        TileIndex++)
    {
        *DestTile++ = *SourceTile++;
    }
    
    Result.Width = Width;
    Result.Height = Height;
    return Result;
}

internal void
CopyLevel(level *Source, level *Dest)
{
    Assert(Source->Width == Dest->Width);
    Assert(Source->Height == Dest->Height);
    
    int NumTiles = (Dest->Width*Dest->Height);
    for(int TileIndex = 0;
        TileIndex < NumTiles;
        TileIndex++)
    {
        Dest->Tiles[TileIndex] = Source->Tiles[TileIndex];
    }
}

internal b32
AreLevelsIdentical(level *A, level *B)
{
    b32 Result = ((A->Width == B->Width) && (A->Height == B->Height));
    
    if(Result)
    {
        int NumTiles = A->Width*A->Height;
        Result = MemoryBlocksEqual((u8 *)A->Tiles, (u8 *)B->Tiles, sizeof(u32)*NumTiles);
    }
    
    return Result;
}

inline int
CanonicalizeHistoryIndex(game_state *GameState, int Index)
{
    int Result = Index;
    if(Index < 0)
    {
        Result = ArrayCount(GameState->LevelHistory) - Index;
    }
    Result &= (ArrayCount(GameState->LevelHistory) - 1);
    return Result;
}

internal void
SaveLevelToHistory(game_state *GameState)
{
    int NewHistoryIndex = GameState->HistoryLastInsertedIndex - GameState->UndoOffset;
    GameState->HistoryLastInsertedIndex = CanonicalizeHistoryIndex(GameState, NewHistoryIndex);
    GameState->LevelHistoryCount -= GameState->UndoOffset;
    GameState->UndoOffset = 0;
    
    int HistoryInsertIndex = CanonicalizeHistoryIndex(GameState, GameState->HistoryLastInsertedIndex + 1);
    CopyLevel(&GameState->LevelAtFrameStart, GameState->LevelHistory + HistoryInsertIndex);
    GameState->HistoryLastInsertedIndex = HistoryInsertIndex;
    
    if(GameState->LevelHistoryCount < ArrayCount(GameState->LevelHistory))
    {
        GameState->LevelHistoryCount++;
    }
}

internal void
UndoFromLevelHistory(game_state *GameState)
{
    if((GameState->LevelHistoryCount - GameState->UndoOffset) > 0)
    {
        if(GameState->UndoOffset == 0)
        {
            SaveLevelToHistory(GameState);
        }
        
        GameState->UndoOffset++;
        int UndoIndex = CanonicalizeHistoryIndex(GameState, GameState->HistoryLastInsertedIndex - GameState->UndoOffset);
        CopyLevel(GameState->LevelHistory + UndoIndex, &GameState->Level);
    }
}

internal void
RedoFromLevelHistory(game_state *GameState)
{
    if(GameState->UndoOffset > 0)
    {
        GameState->UndoOffset--;
        int RedoIndex = CanonicalizeHistoryIndex(GameState, GameState->HistoryLastInsertedIndex - GameState->UndoOffset);
        CopyLevel(GameState->LevelHistory + RedoIndex, &GameState->Level);
    }
}

internal int
NextNumber(char **Start, char *LastChar)
{
    int Result = 0;
    if(*Start <= LastChar)
    {
        while((**Start == ' ') || (**Start == '\n') || (**Start == '\r'))
        {
            (*Start)++;
        }
        
        char *End = *Start;
        while((*End != ' ') && (*End != '\n') && (End < LastChar))
        {
            End++;
        }
        
        if(*End == ' ')
        {
            End--;
        }
        if(*End == '\n')
        {
            End -= 2;
        }
        
        char NumberAsString[64] = {};
        int NumberIndex = 0;
        
        for(char *Current = *Start;
            Current <= End;
            NumberIndex++, Current++)
        {
            NumberAsString[NumberIndex] = *Current;
        }
        Assert(NumberIndex < ArrayCount(NumberAsString));
        
        *Start = End + 1;
        Result = StringToInt(NumberAsString);
    }
    else
    {
        InvalidCodePath;
    }
    
    return Result;
}

internal level
InitCopiedLevel(memory_arena *LevelArena, level *Level)
{
    level Result = {};
    
    int NumTiles = Level->Width*Level->Height;
    Result.Tiles = PushArray(int, NumTiles, LevelArena);
    Result.Width = Level->Width;
    Result.Height = Level->Height;
    
    for(int TileIndex = 0;
        TileIndex < NumTiles;
        TileIndex++)
    {
        Result.Tiles[TileIndex] = Level->Tiles[TileIndex];
    }
    
    return Result;
}

internal void
LoadLevel(game_state *GameState, platform_read_file *ReadFile, platform_free_file *FreeFile, int LevelId)
{
    memory_arena *LevelArena = &GameState->LevelArena;
    LevelArena->Used = 0;
    
    level Result = {};
    
    char LevelPath[64] = "levels/";
    char LevelPathSuffix[64] = ".level";
    
    int LevelPathLength = StringLength(LevelPath);
    IntToString(LevelId, LevelPath + LevelPathLength);
    
    LevelPathLength = StringLength(LevelPath);
    CopySubString(LevelPath + LevelPathLength, LevelPathSuffix);
    
    file_contents FileHandle = ReadFile(LevelPath);
    
    char *FileData = (char *)FileHandle.Memory;
    char *LastChar = FileData + FileHandle.Size - 1;
    
    Result.Width  = NextNumber(&FileData, LastChar);
    Result.Height = NextNumber(&FileData, LastChar);
    int NumTiles = Result.Width*Result.Height;
    
    Result.Tiles = PushArray(int, NumTiles, LevelArena);
    
    for(int TileIndex = 0;
        TileIndex < NumTiles;
        TileIndex++)
    {
        int TileValue = NextNumber(&FileData, LastChar);
        Result.Tiles[TileIndex] = TileValue;
    }
    
    FreeFile(FileHandle.Memory);
    
    GameState->Level = Result;
    
    for(int LevelIndex = 0;
        LevelIndex < ArrayCount(GameState->LevelHistory);
        LevelIndex++)
    {
        GameState->LevelHistory[LevelIndex] = InitCopiedLevel(&GameState->LevelArena, &GameState->Level);
    }
    
    GameState->LevelAtFrameStart = InitCopiedLevel(&GameState->LevelArena, &GameState->Level);
    GameState->HistoryLastInsertedIndex = 0;
    GameState->LevelHistoryCount = 0;
    GameState->UndoOffset = 0;
}

inline void
ClearLevel(level *Level)
{
    int NumTiles = Level->Width*Level->Height;
    for(int TileIndex = 0;
        TileIndex < NumTiles;
        TileIndex++)
    {
        *(Level->Tiles + TileIndex) = 0;
    }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    game_state *GameState = (game_state *)Memory->PermanentMemory;
    if(!GameState->Initialized)
    {
        GameState->Initialized = true;
        GameState->Arena = InitMemoryArena((u8 *)Memory->PermanentMemory + sizeof(game_state),
                                           Memory->PermanentMemorySize - sizeof(game_state));
        GameState->LevelArena = InitMemoryArena((u8 *)Memory->TempMemory, Memory->TempMemorySize);
        GameState->TileBitmaps.Empty = LoadBitmap("tile_empty.bmp");
        GameState->TileBitmaps.Red = LoadBitmap("tile_red.bmp");
        GameState->TileBitmaps.Green = LoadBitmap("tile_green.bmp");
        GameState->TileBitmaps.Blue = LoadBitmap("tile_blue.bmp");
        
        GameState->SpriteSheet = LoadImage("tiles_linked.png");
        // NOTE:                     [r][u][l][d]
        GameState->TileBitmaps.Linked[0][0][0][0] = SubBitmap(&GameState->Arena, &GameState->SpriteSheet, 1*60, 1*60, 60, 60);
        GameState->TileBitmaps.Linked[1][0][0][0] = SubBitmap(&GameState->Arena, &GameState->SpriteSheet, 3*60, 2*60, 60, 60);
        GameState->TileBitmaps.Linked[0][1][0][0] = SubBitmap(&GameState->Arena, &GameState->SpriteSheet, 7*60, 3*60, 60, 60);
        GameState->TileBitmaps.Linked[0][0][1][0] = SubBitmap(&GameState->Arena, &GameState->SpriteSheet, 5*60, 2*60, 60, 60);
        GameState->TileBitmaps.Linked[0][0][0][1] = SubBitmap(&GameState->Arena, &GameState->SpriteSheet, 7*60, 1*60, 60, 60);
        GameState->TileBitmaps.Linked[1][1][0][0] = SubBitmap(&GameState->Arena, &GameState->SpriteSheet, 11*60, 3*60, 60, 60);
        GameState->TileBitmaps.Linked[1][0][1][0] = SubBitmap(&GameState->Arena, &GameState->SpriteSheet, 4*60, 2*60, 60, 60);
        GameState->TileBitmaps.Linked[1][0][0][1] = SubBitmap(&GameState->Arena, &GameState->SpriteSheet, 11*60, 1*60, 60, 60);
        GameState->TileBitmaps.Linked[0][1][1][0] = SubBitmap(&GameState->Arena, &GameState->SpriteSheet, 13*60, 3*60, 60, 60);
        GameState->TileBitmaps.Linked[0][1][0][1] = SubBitmap(&GameState->Arena, &GameState->SpriteSheet, 7*60, 2*60, 60, 60);
        GameState->TileBitmaps.Linked[0][0][1][1] = SubBitmap(&GameState->Arena, &GameState->SpriteSheet, 13*60, 1*60, 60, 60);
        GameState->TileBitmaps.Linked[1][1][1][0] = SubBitmap(&GameState->Arena, &GameState->SpriteSheet, 3*60, 5*60, 60, 60);
        GameState->TileBitmaps.Linked[1][1][0][1] = SubBitmap(&GameState->Arena, &GameState->SpriteSheet, 3*60, 8*60, 60, 60);
        GameState->TileBitmaps.Linked[1][0][1][1] = SubBitmap(&GameState->Arena, &GameState->SpriteSheet, 11*60, 8*60, 60, 60);
        GameState->TileBitmaps.Linked[0][1][1][1] = SubBitmap(&GameState->Arena, &GameState->SpriteSheet, 8*60, 8*60, 60, 60);
        GameState->TileBitmaps.Linked[1][1][1][1] = SubBitmap(&GameState->Arena, &GameState->SpriteSheet, 4*60, 12*60, 60, 60);
        
        GameState->TileBitmaps.LinkedLeftSide = LoadImage("tile_left_cover.png");
        GameState->TileBitmaps.LinkedRightSide = LoadImage("tile_right_cover.png");
        
        GameState->TileSideInPixels = 60.0f;
        
        GameState->CurrentLevelId = 1;
        GameState->MaxLevelId = 11;
        LoadLevel(GameState, Platform->ReadFile, Platform->FreeFile, GameState->CurrentLevelId);
    }
    
    if(LevelCompleted(&GameState->Level) && (GameState->CurrentLevelId < GameState->MaxLevelId))
    {
        if(!GameState->NextLevelTimer.Set)
        {
            r32 SecondsBetweenLevels = 1.0f;
            SetTimer(&GameState->NextLevelTimer, SecondsBetweenLevels);
        }
        else if(IsTimerDone(&GameState->NextLevelTimer))
        {
            GameState->CurrentLevelId++;
            LoadLevel(GameState, Platform->ReadFile, Platform->FreeFile, GameState->CurrentLevelId);
        }
        else
        {
            TickTimer(&GameState->NextLevelTimer, dT);
        }
    }
    
    for(int ButtonIndex = 1;
        ButtonIndex < (TileType_Count + 1);
        ButtonIndex++)
    {
        if(PressedOnce(Controller->Numbers[ButtonIndex]))
        {
            GameState->SelectedTileType = (ButtonIndex - 1);
        }
    }
    
    level Level = GameState->Level;
    level LevelAtFrameStart = GameState->LevelAtFrameStart;
    CopyLevel(&Level, &LevelAtFrameStart);
    
    b32 DidUndoRedo = false;
    if(PressedOnce(Controller->ActionLeft))
    {
        UndoFromLevelHistory(GameState);
        DidUndoRedo = true;
    }
    else if(PressedOnce(Controller->ActionRight))
    {
        RedoFromLevelHistory(GameState);
        DidUndoRedo = true;
    }
    
    v2 ScreenCenter = V2(0.5f*Screen->Width, 0.5f*Screen->Height);
    // TODO: Buggy with off by one pixel?
    v2 LevelDrawMin = ScreenCenter - V2(0.5f*Level.Width*GameState->TileSideInPixels, 
                                        0.5f*Level.Height*GameState->TileSideInPixels);
    v2 LevelDrawMax = ScreenCenter + V2(0.5f*Level.Width*GameState->TileSideInPixels,
                                        0.5f*Level.Height*GameState->TileSideInPixels);
    rect2 LevelDrawRegion = Rect2(LevelDrawMin, LevelDrawMax);
    
    if(Controller->MouseLeftDownEvent)
    {
        GameState->MouseDownP = GetMouseP(Screen, Controller);
    }
    else if(Controller->MouseLeftUpEvent)
    {
        if(IsPointInRect(GameState->MouseDownP, LevelDrawRegion))
        {
            v2 MouseRelP = (GameState->MouseDownP - LevelDrawRegion.Min);
            int TileX = (int)(MouseRelP.X/GameState->TileSideInPixels);
            int TileY = (int)(MouseRelP.Y/GameState->TileSideInPixels);
            
            v2 MouseP = GetMouseP(Screen, Controller);
            v2 Delta = (MouseP - GameState->MouseDownP);
            v2 Direction = Delta = GetMostSignfiicantCardinalDirection(Delta);
            
            PushTile(&Level, TileX, TileY, Direction);
        }
    }
    else if(Controller->MouseRightDownEvent)
    {
        v2 MouseP = GetMouseP(Screen, Controller);
        if(IsPointInRect(MouseP, LevelDrawRegion))
        {
            v2 MouseRelP = (MouseP - LevelDrawRegion.Min);
            int TileX = (int)(MouseRelP.X/GameState->TileSideInPixels);
            int TileY = (int)(MouseRelP.Y/GameState->TileSideInPixels);
            
            SetTile(&Level, TileX, TileY, GameState->SelectedTileType);
        }
    }
    
    if(!DidUndoRedo && !AreLevelsIdentical(&LevelAtFrameStart, &Level))
    {
        SaveLevelToHistory(GameState);
    }
    
    ClearScreen(Screen, 0x968246);
    
    for(int TileY = 0; TileY < Level.Height; TileY++)
    {
        for(int TileX = 0; TileX < Level.Width; TileX++)
        {
            int TileValue = GetTile(&Level, TileX, TileY);
            
            bitmap *TileBitmap = 0;
            b32 LeftSide = false;
            b32 RightSide = false;
            
            switch(TileValue)
            {
                case TileType_Empty:
                {
                    TileBitmap = &GameState->TileBitmaps.Empty;
                } break;

                case TileType_Linked:
                {
                    b32 Right = IsTileLinked(&Level, TileX + 1, TileY);
                    b32 Up = IsTileLinked(&Level, TileX, TileY + 1);
                    b32 Left = IsTileLinked(&Level, TileX - 1, TileY);
                    b32 Down = IsTileLinked(&Level, TileX, TileY - 1);
                    b32 LeftDown = IsTileLinked(&Level, TileX - 1, TileY - 1);
                    b32 RightDown = IsTileLinked(&Level, TileX + 1, TileY - 1);
                    
                    if(Left && Down && LeftDown)
                    {
                        LeftSide = true;
                    }
                    
                    if(Right && Down && RightDown)
                    {
                        RightSide = true;
                    }
                    
                    TileBitmap = &GameState->TileBitmaps.Linked[Right][Up][Left][Down];
                } break;
                
                case TileType_Red:
                {
                    TileBitmap = &GameState->TileBitmaps.Red;
                } break;
                
                case TileType_Green:
                {
                    TileBitmap = &GameState->TileBitmaps.Green;
                } break;

#if 0
                case TileType_Blue:
                {
                    TileBitmap = &GameState->TileBitmaps.Blue;
                } break;
#endif
                default:
                {
                    InvalidCodePath;
                }
            }
            
            v2 TileOffset = GameState->TileSideInPixels*V2((r32)TileX, (r32)TileY);
            v2 TileP = LevelDrawMin + TileOffset;
            DrawBitmap(Screen, &GameState->TileBitmaps.Empty, TileP, V2(0, GameState->TileSideInPixels));
            DrawBitmap(Screen, TileBitmap, TileP, V2(0, GameState->TileSideInPixels));
            
            if(LeftSide)
            {
                DrawBitmap(Screen, &GameState->TileBitmaps.LinkedLeftSide, TileP, V2(0, GameState->TileSideInPixels));
            }
            if(RightSide)
            {
                DrawBitmap(Screen, &GameState->TileBitmaps.LinkedRightSide, TileP, V2(0, GameState->TileSideInPixels));
            }
        }
    }
}