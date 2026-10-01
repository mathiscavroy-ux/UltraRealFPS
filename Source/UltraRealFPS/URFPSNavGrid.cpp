#include "URFPSNavGrid.h"

namespace
{
    constexpr float DiagonalStepCost = 1.41421356f;

    double Cross2D(const FVector2D& O, const FVector2D& A, const FVector2D& B)
    {
        return (A.X - O.X) * (B.Y - O.Y) - (A.Y - O.Y) * (B.X - O.X);
    }

    // Andrew's monotone chain. The result is counter-clockwise; it can degenerate to a segment
    // or a point for flat shapes, which the rasteriser handles through edge distances.
    void BuildConvexHull(TArray<FVector2D> Points, TArray<FVector2D>& OutHull)
    {
        // Insertion sort: at most eight corners.
        for (int32 I = 1; I < Points.Num(); ++I)
        {
            const FVector2D Key = Points[I];
            int32 J = I - 1;
            while (J >= 0 && (Points[J].X > Key.X || (Points[J].X == Key.X && Points[J].Y > Key.Y)))
            {
                Points[J + 1] = Points[J];
                --J;
            }
            Points[J + 1] = Key;
        }

        OutHull.Reset();
        for (int32 I = 0; I < Points.Num(); ++I)
        {
            while (OutHull.Num() >= 2 && Cross2D(OutHull[OutHull.Num() - 2], OutHull[OutHull.Num() - 1], Points[I]) <= 0.0)
            {
                OutHull.RemoveAt(OutHull.Num() - 1);
            }
            OutHull.Add(Points[I]);
        }
        const int32 LowerCount = OutHull.Num() + 1;
        for (int32 I = Points.Num() - 2; I >= 0; --I)
        {
            while (OutHull.Num() >= LowerCount && Cross2D(OutHull[OutHull.Num() - 2], OutHull[OutHull.Num() - 1], Points[I]) <= 0.0)
            {
                OutHull.RemoveAt(OutHull.Num() - 1);
            }
            OutHull.Add(Points[I]);
        }
        if (OutHull.Num() > 1)
        {
            OutHull.RemoveAt(OutHull.Num() - 1);
        }
    }

    double DistanceToSegmentSquared(const FVector2D& P, const FVector2D& A, const FVector2D& B)
    {
        const FVector2D AB = B - A;
        const double LengthSquared = AB.X * AB.X + AB.Y * AB.Y;
        double T = 0.0;
        if (LengthSquared > 1e-9)
        {
            T = ((P.X - A.X) * AB.X + (P.Y - A.Y) * AB.Y) / LengthSquared;
            T = T < 0.0 ? 0.0 : (T > 1.0 ? 1.0 : T);
        }
        const double DX = P.X - (A.X + AB.X * T);
        const double DY = P.Y - (A.Y + AB.Y * T);
        return DX * DX + DY * DY;
    }

    float OctileDistance(int32 X0, int32 Y0, int32 X1, int32 Y1)
    {
        const int32 DX = FMath::Abs(X1 - X0);
        const int32 DY = FMath::Abs(Y1 - Y0);
        const int32 Straight = FMath::Max(DX, DY) - FMath::Min(DX, DY);
        return static_cast<float>(Straight) + DiagonalStepCost * static_cast<float>(FMath::Min(DX, DY));
    }
}

void FURFPSNavGrid::Initialize(float InHalfSize, float InCellSize, float InAgentRadius, float InGroundZ)
{
    CellSize = FMath::Max(5.f, InCellSize);
    AgentRadius = FMath::Max(0.f, InAgentRadius);
    GroundZ = InGroundZ;
    OriginX = -InHalfSize;
    OriginY = -InHalfSize;
    Width = FMath::Max(1, FMath::CeilToInt(2.f * InHalfSize / CellSize));
    Height = Width;
    Blocked.Init(0, Width * Height);
    Component.Reset();
    bReady = false;
}

