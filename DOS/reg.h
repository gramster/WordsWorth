#define SERIAL	"1.3-138"

char Register[] = {	(char)('['|0x80),
#ifdef REGISTERED
	(char)('L'|0x80),
	(char)('a'|0x80),
	(char)('r'|0x80),
	(char)('r'|0x80),
	(char)('y'|0x80),
	(char)(' '|0x80),
	(char)('R'|0x80),
	(char)('u'|0x80),
	(char)('s'|0x80),
	(char)('s'|0x80),
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
