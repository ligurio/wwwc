/***************************************************************************
                          speller.h  -  spelling, ban, html cheker header
                             -------------------
    begin                : Mon Mar 19 2001
    copyright            : (C) 2001 by Alexander Bilichenko
    email                : pricer@mail.ru
 ***************************************************************************/

#ifndef SPELLER_H_INCLUDED
#define SPELLER_H_INCLUDED

#include "basetypes.h"
#include "profiles.h"
#include "dbase.h"

/* error codes for CheckSpellingBan() */
#define MSG_CHK_ERROR_PASSED			1
#define MSG_CHK_ERROR_NONAME			2
//#define MSG_CHK_ERROR_NOEMAIL			3
#define MSG_CHK_ERROR_NOMSGHEADER		4
#define MSG_CHK_ERROR_NOMSGBODY			5
#define MSG_CHK_ERROR_BADSPELLING		6
#define MSG_CHK_ERROR_BANNED			7
#define MSG_CHK_ERROR_CLOSED			8
#define MSG_CHK_ERROR_INVALID_REPLY		9
#define MSG_CHK_ERROR_INVALID_PASSW		10
#define MSG_CHK_ERROR_ROBOT_MESSAGE		11

#define PROFILE_CHK_ERROR_ALLOK					1
#define PROFILE_CHK_ERROR_ALREADY_EXIST			2
#define PROFILE_CHK_ERROR_NOT_EXIST				3
#define PROFILE_CHK_ERROR_INVALID_LOGIN_SPELL	4
#define PROFILE_CHK_ERROR_INVALID_PASSWORD		5
#define PROFILE_CHK_ERROR_INVALID_PASSWORD_REP	6
#define PROFILE_CHK_ERROR_SHORT_PASSWORD		7
#define PROFILE_CHK_ERROR_INVALID_EMAIL			8
#define PROFILE_CHK_ERROR_CANNOT_DELETE_USR		9
#define PROFILE_CHK_ERROR_UNKNOWN_ERROR			10

#define SPELLER_FILTER_HTML						0x0002
#define SPELLER_PARSE_TAGS						0x0001

/* bit mask of CFlags format in CheckSpellingBan() */
#define MSG_CHK_DISABLE_WWWCONF_TAGS			0x0001
#define MSG_CHK_DISABLE_SMILE_CODES				0x0002
#define MSG_CHK_ENABLE_EMAIL_ACKNL				0x0004
#define MSG_CHK_ALLOW_HTML						0x0008
#define MSG_CHK_DISABLE_SIGNATURE				0x0010

#define SPELLER_INTERNAL_BUFFER_SIZE			10000

/* code string to http format, if allocmem = 1 - function will allocate memory for you,
 * otherwise internal buffer will be used (10K buffer) */
char* CodeHttpString(char *s, int allocmem = 1);

/* check email (with current #define settings) */
int IsMailCorrect(char *s);

/* Preprare message under WIN32 for preview - actually filter char #10 */
void FilterMessageForPreview(char *s, char **dd);

/* filter html tags, if allocmem = 1 - function will allocate memory for you,
 * otherwise internal buffer will be used (10K buffer) */
char* FilterHTMLTags(char *s, WORD ml, int allocmem = 1);

char* FilterWhitespaces(char *s);

/* prepare every text in this board to be printed to browser */
int PrepareTextForPrint(char *msg, char **res, BYTE security, int flags, int spfl = SPELLER_FILTER_HTML | SPELLER_PARSE_TAGS);

/* check message for correct and check HTML Tags, bad words list, and banned user */
int CheckSpellingBan(struct SMessage *mes, char **body, char **Reason, 
					 DWORD CFlags, DWORD *RetFlags, bool fRegged = true);

#endif
