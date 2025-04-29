#include <string>
#include <stdint.h>
#include <format>
#include <iomanip>

template<typename Ch, typename T>
std::basic_string<Ch> to_hex_string(const T i){
	std::basic_stringstream<Ch> stream;
	stream << "0x" << std::setfill(u8'0') << std::setw(sizeof(T) * 2) << std::hex << i;
	return stream.str();
}

// Some very ugly hax to pretend to be a string literal, except its a secret flag!
#define BODGE_VAL(name, value)						\
	static const char8_t* name = (char8_t*) value;	\
	static const uintptr_t name##_ptr = (uintptr_t) value;

BODGE_VAL(ESC1, 0x1)
BODGE_VAL(ESC2, 0x2)
BODGE_VAL(ESC3, 0x3)
BODGE_VAL(VAR, 0x4)
BODGE_VAL(END, 0x6)

static const char8_t* emeraldCharLUT1[] = {
	u8" ", u8"À", u8"Á", u8"Â", u8"Ç", u8"È", u8"É", u8"Ê",		// 0x00 - 0x07
	u8"Ë", u8"Ì", NULL,  u8"Î", u8"Ï", u8"Ò", u8"Ó", u8"Ô",		// 0x08 - 0x0F
	u8"Œ", u8"Ù", u8"Ú", u8"Û", u8"Ñ", u8"ß", u8"à", u8"á",		// 0x10 - 0x17
	NULL,  u8"ç", u8"è", u8"é", u8"ê", u8"ë", u8"ì", NULL,		// 0x18 - 0x1F
	u8"î", u8"ï", u8"ò", u8"ó", u8"ô", u8"œ", u8"ù", u8"ú",		// 0x20 - 0x27
	u8"û", u8"ñ", u8"º", u8"ª", u8"ᵉʳ",u8"&", u8"+", NULL,		// 0x28 - 0x2F
	NULL,  NULL,  NULL,  NULL,  u8"Lv",u8"=", u8";", NULL,		// 0x30 - 0x37
	NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,		// 0x38 - 0x3F

	NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,		// 0x40 - 0x47
	NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,		// 0x48 - 0x4F
	u8"▯",u8"¿", u8"¡", u8"🇵​​🇰​​",u8"🇲​​🇳",u8"​🇵​​🇴",u8"🇰​​🇪",u8"🇧​​🇱​​", 	// 0x50 - 0x57
	u8"​🇴​​🇨​",u8"🇰​", u8"Í", u8"%", u8"(", u8")", NULL,  NULL,		// 0x58 - 0x5F
	NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,		// 0x60 - 0x67
	u8"â", NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  u8"í",		// 0x68 - 0x6F
	NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,		// 0x70 - 0x77
	NULL,  u8"↑", u8"↓", u8"←", u8"→", u8" ", u8" ", u8" ", 	// 0x78 - 0x7F

	u8" ", u8"  ",u8" ", u8" ", u8"ᵉ", u8"<", u8">", NULL,		// 0x80 - 0x87
	NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,		// 0x88 - 0x8F
	NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,		// 0x90 - 0x97
	NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,		// 0x98 - 0x9F
	u8"ʳᵉ",u8"0", u8"1", u8"2", u8"3", u8"4", u8"5", u8"6",		// 0xA0 - 0xA7
	u8"7", u8"8", u8"9", u8"!", u8"?", u8".", u8"-", u8"･",		// 0xA8 - 0xAF
	u8"‥", u8"“", u8"”", u8"‘", u8"'", u8"♂", u8"♀", u8"$", 	// 0xB0 - 0xB7
	u8",", u8"×", u8"/", u8"A", u8"B", u8"C", u8"D", u8"E",		// 0xB8 - 0xBF

	u8"F", u8"G", u8"H", u8"I", u8"J", u8"K", u8"L", u8"M", 	// 0xC0 - 0xC7
	u8"N", u8"O", u8"P", u8"Q", u8"R", u8"S", u8"T", u8"U",		// 0xC8 - 0xCF
	u8"V", u8"W", u8"X", u8"Y", u8"Z", u8"a", u8"b", u8"c",  	// 0xD0 - 0xD7
	u8"d", u8"e", u8"f", u8"g", u8"h", u8"i", u8"j", u8"k",		// 0xD8 - 0xDF
	u8"l", u8"m", u8"n", u8"o", u8"p", u8"q", u8"r", u8"s",  	// 0xE0 - 0xE7
	u8"t", u8"u", u8"v", u8"w", u8"x", u8"y", u8"z", u8"►", 	// 0xE8 - 0xEF
	u8":", u8"Ä", u8"Ö", u8"Ü", u8"ä", u8"ö", u8"ü", u8"ᴰᴬᵀᴬ",	// 0xF0 - 0xF7
	ESC3,  ESC1,  u8"⇓", u8"⇓",  ESC2, VAR,   u8"␤",END,		// 0xF8 - 0xFF
};

