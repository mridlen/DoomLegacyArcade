// FraggleScript constants

// Thing numbers
const PLAYER = 0;
const TROOPER = 1;
const SHOTGUY = 2;
const ARCHVILE = 3;
const VILEFIRE = 4;
const REVENANT = 5;
const REVENANTMISL = 6;
const SMOKE = 7;             // Revenant fireball trail
const MANCUBUS = 8;
const MANCUBUSSHOT = 9;
const CHAINGUY = 10;
const IMP = 11;
const DEMON = 12;
const SPECTRE = 13;
const CACODEMON = 14;
const BARONOFHELL = 15;
const BARONSHOT = 16;
const HELLKNIGHT = 17;
const LOSTSOUL = 18;
const SPIDERMASTERMIND = 19;
const ARACHNOTRON = 20;
const CYBERDEMON = 21;
const PAINELEMENTAL = 22;
const WOLFSS = 23;
const KEEN = 24;
const BOSSBRAIN = 25;
const BOSSSPIT = 26;
const BOSSTARGET = 27;
const SPAWNSHOT = 28;
const SPAWNFIRE = 29;
const BARREL = 30;
const IMPSHOT = 31;
const CACOSHOT = 32;
const FLYINGROCKET = 33;
const FLYINGPLASMA = 34;
const FLYINGBFG = 35;
const ARACHPLAZ = 36;
const PUFF = 37;                   // Bullet puff
const BLOOD = 38;
const TFOG = 39;                   // Teleport fog
const IFOG = 40;                   // Item respawn fog
const TELEPORTMAN = 41;
const EXTRABFG = 42;
const GREENARMOR = 43;
const BLUEARMOR = 44;
const HEALTHPOTION = 45;
const ARMORHELMET = 46;
const BLUEKEYCARD = 47;
const REDKEYCARD = 48;
const YELLOWKEYCARD = 49;
const YELLOWSKULLKEY = 50;
const REDSKULLKEY = 51;
const BLUESKULLKEY = 52;
const STIMPACK = 53;
const MEDIKIT = 54;
const SUPERCHARGE = 55;
const INVULNERABILITY = 56;
const BESERKPACK = 57;
const INVISIBILITY = 58;
const RADSUIT = 59;
const AUTOMAP = 60;
const LITEAMP = 61;
const MEGASPHERE = 62;
const CLIP = 63;
const BULLETBOX = 64;
const ROCKET = 65;
const ROCKETBOX = 66;
const ECELL = 67;
const ECELLPACK = 68;
const SHELLS = 69;
const SHELLBOX = 70;
const BACKPACK = 71;
const BFG = 72;
const CHAINGUN = 73;
const CHAINSAW = 74;
const RLAUNCHER = 75;
const PLASMAGUN = 76;
const SHOTGUN = 77;
const SUPERSHOTGUN = 78;
const TALLTECHLAMP = 79;
const SHORTTECHLAMP = 80;
const FLOORLAMP = 81;
const TALLGRNPILLAR = 82;
const SHRTGRNPILLAR = 83;
const TALLREDPILLAR = 84;
const SHRTREDPILLAR = 85;
const SKULLCOLUMN = 86;
const HEARTCOLUMN = 87;
const EVILEYE = 88;
const SKULLROCK = 89;
const GRAYTREE = 90;
const TALLBLUFIRESTICK = 91;
const TALLGRNFIRESTICK = 92;
const TALLREDFIRESTICK = 93;
const SHRTBLUFIRESTICK = 94;
const SHRTGRNFIRESTICK = 95;
const SHRTREDFIRESTICK = 96;
const STALAGMITE = 97;
const TALLTECHPILLAR = 98;
const CANDLE = 99;
const CANDELABRA = 100;
const TWITCHCORPSE1 = 101;
const HANGINGMAN1 = 102;
const HANGINGMAN2 = 103;
const HANGINGMAN3 = 104;
const HANGINGMAN4 = 105;
const HANGINGMAN5 = 106;
const HANGINGMAN6 = 107;
const HANGINGMAN7 = 108;
const HANGINGMAN8 = 109;
const TWITCHCORPSE2 = 110;
const DEADCACO = 111;
const DEADPLAYER = 112;
const DEADTROOPER = 113;
const DEADDEMON = 114;
const DEADLOSTSOUL = 115;
const DEADIMP = 116;
const DEADSERGEANT = 117;
const SLOP = 118;
const SLOP2 = 119;
const SKULLPOLE1 = 120;
const BLOODPOOL1 = 121;
const SKULLPOLE2 = 122;
const SKULLPILE = 123;
const DEADCORPSE1 = 124;
const TWITCHCORPSE3 = 125;
const TREE = 126;
const BURNINGBARREL = 127;
const HANGINGMAN9 = 128;
const HANGINGMAN10 = 129;
const HANGINGMAN11 = 130;
const HANGINGMAN12 = 131;
const HANGINGMAN13 = 132;
const HANGINGMAN14 = 133;
const BLOODPOOL2 = 134;
const BLOODPOOL3 = 135;
const BLOODPOOL4 = 136;

const PUSH = 141;
const PULL = 142;
const DOGS = 143;
const PLASMA1 = 144;
const PLASMA2 = 145;
const CAMERA = 146;
const NODE = 147;

//const SCEPTRE = ;
//const BIBLE = ;
//const PARTICLE = ;