int32 FURFPSNavGrid::WorldToCellX(double WorldX) const
{
    return FMath::FloorToInt((WorldX - OriginX) / CellSize);
}

int32 FURFPSNavGrid::WorldToCellY(double WorldY) const
{
    return FMath::FloorToInt((WorldY - OriginY) / CellSize);
}

FVector FURFPSNavGrid::CellCenter(int32 Cell, double Z) const
{
    const int32 X = Cell % Width;
    const int32 Y = Cell / Width;
    return FVector(OriginX + (X + 0.5) * CellSize, OriginY + (Y + 0.5) * CellSize, Z);
}

bool FURFPSNavGrid::IsBodyBandObstacle(double MinZ, double MaxZ) const
{
    // Low kerbs and slabs can be stepped over; roofs, beams and decks pass overhead.
    return MaxZ > GroundZ + StepHeight && MinZ < GroundZ + ClearanceHeight;
}

void FURFPSNavGrid::AddBox(const FVector& Center, const FVector& HalfExtents, const FRotator& Rotation)
{
    if (Width <= 0 || bReady) return;

    TArray<FVector2D> Corners;
    Corners.Reserve(8);
    double MinZ = TNumericLimits<double>::Max();
    double MaxZ = TNumericLimits<double>::Lowest();
    for (int32 Index = 0; Index < 8; ++Index)
    {
        const FVector Local(
            (Index & 1) ? HalfExtents.X : -HalfExtents.X,
            (Index & 2) ? HalfExtents.Y : -HalfExtents.Y,
            (Index & 4) ? HalfExtents.Z : -HalfExtents.Z);
        const FVector World = Center + Rotation.RotateVector(Local);
        MinZ = FMath::Min(MinZ, World.Z);
        MaxZ = FMath::Max(MaxZ, World.Z);
        Corners.Add(FVector2D(World.X, World.Y));
    }
    if (!IsBodyBandObstacle(MinZ, MaxZ)) return;

    TArray<FVector2D> Hull;
    BuildConvexHull(Corners, Hull);
    RasterizeConvex(Hull);
}

void FURFPSNavGrid::AddCylinder(const FVector& Center, float Radius, float HalfHeight)
{
    if (Width <= 0 || bReady) return;
    if (!IsBodyBandObstacle(Center.Z - HalfHeight, Center.Z + HalfHeight)) return;

    const double Reach = Radius + AgentRadius;
    const int32 MinX = FMath::Max(0, WorldToCellX(Center.X - Reach));
    const int32 MaxX = FMath::Min(Width - 1, WorldToCellX(Center.X + Reach));
    const int32 MinY = FMath::Max(0, WorldToCellY(Center.Y - Reach));
    const int32 MaxY = FMath::Min(Height - 1, WorldToCellY(Center.Y + Reach));
    for (int32 Y = MinY; Y <= MaxY; ++Y)
    {
        for (int32 X = MinX; X <= MaxX; ++X)
        {
            const double DX = OriginX + (X + 0.5) * CellSize - Center.X;
            const double DY = OriginY + (Y + 0.5) * CellSize - Center.Y;
            if (DX * DX + DY * DY <= Reach * Reach)
            {
                Blocked[CellIndex(X, Y)] = 1;
            }
        }
    }
}

