#ifndef PATCH_H
#define PATCH_H

#define FILE_PATCH	255
#define INSERT_PATCH	254
#define COPY_PATCH	253
#define EXEC_PATCH	252
#define NEW_PATCH	251
#define DEL_PATCH	250
#define ATTR_PATCH	249
#define REF_PATCH	248

// Reserved for future use

#define RES2_PATCH	247
#define RES3_PATCH	246
#define RES4_PATCH	245
#define RES5_PATCH	244
#define RES6_PATCH	243
#define RES7_PATCH	242
#define RES8_PATCH	241

#define IS_LEN_PATCH(a)		((a)>=128 && (a)<=240)
#define LEN_PATCH(a)		((a)+127)
#define LEN_PATCH_LEN(a)	((a)-127)
#define MAX_LEN_PATCH_LEN	(240-128+1)
#define IS_ASCII_PATCH(a)	(/*(a)>=0 &&*/ (a)<128)

#endif









