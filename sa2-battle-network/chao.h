#pragma once

#include <ninja.h>
#include <cstdint>
#include <SA2Structs.h>

#pragma pack(push, 1)

/*
 * NJS_CNK_OBJECT
 */
struct NJS_CNK_OBJECT
{
	Uint32 evalflags;        /* evalation flags              */
	NJS_CNK_MODEL* model;    /* model data pointer           */
	Float pos[3];            /* translation                  */
	Angle ang[3];            /* rotation                     */
	Float scl[3];            /* scaling                      */
	NJS_CNK_OBJECT* child;   /* child object                 */
	NJS_CNK_OBJECT* sibling; /* sibling object               */
};

/* 508 */
struct ChunkObjectPointer
{
	NJS_CNK_OBJECT base;
	int field_34;
	NJS_VECTOR differenceOrigOther;
	int field_44;
	int field_48;
	int field_4C;
	NJS_VECTOR originalPosition;
	NJS_VECTOR field_5C;
	float field_68;
	int field_6C;
	int field_70;
	int field_74;
	int field_78;
	int field_7C;
	int field_80;
	int field_84;
	int field_88;
	int field_8C;
	int field_90;
	NJS_CNK_OBJECT* animalPart;
	ChaoToyChunk toy;
	int useTransform;
	NJS_VECTOR position;
	Rotation rotation;
	NJS_MATRIX* Matrix;
	NJS_VECTOR* field_C8;
	int field_CC;
};

/* 554 */
struct ChaoFacialData_
{
	int eyeTimer;
	int16_t field_4;
	int16_t field_6;
	int16_t Eye;
	int16_t field_A;
	int mouthTimer;
	int16_t field_10;
	int16_t Mouth;
	NJS_VECTOR somekindaposition;
	float field_20;
	int field_24;
	NJS_CNK_MODEL* Eye1;
	NJS_CNK_MODEL* Eye2;
	int field_30;
	int blinkState;
	int blinkTimer;
	int dword3C;
	unsigned int unsigned40;
	int gap44;
	int field_48;
	int VertRotAlt;
	int VertRot;
	int field_54;
	int HorizRotAlt;
	int HorizRot;
};


/* 583 */
struct ChaoEmotions_ // fuck it it's probably a keeper
{
	int16_t field_124;
	int16_t Category1Timer;
	int16_t SicknessTimer;
	int16_t Category2Timer;
	char Joy;
	char Anger;
	char UrgeToCry;
	char Fear;
	char Surprise;
	char Dizziness;
	char Relax;
	char Total;
	int16_t Sleepiness;
	int16_t Tiredness;
	int16_t Hunger;
	int16_t MateDesire;
	int16_t Boredom;
	int16_t Lonely;
	int16_t Tire;
	int16_t Stress;
	int16_t Nourish;
	int16_t Conditn;
	int16_t Energy;
	char Normal_Curiosity;
	char Kindness;
	char CryBaby_Energetic;
	char Naive_Normal;
	char Solitude;
	char Vitality;
	char Glutton;
	char Regain;
	char Skillful;
	char Charm;
	char Chatty;
	char Normal_Carefree;
	char Fickle;
	char FavoriteFruit;
	char field_34;
	char field_35;
	char CoughLevel;
	char ColdLevel;
	char RashLevel;
	char RunnyNoseLevel;
	char HiccupsLevel;
	char StomachAcheLevel;
};

/* 604 */
struct ChaoKWLGBondThing
{
	int16_t field_0;
	int16_t field_2;
	int field_4;
	int field_8;
	int field_C;
	int field_10;
	int field_14;
	int field_18;
};

/* 603 */
struct ChaoKWLG
{
	char ClassRoomFlags[4];
	int16_t SA2BToys;
	int16_t field_166;
	int someTimer;
	ChaoCharacterBond SA2BCharacterBonds[6];
	char field_190[22];
	ChaoKWLGBondThing field_46[20];
	char field_276[98];
};

/* 567 */
struct ChaoDNAGrade
{
	uint8_t gradeA;
	uint8_t gradeB;
};