void FURFPSNavGrid::RasterizeConvex(const TArray<FVector2D>& Hull)
{
    if (Hull.Num() == 0) return;

    double MinWX = Hull[0].X, MaxWX = Hull[0].X, MinWY = Hull[0].Y, MaxWY = Hull[0].Y;
    for (const FVector2D& Point : Hull)
    {
        MinWX = FMath::Min(MinWX, Point.X);
        MaxWX = FMath::Max(MaxWX, Point.X);
        MinWY = FMath::Min(MinWY, Point.Y);
        MaxWY = FMath::Max(MaxWY, Point.Y);
    }

    const int32 MinX = FMath::Max(0, WorldToCellX(MinWX - AgentRadius));
    const int32 MaxX = FMath::Min(Width - 1, WorldToCellX(MaxWX + AgentRadius));
    const int32 MinY = FMath::Max(0, WorldToCellY(MinWY - AgentRadius));
    const int32 MaxY = FMath::Min(Height - 1, WorldToCellY(MaxWY + AgentRadius));
    const double ReachSquared = static_cast<double>(AgentRadius) * AgentRadius;
    const int32 Count = Hull.Num();

    for (int32 Y = MinY; Y <= MaxY; ++Y)
    {
        for (int32 X = MinX; X <= MaxX; ++X)
        {
            const int32 Cell = CellIndex(X, Y);
            if (Blocked[Cell]) continue;

            const FVector2D P(OriginX + (X + 0.5) * CellSize, OriginY + (Y + 0.5) * CellSize);
            bool bInside = Count >= 3;
            for (int32 Edge = 0; Edge < Count && bInside; ++Edge)
            {
                if (Cross2D(Hull[Edge], Hull[(Edge + 1) % Count], P) < 0.0) bInside = false;
            }
            if (!bInside)
            {
                for (int32 Edge = 0; Edge < Count; ++Edge)
                {
                    const FVector2D& A = Hull[Edge];
                    const FVector2D& B = Hull[Count == 1 ? Edge : (Edge + 1) % Count];
                    if (DistanceToSegmentSquared(P, A, B) <= ReachSquared)
                    {
                        bInside = true;
                        break;
                    }
                }
            }
            if (bInside)
            {
                Blocked[Cell] = 1;
            }
        }
    }
}

void FURFPSNavGrid::Finalize()
{
    if (Width <= 0) return;

    const int32 CellCount = Width * Height;
    Component.Init(INDEX_NONE, CellCount);

    // Connected regions (same moves as the path search) make unreachable goals cheap to detect.
    TArray<int32> Queue;
    Queue.Reserve(CellCount / 4);
    int32 NextComponent = 0;
    for (int32 Seed = 0; Seed < CellCount; ++Seed)
    {
        if (Blocked[Seed] || Component[Seed] != INDEX_NONE) continue;

        Queue.Reset();
        Queue.Add(Seed);
        Component[Seed] = NextComponent;
        for (int32 Head = 0; Head < Queue.Num(); ++Head)
        {
            const int32 Cell = Queue[Head];
            const int32 X = Cell % Width;
            const int32 Y = Cell / Width;
            for (int32 DY = -1; DY <= 1; ++DY)
            {
                for (int32 DX = -1; DX <= 1; ++DX)
                {
                    if (DX == 0 && DY == 0) continue;
                    if (!IsFreeCell(X + DX, Y + DY)) continue;
                    if (DX != 0 && DY != 0 && (!IsFreeCell(X + DX, Y) || !IsFreeCell(X, Y + DY))) continue;
                    const int32 Next = CellIndex(X + DX, Y + DY);
                    if (Component[Next] != INDEX_NONE) continue;
                    Component[Next] = NextComponent;
                    Queue.Add(Next);
                }
            }
        }
        ++NextComponent;
    }

    PathCost.Init(0.f, CellCount);
    PathParent.Init(INDEX_NONE, CellCount);
    SeenStamp.Init(0u, CellCount);
    ClosedStamp.Init(0u, CellCount);
    OpenHeap.Reset();
    OpenCount = 0;
    QueryStamp = 0;
    bReady = true;
}

bool FURFPSNavGrid::IsWalkable(const FVector& Location) const
{
    return bReady && IsFreeCell(WorldToCellX(Location.X), WorldToCellY(Location.Y));
}

int32 FURFPSNavGrid::GetWalkableCellCount() const
{
    int32 Count = 0;
    for (const uint8 Value : Blocked)
    {
        if (Value == 0) ++Count;
    }
    return Count;
}

