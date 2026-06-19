#pragma once

namespace Wink
{
	using Code = i32;
	using Flags = i32;

	namespace Key
	{
		/* --- Printable keys --- */
		constexpr Code Space = 32;
		constexpr Code Apostrophe = 39;		// '
		constexpr Code Comma = 44;			// ,
		constexpr Code Minus = 45;			// -
		constexpr Code Period = 46;			// .
		constexpr Code Slash = 47;			// /
		constexpr Code Num0 = 48;
		constexpr Code Num1 = 49;
		constexpr Code Num2 = 50;
		constexpr Code Num3 = 51;
		constexpr Code Num4 = 52;
		constexpr Code Num5 = 53;
		constexpr Code Num6 = 54;
		constexpr Code Num7 = 55;
		constexpr Code Num8 = 56;
		constexpr Code Num9 = 57;
		constexpr Code Semicolon = 59;		// ;
		constexpr Code Equal = 61;			// =
		constexpr Code A = 65;
		constexpr Code B = 66;
		constexpr Code C = 67;
		constexpr Code D = 68;
		constexpr Code E = 69;
		constexpr Code F = 70;
		constexpr Code G = 71;
		constexpr Code H = 72;
		constexpr Code I = 73;
		constexpr Code J = 74;
		constexpr Code K = 75;
		constexpr Code L = 76;
		constexpr Code M = 77;
		constexpr Code N = 78;
		constexpr Code O = 79;
		constexpr Code P = 80;
		constexpr Code Q = 81;
		constexpr Code R = 82;
		constexpr Code S = 83;
		constexpr Code T = 84;
		constexpr Code U = 85;
		constexpr Code V = 86;
		constexpr Code W = 87;
		constexpr Code X = 88;
		constexpr Code Y = 89;
		constexpr Code Z = 90;
		constexpr Code LeftBracket = 91;		// [
		constexpr Code Backslash = 92;			/* \ */
		constexpr Code RightBracket = 93;		// ]
		constexpr Code GraveAccent = 96;		// `
		constexpr Code World1 = 161;			// non-US #1
		constexpr Code World2 = 162;			// non-US #2

		/* --- Function keys --- */
		constexpr Code Escape = 256;
		constexpr Code Enter = 257;
		constexpr Code Tab = 258;
		constexpr Code Backspace = 259;
		constexpr Code Insert = 260;
		constexpr Code Delete = 261;
		constexpr Code Right = 262;
		constexpr Code Left = 263;
		constexpr Code Down = 264;
		constexpr Code Up = 265;
		constexpr Code PageUp = 266;
		constexpr Code PageDown = 267;
		constexpr Code Home = 268;
		constexpr Code End = 269;
		constexpr Code CapsLock = 280;
		constexpr Code ScrollLock = 281;
		constexpr Code NumLock = 282;
		constexpr Code PrintScreen = 283;
		constexpr Code Pause = 284;
		constexpr Code F1 = 290;
		constexpr Code F2 = 291;
		constexpr Code F3 = 292;
		constexpr Code F4 = 293;
		constexpr Code F5 = 294;
		constexpr Code F6 = 295;
		constexpr Code F7 = 296;
		constexpr Code F8 = 297;
		constexpr Code F9 = 298;
		constexpr Code F10 = 299;
		constexpr Code F11 = 300;
		constexpr Code F12 = 301;
		constexpr Code F13 = 302;
		constexpr Code F14 = 303;
		constexpr Code F15 = 304;
		constexpr Code F16 = 305;
		constexpr Code F17 = 306;
		constexpr Code F18 = 307;
		constexpr Code F19 = 308;
		constexpr Code F20 = 309;
		constexpr Code F21 = 310;
		constexpr Code F22 = 311;
		constexpr Code F23 = 312;
		constexpr Code F24 = 313;
		constexpr Code F25 = 314;

		/* --- Numpad --- */
		constexpr Code KP0 = 320;
		constexpr Code KP1 = 321;
		constexpr Code KP2 = 322;
		constexpr Code KP3 = 323;
		constexpr Code KP4 = 324;
		constexpr Code KP5 = 325;
		constexpr Code KP6 = 326;
		constexpr Code KP7 = 327;
		constexpr Code KP8 = 328;
		constexpr Code KP9 = 329;
		constexpr Code KPDecimal = 330;
		constexpr Code KPDivide = 331;
		constexpr Code KPMultiply = 332;
		constexpr Code KPSubtract = 333;
		constexpr Code KPAdd = 334;
		constexpr Code KPEnter = 335;
		constexpr Code KPEqual = 336;

		/* --- Modifiers --- */
		constexpr Code LeftShift = 340;
		constexpr Code LeftControl = 341;
		constexpr Code LeftAlt = 342;
		constexpr Code LeftSuper = 343;
		constexpr Code RightShift = 344;
		constexpr Code RightControl = 345;
		constexpr Code RightAlt = 346;
		constexpr Code RightSuper = 347;
		constexpr Code Menu = 348;
	}

	namespace MouseButton
	{
		constexpr Code B1 = 0;
		constexpr Code B2 = 1;
		constexpr Code B3 = 2;
		constexpr Code B4 = 3;
		constexpr Code B5 = 4;
		constexpr Code B6 = 5;
		constexpr Code B7 = 6;
		constexpr Code B8 = 7;
		constexpr Code Left = B1;
		constexpr Code Right = B2;
		constexpr Code Middle = B3;
	}

	namespace Mod
	{
		constexpr Flags Shift = 0x0001;
		constexpr Flags Control = 0x0002;
		constexpr Flags Alt = 0x0004;
		constexpr Flags Super = 0x0008;
		constexpr Flags CapsLock = 0x0010;
		constexpr Flags NumLock = 0x0020;
	}
}