// ImHex Pattern Script for Fumen V2 Data Structure

// NoteType Enum
enum NoteType : u32 {
    NoteType_Don = 1,
    NoteType_Do = 2,
    NoteType_Ko = 3,
    NoteType_Katsu = 4,
    NoteType_Ka = 5,
    NoteType_Renda = 6,
    NoteType_BigDon = 7,
    NoteType_BigKatsu = 8,
    NoteType_BigRenda = 9,
    NoteType_Balloon = 10,
    NoteType_Bell = 12
};

// Judge Timing Block
struct JudgeTimingBlock {
    float GoodTiming;
    float OkTiming;
    float BadTiming;
};

// Fumen V2 Header
struct Header {
    JudgeTimingBlock JudgeTimings[36];
    u32 HasDivergentPaths;
    u32 MaxHP;
    u32 ClearHP;
    s32 HPPerGood;
    s32 HPPerOk;
    s32 HPPerBad;
    u32 MaxCombo;
    u32 HPIncreaseRatio;
    u32 HPIncreaseRatioMaster;
    u32 GoodDivergePoints;
    u32 OkDivergePoints;
    u32 BadDivergePoints;
    u32 DrumrollDivergePoints;
    u32 GoodDivergePointsBig;
    u32 OkDivergePointsBig;
    u32 DrumrollDivergePointsBig;
    u32 BalloonDivergePoints;
    u32 BellYamDivergePoints;
    u32 NumberOfDivergePoints;
    u32 MaxScoreValue;
    u32 NumberOfMeasures;
    u32 _Unknown1;
};

// Note Data
struct NoteData {
    NoteType Type;
    float NoteOffset;
    u32 _Padding;
    u16 InitialScoreValue;
    u16 ScoreDifferenceTimes4;
    u32 _UnknownNoteData2;
    float Length;
    if (Type == NoteType::NoteType_Renda || Type == NoteType::NoteType_BigRenda)
    {
        u32 _RendaPadding3;
        u32 _RendaPadding4;
    }
};

struct MeasureBranchData {
    u16 NoteCount;
    u16 _Unknown1;
    float ScrollSpeed;
    NoteData NormalNotes[NoteCount];   // Array of Normal Notes
};

// Measure Data
struct MeasureData {
    float BPM;
    float MeasureOffset;
    u8 IsGogoTime;
    u8 IsBarLineVisible;
    u16 _Padding1;
    u32 InNormalToAdvancedDivergePointReq;
    u32 InNormalToMasterDivergePointReq;
    u32 InAdvancedToMasterDivergePointReq;
    u32 InAdvancedKeepAdvancedDivergePointReq;
    u32 InMasterToAdvancedDivergePointReq;
    u32 InMasterKeepMasterDivergePointReq;
    u32 _Padding2;
};

// Measure
struct Measure {
    MeasureData Data;
    MeasureBranchData NormalBranch;
    MeasureBranchData AdvancedBranch;
    MeasureBranchData MasterBranch;
};

// Fumen Chart
struct FumenChart {
    Header ChartHeader;
    Measure Measures[ChartHeader.NumberOfMeasures]; // Array of Measures
};

FumenChart chart @ 0x00;