int32 FURFPSNavGrid::FindNearestFreeCell(int32 X, int32 Y, int32 MaxRadius, int32 RequiredComponent) const
{
    int32 Best = INDEX_NONE;
    int32 BestDistance = MAX_int32;
    int32 Limit = MaxRadius;

    auto Consider = [&](int32 CX, int32 CY)
    {
        if (!IsFreeCell(CX, CY)) return;
        const int32 Cell = CellIndex(CX, CY);
        if (RequiredComponent != INDEX_NONE && Component[Cell] != RequiredComponent) return;
        const int32 Distance = (CX - X) * (CX - X) + (CY - Y) * (CY - Y);
        if (Distance < BestDistance)
        {
            BestDistance = Distance;
            Best = Cell;
        }
    };

    for (int32 Radius = 0; Radius <= Limit; ++Radius)
    {
        if (Radius == 0)
        {
            Consider(X, Y);
        }
        else
        {
            for (int32 Offset = -Radius; Offset <= Radius; ++Offset)
            {
                Consider(X + Offset, Y - Radius);
                Consider(X + Offset, Y + Radius);
            }
            for (int32 Offset = -Radius + 1; Offset <= Radius - 1; ++Offset)
            {
                Consider(X - Radius, Y + Offset);
                Consider(X + Radius, Y + Offset);
            }
        }
        // A closer cell can still sit on the next square ring (diagonal versus straight).
        if (Best != INDEX_NONE && Limit > Radius + 1)
        {
            Limit = Radius + 1;
        }
    }
    return Best;
}

bool FURFPSNavGrid::IsLineClear(int32 X0, int32 Y0, int32 X1, int32 Y1) const
{
    if (!IsFreeCell(X0, Y0) || !IsFreeCell(X1, Y1)) return false;

    // Grid traversal visiting every cell the segment touches; an exact corner crossing must
    // have both side cells free, like a diagonal step of the path search.
    const int32 DX = X1 - X0;
    const int32 DY = Y1 - Y0;
    const int32 NX = FMath::Abs(DX);
    const int32 NY = FMath::Abs(DY);
    const int32 SX = DX > 0 ? 1 : -1;
    const int32 SY = DY > 0 ? 1 : -1;
    int32 X = X0;
    int32 Y = Y0;
    int32 IX = 0;
    int32 IY = 0;
    while (IX < NX || IY < NY)
    {
        const int64 Decision = static_cast<int64>(1 + 2 * IX) * NY - static_cast<int64>(1 + 2 * IY) * NX;
        if (Decision == 0)
        {
            if (!IsFreeCell(X + SX, Y) || !IsFreeCell(X, Y + SY)) return false;
            X += SX;
            Y += SY;
            ++IX;
            ++IY;
        }
        else if (Decision < 0)
        {
            X += SX;
            ++IX;
        }
        else
        {
            Y += SY;
            ++IY;
        }
        if (!IsFreeCell(X, Y)) return false;
    }
    return true;
}

bool FURFPSNavGrid::HasClearLine(const FVector& From, const FVector& To) const
{
    return bReady && IsLineClear(WorldToCellX(From.X), WorldToCellY(From.Y), WorldToCellX(To.X), WorldToCellY(To.Y));
}

bool FURFPSNavGrid::ProjectToWalkable(const FVector& Location, float SearchRadius, FVector& OutLocation) const
{
    if (!bReady) return false;
    const int32 MaxRadius = FMath::Max(0, FMath::CeilToInt(SearchRadius / CellSize));
    const int32 Cell = FindNearestFreeCell(WorldToCellX(Location.X), WorldToCellY(Location.Y), MaxRadius, INDEX_NONE);
    if (Cell == INDEX_NONE) return false;
    OutLocation = CellCenter(Cell, Location.Z);
    return true;
}

