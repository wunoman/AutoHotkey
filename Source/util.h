/*
AutoHotkey

Copyright 2003 Chris Mallett

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef util_h
#define util_h

#include "stdafx.h" // pre-compiled headers
#include "defines.h"

#define IS_SPACE_OR_TAB(c) (c == ' ' || c == '\t')
#define IS_SPACE_OR_TAB_OR_NBSP(c) (c == ' ' || c == '\t' || c == -96) // Use a negative to support signed chars.

//inline int iround(double x)  // Taken from someone's "Snippets".
//{
//	return (int)floor(x + ((x >= 0) ? 0.5 : -0.5));
//}


inline char *StrToTitleCase(char *aStr)
{
	if (!aStr) return aStr;
	char *aStr_orig = aStr;	
	for (bool convert_next_alpha_char_to_upper = true; *aStr; ++aStr)
	{
		if (IsCharAlpha(*aStr)) // Use this to better support chars from non-English languages.
		{
			if (convert_next_alpha_char_to_upper)
			{
				*aStr = (char)CharUpper((LPTSTR)(UCHAR)*aStr);
				convert_next_alpha_char_to_upper = false;
			}
			else
				*aStr = (char)CharLower((LPTSTR)(UCHAR)*aStr);
		}
		else
			if (isspace((UCHAR)*aStr))
				convert_next_alpha_char_to_upper = true;
		// Otherwise, it's a digit, punctuation mark, etc. so nothing needs to be done.
	}
	return aStr_orig;
}



inline size_t strnlen(char *aBuf, size_t aMax)
// Returns the length of aBuf or aMax, whichever is least.
// But it does so efficiently, in case aBuf is huge.
{
	if (!aMax || !aBuf || !*aBuf) return 0;
	size_t i;
	for (i = 0; aBuf[i] && i < aMax; ++i);
	return i;
}



inline char *StrChrAny(char *aStr, char *aCharList)
// Returns the position of the first char in aStr that is of any one of
// the characters listed in aCharList.  Returns NULL if not found.
// Update: Yes, this seems identical to strpbrk().  However, since the corresponding code would
// have to be added to the EXE regardless of which was used, there doesn't seem to be much
// advantage to switching (especially since if the two differ in behavior at all, things might
// get broken).  Another reason is the name "strpbrk()" is not as easy to remember.
{
	if (aStr == NULL || aCharList == NULL) return NULL;
	if (!*aStr || !*aCharList) return NULL;
	// Don't use strchr() because that would just find the first occurrence
	// of the first search-char, which is not necessarily the first occurrence
	// of *any* search-char:
	char *look_for_this_char;
	for (; *aStr; ++aStr) // It's safe to use the value-parameter itself.
		// If *aStr is any of the search char's, we're done:
		for (look_for_this_char = aCharList; *look_for_this_char; ++look_for_this_char)
			if (*aStr == *look_for_this_char)
				return aStr;  // Match found.
	return NULL; // No match.
}



inline char *omit_leading_whitespace(char *aBuf)
// While aBuf points to a whitespace, moves to the right and returns the first non-whitespace
// encountered.
{
	for (; IS_SPACE_OR_TAB(*aBuf); ++aBuf);
	return aBuf;
}



inline char *omit_leading_any(char *aBuf, char *aOmitList, size_t aLength)
// Returns the address of the first character in aBuf that isn't a member of aOmitList.
// But no more than aLength characters of aBuf will be considered.  If aBuf is composed
// entirely of omitted characters, the address of the char after the last char in the
// string will returned (that char will be the zero terminator unless aLength explicitly
// caused only part of aBuf to be considered).
{
	char *cp;
	for (size_t i = 0; i < aLength; ++i, ++aBuf)
	{
		// Check if the current char is a member of the omitted-char list:
		for (cp = aOmitList; *cp; ++cp)
			if (*aBuf == *cp) // Match found.
				break;
		if (!*cp) // No match found, so this character is not omitted, thus we immediately return it's position.
			return aBuf;
	}
	// Since the above didn't return, aBuf is the position of the zero terminator or (if aLength
	// indicated only a substring) the position of the char after the last char in the substring.
	return aBuf;
}



inline char *omit_trailing_whitespace(char *aBuf, char *aBuf_marker)
// aBuf_marker must be a position in aBuf (to the right of it).
// Starts at aBuf_marker and keeps moving to the left until a non-whitespace
// char is encountered.  Returns the position of that char.
{
	for (; aBuf_marker > aBuf && IS_SPACE_OR_TAB(*aBuf_marker); --aBuf_marker);
	return aBuf_marker;  // Can equal aBuf.
}



inline size_t omit_trailing_any(char *aBuf, char *aOmitList, char *aBuf_marker)
// aBuf_marker must be a position in aBuf (to the right of it).
// Starts at aBuf_marker and keeps moving to the left until a char that isn't a member
// of aOmitList is found.  The length of the remaining substring is returned.
// That length will be zero if the string consists entirely of omitted characters.
{
	char *cp;
	for (; aBuf_marker > aBuf; --aBuf_marker)
	{
		// Check if the current char is a member of the omitted-char list:
		for (cp = aOmitList; *cp; ++cp)
			if (*aBuf_marker == *cp) // Match found.
				break;
		if (!*cp) // No match found, so this character is not omitted, thus we immediately return.
			return (aBuf_marker - aBuf) + 1; // The length of the string when trailing chars are omitted.
	}
	// Since the above didn't return, aBuf_marker is now equal to aBuf.  If this final character is itself
	// a member of the omitted-list, the length returned will be zero.  Otherwise it will be 1:
	for (cp = aOmitList; *cp; ++cp)
		if (*aBuf_marker == *cp) // Match found.
			return 0;
	return 1;
}



inline size_t ltrim(char *aStr, size_t aLength = -1)
// Caller must ensure that aStr is not NULL.
// v1.0.25: Returns the length if it was discovered as a result of the operation, or aLength otherwise.
// This greatly improves the performance of PerformAssign().
// NOTE: THIS VERSION trims only tabs and spaces.  It specifically avoids
// trimming newlines because some callers want to retain those.
{
	if (!*aStr) return 0;
	char *ptr;
	// Find the first non-whitespace char (which might be the terminator):
	for (ptr = aStr; IS_SPACE_OR_TAB(*ptr); ++ptr);
	// v1.0.25: If no trimming needed, don't do the memmove.  This seems to make a big difference
	// in the performance of critical sections of the program such as PerformAssign():
	if (ptr > aStr)
	{
		if (aLength == -1)
			aLength = strlen(ptr); // Set aLength as new/trimmed length, for use below and also as the return value.
		memmove(aStr, ptr, aLength + 1); // +1 to include the '\0'.  memmove() permits source & dest to overlap.
	}
	return aLength;
}

inline size_t rtrim(char *aStr, size_t aLength = -1)
// Caller must ensure that aStr is not NULL.
// v1.0.25: Always returns the new length of the string.  This greatly improves the performance of
// PerformAssign().
// NOTE: THIS VERSION trims only tabs and spaces.  It specifically avoids trimming newlines because
// some callers want to retain those.
{
	if (!*aStr) return 0; // The below relies upon this check having been done.
	// It's done this way in case aStr just happens to be address 0x00 (probably not possible
	// on Intel & Intel-clone hardware) because otherwise --cp would decrement, causing an
	// underflow since pointers are probably considered unsigned values, which would
	// probably cause an infinite loop.  Extremely unlikely, but might as well try
	// to be thorough:
	if (aLength == -1)
		aLength = strlen(aStr); // Set aLength for use below and also as the return value.
	for (char *cp = aStr + aLength - 1; ; --cp, --aLength)
	{
		if (!IS_SPACE_OR_TAB(*cp))
		{
			*(cp + 1) = '\0';
			return aLength;
		}
		// Otherwise, it is a space or tab...
		if (cp == aStr) // ... and we're now at the first character of the string...
		{
			if (IS_SPACE_OR_TAB(*cp)) // ... and that first character is also a space or tab...
				*cp = '\0'; // ... so the entire string is made empty...
			return aLength; // ... and we return in any case.
		}
		// else it's a space or tab, and there are still more characters to check.  Let the loop
		// do its decrements.
	}
}

inline void rtrim_with_nbsp(char *aStr)
// Caller must ensure that aStr is not NULL.
// Same as rtrim but also gets rid of those annoying nbsp (non breaking space) chars that sometimes
// wind up on the clipboard when copied from an HTML document, and thus get pasted into the text
// editor as part of the code (such as the sample code in some of the examples).
{
	if (!*aStr) return;
	for (char *cp = aStr + strlen(aStr) - 1; ; --cp)
	{
		if (!IS_SPACE_OR_TAB_OR_NBSP(*cp))
		{
			*(cp + 1) = '\0';
			return;
		}
		if (cp == aStr)
		{
			if (IS_SPACE_OR_TAB_OR_NBSP(*cp))
				*cp = '\0';
			return;
		}
	}
}

inline size_t trim(char *aStr, size_t aLength = -1)
// Caller must ensure that aStr is not NULL.
// NOTE: THIS VERSION trims only tabs and spaces.  It specifically avoids
// trimming newlines because some callers want to retain those.
{
	aLength = ltrim(aStr, aLength);  // It may return -1 to indicate that it still doesn't know the length.
    return rtrim(aStr, aLength);
	// v1.0.25: rtrim() always returns the new length of the string.  This greatly improves the
	// performance of PerformAssign() and possibly other things.
}



// Transformation is the same in either direction because the end bytes are swapped
// and the middle byte is left as-is:
#define bgr_to_rgb(aBGR) rgb_to_bgr(aBGR)
inline COLORREF rgb_to_bgr(DWORD aRGB)
// Fancier methods seem prone to problems due to byte alignment or compiler issues.
{
	return RGB(GetBValue(aRGB), GetGValue(aRGB), GetRValue(aRGB));
}



inline bool IsHex(char *aBuf)
// Note: AHK support for hex ints reduces performance by only 10% for decimal ints, even in the tightest
// of math loops that have SetBatchLines set to -1.
{
	aBuf = omit_leading_whitespace(aBuf); // i.e. caller doesn't have to have ltrimmed.
	if (!*aBuf)
		return false;
	if (*aBuf == '-' || *aBuf == '+')
		++aBuf;
	// The "0x" prefix must be followed by at least one hex digit, otherwise it's not considered hex:
	#define IS_HEX(buf) (*buf == '0' && (*(buf + 1) == 'x' || *(buf + 1) == 'X') && isxdigit(*(buf + 2)))
	return IS_HEX(aBuf);
}



// A more complex macro is used for ATOI64(), since it is more often called from places where
// performance matters (e.g. ACT_ADD).  It adds about 500 bytes to the code size in exchance for
// a 8% faster math loops.  But it's probably about 8% slower when used with hex integers, but
// those are so rare that the speed-up seems worth the extra code size:
//#define ATOI64(buf) _strtoi64(buf, NULL, 0) // formerly used _atoi64()
#define ATOI64(buf) (IsHex(buf) ? _strtoi64(buf, NULL, 16) : _atoi64(buf))
#define ATOU64(buf) _strtoui64(buf, NULL, IsHex(buf) ? 16 : 10)

// Below has been updated because values with leading zeros were being intepreted as
// octal, which is undesirable.
//#define ATOI(buf) strtol(buf, NULL, 0) // Use zero as last param to support both hex & dec.
#define ATOI(buf) (IsHex(buf) ? strtol(buf, NULL, 16) : atoi(buf))
#define ATOU(buf) (strtoul(buf, NULL, IsHex(buf) ? 16 : 10))

// Unlike some Unix versions of strtod(), the VC++ version does not seem to handle hex strings
// such as "0xFF" automatically.  So this macro must check for hex because some callers rely on that.
// Also, it uses _strtoi64() vs. strtol() so that more of a double's capacity can be utilized:
#define ATOF(buf) (IsHex(buf) ? (double)_strtoi64(buf, NULL, 16) : atof(buf))

// Negative hex numbers need special handling, otherwise something like zero minus one would create
// a huge 0xffffffffffffffff value, which would subsequently not be read back in correctly as
// a negative number (but UTOA() doesn't need this since there can't be negatives in that case).
#define ITOA(value, buf) \
	{\
		if (g.FormatIntAsHex)\
		{\
			char *our_buf_temp = buf;\
			if (value < 0)\
				*our_buf_temp++ = '-';\
			*our_buf_temp++ = '0';\
			*our_buf_temp++ = 'x';\
			_itoa(value < 0 ? -(int)value : value, our_buf_temp, 16);\
		}\
		else\
			_itoa(value, buf, 10);\
	}
#define ITOA64(value, buf) \
	{\
		if (g.FormatIntAsHex)\
		{\
			char *our_buf_temp = buf;\
			if (value < 0)\
				*our_buf_temp++ = '-';\
			*our_buf_temp++ = '0';\
			*our_buf_temp++ = 'x';\
			_i64toa(value < 0 ? -(__int64)value : value, our_buf_temp, 16);\
		}\
		else\
			_i64toa(value, buf, 10);\
	}
#define UTOA(value, buf) \
	{\
		if (g.FormatIntAsHex)\
		{\
			*buf = '0';\
			*(buf + 1) = 'x';\
			_ultoa(value, buf + 2, 16);\
		}\
		else\
			_ultoa(value, buf, 10);\
	}
#define UTOA64(value, buf) \
	{\
		if (g.FormatIntAsHex)\
		{\
			*buf = '0';\
			*(buf + 1) = 'x';\
			_ui64toa(value, buf + 2, 16);\
		}\
		else\
			_ui64toa(value, buf, 10);\
	}



inline void strlcpy (char *aDst, const char *aSrc, size_t aDstSize)
// Same as strncpy() but guarantees null-termination of aDst upon return.
// No more than aDstSize - 1 characters will be copied from aSrc into aDst
// (leaving room for the zero terminator).
// This function is defined in some Unices but is not standard.  But unlike
// other versions, this one uses void for return value for reduced code size
// (since it's called in so many places).
{
	// Disabled for performance and reduced code size:
	//if (!aDst || !aSrc || !aDstSize) return aDstSize;  // aDstSize must not be zero due to the below method.
	strncpy(aDst, aSrc, aDstSize - 1);
	aDst[aDstSize - 1] = '\0';
}



//inline char *strcatmove(char *aDst, char *aSrc)
//// Same as strcat() but allows aSrc and aDst to overlap.
//// Unlike strcat(), it doesn't return aDst.  Instead, it returns the position
//// in aDst where aSrc was appended.
//{
//	if (!aDst || !aSrc || !*aSrc) return aDst;
//	char *aDst_end = aDst + strlen(aDst);
//	return (char *)memmove(aDst_end, aSrc, strlen(aSrc) + 1);  // Add 1 to include aSrc's terminator.
//}



#define DATE_FORMAT_LENGTH 14 // "YYYYMMDDHHMISS"
#define IS_LEAP_YEAR(year) ((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))

int GetYDay(int aMon, int aDay, bool aIsLeapYear);
int GetISOWeekNumber(char *aBuf, int aYear, int aYDay, int aWDay);
ResultType YYYYMMDDToFileTime(char *aYYYYMMDD, FILETIME &aFileTime);
ResultType YYYYMMDDToSystemTime(char *aYYYYMMDD, SYSTEMTIME &aSystemTime, bool aDoValidate);
char *FileTimeToYYYYMMDD(char *aBuf, FILETIME &aTime, bool aConvertToLocalTime = false);
char *SystemTimeToYYYYMMDD(char *aBuf, SYSTEMTIME &aTime);
__int64 YYYYMMDDSecondsUntil(char *aYYYYMMDDStart, char *aYYYYMMDDEnd, bool &aFailed);
__int64 FileTimeSecondsUntil(FILETIME *pftStart, FILETIME *pftEnd);

enum SymbolType // For use with ExpandExpression() and IsPureNumeric().
{
	// The sPrecedence array in ExpandExpression() must be kept in sync with any additions, removals,
	// or re-ordering of the below.  Also, callers rely on PURE_NOT_NUMERIC being zero/false,
	// so that should be listed first.  Finally, IS_OPERAND() relies on all operand types being
	// at the beginning of the list:
	  PURE_NOT_NUMERIC, PURE_INTEGER, PURE_FLOAT
	, SYM_STRING = PURE_NOT_NUMERIC, SYM_INTEGER = PURE_INTEGER, SYM_FLOAT = PURE_FLOAT // Specific operand types.
	, SYM_OPERAND // Generic/undetermined type of operand.
	, SYM_OPERAND_END // Marks the symbol after the last operand.  This value is used below.
	, SYM_BEGIN = SYM_OPERAND_END  // SYM_BEGIN is a special marker to simplify the code.
	, SYM_OPAREN, SYM_CPAREN  // Open and close parentheses.
	, SYM_OR, SYM_AND, SYM_LOWNOT  // LOWNOT is the word "not", the low precedence counterpart of !
	, SYM_EQUAL, SYM_EQUALCASE, SYM_NOTEQUAL // =, ==, <>
	, SYM_GT, SYM_LT, SYM_GTOE, SYM_LTOE  // >, <, >=, <=
	, SYM_BITOR // Seems more intuitive to have these higher in prec. than the above, unlike C and Perl, but like Python.
	, SYM_BITXOR
	, SYM_BITAND
	, SYM_BITSHIFTLEFT, SYM_BITSHIFTRIGHT // << >>
	, SYM_PLUS, SYM_MINUS
	, SYM_TIMES, SYM_DIVIDE
	, SYM_NEGATIVE, SYM_HIGHNOT, SYM_BITNOT // Unary minus (unary plus is handled without needing a value here), !, and ~.
	, SYM_POWER    // See below for why this takes precedence over negative.
	, SYM_COUNT    // Must be last.
};
#define IS_OPERAND(symbol) (symbol < SYM_OPERAND_END)

struct map_item
{
	#define EXP_RAW          0  // The "5 + " in the following: 5 + y - %z% * Array%i%
	#define EXP_DEREF_SINGLE 1  // The y in the above.
	#define EXP_DEREF_DOUBLE 2  // The %z% and %i% in the above.
	int type;
	char *marker;
};
struct ExprTokenType  // Something in the compiler hates the name TokenType, so using a different name.
{
	SymbolType symbol;
	union {__int64 value_int64; double value_double; char *marker;}; // Depends on the value of symbol, above.
};

SymbolType IsPureNumeric(char *aBuf, bool aAllowNegative = false
	, bool aAllowAllWhitespace = true, bool aAllowFloat = false, bool aAllowImpure = false);

int snprintf(char *aBuf, size_t aBufSize, const char *aFormat, ...);
int snprintfcat(char *aBuf, size_t aBufSize, const char *aFormat, ...);
// Not currently used by anything, so commented out to possibly reduce code size:
//int strlcmp (char *aBuf1, char *aBuf2, UINT aLength1 = UINT_MAX, UINT aLength2 = UINT_MAX);
int strlicmp(char *aBuf1, char *aBuf2, UINT aLength1 = UINT_MAX, UINT aLength2 = UINT_MAX);
char *strrstr(char *aStr, char *aPattern, bool aCaseSensitive = true, int aOccurrence = 1);
char *strcasestr (const char *phaystack, const char *pneedle);
char *StrReplace(char *aBuf, char *aOld, char *aNew, bool aCaseSensitive = true);
char *StrReplaceAll(char *aBuf, char *aOld, char *aNew, bool aAlwaysUseSlow, bool aCaseSensitive = true
	, UINT aReplacementsNeeded = UINT_MAX); // Caller can provide this value to avoid having to calculate it again.
char *StrReplaceAllSafe(char *aBuf, size_t aBuf_size, char *aOld, char *aNew, bool aCaseSensitive = true);
char *TranslateLFtoCRLF(char *aString);
bool DoesFilePatternExist(char *aFilePattern);
#ifdef _DEBUG
	ResultType FileAppend(char *aFilespec, char *aLine, bool aAppendNewline = true);
#endif
char *ConvertFilespecToCorrectCase(char *aFullFileSpec);
char *FileAttribToStr(char *aBuf, DWORD aAttr);
unsigned __int64 GetFileSize64(HANDLE aFileHandle);
char *GetLastErrorText(char *aBuf, size_t aBuf_size);
void AssignColor(char *aColorName, COLORREF &aColor, HBRUSH &aBrush);
COLORREF ColorNameToBGR(char *aColorName);
HRESULT MySetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);
//HRESULT MyEnableThemeDialogTexture(HWND hwnd, DWORD dwFlags);
char *ConvertEscapeSequences(char *aBuf, char aEscapeChar);
POINT CenterWindow(int aWidth, int aHeight);
bool FontExist(HDC aHdc, char *aTypeface);
void GetVirtualDesktopRect(RECT &aRect);
ResultType RegReadString(HKEY aRootKey, char *aSubkey, char *aValueName, char *aBuf, size_t aBufSize);
HBITMAP LoadPicture(char *aFilespec, int aWidth, int aHeight, int &aImageType, int aIconIndex
	, bool aUseGDIPlusIfAvailable);
int CALLBACK FontEnumProc(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD FontType, LPARAM lParam);
bool IsStringInList(char *aStr, char *aList, bool aFindExactMatch, bool aCaseSensitive);

#endif