/* 279 */
struct ChaoDNA_
{
	char ResetTrigger;
	char forceColor;
	char field_2[90];
	ChaoDNAGrade StatGrades[9];
	char PowerRun1;
	char PowerRun2;
	char FlySwim1;
	char FlySwim2;
	char Alignment1;
	char Alignment2;
	char Normal_Curiosity1;
	char Normal_Curiosity2;
	char Kindness1;
	char Kindness2;
	char CryBaby_Energetic1;
	char CryBaby_Energetic2;
	char Naive_Normal1;
	char Naive_Normal2;
	char Solitude1;
	char Solitude2;
	char Vitality1;
	char Vitality2;
	char Glutton1;
	char Glutton2;
	char Regain1;
	char Regain2;
	char Skillful1;
	char Skillful2;
	char Charm1;
	char Charm2;
	char Chatty1;
	char Chatty2;
	char Normal_CareFree1;
	char Normal_Carefree2;
	char Fickle1;
	char Fickle2;
	char FavoriteFruit1;
	char FavoriteFruit2;
	char field_34_1;
	char field_34_2;
	char field_35_1;
	char field_35_2;
	char Color1;
	char Color2;
	char MonotoneFlag1;
	char MonotoneFlag2;
	char Texture1;
	char Texture2;
	char ShinyFlag1;
	char ShinyFlag2;
	char EggColor1;
	char EggColor2;
	char gap_4D6;
	char field_9F;
	int field_A0;
};


/* 277 */
struct ChaoDataBase_ // keeper
{
	char GBAItems[12];
	int GBARings;
	int16_t field_10;
	char Name[7];
	char field_19;
	char GBATexture;
	char field_1B[5];
	char SwimFraction;
	char FlyFraction;
	char RunFraction;
	char PowerFraction;
	char StaminaFraction;
	char LuckyFraction;
	char IntelligenceFraction;
	char UnknownFraction;
	char StatGrades[8];
	char StatLevels[8];
	int16_t StatPoints[8];
	int field_48;
	int field_4C;
	int field_50;
	int field_54;
	int field_58;
	int field_5C;
	int field_60;
	int field_64;
	int someGBAthingstart;
	int field_6C;
	int field_70;
	int field_74;
	int field_78;
	int someGBAthingend;
	ChaoType Type;
	char Garden;
	int16_t Happiness;
	int16_t InKindergarten;
	int16_t ClockRollovers;
	int16_t AdultClockRollovers;
	int16_t Lifespan;
	int16_t Lifespan2;
	int16_t Reincarnations;
	int field_90;
	int field_94;
	int Seed;
	int field_9C;
	int field_A0;
	int TimescaleTimer;
	float PowerRun;
	float FlySwim;
	float Alignment;
	int field_B4;
	int field_B8;
	float LastLifeAlignment;
	float EvolutionProgress;
	char gap_C4[13];
	char EyeType;
	char MouthType;
	char BallType;
	char gap_D4[1];
	char Headgear;
	char HideFeet;
	char Medal;
	char Color;
	char MonotoneHighlights;
	char Texture;
	char Shiny;
	char EggColor;
	SADXBodyType BodyType;
	char BodyTypeAnimal;
	char field_DF[41];
	int16_t DoctorMedal;
	char field_10A[14];
	int SA2AnimalBehavior;
	SA2BAnimal SA2BArmType;
	SA2BAnimal SA2BEarType;
	SA2BAnimal SA2BForeheadType;
	SA2BAnimal SA2BHornType;
	SA2BAnimal SA2BLegType;
	SA2BAnimal SA2BTailType;
	SA2BAnimal SA2BWingType;
	SA2BAnimal SA2BFaceType;
	ChaoEmotions_ Emotion;
	ChaoKWLG KWLG;
	ChaoDNA_ DNA;
};


/* 37 */
struct ChaoData_ // keeper
{
	ChaoDataBase_ data;
	char SADXReserved[68];
	char Padding1[114];
	char ShinyFruitValue;
	char NewMedalAccessory;
	char FreeSpace;
	char LobbyTextureValue;
	char freeSpace1[4];
	char EyeColor;
	char freeSpace2;
	char UpgradeCounter;
	char freeSpace3;
	char XGradeValue;
	char Padding2[609];
};