bool FURFPSNavGrid::FindRandomPointNear(const FVector& Origin, const FVector& Center, float Radius, FVector& OutLocation) const
{
    if (!bReady) return false;

    const int32 OriginCell = FindNearestFreeCell(WorldToCellX(Origin.X), WorldToCellY(Origin.Y), 16, INDEX_NONE);
    if (OriginCell == INDEX_NONE) return false;
    const int32 RequiredComponent = Component[OriginCell];

    for (int32 Attempt = 0; Attempt < 16; ++Attempt)
    {
        const float Angle = FMath::FRand() * 2.f * PI;
        const float Distance = Radius * FMath::Sqrt(FMath::FRand());
        const int32 X = WorldToCellX(Center.X + FMath::Cos(Angle) * Distance);
        const int32 Y = WorldToCellY(Center.Y + FMath::Sin(Angle) * Distance);
        if (IsFreeCell(X, Y) && Component[CellIndex(X, Y)] == RequiredComponent)
        {
            OutLocation = CellCenter(CellIndex(X, Y), Origin.Z);
            return true;
        }
    }

    const int32 MaxRadius = FMath::Max(1, FMath::CeilToInt(Radius / CellSize));
    const int32 Fallback = FindNearestFreeCell(WorldToCellX(Center.X), WorldToCellY(Center.Y), MaxRadius, RequiredComponent);
    if (Fallback == INDEX_NONE) return false;
    OutLocation = CellCenter(Fallback, Origin.Z);
    return true;
}

void FURFPSNavGrid::PushOpen(float Cost, int32 Cell) const
{
    FOpenEntry Entry;
    Entry.Cost = Cost;
    Entry.Cell = Cell;
    if (OpenCount < OpenHeap.Num())
    {
        OpenHeap[OpenCount] = Entry;
    }
    else
    {
        OpenHeap.Add(Entry);
    }

    int32 Index = OpenCount++;
    while (Index > 0)
    {
        const int32 ParentIndex = (Index - 1) / 2;
        if (OpenHeap[ParentIndex].Cost <= OpenHeap[Index].Cost) break;
        const FOpenEntry Moved = OpenHeap[ParentIndex];
        OpenHeap[ParentIndex] = OpenHeap[Index];
        OpenHeap[Index] = Moved;
        Index = ParentIndex;
    }
}

int32 FURFPSNavGrid::PopOpen() const
{
    const int32 Top = OpenHeap[0].Cell;
    --OpenCount;
    if (OpenCount > 0)
    {
        OpenHeap[0] = OpenHeap[OpenCount];
        int32 Index = 0;
        for (;;)
        {
            const int32 Left = Index * 2 + 1;
            const int32 Right = Left + 1;
            int32 Smallest = Index;
            if (Left < OpenCount && OpenHeap[Left].Cost < OpenHeap[Smallest].Cost) Smallest = Left;
            if (Right < OpenCount && OpenHeap[Right].Cost < OpenHeap[Smallest].Cost) Smallest = Right;
            if (Smallest == Index) break;
            const FOpenEntry Moved = OpenHeap[Smallest];
            OpenHeap[Smallest] = OpenHeap[Index];
            OpenHeap[Index] = Moved;
            Index = Smallest;
        }
    }
    return Top;
}

