#pragma once

#include "CoreMinimal.h"

/**
 * Walkability grid of the procedural compound, used by the enemies to plan their routes.
 *
 * The compound is spawned at runtime on an empty map, so there is no baked NavMesh to query.
 * The GameMode registers every colliding shape it spawns; footprints that stand in the body
 * band of a walking enemy are inflated by the capsule radius and rasterised into the grid.
 * Paths come from A* (8 neighbours, no corner cutting) followed by line-of-sight smoothing.
 * Doors are not obstacles: the enemies open them on their way.
 */
class FURFPSNavGrid
{
public:
    void Initialize(float InHalfSize, float InCellSize, float InAgentRadius, float InGroundZ);
    void AddBox(const FVector& Center, const FVector& HalfExtents, const FRotator& Rotation);
    void AddCylinder(const FVector& Center, float Radius, float HalfHeight);
    void Finalize();

    bool IsReady() const { return bReady; }
    bool IsWalkable(const FVector& Location) const;
    bool HasClearLine(const FVector& From, const FVector& To) const;
    bool ProjectToWalkable(const FVector& Location, float SearchRadius, FVector& OutLocation) const;
    bool FindPath(const FVector& Start, const FVector& Goal, TArray<FVector>& OutWaypoints) const;
    // Random walkable point within Radius of Center that can be reached from Origin.
    bool FindRandomPointNear(const FVector& Origin, const FVector& Center, float Radius, FVector& OutLocation) const;
    int32 GetWalkableCellCount() const;

private:
    struct FOpenEntry
    {
        float Cost = 0.f;
        int32 Cell = INDEX_NONE;
    };

    int32 CellIndex(int32 X, int32 Y) const { return Y * Width + X; }
    bool IsInside(int32 X, int32 Y) const { return X >= 0 && Y >= 0 && X < Width && Y < Height; }
    bool IsFreeCell(int32 X, int32 Y) const { return IsInside(X, Y) && Blocked[CellIndex(X, Y)] == 0; }
    int32 WorldToCellX(double WorldX) const;
    int32 WorldToCellY(double WorldY) const;
    FVector CellCenter(int32 Cell, double Z) const;
    bool IsBodyBandObstacle(double MinZ, double MaxZ) const;
    void RasterizeConvex(const TArray<FVector2D>& Hull);
    int32 FindNearestFreeCell(int32 X, int32 Y, int32 MaxRadius, int32 RequiredComponent) const;
    bool IsLineClear(int32 X0, int32 Y0, int32 X1, int32 Y1) const;
    void PushOpen(float Cost, int32 Cell) const;
    int32 PopOpen() const;

    TArray<uint8> Blocked;
    TArray<int32> Component;
    int32 Width = 0;
    int32 Height = 0;
    float CellSize = 25.f;
    float AgentRadius = 42.f;
    double OriginX = 0.0;
    double OriginY = 0.0;
    double GroundZ = -100.0;
    double StepHeight = 40.0;
    double ClearanceHeight = 185.0;
    bool bReady = false;

    // A* scratch buffers, reused between queries (game thread only).
    mutable TArray<float> PathCost;
    mutable TArray<int32> PathParent;
    mutable TArray<uint32> SeenStamp;
    mutable TArray<uint32> ClosedStamp;
    mutable TArray<FOpenEntry> OpenHeap;
    mutable int32 OpenCount = 0;
    mutable uint32 QueryStamp = 0;
};