static const char8_t* emeraldCharLUT2[] = {
	u8"↑", u8"↓", u8"←", u8"→", u8"＋",u8"​🇱​​🇻",u8"🇵​​🇵",u8"​🇮​​🇩​",		// 0x00 - 0x07
	u8"🇳​​º",u8"＿",u8"①", u8"②", u8"③", u8"④", u8"⑤",u8"⑥",		// 0x08 - 0x0F
	u8"⑦", u8"⑧", u8"⑨", u8"（",u8"）",u8"◎",u8"△",u8"×",		// 0x10 - 0x17
};

static const char8_t* emeraldCharLUT3[] = {
	u8"_",  u8"|", u8"‾", u8"~", u8"(",  u8")", u8"⊂", u8">",		// 0xD0 - 0xD7
	u8">",  u8"<", u8"@", u8":", u8"+",  u8"-", u8"=", u8"@",		// 0xD8 - 0xDF
	u8"𝅓",  u8"▵", u8"´", u8"`", u8"⚫",u8"⏷", u8"◼",u8"♥",		// 0xE0 - 0xE7
	u8"🌙",u8"♪", u8"◓",u8"🗲",u8"🙒", u8"🔥",u8"💧",u8"🤛",		// 0xE8 - 0xEF
	u8"🤜",u8"⁕", u8"👁", u8"👁", u8"💢",u8"🤫",u8"😀",u8"😠",		// 0xF0 - 0xF7
	u8"😲",u8"😄",u8"😈",u8"😫",u8"😐",u8"😮",u8"😡",  END,		// 0xF8 - 0xFF
};

static const char8_t* emeraldCharLUT4[] = {
	u8"Ⓐ", u8"Ⓑ", u8"Ⓛ", u8"Ⓡ", u8"𝗦𝗧𝗔𝗥𝗧",u8"𝗦𝗘𝗟𝗘𝗖𝗧",
	u8"↑",u8"↓",u8"←", u8"→", u8"↕", u8"↔", u8" ",
};

static bool tryAppendEmeraldCharExtToUTF8(std::u8string& str, const uint8_t*& ch) {
	const uint8_t result = *ch;

	if(result < sizeof(emeraldCharLUT2)) {
		// Char is part of LUT2
		str += emeraldCharLUT2[result];
		ch++;
		return true;
	}

	if(result >= 0x100 - sizeof(emeraldCharLUT3)) {
		// Char is part of LUT3
		const char8_t index = result - (0x100 - sizeof(emeraldCharLUT3));

		if(emeraldCharLUT3[index] != END) {
			// If the character is end of string, then the result must be invalid somehow
			str += emeraldCharLUT3[index];
			ch++;
			return true;
		}
	}

	return false;
}

static bool tryAppendEmeraldCharExt2ToUTF8(std::u8string& str, const uint8_t*& ch) {
	const uint8_t result = *ch;

	if(result < sizeof(emeraldCharLUT4)) {
		// Char is part of LUT4
		str += emeraldCharLUT4[result];
		ch++;
		return true;
	}

	return false;
}

