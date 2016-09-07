/***************************************************************************
                          boardtags.h  -  board tags support include
                             -------------------
    begin                : Sun Apr 29 2001
    copyright            : (C) 2001 by Alexander Bilichenko
    email                : pricer@mail.ru
 ***************************************************************************/

#ifndef BOARDTAGS_H_INCLUDED
#define BOARDTAGS_H_INCLUDED

#include "basetypes.h"

#define WC_TAG_OPEN	'['
#define WC_TAG_CLOSE	']'

#define TRY_AUTO_URL_PREPARSE	1
#define PARSED_URL_TMPL		"<A HREF=\"%s\" STYLE=\"text-decoration:underline;\" TARGET=_blank>%s</A>"

#define MAX_NESTED_TAGS			8

#define BOARDTAGS_CUT_TAGS		0x08000000
#define BOARDTAGS_TAG_PREPARSE	0x10000000
#define BOARDTAGS_EXPAND_ENTER	0x20000000
#define BOARDTAGS_PURL_ENABLE	0x40000000

#define BoardTagCount 18
#define BoardPicCount 52

#define URL_TAG_TYPE 10
#define PIC_TAG_TYPE 11
#define PRE_TAG_TYPE 16

#define WC_TAG_TYPE_DISABLED	0
#define WC_TAG_TYPE_1			1
#define WC_TAG_TYPE_2			2
#define WC_TAG_TYPE_12			3
#define WC_TAG_TYPE_ONLYOPEN	4

/* element of table for converting WWWConf Tags to HTML */
struct STagConvert {
	/* board tag */
	char *tag;
	/* corresponding opening and closing HTML tags */
	char *topentag;
	char typeopen;
	char *tclosetag;
	char typeclose;
	/* security level of tag
	 * zero - is highest level
	 */
	BYTE security;
};

struct SPicConvert {
	/* board code */
	char *tag;
	/* Pic URL */
	char *url;
	/* security level of smile
	 * zero - is highest level
	 */
	BYTE security;
};

/* Struct for saving last opened tag */
struct SSavedTag {
	/* tag type and tag length */
	int tt, tl;
	
	/* tag old and expanded expression */
	char *tagexp;
	char *oldexp;
	/* insert expanded expression index */
	int index;
};

int FilterBoardTags(char *s, char **r, BYTE security, DWORD ml, DWORD Flags, DWORD *RF);

#endif