/* 38 */
struct KarateOpponent
{
	ChaoType ChaoType;
	uint8_t EyeType;
	uint8_t MouthType;
	uint8_t BallType;
	uint8_t Headgear;
	uint8_t HideFeet;
	uint8_t Medal;
	uint8_t Color;
	uint8_t Monotone;
	uint8_t Texture;
	uint8_t Shiny;
	SA2BAnimal SA2BArmType;
	SA2BAnimal SA2BEarType;
	SA2BAnimal SA2BForeheadType;
	SA2BAnimal SA2BHornType;
	SA2BAnimal SA2BLegType;
	SA2BAnimal SA2BTailType;
	SA2BAnimal SA2BWingType;
	SA2BAnimal SA2BFaceType;
	uint8_t f13[1];
	int16_t PowerRun;
	int16_t FlySwim;
	int16_t Alignment;
	int16_t Magnitude;
	char Name_unused[7];
	char unused;
	int16_t StatPoints[8];
};

/* 281 */
struct ChaoUnknownB
{
	int16_t field_0;
	int16_t field_2;
	int16_t field_4;
	int16_t field_6;
	float field_8;
	int field_C;
	int field_10;
	int field_14;
	int field_18;
};

/* 282 */
struct ChaoUnknown
{
	int16_t field_0;
	int16_t field_2;
	int field_4;
	int field_8;
	int field_C;
	float field_10;
	int16_t field_14;
	int16_t field_16;
	ChaoUnknownB field_18[32];
};

/* 283 */
struct ChaoUnknownC_
{
	char gap_0[2];
	int16_t field_2;
	char gap_4[8];
	int field_C;
	int field_10;
	char gap_14[16];
	char field_24[72];
	char gap_6C[516];
	float field_270;
	NJS_VECTOR vector;
	char gap_280[384];
	char field_400;
};

/* 284 */
struct ChaoCurrentActionInfo_
{
	int16_t field_0;
	uint16_t Index;
	int16_t field_4;
	int16_t field_6;
	int field_8;
	int Timer;
	int field_10;
	int field_14;
	int16_t BehaviourTime;
	int16_t field_1A;
};

/* 592 */
struct ChaoBehaviourInfo_ // keeper
{
	ChaoCurrentActionInfo CurrentActionInfo;
	int16_t CanThink_;
	int16_t field_1E;
	int field_20;
	int field_24;
	ChaoBehaviourInfothing field_28;
	ChaoBehaviourInfothing field_48;
	char field_68[8];
	char field_70[72];
	int field_B8;
	int field_BC;
	int field_C0;
	char field_288[420];
	int field_268;
	int field_430;
	float someFloat;
	NJS_VECTOR someKindaPosition;
	int behaviourCount;
	int behaviourIndex;
	int behaviourQueue[16];
	int behaviourTimer[16];
};