bool FURFPSNavGrid::FindPath(const FVector& Start, const FVector& Goal, TArray<FVector>& OutWaypoints) const
{
    OutWaypoints.Reset();
    if (!bReady) return false;

    // An enemy standing on a platform or brushing a wall starts outside the free cells.
    const int32 StartCell = FindNearestFreeCell(WorldToCellX(Start.X), WorldToCellY(Start.Y), 16, INDEX_NONE);
    if (StartCell == INDEX_NONE) return false;
    // Goals on decks, platforms or inside obstacles fall back to the nearest reachable cell.
    const int32 GoalCell = FindNearestFreeCell(WorldToCellX(Goal.X), WorldToCellY(Goal.Y), 80, Component[StartCell]);
    if (GoalCell == INDEX_NONE) return false;

    if (StartCell == GoalCell)
    {
        OutWaypoints.Add(CellCenter(GoalCell, Start.Z));
        return true;
    }

    ++QueryStamp;
    if (QueryStamp == 0)
    {
        SeenStamp.Init(0u, Width * Height);
        ClosedStamp.Init(0u, Width * Height);
        QueryStamp = 1;
    }

    const int32 GoalX = GoalCell % Width;
    const int32 GoalY = GoalCell / Width;
    OpenCount = 0;
    PathCost[StartCell] = 0.f;
    PathParent[StartCell] = INDEX_NONE;
    SeenStamp[StartCell] = QueryStamp;
    PushOpen(OctileDistance(StartCell % Width, StartCell / Width, GoalX, GoalY), StartCell);

    // Raw pointers in the inner loop: this runs over up to 190k cells and the checked TArray
    // accessors of Development builds would double its cost.
    const uint8* BlockedData = Blocked.GetData();
    float* CostData = PathCost.GetData();
    int32* ParentData = PathParent.GetData();
    uint32* SeenData = SeenStamp.GetData();
    uint32* ClosedData = ClosedStamp.GetData();

    bool bFound = false;
    while (OpenCount > 0)
    {
        const int32 Cell = PopOpen();
        if (ClosedData[Cell] == QueryStamp) continue;
        ClosedData[Cell] = QueryStamp;
        if (Cell == GoalCell)
        {
            bFound = true;
            break;
        }

        const int32 X = Cell % Width;
        const int32 Y = Cell / Width;
        for (int32 DY = -1; DY <= 1; ++DY)
        {
            const int32 NY = Y + DY;
            if (NY < 0 || NY >= Height) continue;
            for (int32 DX = -1; DX <= 1; ++DX)
            {
                if (DX == 0 && DY == 0) continue;
                const int32 NX = X + DX;
                if (NX < 0 || NX >= Width) continue;
                const int32 Next = NY * Width + NX;
                if (BlockedData[Next]) continue;
                const bool bDiagonal = DX != 0 && DY != 0;
                // No corner cutting: both side cells of a diagonal step must be free.
                if (bDiagonal && (BlockedData[Y * Width + NX] || BlockedData[NY * Width + X])) continue;
                if (ClosedData[Next] == QueryStamp) continue;

                const float NewCost = CostData[Cell] + (bDiagonal ? DiagonalStepCost : 1.f);
                if (SeenData[Next] == QueryStamp && NewCost >= CostData[Next]) continue;

                SeenData[Next] = QueryStamp;
                CostData[Next] = NewCost;
                ParentData[Next] = Cell;
                // A slightly inflated heuristic keeps the explored area small; the routes stay
                // within a few percent of the shortest one, which is plenty for walking enemies.
                PushOpen(NewCost + OctileDistance(NX, NY, GoalX, GoalY) * 1.15f, Next);
            }
        }
    }
    if (!bFound) return false;

    TArray<int32> Cells;
    for (int32 Cell = GoalCell; Cell != INDEX_NONE; Cell = PathParent[Cell])
    {
        Cells.Add(Cell);
    }
    for (int32 Low = 0, High = Cells.Num() - 1; Low < High; ++Low, --High)
    {
        const int32 Moved = Cells[Low];
        Cells[Low] = Cells[High];
        Cells[High] = Moved;
    }

    // String pulling: keep only the cells where the straight line would leave the free area.
    const int32 LastIndex = Cells.Num() - 1;
    int32 Anchor = 0;
    while (Anchor < LastIndex)
    {
        int32 Next = Anchor + 1;
        const int32 AnchorX = Cells[Anchor] % Width;
        const int32 AnchorY = Cells[Anchor] / Width;
        for (int32 Candidate = Anchor + 2; Candidate <= LastIndex; ++Candidate)
        {
            if (!IsLineClear(AnchorX, AnchorY, Cells[Candidate] % Width, Cells[Candidate] / Width)) break;
            Next = Candidate;
        }
        OutWaypoints.Add(CellCenter(Cells[Next], Start.Z));
        Anchor = Next;
    }
    return OutWaypoints.Num() > 0;
}