static bool tryPrintEmeraldFunction(std::u8string& str, const uint8_t*& ch) {
	switch(*ch++) {
		case 0x00:
			str += u8"​🇳​​🇴​​🇴​​🇵​()";
			return true;

		case 0x01:
			str += u8"​🇹​​🇽​​🇹​​🇨​​🇴​​🇱​​🇴​​🇷​(" + to_hex_string<char8_t>(*ch++) + u8")";
			return true;

		case 0x02:
			str += u8"​🇭​​🇮​​🇱​​🇮​​🇹​​🇪​​🇨​​🇴​​🇱​​🇴​​🇷​(" + to_hex_string<char8_t>(*ch++) + u8")";
			return true;

		case 0x03:
			str += u8"​🇸​​🇭​​🇦​​🇩​​🇴​​🇼​​🇨​​🇴​​🇱​​🇴​​🇷​(" + to_hex_string<char8_t>(*ch++) + u8")";
			return true;

		case 0x04:
			str += u8"​🇦​​🇱​​🇱​​🇨​​🇴​​🇱​​🇴​​🇷​(" + to_hex_string<char8_t>(*ch++);
			str += to_hex_string<char8_t>(*ch++) + u8", ";
			str += to_hex_string<char8_t>(*ch++) + u8", ";
			str += to_hex_string<char8_t>(*ch++) + u8", ";
			str += to_hex_string<char8_t>(*ch++) + u8")";
			return true;

		case 0x05:
			str += u8"​🇵​​🇦​​🇱​​🇪​​🇹​​🇹​​🇪​(" + to_hex_string<char8_t>(*ch++) + u8")";
			return true;

		case 0x06:
			str += u8"​🇫​​🇴​​🇳​​🇹​(" + to_hex_string<char8_t>(*ch++) + u8")";
			return true;

		case 0x07:
			str += u8"​🇫​​🇴​​🇳​​🇹​(​🇩​​🇪​​🇫​​🇦​​🇺​​🇱​​🇹​)";
			return true;

		case 0x08:
			str += u8"​🇵​​🇦​​🇺​​🇸​​🇪​(" + to_hex_string<char8_t>(*ch++) + u8")";
			return true;

		case 0x09:
			str += u8"​🇼​​🇦​​🇮​​🇹​​🇯​​🇴​​🇾​(" + to_hex_string<char8_t>(*ch++) + u8")";
			return true;

		case 0x0A:
			str += u8"​🇼​​🇦​​🇮​​🇹​​🇸​​🇫​​🇽​(" + to_hex_string<char8_t>(*ch++) + u8")";
			return true;

		case 0x0B: {
			uint16_t id = (*ch++) | ((*ch++) << 8);
			str += u8"​🇲​​🇺​​🇸​​🇮​​🇨​(" + to_hex_string<char8_t>(id) + u8")";
			return true;
		}

		case 0x10: {
			uint16_t id = *ch++ | (*ch++ << 8);
			str += u8"🇸​​🇫​​🇽​(" + to_hex_string<char8_t>(id) + u8")";
			return true;
		}

		case 0x0D: case 0x12:
			str += u8"​🇨​​🇺​​🇷​​🇸​​🇴​​🇷​​🇽​(" + to_hex_string<char8_t>(*ch++) + u8")";
			return true;

		case 0x0E:
			str += u8"​🇨​​🇺​​🇷​​🇸​​🇴​​🇷​​🇾​(" + to_hex_string<char8_t>(*ch++) + u8")";
			return true;

		case 0x0F:
			str += u8"​🇨​​🇱​​🇪​​🇦​​🇷()";
			return true;

		case 0x11:
			str += u8"​🇸​​🇵​​🇦​​🇨​​🇪​(" + to_hex_string<char8_t>(*ch++) + u8")";
			return true;

		case 0x13:
			str += u8"​​🇨​​🇱​​🇪​​🇦​​🇷​​🇽​(" + to_hex_string<char8_t>(*ch++) + u8")";
			return true;

		case 0x14:
			str += u8"​🇲​​🇮​​🇳​​🇼​(" + to_hex_string<char8_t>(*ch++) + u8")";
			return true;

		case 0x15:
			str += u8"​🇯​​🇵​​🇳​()";
			return true;

		case 0x16:
			str += u8"🇮​​🇳​​🇹​​🇱()";
			return true;

		case 0x17:
			str += u8"​🇲​​🇺​​🇸​​🇮​​🇨​(​🇵​​🇦​​🇺​​🇸​​🇪)";
			return true;

		case 0x18:
			str += u8"​🇲​​🇺​​🇸​​🇮​​🇨​(🇵​​🇱​​🇦​​🇾)";
			return true;

		default:	// unknown function
			--ch;
			return false;
	}
}

bool appendEmeraldCharToUTF8(std::u8string& str, const uint8_t*& ch) {
	const char8_t* result = emeraldCharLUT1[*ch++];

	if((uintptr_t) result > 0x10) {
		// This was a regular characters, so we can just encode it directly
		str += result;
		return true;
	}

	// uh oh, this is a special character code. result is actually a special ID below:
	switch((uintptr_t) result) {
		case END_ptr: return false;

		case ESC1_ptr:
			if(tryAppendEmeraldCharExtToUTF8(str, ch)) {
				return true;
			}
			goto fail;

		case ESC2_ptr:
			// Check if immediately followed by 0x0C
			if(*(ch + 1) == 0x0C) {
				ch++;
				// Check if this is a valid extended char
				if(tryAppendEmeraldCharExtToUTF8(str, ch)) {
					return true;
				}

				// OOPS - it wasn't. Undo whatever and just append invalid char marker.
				--ch;

			} else if(tryPrintEmeraldFunction(str, ch)){
				return true;
			}

			goto fail;

		case ESC3_ptr:
			if(tryAppendEmeraldCharExt2ToUTF8(str, ch)) {
				return true;
			}
			goto fail;

		case VAR_ptr:
			str += u8"🇻​​🇦​​🇷(" + to_hex_string<char8_t>(*ch++) + u8")";
			return true;

		default: fail:
			// This character should be invalid. Use special code point to identify it.
			str += u8"�";
			return true;
	}
}

std::u8string EmeraldStringToUTF8(const uint8_t* str) {
	std::u8string result;
	while(appendEmeraldCharToUTF8(result, str)) ;

	return result;
}