/* 288 */
struct ChaoData1_
{
	EntityData1 entity;
	int gap_30;
	ObjectMaster* ObjectMaster_ptr1;
	ObjectMaster* field_38;
	ObjectMaster* field_3C;
	ObjectMaster* ObjectMaster_ptr2;
	ObjectMaster* field_44;
	float field_54;
	int field_4C;
	int field_50;
	int field_60;
	int field_58;
	ChaoDataBase_* ChaoDataBase_ptr;
	char field_70[40];
	int field_88;
	int field_8C;
	char field_90[16];
	int Flags;
	int16_t field_B4;
	int16_t field_A6;
	float waypointID;
	MotionTableData MotionTable;
	MotionTableData BodyTypeNone_MotionTable;
	uint8_t gap144[112];
	ChaoBehaviourInfo_ ChaoBehaviourInfo;
	int field_4BC[21];
	int PointerToStructWithCnkObject;
	ChunkObjectPointer* ChaoNodes[40];
	ChaoEvos* NormalModels;
	ChaoEvos* HeroModels;
	ChaoEvos* DarkModels;
	NJS_VECTOR BaseTranslationPos;
	NJS_VECTOR HeadTranslationPos;
	NJS_VECTOR LeftHandTranslationPos;
	NJS_VECTOR RightHandTranslationPos;
	NJS_VECTOR LeftLegTranslationPos;
	NJS_VECTOR RightLegTranslationPos;
	NJS_VECTOR NoseTranslationPos;
	NJS_VECTOR NoseUnitTransPortion;
	NJS_VECTOR LeftEyeTranslationPos;
	NJS_VECTOR LeftEyeUnitTransPortion;
	NJS_VECTOR RightEyeTranslationPos;
	NJS_VECTOR RightEyeUnitTransPortion;
	ChaoToyChunk LeftHandToyChunk;
	ChaoToyChunk RightHandToyChunk;
	int field_670;
	int16_t field_674;
	int16_t field_676;
	int field_678;
	int field_67C;
	int field_680;
	int field_684;
	int field_688;
	ChaoFacialData_ ChaoFacialData;
	EmotionBallData EmotionBallData;
	NJS_VECTOR field_7A4;
	float waypointLerpFactor;
	NJS_VECTOR field_7B4;
	NJS_VECTOR field_7C0;
	ChaoObjectListInfo ObjectListInfo;
	ChaoObjectList PlayerObjects;
	ChaoObjectList ChaoObjects;
	ChaoObjectList FruitObjects;
	ChaoObjectList TreeObjects;
	ChaoObjectList ToyObjects;
	char ObjectListEnd;
	char field_19D9[927];
};

/* 289 */
union Data1Ptr_
{
	EntityData1* Undefined;
	EntityData1* Entity;
	ChaoData1_* Chao;
	ChaoDebugData1* ChaoDebug;
};

/* 329 */
struct ChaoHudThingB
{
	int field_0;
	float field_4;
	float field_8;
	float field_C;
	float field_10;
	float field_14;
	float field_18;
	NJS_TEXLIST* texlist;
	Uint32 num;
};

/* 330 */
struct ChaoHudThing
{
	NJS_POINT2I position;
	NJS_POINT2I scale;
	NJS_POINT2I c;
	NJS_POINT2I d;
};

struct StatUIStats
{
	int field_4;
	int field_8;
	char StatLevelsOriginal;
	char StatFractionOriginal;
	__int16 StatPointsOriginal;
	char StatLevels;
	char StatFraction;
	__int16 StatPoints;
};


/* 335 */
struct StatUIBptr
{
	uint8_t Status;
	char gap1;
	int field_4;
	uint8_t gap8[4];
	uint8_t NumberOfStats;
	uint8_t gapD[3];
	float xPos;
	float yPos;
	float xPosGoal;
	float yPosGoal;
	float AlphaColor;
	float gap24;
	int Unused2;
	int Unused1;
	int field_30;
	int Timer;
	ObjectMaster* PointerToChao;
	ChaoDataBase_* ChaoDataBase_;
	int gap40;
	StatUIStats Stats[5];
	int field_98;
	int asd;
};

/* 336 */
struct ChaoModelTestThing
{
	int field_510;
	int field_524;
	int field_528[40];
	ChaoEvos* unknown_e_1;
	ChaoEvos* unknown_e_2;
	NJS_VECTOR BaseTranslationPos;
	NJS_VECTOR HeadTranslationPos;
	NJS_VECTOR LeftHandTranslationPos;
	NJS_VECTOR RightHandTranslationPos;
	NJS_VECTOR LeftLegTranslationPos;
	NJS_VECTOR RightLegTranslationPos;
	NJS_VECTOR NoseTranslationPos;
	NJS_VECTOR NoseUnitTransPortion;
	NJS_VECTOR LeftEyeTranslationPos;
	NJS_VECTOR LeftEyeUnitTransPortion;
	NJS_VECTOR RightEyeTranslationPos;
	NJS_VECTOR RightEyeUnitTransPortion;
};

