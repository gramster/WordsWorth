#define SERIAL	"2.0-7"

char Register[] = {	(char)('['|0x80),
#ifdef REGISTERED
	(char)('D'|0x80),
	(char)('e'|0x80),
	(char)('e'|0x80),
	(char)(' '|0x80),
	(char)('M'|0x80),
	(char)('c'|0x80),
	(char)('K'|0x80),
	(char)('e'|0x80),
	(char)('e'|0x80),
#else
	(char)('U'|0x80),
	(char)('N'|0x80),
	(char)('R'|0x80),
	(char)('E'|0x80),
	(char)('G'|0x80),
	(char)('I'|0x80),
	(char)('S'|0x80),
	(char)('T'|0x80),
	(char)('E'|0x80),
	(char)('R'|0x80),
	(char)('E'|0x80),
	(char)('D'|0x80),
#endif
	(char)(']'|0x80),
	(char)0x80,
	0
};
