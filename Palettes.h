//+--------------------------------------------------------------------------
//
// SoundFrameIQR - (c) 2018 Dave Plummer.  All Rights Reserved.
//
// File:        Palettes.h
//
// Description:
//
//   Predefined palette color schemes to be used by the "Scheme" knob
//   to control what colors the bars are in the scpectrum analyzer
//
// History:     Oct-02-2018         Davepl      Created/Documented
//
//---------------------------------------------------------------------------
//    
//                  XXXX
//                 X    XX
//                X  ***  X                XXXXX
//               X  *****  X            XXX     XX
//            XXXX ******* XXX      XXXX          XX
//          XX   X ******  XXXXXXXXX                XX XXX
//        XX      X ****  X                           X** X
//       X        XX    XX     X                      X***X
//      X         //XXXX       X                      XXXX
//     X         //   X                             XX
//    X         //    X          XXXXXXXXXXXXXXXXXX/
//    X     XXX//    X          X
//    X    X   X     X         X
//    X    X    X    X        X
//     X   X    X    X        X                    XX
//     X    X   X    X        X                 XXX  XX
//      X    XXX      X        X               X  X X  X
//      X             X         X              XX X  XXXX
//       X             X         XXXXXXXX/     XX   XX  X
//        XX            XX              X     X    X  XX
//          XX            XXXX   XXXXXX/     X     XXXX
//            XXX             XX***         X     X
//               XXXXXXXXXXXXX *   *       X     X
//                            *---* X     X     X
//                           *-* *   XXX X     X
//                           *- *       XXX   X
//                          *- *X          XXX
//                          *- *X  X          XXX
//                         *- *X    X            XX
//                         *- *XX    X             X
//                        *  *X* X    X             X
//                        *  *X * X    X             X
//                       *  * X**  X   XXXX          X
//                       *  * X**  XX     X          X
//                      *  ** X** X     XX          X
//                     *  **  X*  XXX   X         X
//                     *  **    XX   XXXX       XXX
//                    *  * *      XXXX      X     X
//                   *   * *          X     X     X
//     >>>>>>>*******   * *           X     X      XXXXXXXX/
//            *         * *      /XXXXX      XXXXXXXX/      <
//       >>>>>**********  *     X                     <  /  <
//         >>>>*         *     X               /  /   <XXXXX
//    >>>>>>>>>**********       XXXXXXXXXXXXXXXXXXXXXX
//
//---------------------------------------------------------------------------




























































#pragma once

DEFINE_GRADIENT_PALETTE( rainbowsherbet_gp ) {
    0, 255, 33,  4,
   43, 255, 68, 25,
   86, 255,  7, 25,
  127, 255, 82,103,
  170, 255,255,242,
  209,  42,255, 22,
  255,  87,255, 65
};


DEFINE_GRADIENT_PALETTE( Colorfull_gp ) {
    0,  10, 85,  5,
   25,  29,109, 18,
   60,  59,138, 42,
   93,  83, 99, 52,
  106, 110, 66, 64,
  109, 123, 49, 65,
  113, 139, 35, 66,
  116, 192,117, 98,
  124, 255,255,137,
  168, 100,180,155,
  255,  22,121,174
};

DEFINE_GRADIENT_PALETTE( vu_gp ) 
{
      0,     0,   4,   0,   // near black green
     64,     0, 255,   0,   // green
    128,   255, 255,   0,   // yellow
    192,   255,   0,   0,   // red
    255,   255,   0,   0    // red
};
CRGBPalette256 vuPalette256 = vu_gp;

DEFINE_GRADIENT_PALETTE( yellowColors_gp ) 
{
      0,   248,   0,   0,
     64,   248, 220, 103,   
    192,   248, 220, 103,   
    255,   248,   0,   0,   
};
CRGBPalette256 yellowColors = yellowColors_gp;