/* 531 */
struct KarateChao_dwordC
{
	char char0;
	uint8_t byte1;
	uint8_t gap2;
	char Eye;
	char Mouth;
	uint8_t flag;
	int16_t gap6;
	int16_t field_8;
	int16_t field_A;
	int dwordC;
	int gap10;
	float frame;
	float someSin;
	int field_1C;
	float someCos;
	MotionTableData* MotionTablePointer;
};

/* 530 */
struct KarateChaoExec_Data2
{
	int16_t char0;
	short word2;
	short flag;
	short word6;
	ObjectMaster* pointerToChao;
	KarateChao_dwordC* dwordC;
	ChaoData* chaoDataPointer;
	int field_14;
	NJS_VECTOR* BaseTranslationPos;
	NJS_VECTOR* HeadTranslationPos;
	float SwimStat;
	float FlyStat;
	float RunStat;
	float PowerStat;
	int field_30;
	float field_34;
	float zeal;
	float StaminaStat;
	float LuckStat;
	float float44;
};

struct ALO_GrowTreePointerThing2
{
	float size;
	float float4;
	float dword8;
	NJS_VECTOR field_C;
	int field_18;
	int field_1C;
};

struct al_material_object
{
	NJS_CNK_OBJECT object;
	int field_34;
	int field_38;
};

struct TreeSaveThing
{
	uint8_t treeType;
	unsigned __int8 lifeTimeStatus;
	unsigned __int8 growth;
	unsigned __int8 lifeSpanMaybe;
	char fruitSize1;
	char FruitSize2;
	char fruitSize3;
	uint8_t rotation;
};

/* 533 */
struct ALO_GrowTreeExecutor_Data
{
	EntityData1 entityData;
	unsigned __int8 treeType;
	uint8_t byte31;
	float lerpFactor;
	int dword38;
	int dword3C;
	int gap40;
	ALO_GrowTreePointerThing2 fruitLifeData[3];
	float scale;
	float field_A8;
	int field_AC;
	int field_B0;
	int field_B4;
	float field_B8;
	int field_BC;
	int someSine;
	float sineMultiplier;
	int SomeSineArrayLookup;
	int field_CC;
	int field_D0;
	float field_D4;
	Rotation someRotation;
	int field_E4;
	int CurrentChaoArea;
	TreeSaveThing* TreeSaveThing;
	NJS_TEXLIST* texlist;
	NJS_OBJECT* model1;
	NJS_OBJECT* model2;
	al_material_object* al_material;
};

/* 535 */
enum ChaoTreeType
{
	ChaoTreeType_1 = 0x1,
	ChaoTreeType_2 = 0x2,
	ChaoTreeType_3 = 0x3,
	ChaoTreeType_4 = 0x4,
	ChaoTreeType_GardenTree = 0x5,
};

/* 544 */
struct ChaoSelectWindow_Data2
{
	char char0;
	int16_t gap1;
	int16_t field_3;
	char field_5;
	int16_t field_6;
	int field_8;
	int posY;
	float alpha;
	float field_14;
	int field_18;
	int field_1C;
	int dword20;
	uint8_t gap24[4];
	int saveIndex;
	ObjectMaster* chao[8];
	float field_4C;
	float field_50;
	float field_54;
	float field_58;
	char field_5C;
	int field_60;
	int field_64;
};

/* 548 */
struct ChaoSelectMenuManager_Data
{
	int menuID;
	uint8_t gap4[8];
	ObjectMaster* PrevObject;
	int field_10;
	int field_14;
	int field_18;
	int field_1C;
	int dword20;
	int field_24;
	int horizontal;
	int vertical;
	int selectedGarden;
	int gap34;
	int field_38;
	int dword3C;
};

/* 549 */
struct AL_ChaoParamWindowExecutor_Data
{
	char Action;
	uint8_t byte4;
	short word6;
	signed int posX;
	signed int posY;
	float alpha;
	float float14;
	int dword20;
	int dword24;
	int dword30;
	int dword34;
	ObjectMaster* PointerToChao;
	ChaoData* ChaoData;
};

struct ClassroomThing
{
	int lessonID;
	char dword4;
	char field_5;
	char field_6;
	char field_7;
	int dword8;
	ObjectMaster* pobjectmasterC;
};