// Mapthing property constants, for use with the set/getobjproperty() and objstate() functions
const RADIUS = 0;
const HEIGHT = 1;
const MASS = 2;
const HEALTH = 3;
const DAMAGE = 4;
const SPEED = 5;
const REACTIONTIME = 6;
const PAINCHANCE = 7;
const ST_SPAWNSTATE = 8;
const ST_SEESTATE = 9;
const ST_MELEESTATE = 10;
const ST_MISSILESTATE = 11;
const ST_PAINSTATE = 12;
const ST_DEATHSTATE = 13;
const ST_XDEATHSTATE = 14;
const ST_CRASHSTATE = 15;
const ST_RAISESTATE = 16;
const SEESOUND = 17;
const ACTIVESOUND = 18;
const ATTACKSOUND = 19;
const PAINSOUND = 20;
const DEATHSOUND = 21;



// Thing flags (objflag)
const MF_SPECIAL = 0;
const MF_SOLID = 1;
const MF_SHOOTABLE = 2;
const MF_NOSECTOR = 3;
const MF_NOBLOCKMAP = 4;
const MF_AMBUSH = 5;
const MF_JUSTHIT = 6;
const MF_JUSTATTACKED = 7;
const MF_SPAWNCEILING = 8;
const MF_NOGRAVITY = 9;
const MF_DROPOFF = 10;
const MF_PICKUP = 11;
const MF_NOCLIP = 12;
const MF_SLIDE = 13;
const MF_FLOAT = 14;
const MF_TELEPORT = 15;
const MF_MISSILE = 16;
const MF_DROPPED = 17;
const MF_SHADOW = 18;
const MF_NOBLOOD = 19;
const MF_CORPSE = 20;
const MF_INFLOAT = 21;
const MF_COUNTKILL = 22;
const MF_COUNTITEM = 23;
const MF_SKULLFLY = 24;
const MF_NOTDMATCH = 25;
const MF_TRANSLATION = 26;
const MF_TOUCHY = 28;
const MF_BOUNCES = 29;
const MF_FRIEND = 30;
const MF_TRANSLUCENT = 31;



// Player property types
const PROP_SPEED = 0;
const PROP_JUMPHEIGHT = 1;
const PROP_LOCKED = 2;


// Line flags
const ML_BLOCKING = 0;        // Solid, blocks things
const ML_BLOCKMONSTERS = 1;   // Blocks monsters
const ML_TWOSIDED = 2;        // Is two sided
const ML_DONTPEGTOP = 3;      // Is upper-unpegged
const ML_DONTPEGBOTTOM = 4;   // Is lower-unpegged
const ML_SECRET = 5;          // Is secret
const ML_SOUNDBLOCK = 6;      // Blocks sound
const ML_DONTDRAW = 7;        // Don't draw on the automap at all.
const ML_MAPPED = 8;          // Set if already seen, thus drawn in automap.
const ML_REPEAT_SPECIAL = 9;  // Special is repeatable
// TODO SPAC takes 3 bits here 
const ML_ALLTRIGGER = 13;     // If flag is set, anything can trigger the line.


// Corona or dynamic light numbers for use with setcorona
const PLASMA_L = 1;
const PLASMAEXP_L = 2;
const ROCKET_L = 3;
const ROCKETEXP_L = 4;
const BFG_L = 5;
const BFGEXP_L = 6;
const BLUETALL_L = 7;
const GREENTALL_L = 8;
const REDTALL_L = 9;
const BLUESMALL_L = 10;
const GREENSMALL_L = 11;
const REDSMALL_L = 12;
const TECHLAMP_L = 13;
const TECHLAMP2_L = 14;
const COLUMN_L = 15;
const CANDLE_L = 16;
const CANDLEABRE_L = 17;
const REDBALL_L = 18;
const GREENBALL_L = 19;
const ROCKET2_L = 20;
const FX03_L = 21;  // Shows up for sprite no. 224
const FX17_L = 22;  // Shows up for sprite no. 204
const FX00_L = 23;  // Shows up for sprite no. 210
const FX08_L = 24;  // 220
const FX04_L = 25;  // 219
const FX02_L = 26;  // 207
const WTRH_L = 27;  // 182
const SRTC_L = 28;  // 171
const CHDL_L = 29;  // 170
const KFR1_L = 30;  // 177

// Setcorona constants
const CORONA_TYPE = 0;
const CORONA_OFFX = 1;
const CORONA_OFFY = 2;
const CORONA_COLOR = 3;
const CORONA_SIZE = 4;
const LIGHT_COLOR = 5;
const LIGHT_RADIUS = 6;

// Corona sprites, for use with the CORONA_TYPE function
// bit flags
const CORONA_SPR = 1;
const DYNLIGHT_SPR = 2;
// types
const UNDEFINED_SPR = 0;
const LAMP_C = 32;     // 0x20
const FIRE_C = 48;     // 0x30
const LIGHT_C = 96;    // 0x60
const FIREFLY_C = 208; // 0xD0
const RANDOM_C = 224;  // 0xE0
const PULSE_C = 240;   // 0xF0
// combinations
const LIGHT_SPR = 3;
const ROCKET_SPR = 19; // 0x13
const LAMP_SPR = 35;   // 0x23
const FIRE_SPR = 51;   // 0x33