const CRGBPalette256 bandColors  =
{
	CRGB(0xFD0E35),                     // Red
	CRGB(0xFF8833),                     // Orange
	CRGB(0xFFEB00),                     // Middle Yellow
	CRGB(0xAFE313),                     // Inchworm
    CRGB(0x3AA655),                     // Green
    CRGB(0x8DD9CC),                     // Middle Blue Green
    CRGB(0x0066FF),                     // Blue III
    CRGB(0xDB91EF),                     // Lilac
    CRGB(0xFD0E35),                     // Red
	CRGB(0xFF8833),                     // Orange
	CRGB(0xFFEB00),                     // Middle Yellow
	CRGB(0xAFE313),                     // Inchworm
    CRGB(0x3AA655),                     // Green
    CRGB(0x8DD9CC),                     // Middle Blue Green
    CRGB(0x0066FF),                     // Blue III
    CRGB(0xDB91EF)                      // Lilac
};

const CRGBPalette256 USAColors  =
{
	CRGB::Blue,	 						
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Red,
    CRGB::White,
    CRGB::Red,
    CRGB::White,
    CRGB::Red,
    CRGB::White,
    CRGB::Red,
    CRGB::White,
    CRGB::Red,
    CRGB::White,
    CRGB::Red,
};

const CRGBPalette256 CanadaColors  =
{
    CRGB::Red,
    CRGB::Red,
    CRGB::Red,
    CRGB::Red,

    CRGB::White,
    CRGB::White,
    CRGB::White,
    CRGB::Red,
    CRGB::Red,
    CRGB::White,
    CRGB::White,
    CRGB::White,

    CRGB::Red,
    CRGB::Red,
    CRGB::Red,
    CRGB::Red,
};

const CRGBPalette256 blueColors  =
{
    CRGB::Blue,
    CRGB::DarkBlue,
    CRGB::DarkBlue,
    CRGB::Blue,

    CRGB::Blue,
    CRGB::DarkBlue,
    CRGB::DarkBlue,
    CRGB::DarkBlue,

    CRGB::Blue,
    CRGB::DarkBlue,
    CRGB::SkyBlue,
    CRGB::SkyBlue,

    CRGB::LightBlue,
    CRGB::SkyBlue,
    CRGB::LightBlue,
    CRGB::SkyBlue
};

const CRGBPalette256 redColors  =
{
    CRGB::Maroon,
    CRGB::Maroon,
    CRGB::Maroon,
    CRGB::Maroon,

    CRGB::Yellow,
    CRGB::Orange,
    CRGB::Red,
    CRGB::DarkRed,

    CRGB::DarkRed,
    CRGB::DarkRed,
    CRGB::Red,
    CRGB::Orange,

    CRGB::Yellow,
    CRGB::Orange,
    CRGB::Red,
    CRGB::DarkRed
};

const CRGBPalette256 greenColors  =
{
    CRGB(0x126412),
    CRGB(0x32CD32),
    CRGB(0x90EE90),
    CRGB(0x006400),

    CRGB(0x126412),
    CRGB(0x32CD32),
    CRGB(0x90EE90),
    CRGB(0x006400),

    CRGB(0x126412),
    CRGB(0x32CD32),
    CRGB(0x90EE90),
    CRGB(0x006400),

    CRGB(0x126412),
    CRGB(0x32CD32),
    CRGB(0x90EE90),
    CRGB(0x006400)
};

const CRGBPalette256 purpleColors  =
{
    CRGB(0x8F47B3),
    CRGB(0xC9A0DC),
    CRGB(0xBF8FCC),
    CRGB(0x803790),
    CRGB(0x733380),
    CRGB(0xD6AEDD),
    CRGB(0xC154C1),
    CRGB(0xFC74FD),
    CRGB(0x732E6C),
    CRGB(0xE667CE),
    CRGB(0xE29CD2),
    CRGB(0x8E3179),
    CRGB(0xD96CBE),
    CRGB(0xEBB0D7),
    CRGB(0xC8509B),
    CRGB(0xBB3385)
};

const CRGBPalette256 allPalettes[] = 
{
    bandColors,                                         
    blueColors,                                         
    redColors,      
    greenColors,
    purpleColors,
    yellowColors,                                       
    CRGBPalette256(Colorfull_gp),                       // Green brown and earthy
    CRGBPalette256(rainbowsherbet_gp),                  // Red and green watermelon
    CRGBPalette256(CanadaColors),                  
    CRGBPalette256(USAColors)                    
};