/* 555 */
struct ClassroomData
{
	int16_t byte0;
	char field_2;
	char field_3;
	uint8_t gap4[2];
	char currentLesson;
	char field_7;
	char field_8;
	char field_9;
	int16_t field_C;
	int16_t field_E;
	char timer;
	char field_14;
	ClassroomThing field_18[4];
	float byte58;
	int dword5C;
	ObjectMaster* chao;
	ObjectMaster* gap64;
	ObjectMaster* lessonsInProgress;
	int field_6C;
	int field_70;
	int field_74;
	int field_78;
	int dword7C;
};

/* 566 */
struct KarateMainExecData
{
	char gap0;
	char field_1;
	char field_2;
	char field_3;
	char field_4;
	char byte5;
	char byte6;
	char Round;
	char opponent;
	char field_9;
	char MenuSelection;
	char field_B;
	ObjectMaster* playerChao;
	ObjectMaster* pointerToSaveThing;
	ObjectMaster* PointerToOtherSaveThing;
	ChaoData* field_18;
	ChaoData* field_1C;
	int field_20;
	int field_24;
	int field_28;
	int field_2C;
	float field_30;
	int field_34;
	int field_38;
	ObjectMaster* field_3C;
};

/* 573 */
struct ALFSave
{
	int ChaoSaveStart;
	int field_4;
	int field_8;
	int ChaoGardensUnlocked;
	int ChaoToysUnlocked;
	int field_14;
	int field_18;
	int field_1C[43];
	char ChaoFruitSlots[480];
	char field_2A8[320];
	char ChaoSeedSlots[240];
	char ChaoHatSlots[480];
	char ChaoAnimalSlots[200];
	char field_780[624];
	char field_9F0[116];
	ChaoData ChaoSlots[25];
};

/* 580 */
struct GBAManagerThing
{
	int thing;
	int field_4;
	int16_t field_8;
	int16_t field_A;
	int mode;
	int field_10;
	int field_14;
	int hasItem[11];
	int hasItem___[11];
	int chaoDataPointer;
	int eggType;
	int rings;
	char fruitType[8];
	ChaoData chaoData1;
	ChaoData chaoData2;
};

struct MessageFontThing
{
	signed __int16 signed0;
	__int16 gap2;
	__int16 field_4;
	__int16 field_6;
	signed __int16* psigned8;
};

struct MessageField0
{
	MessageFontThing* dword0;
	float float4;
	float float8;
	float xSize;
	float ySize;
	int field_14;
	float field_18;
};

struct MessageField0Array
{
	MessageField0 array[9];
};

struct Message380
{
	int Mode;
	Buttons ButtonPress;
	int ButtonOn;
	float floatC;
	float float10;
	float float14;
	int dword18;
};

struct Message380Array
{
	Message380 array[32];
};

struct KinderCoMessageThing
{
	MessageField0Array* gap0;
	Message380Array* pointerToSomething0x380;
	__int16 someKindaCount;
	__int16 wordsLeftMaybe;
	float floatC;
	float posX;
	float posY;
	float rectSizeX;
	float rectSizeY;
	float field_20;
	float field_24;
	__int16 field_28;
	char enabled;
	char field_2B;
	int ContinueDotColor;
};

/* 586 */
struct HealthCenter
{
	int gap0;
	int field_4;
	ObjectMaster* field_8;
	int field_C;
	int field_10;
	int field_14;
	ObjectMaster* medicalChartChao_;
	uint8_t gap1C[8];
	ObjectMaster* medicalChartChao;
	uint8_t gap28[20];
	KinderCoMessageThing* pkindercomessagething3C;
	uint8_t gap40[32];
	int dword60;
};

/* 598 */
struct GBABuffer
{
	int field_0;
	float field_4;
	int field_8;
	int field_C;
	int field_10;
	int field_14;
	int field_18;
	int pointerToChaoData;
	int field_20;
	int field_24;
	int field_28;
	int field_2C;
	int field_30;
	int field_34;
	int field_38;
};

#pragma pack(pop)
