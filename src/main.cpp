/***************************************************************************
                          main.cpp  -  main module
                             -------------------
    begin                : Thu Mar 14 21:54:15 MSK 2001
    copyright            : (C) 2001 by Alexander Bilichenko
    email                : pricer@mail.ru

:set encoding=8bit-windows-1251
:set termencoding=8bit-windows-1251
:set termencoding=8bit-koi8-r
 ***************************************************************************/

//#ifdef HAVE_CONFIG_H
//#include <config.h>
//#endif

#include "basetypes.h"
#include "dbase.h"
#include "error.h"
#include "main.h"
#include "messages.h"
#include "speller.h"
//#include "design.h"
#include "security.h"
#include "boardtags.h"
#include "profiles.h"
#include "logins.h"
#include "searcher.h"
#include "sendmail.h"
#include "announces.h"
#include "colornick.h"
#include "activitylog.h"
#include "statfs.h" 


char *ConfTitle;

char *MESSAGEHEAD_timetypes[4] = {
	"���(�)",		// coded as "1"
	"���/����",		// coded as "2"
	"������(�)",	// coded as "3"
	"�����(��)"		// coded as "4"
};

#if TOPICS_SYSTEM_SUPPORT
char *Topics_List[TOPICS_COUNT] = {
	"��� ����",
	"��������",
	"����������������",
	"������",
	"�����",
	"����������",
	"�����",
	"������ �����",
	"�����������",
	"������",
	"Temp",
	"������",
	"Unix/Linux",
	"Windows",
	"�������� ����",
	"�������",
	"�����������",
	"��������/�������"
};
int Topics_List_map[TOPICS_COUNT] = {
	0, //"��� ����",
	4, //"�����",
	3, //"������",
	1, //"��������",
	5, //"����������",
	15,//"�������"
	8, //"�����������",
	7, //"������ �����",
	2, //"����������������",
	6, //"�����",
	9, //"������",
	11,//"������",
	13,//"Windows",
	12,//"Unix/Linux",
	14,//"�������� ����",
	16,//"�����������",
	17,//"lost/found",
	10,//"Temp",
};
#endif

char *UserStatus_List[USER_STATUS_COUNT] = {
	"�������",
	"�������",
	"�������",
	"����",
	"������"
};

char *UserRight_List[USERRIGHT_COUNT] = {
	"SUPERUSER",
	"VIEW_MESSAGE",
	"MODIFY_MESSAGE",
	"CLOSE_MESSAGE",
	"OPEN_MESSAGE",
	"CREATE_MESSAGE",
	"CREATE_MESSAGE_THREAD",
	"ALLOW_HTML",
	"PROFILE_MODIFY",
	"PROFILE_CREATE",
	"ROLL_MESSAGE",
	"UNROLL_MESSAGE",
	"POST_GLOBAL_ANNOUNCE",
	"ALT_DISPLAY_NAME"
};

int GlobalNewSession = 1;

enum {
	ACTION_POST,
	ACTION_PREVIEW,
	ACTION_EDIT
};

char* strget(char *par,const char *find, WORD maxl, char end, bool argparsing = true);

int getAction(char* par)
{
#define JS_POST_FIELD "jpost"
	int i = -1;
	char* szActions[] = {
		"post",
		"preview",
		"edit"
	};
	int cActions = sizeof(szActions) / sizeof(char*);
	char* st = strget(par, JS_POST_FIELD"=", MAX_STRING, '&');
	if(st && *st) {
		for(i = 0; i < cActions; i++)
			if(!strcmp(st, szActions[i]))
				break;
	} else {
		char buf[MAX_STRING];
		for(i = 0; i < cActions; i++)
		{
			sprintf(buf, "%s=", szActions[i]);
			st = strget(par, buf, MAX_STRING, '&');
			if(st)
				break;
		}
	}
	if(st)
		free(st);
	if(i < cActions)
		return i;
	return -1;
}

static void HttpRedirect(char *url)
{
	printf("Status: 302 Moved\n");
	printf("Pragma: no-cache\n");
	printf("Location: %s\n\n",url);
}

static void PrintBoardError(char *s1, char *s2, int code, DWORD msgid = 0)
{
	printf(DESIGN_GLOBAL_BOARD_MESSAGE, s1, s2);
	if(code & HEADERSTRING_REFRESH_TO_MAIN_PAGE)
		printf("%s", MESSAGEMAIN_browser_return);
	if(code & HEADERSTRING_REFRESH_TO_THREAD && msgid != 0)
		printf(MESSAGEMAIN_browser_to_thread, msgid);
}

static void PrintBanList  ()
{
	FILE *f;
	int readed;
	void *buf = malloc(MAX_HTML_FILE_SIZE);
	if((f = fopen(F_BANNEDIP, FILE_ACCESS_MODES_R)) == NULL) printhtmlerror();
	if((readed = fread(buf, 1, MAX_HTML_FILE_SIZE, f)) == 0) printhtmlerror();
	printf(DESIGN_BAN_FORM);
	if(fwrite(buf, 1, readed, stdout) != readed) printhtmlerror();
	printf(DESIGN_BAN_FORM2);
	free(buf);
	fclose(f);

}

/* Prints post message form
 */
static void PrintMessageForm(SMessage *msg, char *body, DWORD s, int code, DWORD flags = 0)
{
	char tstr[2][100];

	printf("<CENTER><FORM METHOD=POST NAME=\"postform\" onSubmit=\"return false;\" ACTION=\"%s?xpost=%lu\">",
			MY_CGI_URL, s);

	printf(DESIGN_POST_NEW_MESSAGE_TABLE "<TR><TD COLSPAN=2 ALIGN=CENTER><BIG>");
	if(code & ACTION_BUTTON_EDIT) printf(MESSAGEMAIN_post_editmessage);
	else if(code & ACTION_BUTTON_FAKEREPLY) printf("<A NAME=Reply>" MESSAGEMAIN_post_replymessage "</A>");
	else printf(MESSAGEMAIN_post_newmessage);

	printf("</BIG></TD></TR>");
	printf("<TR><TD COLSPAN=2><HR ALIGN=CENTER WIDTH=80%% NOSHADE></TR>");

	if(!(code & ACTION_BUTTON_EDIT)) {
		if(ULogin.LU.ID[0] == 0) {
			// print name/password form
			printf("<TR><TD ALIGN=CENTER>%s &nbsp;"
			"<INPUT TYPE=TEXT NAME=\"name\" SIZE=22 MAXLENGTH=%d VALUE=\"%s\"></TD>\n",
				MESSAGEMAIN_post_you_name, AUTHOR_NAME_LENGTH - 1, msg->AuthorName);
			// print password
			printf("<TD ALIGN=LEFT>%s <INPUT TYPE=PASSWORD NAME=\"pswd\" SIZE=22"
				" MAXLENGTH=%d VALUE=\"\">&nbsp;&nbsp;&nbsp;%s<INPUT TYPE=CHECKBOX NAME=\"lmi\"></TD></TR>\n",
				MESSAGEMAIN_post_your_password, PROFILES_MAX_PASSWORD_LENGTH - 1, MESSAGEMAIN_post_login_me);
		}
		else {
			// print name
			printf("<TR><TD ALIGN=CENTER>%s </TD><TD><B>%s</B>&nbsp;&nbsp;&nbsp;<A HREF=\"%s?login=logoff\"><SMALL>[%s] </SMALL></A></TD></TR>\n",
				MESSAGEMAIN_post_you_name, msg->AuthorName, MY_CGI_URL, MESSAGEHEAD_logoff);
		}
	}
	else {
		// print edit name and ip
		if(ULogin.LU.ID[0] != 0 && (ULogin.LU.right & USERRIGHT_SUPERUSER) != 0) {
			printf("<TR><TD ALIGN=CENTER>%s &nbsp;" \
				"<INPUT TYPE=TEXT NAME=\"name\" SIZE=29 MAXLENGTH=%d VALUE=\"%s\"></TD>" \
				"<TD>%s &nbsp;<INPUT TYPE=TEXT NAME=\"host\" SIZE=40 MAXLENGTH=%d" \
				" VALUE=\"%s\"></TD></TR>", MESSAGEMAIN_post_you_name, AUTHOR_NAME_LENGTH - 1,
				msg->AuthorName, MESSAGEMAIN_post_hostname, HOST_NAME_LENGTH - 1, msg->HostName);
		}
		
	}

#if TOPICS_SYSTEM_SUPPORT
	// subject and topic
	if(s == 0) {
		printf("<TR><TD ALIGN=CENTER>%s ", MESSAGEMAIN_post_message_subject);
		// Only for ROOT messages
		printf("<SELECT NAME=\"topic\">");
		for(DWORD i = 0; i < TOPICS_COUNT; i++) {
			if(Topics_List_map[i] == msg->Topics) {
				// define default choise
				printf("<OPTION VALUE=\"%d\"" LISTBOX_SELECTED ">%s\n",
					Topics_List_map[i], Topics_List[Topics_List_map[i]]);
			}
			else {
				printf("<OPTION VALUE=\"%d\">%s\n", Topics_List_map[i], Topics_List[Topics_List_map[i]]);
			}
		}
		printf("</SELECT></TD><TD>");
	}
	else {
		printf("<TR><TD COLSPAN=2 ALIGN=CENTER>%s ", MESSAGEMAIN_post_message_subject);
	}

	printf("<INPUT TYPE=TEXT NAME=\"subject\" SIZE=%d MAXLENGTH=%d VALUE=\"%s\"></TD></TR>\n",
		s? 88: 62, MESSAGE_HEADER_LENGTH - 1, msg->MessageHeader);

#else
	printf("<TR><TD COLSPAN=2 ALIGN=CENTER>%s ", MESSAGEMAIN_post_message_subject);
	printf("<INPUT TYPE=TEXT NAME=\"subject\" SIZE=88 MAXLENGTH=%d VALUE=\"%s\"></TD></TR>\n",
		MESSAGE_HEADER_LENGTH - 1, msg->MessageHeader);
#endif	 		

	printf("<TR><TD COLSPAN=2 ALIGN=CENTER><STRONG>%s</STRONG>\n",
		MESSAGEMAIN_post_message_body);
				   
	
	printf("</TD></TD></TR><TR><TD COLSPAN=2 ALIGN=CENTER><TEXTAREA COLS=75 ROWS=12 NAME=\"body\" WRAP=VIRTUAL>%s</TEXTAREA></TD></TR>",
		body);

	tstr[0][0] = tstr[1][0] = 0;
	if(flags & MSG_CHK_DISABLE_SMILE_CODES) strcpy(tstr[0], RADIO_CHECKED);
	if(flags & MSG_CHK_DISABLE_WWWCONF_TAGS) strcpy(tstr[1], RADIO_CHECKED);
	printf("<TR><TD COLSPAN=2 ALIGN=RIGHT class=cl>%s <INPUT TYPE=CHECKBOX NAME=\"dct\"%s class=cl></TD></TR>"
	"<TR><TD COLSPAN=2 ALIGN=RIGHT class=cl>%s <INPUT TYPE=CHECKBOX NAME=\"dst\"%s class=cl></TD></TR>",
		MESSAGEMAIN_post_disable_wwwconf_tags, tstr[1], MESSAGEMAIN_post_disable_smile_tags, tstr[0]);
						  
//	if(ULogin.LU.ID != 0) {
		tstr[0][0] = 0;
		if(flags & MSG_CHK_ENABLE_EMAIL_ACKNL) strcpy(tstr[0], RADIO_CHECKED);
		printf("<TR><TD COLSPAN=2 ALIGN=RIGHT class=cl>%s<INPUT TYPE=CHECKBOX NAME=\"wen\"%s class=cl></TD></TR>",
			 MESSAGEMAIN_post_reply_acknl, tstr[0]);
//	}

	printf("<TR><TD COLSPAN=2><HR ALIGN=CENTER WIDTH=80%% NOSHADE></TR><BR>");
								 
	printf("<TR><TD COLSPAN=2 ALIGN=CENTER>");

	char onSubmitScript[] = 
		"<script>\n"
		"var cTries=0\n"
		"function onSubmit(obj)\n"
		"{\n"
		"	var x=document.postform\n"
		"	if (x.subject.value.length == 0) {\n"
		"		alert(\"" MESSAGEMAIN_add_emptymsg_java "\");\n"
		"		return false;\n"
		"	}\n"
		"	if (x.subject.value.length > 65530) {\n"
		"		alert(\"" MESSAGEMAIN_add_tolong_java "\");\n"
		"		return false;\n"
		"	}\n"
		"	if(obj)x.jpost.value = obj.name;\n"
		"	if(document.all||document.getElementById) {\n"
		"		for(i = 0; i < x.length; i++) {\n"
		"			var tempobj = x.elements[i]\n"
		"			if(\n"
		"				tempobj.type &&\n"
		"				(tempobj.type.toLowerCase() == \"submit\" || tempobj.type.toLowerCase() == \"reset\"))\n"
		"			tempobj.disabled=true\n"
		"		}\n"
		"	}\n"
		"	if(cTries++ == 0)\n"
		"		x.submit();\n"
		"	return false;\n"
		"}\n"
		"</script>\n";
	printf("%s", onSubmitScript);

	if(code & ACTION_BUTTON_EDIT) {
		printf("\n<INPUT TYPE=SUBMIT NAME=\"edit\" onClick=\"onSubmit(this)\" VALUE=\"%s\">", MESSAGEMAIN_post_edit_message);
	}
	else {
		if(code & ACTION_BUTTON_PREVIEW) {
			printf("\n<INPUT TYPE=SUBMIT NAME=\"preview\" onClick=\"onSubmit(this)\" VALUE=\"%s\">&nbsp;",MESSAGEMAIN_post_preview_message);
		}
		if(code & ACTION_BUTTON_POST) {
			printf("<INPUT TYPE=SUBMIT NAME=\"post\" onClick=\"onSubmit(this)\" VALUE=\"%s\">", MESSAGEMAIN_post_post_message);
		}
	}
	printf("<INPUT TYPE=HIDDEN NAME=\"jpost\" VALUE=\"%s\">", "");

	printf("</TD></TR></TABLE></FORM></CENTER><P>&nbsp;");
}

static void PrintLoginForm()
{
	printf("<P><FORM METHOD=POST ACTION=\"%s?login=action\">", MY_CGI_URL);
	
	printf(DESIGN_BEGIN_LOGIN_OPEN);

	printf("<TR><TD ALIGN=RIGHT>%s</TD><TD><INPUT TYPE=TEXT NAME=\"mname\" SIZE=20 " \
	       "MAXLENGTH=%d VALUE=\"%s\"></TD></TR><TR><TD ALIGN=RIGHT>%s</TD><TD>" \
	       "<INPUT TYPE=PASSWORD NAME=\"mpswd\" SIZE=20 MAXLENGTH=30 VALUE=\"\"></TD></TR>",
		MESSAGEMAIN_login_loginname, AUTHOR_NAME_LENGTH - 1,
		FilterHTMLTags(cookie_name, 1000, 0), MESSAGEMAIN_login_password);

	printf("<TR><TD COLSPAN=2><CENTER><INPUT TYPE=CHECKBOX NAME=\"ipoff\" VALUE=1>" \
		MESSAGEMAIN_login_ipcheck "</CENTER></TD></TR>");
	
	printf("<P><TR><TD COLSPAN=2 ALIGN=CENTER><INPUT TYPE=SUBMIT VALUE=\"Enter\"></TD></TR>" DESIGN_END_LOGIN_CLOSE "</FORM>");

	printf(MESSAGEMAIN_login_lostpassw);
}

static void PrintPrivateMessageForm(char *name, char *body)
{
	printf("<CENTER><FORM METHOD=POST ACTION=\"%s?persmsgpost\"><P>&nbsp;<P>", MY_CGI_URL);

	printf(DESIGN_POST_NEW_MESSAGE_TABLE "<TH COLSPAN=2><BIG>" MESSAGEMAIN_privatemsg_send_msg_hdr
		"</BIG></TR><TR><TD COLSPAN=2><HR ALIGN=CENTER WIDTH=80%% NOSHADE></TR>");
	printf("<TR><TH ALIGN=RIGHT>%s </TH><TD>"
		"<INPUT TYPE=TEXT NAME=\"name\" SIZE=30 MAXLENGTH=%d VALUE=\"%s\"></TD></TR>"
		"<TR><TD COLSPAN=2 ALIGN=CENTER><STRONG>%s</STRONG>", 
		MESSAGEMAIN_privatemsg_send_msg_usr, PROFILES_MAX_USERNAME_LENGTH - 1,
		name, MESSAGEMAIN_privatemsg_send_msg_bdy);

	printf("<BR><TEXTAREA COLS=50 ROWS=7 NAME=\"body\" WRAP=VIRTUAL>%s</TEXTAREA></TD></TR>", body);

	printf("<TR><TD COLSPAN=2><HR ALIGN=CENTER WIDTH=80%% NOSHADE></TR><BR><TR><TD COLSPAN=2 ALIGN=CENTER>"
		"<INPUT TYPE=SUBMIT NAME=\"Post\" VALUE=\"%s\">&nbsp;<INPUT TYPE=SUBMIT NAME=\"Post\" VALUE=\"%s\"></TD></TR></TABLE></FORM></CENTER>",
		MESSAGEMAIN_privatemsg_prev_msg_btn, MESSAGEMAIN_privatemsg_send_msg_btn);
}

static void PrintAnnounceForm(char *body, int ChangeAnn = 0)
{
	printf("<CENTER><FORM METHOD=POST ACTION=\"%s?globann=post\"><P>&nbsp;<P>", MY_CGI_URL);

	printf(DESIGN_POST_NEW_MESSAGE_TABLE "<TH COLSPAN=2><BIG>%s</BIG></TR><TR><TD COLSPAN=2><HR ALIGN=CENTER WIDTH=80%% NOSHADE></TR>",
		ChangeAnn ? MESSAGEMAIN_globann_upd_ann_hdr : MESSAGEMAIN_globann_send_ann_hdr);

	printf("<TR><TD COLSPAN=2 ALIGN=CENTER><STRONG>%s</STRONG><BR>"
		"<TEXTAREA COLS=50 ROWS=7 NAME=\"body\" WRAP=VIRTUAL>%s</TEXTAREA></TD></TR>",
			MESSAGEMAIN_globann_send_ann_body, body);

	if(ChangeAnn) printf("<TR><TD COLSPAN=2><CENTER><INPUT TYPE=CHECKBOX NAME=\"refid\" VALUE=1>" \
		MESSAGEMAIN_globann_upd_ann_id "</CENTER></TD></TR>");
	printf("<TR><TD COLSPAN=2><HR ALIGN=CENTER WIDTH=80%% NOSHADE></TR><BR><TR><TD COLSPAN=2 ALIGN=CENTER>");
	if(ChangeAnn) printf("<INPUT TYPE=HIDDEN NAME=\"cgann\" VALUE=\"%d\">", ChangeAnn);
	printf("<INPUT TYPE=SUBMIT NAME=\"Post\" VALUE=\"%s\"><INPUT TYPE=SUBMIT NAME=\"Post\" VALUE=\"%s\"></TD></TR></TABLE></FORM></CENTER>",
		MESSAGEMAIN_globann_prev_ann_btn, MESSAGEMAIN_globann_send_ann_btn);
}

static void PrintLostPasswordForm()
{
	printf("<P><FORM METHOD=POST ACTION=\"%s?login=lostpasswaction\">", MY_CGI_URL);

	printf(DESIGN_BEGIN_LOSTPASSW_OPEN);

	printf("<TR><TD ALIGN=RIGHT>%s</TD><TD><INPUT TYPE=TEXT NAME=\"mname\" SIZE=20 MAXLENGTH=%d "
		"VALUE=\"%s\"></TD></TR><TR><TD ALIGN=RIGHT>%s</TD><TD><INPUT TYPE=TEXT NAME=\"memail\" SIZE=20 "
		"MAXLENGTH=%d VALUE=\"\"></TD></TR>", MESSAGEMAIN_lostpassw_loginname, AUTHOR_NAME_LENGTH - 1,
		FilterHTMLTags(cookie_name, 1000, 0), MESSAGEMAIN_lostpassw_email, PROFILES_FULL_USERINFO_MAX_EMAIL - 1);

	printf("<P><TR><TD COLSPAN=2 ALIGN=CENTER><INPUT TYPE=SUBMIT VALUE=\"" MESSAGEMAIN_lostpassw_getpassw "\"><CENTER></TD></TR>" DESIGN_END_LOSTPASSW_CLOSE "</FORM>");
}

/* print configuration form */
static void PrintConfig()	
{
	char str1[20], str2[20], str3[20], str4[20], str5[20], str6[20], str7[20], str8[20], str9[20];
	char strm[10][20];
	int i;

	printf("<TABLE align=center width=100%%><tr><td><FORM METHOD=POST ACTION=\"%s?configure=action\" name=\"configure\">",
		MY_CGI_URL);
	
	printf("<CENTER><P><B>%s</B><BR><P>", MESSAGEHEAD_configure);
	
	printf("<P><B>%s</B><BR><P>", MESSAGEHEAD_configure_showmsgs);

	str1[0] = str2[0] = str3[0] = str4[0] = str5[0] = 0;
	if(currentlsel == 1) strcpy(str1, RADIO_CHECKED);

	switch(currenttt) {
	case 1:
		strcpy(str2, LISTBOX_SELECTED);
		break;
	case 2:
		strcpy(str3, LISTBOX_SELECTED);
		break;
	case 3:
		strcpy(str4, LISTBOX_SELECTED);
		break;
	case 4:
		strcpy(str5, LISTBOX_SELECTED);
		break;
	}
	printf("<INPUT TYPE=RADIO NAME=lsel VALUE=1%s>%s" \
		"<INPUT TYPE=TEXT NAME=\"tv\" SIZE=2 VALUE=%d><SELECT NAME=\"tt\">" \
		"<OPTION VALUE=\"1\"%s>%s<OPTION VALUE=\"2\"%s>%s<OPTION VALUE=\"3\"%s>" \
		"%s<OPTION VALUE=\"4\"%s>%s</SELECT><BR>",
		str1, MESSAGEHEAD_configure_msgslast, currenttv,
		str2, MESSAGEHEAD_timetypes[0], str3, MESSAGEHEAD_timetypes[1], str4,
		MESSAGEHEAD_timetypes[2], str5, MESSAGEHEAD_timetypes[3]
	);

	str1[0] = str2[0] = str3[0] = str4[0] = str5[0] = 0;
	if(currentlsel == 2) strcpy(str1, RADIO_CHECKED);

	switch(currentss) {
	case 1:
		strcpy(str2, LISTBOX_SELECTED);
		break;
	case 2:
		strcpy(str3, LISTBOX_SELECTED);
		break;
	case 3:
		strcpy(str4, LISTBOX_SELECTED);
		break;
	case 4:
		strcpy(str5, LISTBOX_SELECTED);
		break;
	}

	printf("<INPUT TYPE=RADIO NAME=lsel VALUE=2%s>%s<INPUT TYPE=TEXT NAME=\"tc\" SIZE=3 VALUE=%d>"
		"%s<P>%s<SELECT NAME=\"ss\"><OPTION VALUE=\"2\"%s>%s<OPTION VALUE=\"3\"%s>%s<OPTION VALUE=\"4\"%s>"
		"%s</SELECT>",
		str1, MESSAGEHEAD_configure_lastnum, currenttc, MESSAGEHEAD_configure_lastnum2,
		MESSAGEHEAD_configure_showstyle, str3,
		MESSAGEHEAD_configure_showhronbackward, str4,
		MESSAGEHEAD_configure_showhronwothreads, str5,
		MESSAGEHEAD_configure_showhrononlyheaders);

	printf("<BR><BR>��������� ����: <SELECT NAME=\"tz\">");
	for(i = -12; i <= 12; i++)
		printf("<OPTION VALUE=\"%d\"%s>Timezone GMT%s%02d", i,
		(i == currenttz) ? LISTBOX_SELECTED : "", (i>=0) ? "+" : "-", (i>0)? i : -i);
	printf("</SELECT><BR>");


#if TOPICS_SYSTEM_SUPPORT
	// use str4 for temporaty buffer
	printf("<BR><P><TABLE BORDER=1 CELLSPACING=0 CELLPADDING=6 BGCOLOR=#BBAAAA>\
<TR><TD ALIGN=CENTER>" DESIGN_CONFIGURE_CHECKALL "</TD></TR><TR><TD ALIGN=RIGHT>");
	for(DWORD i = 0; i < TOPICS_COUNT; i++)
	{
		if(currenttopics & (1<<Topics_List_map[i]))
			strcpy(str4, RADIO_CHECKED);
		else str4[0] = 0;
		printf("%s <INPUT TYPE=CHECKBOX NAME=\"topic%d\"%s><br>\n",
			Topics_List[Topics_List_map[i]], Topics_List_map[i], str4);
	}
	printf("</TD></TR></TABLE>");
#endif

	if((currentdsm & 0x01) != 0) strcpy(str3, RADIO_CHECKED);
	else str3[0] = 0;
	if((currentdsm & 0x02) != 0) strcpy(str4, RADIO_CHECKED);
	else str4[0] = 0;
	if((currentdsm & 0x04) != 0) strcpy(str5, RADIO_CHECKED);
	else str5[0] = 0;
	if((currentdsm & 0x08) != 0) strcpy(str2, RADIO_CHECKED);
	else str2[0] = 0;
	if((currentdsm & 0x10) != 0) strcpy(str1, RADIO_CHECKED);
	else str1[0] = 0;
	if((currentdsm & 0x20) != 0) strcpy(str6, RADIO_CHECKED);
	else str6[0] = 0;
	if((currentdsm & 0x40) != 0) strcpy(str7, RADIO_CHECKED);
	else str7[0] = 0;
	if((currentdsm & 0x80) != 0) strcpy(str8, RADIO_CHECKED);
	else str8[0] = 0;
	if((currentdsm & 0x100) != 0) strcpy(str9, RADIO_CHECKED);
	else str9[0] = 0;

	printf("<TABLE><TR><TD ALIGN=RIGHT>%s<INPUT TYPE=CHECKBOX NAME=\"dsm\" VALUE=1%s>",
		MESSAGEHEAD_configure_disablesmiles, str3);
	printf("<BR>%s<INPUT TYPE=CHECKBOX NAME=\"dup\" VALUE=1%s>",
		MESSAGEHEAD_configure_disableuppic, str4);
	printf("<BR>%s<INPUT TYPE=CHECKBOX NAME=\"dul\" VALUE=1%s>",
		MESSAGEHEAD_configure_disable2links, str5);
	printf("<BR>%s<INPUT TYPE=CHECKBOX NAME=\"onh\" VALUE=1%s>",
		MESSAGEHEAD_configure_ownpostshighlight, str2);
	printf("<BR>%s<INPUT TYPE=CHECKBOX NAME=\"host\" VALUE=1%s>",
		MESSAGEHEAD_configure_showhostnames, str6);
	printf("<BR>%s<INPUT TYPE=CHECKBOX NAME=\"nalt\" VALUE=1%s>",
		MESSAGEHEAD_configure_showaltnames, str7);
	printf("<BR>%s<INPUT TYPE=CHECKBOX NAME=\"dsig\" VALUE=1%s>",
		MESSAGEHEAD_configure_showsign, str8);
	printf("<BR>%s<INPUT TYPE=CHECKBOX NAME=\"shrp\" VALUE=1%s>",
		MESSAGEHEAD_configure_showreplyform, str9);
#if ALLOW_MARK_NEW_MESSAGES == 2
	printf("<BR>%s<INPUT TYPE=CHECKBOX NAME=\"plu\" VALUE=1%s>",
		MESSAGEHEAD_configure_plus_is_href, str1);
#endif
	printf("</TD></TR></TABLE>");

	if(ULogin.LU.ID[0] && (ULogin.pui->Flags & PROFILES_FLAG_VIEW_SETTINGS) )
			printf("<P>" MESSAGEHEAD_configure_saving_to_profile "<BR>");
	else printf("<P>" MESSAGEHEAD_configure_saving_to_browser "<BR>");
	if(ULogin.LU.ID[0]) printf(MESSAGEHEAD_configure_view_saving "<BR>");
	
	printf("<P><INPUT TYPE=SUBMIT VALUE=\"%s\"></CENTER></FORM></form></td></tr></table>",
		MESSAGEHEAD_configure_applysettings);
	
}

void PrintTopString(DWORD c, DWORD ind, DWORD ret)
{
	/* print init code */
	printf(DESIGN_COMMAND_TABLE_BEGIN);
	
	/* print messages */
	int g = 0;

	if(c & HEADERSTRING_ENABLE_TO_MESSAGE) {
		if(g) printf(DESIGN_BUTTONS_DIVIDER);
		printf("<A HREF=\"#%ld\" STYLE=\"text-decoration:underline;\">%s</A>", ret, MESSAGEHEAD_to_message);
		g = 1;
	}

	if((c & HEADERSTRING_ENABLE_TO_MESSAGE) || (c & HEADERSTRING_ENABLE_REPLY_LINK)) {
		if(g) printf(DESIGN_BUTTONS_DIVIDER);
		if((c & HEADERSTRING_ENABLE_REPLY_LINK))
			printf("<A HREF=\"%s?form=%ld\" STYLE=\"text-decoration:underline;\">%s</A>",
				MY_CGI_URL, ret, MESSAGEMAIN_post_replymessage);
		else printf("<A HREF=\"#Reply\" STYLE=\"text-decoration:underline;\">%s</A>",
				MESSAGEMAIN_post_replymessage);
		g = 1;
	}

	if(c & HEADERSTRING_RETURN_TO_MAIN_PAGE) {
		if(g) printf(DESIGN_BUTTONS_DIVIDER);
		if(ind == MAINPAGE_INDEX || ind == 0) printf("<A HREF=\"%s?index\" STYLE=\"text-decoration:underline;\">%s</A>",
			MY_CGI_URL, MESSAGEHEAD_return_to_main_page);
		else printf("<A HREF=\"%s?index#%ld\"><font color=\"#F00F0F\" STYLE=\"text-decoration:underline;\">%s</font></A>",
			MY_CGI_URL, ind, MESSAGEHEAD_return_to_main_page);
		g = 1;
	}
	
	if(c & HEADERSTRING_POST_NEW_MESSAGE) {
		if(g) printf(DESIGN_BUTTONS_DIVIDER);
		printf("<A HREF=\"%s?form\" STYLE=\"text-decoration:underline;\">%s</A>", MY_CGI_URL, MESSAGEHEAD_post_new_message);
		g = 1;
	}

	if((c & HEADERSTRING_DISABLE_SEARCH) == 0) {
		if(g) printf(DESIGN_BUTTONS_DIVIDER);
		printf("<A HREF=\"%s?search=form\" STYLE=\"text-decoration:underline;\">%s</A>", MY_CGI_URL, MESSAGEHEAD_search);
		g = 1;
	}
	
	if(c & HEADERSTRING_CONFIGURE) {
		if(g) printf(DESIGN_BUTTONS_DIVIDER);
		printf("<A HREF=\"%s?configure=form\" STYLE=\"text-decoration:underline;\">%s</A>", MY_CGI_URL, MESSAGEHEAD_configure);
		g = 1;
	}

	if(c & HEADERSTRING_ENABLE_RESETNEW) {
		if(g) printf(DESIGN_BUTTONS_DIVIDER);
		printf("<A HREF=\"%s?resetnew\" STYLE=\"text-decoration:underline;\">%s</A>", MY_CGI_URL, MESSAGEHEAD_resetnew);
		g = 1;
	}

	if((c & HEADERSTRING_DISABLE_REGISTER) == 0) {
		if(g) printf(DESIGN_BUTTONS_DIVIDER);
		if(ULogin.LU.ID[0] != 0)
			printf("<A HREF=\"%s?register=form\" STYLE=\"text-decoration:underline;\">%s</A>", MY_CGI_URL, MESSAGEHEAD_registerprf);
		else
			printf("<A HREF=\"%s?register=form\" STYLE=\"text-decoration:underline;\">%s</A>", MY_CGI_URL, MESSAGEHEAD_register);
		g = 1;
	}

	if(c & HEADERSTRING_REG_USER_LIST) {
		if(g) printf(DESIGN_BUTTONS_DIVIDER);
		printf("<A HREF=\"%s?userlist\" STYLE=\"text-decoration:underline;\">%s</A>", MY_CGI_URL, MESSAGEHEAD_userlist);
		g = 1;
	}
	
	if( (ULogin.LU.right & USERRIGHT_SUPERUSER) != 0  && HEADERSTRING_REG_USER_LIST) {
		if(g) printf(DESIGN_BUTTONS_DIVIDER);
		printf("<A HREF=\"%s?banlist\" STYLE=\"text-decoration:underline;\">%s</a>", MY_CGI_URL, MESSAGEHEAD_banlist);
	}

	if((ULogin.LU.ID[0] == 0) && ((c & HEADERSTRING_DISABLE_LOGIN) == 0)) {
		if(g) printf(DESIGN_BUTTONS_DIVIDER);
		printf("<A HREF=\"%s?login=form\" STYLE=\"text-decoration:underline;\">%s</A>", MY_CGI_URL, MESSAGEHEAD_login);
		g = 1;
	}

	if((c & HEADERSTRING_DISABLE_FAQHELP) == 0) {
		if(g) printf(DESIGN_BUTTONS_DIVIDER);
		printf("<A HREF=\"rules.html\" STYLE=\"text-decoration:underline;\">" MESSAGEHEAD_help_showhelp "</A>");
		g = 1;
	}

#if USER_FAVOURITES_SUPPORT
	if(ULogin.LU.ID[0] != 0 && (c & HEADERSTRING_DISABLE_FAVOURITES) == 0) {
		if(g) printf(DESIGN_BUTTONS_DIVIDER);
		printf("<A HREF=\"%s?favs\" STYLE=\"text-decoration:underline;\">%s</A>",
				MY_CGI_URL, MESSAGEHEAD_favourites);
		g = 1;
	}
#endif

	if(ULogin.LU.ID[0] != 0 && (c & HEADERSTRING_DISABLE_PRIVATEMSG) == 0) {
		if(g) printf(DESIGN_BUTTONS_DIVIDER);
		if(ULogin.pui->persmescnt - ULogin.pui->readpersmescnt > 0)
			printf("<A HREF=\"%s?persmsg\" STYLE=\"text-decoration:underline;\"><FONT COLOR=RED><BOLD>%s(%d)</BOLD></FONT></A>",
				MY_CGI_URL, MESSAGEHEAD_personalmsg, ULogin.pui->persmescnt - ULogin.pui->readpersmescnt);
		else
			printf("<A HREF=\"%s?persmsg\" STYLE=\"text-decoration:underline;\">%s</A>", MY_CGI_URL, MESSAGEHEAD_personalmsg);
		g = 1;
	}

	if((ULogin.LU.right & USERRIGHT_POST_GLOBAL_ANNOUNCE) != 0 && (c & HEADERSTRING_POST_NEW_MESSAGE) != 0) {
		if(g) printf(DESIGN_BUTTONS_DIVIDER);
		printf("<A HREF=\"%s?globann=form\" STYLE=\"text-decoration:underline;\">%s</A>", MY_CGI_URL, MESSAGEHEAD_makeannounce);
		g = 1;
	}
	
	if((ULogin.LU.ID[0] != 0) && ((c & HEADERSTRING_DISABLE_LOGOFF) == 0)) {
		if(g) printf(DESIGN_BUTTONS_DIVIDER);
		printf("<A HREF=\"%s?login=logoff\" STYLE=\"text-decoration:underline;\">%s</A>", MY_CGI_URL, MESSAGEHEAD_logoff);
		g = 1;
	}
	
	printf("</TD></TR></TABLE>");
}

void PrintTopStaticLinks(DWORD c)
{
	//print stable links on index page
	if ((c & HEADERSTRING_REG_USER_LIST) != 0 && (currentdsm & 4) == 0) {
		//printf("<TR><TD class=cs align=center>");
		printf(DESIGN_COMMAND_TABLE_BEGIN);
		printf("<!--<A HREF=\"/dev/\" target=_blank STYLE=\"text-decoration:underline;\"><nobr>%s</nobr></A>", "Programming Board-->");
		//gray space after top links
		printf("<TR><TD class=cl>&nbsp;</TD></TR>");
		printf("</TD></TR></TABLE>"); 
	} 

}

/* print HTML header of file, header placed in topbanner.html */
void PrintHTMLHeader(DWORD code, DWORD curind, DWORD retind = 0)
{
	if(!HPrinted) {
		printf("Content-type: text/html\n");
		HPrinted = 1; // header have been printed
	}


	if((code & HEADERSTRING_NO_CACHE_THIS) != 0) printf("Pragma: no-cache\n");

	printf("Set-Cookie: " COOKIE_NAME_STRING "name=%s|ed=%d|lsel=%d|tc=%d|tt=%d|tv=%d|ss=%d|" \
		"lm=%ld|fm=%ld|lt=%ld|ft=%ld|dsm=%d|seq=%08x%08x|topics=%d|lann=%d|tovr=%d|tz=%d&;" \
		" expires=" COOKIE_EXPIRATION_DATE "path=" COOKIE_SERVER_PATH
		"\nSet-Cookie: " COOKIE_SESSION_NAME "on&; path=" COOKIE_SERVER_PATH "\n\n", 
		 CodeHttpString(cookie_name, 0), cookie_lastenter, cookie_lsel, cookie_tc, cookie_tt, cookie_tv,
		 cookie_ss, currentlm, currentfm, currentlt, currentft, cookie_dsm, ULogin.LU.ID[0], ULogin.LU.ID[1],
		cookie_topics, currentlann, topicsoverride,cookie_tz);

	printf(HTML_START);

	if((code & HEADERSTRING_REDIRECT_NOW)) {
	        printf("<meta http-equiv=\"Refresh\" content=\"0; url=%s?index\"></head></html>", MY_CGI_URL);
		return;
	}

	if(code & HEADERSTRING_REFRESH_TO_MAIN_PAGE) {
		if(curind == MAINPAGE_INDEX || curind == 0)
			printf("<meta http-equiv=\"Refresh\" content=\"%d; url=%s?index\">",
			AUTO_REFRESH_TIME, MY_CGI_URL);
		else
			printf("<meta http-equiv=\"Refresh\" content=\"%d; url=%s?index#%ld\">",
			AUTO_REFRESH_TIME, MY_CGI_URL, curind);
	}

	// print output encoding (charset)
	printf(TEXT_ENCODING_HEADER);

	// print title
#if STABLE_TITLE == 0
	printf("<title>%s</title>", ConfTitle);
#else
	printf("<title>%s</title>", TITLE_StaticTitle);
#endif


	printf(TEXT_STYLE_HEADER);
	
	// print header of board
#if USE_TEXT_TOPBANNER
	// print constant expressions
	printf(TEXT_TOPBANNER_HEADER);
	
	if(curind == MAINPAGE_INDEX && (currentdsm & 2) == 0) printf(TEXT_TOPBANNER_MAP);
#else
	// print from TOPBANNER file
	{
		FILE *f;
		DWORD readed;
		void *buf = malloc(MAX_HTML_FILE_SIZE);
		if((f = fopen(F_TOPBANNER,FILE_ACCESS_MODES_R)) == NULL)
			printhtmlerrorat(LOG_UNABLETOLOCATEFILE, F_TOPBANNER);
		if((readed = fread(buf, 1, MAX_HTML_FILE_SIZE, f)) == 0)
			printhtmlerrorat(LOG_UNABLETOLOCATEFILE, F_TOPBANNER);
		if(fwrite(buf, 1, readed, stdout) != readed) printhtmlerror();
		free(buf);
		fclose(f);
	}
#endif

	if((HEADERSTRING_DISABLE_ALL & code) == 0) {
		/* print top string (navigation) */
		PrintTopString(code, curind, retind);
		PrintTopStaticLinks (code);
	}
}

/* print bottom lines from file (banners, etc.) */
void PrintBottomLines(DWORD code, DWORD curind, DWORD retind = 0)
{
	// gap between end of board and bottom header
	printf(DESIGN_INDEX_WELCOME_CLOSE);

/*	if(((HEADERSTRING_DISABLE_ALL & code) == 0) ) {
		PrintTopString(code, curind, retind);
	}*/


#if USE_TEXT_BOTTOMBANNER
	if(curind == MAINPAGE_INDEX && (currentdsm & 2) == 0) 
		printf(TEXT_BOTTOMBANNER);
	else 
		printf(TEXT_BOTTOMBANNER_SHORT);
#else
	{
		FILE *f;
		void *buf;
		DWORD readed;
		if((buf = malloc(MAX_HTML_FILE_SIZE)) == NULL)
			printhtmlerrorat(LOG_FATAL_NOMEMORY, MAX_HTML_FILE_SIZE);
		if((f = fopen(F_BOTTOMBANNER,FILE_ACCESS_MODES_R)) == NULL)
			printhtmlerrorat(LOG_UNABLETOLOCATEFILE, F_BOTTOMBANNER);
		if((readed = fread(buf, 1, MAX_HTML_FILE_SIZE, f)) == 0)
			printhtmlerrorat(LOG_UNABLETOLOCATEFILE, F_BOTTOMBANNER);
		if(fwrite(buf, 1, readed, stdout) != readed) printhtmlerror();
		fclose(f);
		free(buf);
	}
#endif
}

/* print moderation toolbar and keys
 * 
 */
int PrintAdminToolbar(DWORD root, int mflag, DWORD UID)
{
	DWORD fl = 0;	// store bit mask for keys

	/* allow to superuser or author */
	if((ULogin.LU.right & USERRIGHT_SUPERUSER) || (ULogin.LU.ID[0] != 0 &&
		ULogin.LU.UniqID == UID)) {

		if(((mflag & MESSAGE_IS_CLOSED) == 0) && (ULogin.LU.right & USERRIGHT_CLOSE_MESSAGE))
			fl = fl | 0x0001;

		if((mflag & MESSAGE_IS_CLOSED) && (ULogin.LU.right & USERRIGHT_OPEN_MESSAGE))
			fl = fl | 0x0002;

		if(ULogin.LU.right & USERRIGHT_MODIFY_MESSAGE)
			fl = fl | 0x0004;
	}

	/* allow only to superuser */
	if(ULogin.LU.right & USERRIGHT_SUPERUSER) {
		
		fl = fl | 0x0080; /* delete */

		if((mflag & MESSAGE_IS_INVISIBLE) == 0)
			fl = fl | 0x0008;
		
		if(mflag & MESSAGE_IS_INVISIBLE)
			fl = fl | 0x0010;
		
		if((mflag & MESSAGE_COLLAPSED_THREAD) == 0)
			fl = fl | 0x0020;
		
		if(mflag & MESSAGE_COLLAPSED_THREAD)
			fl = fl | 0x0040;
	}

	int g = 0;
	if(fl) {
		/* print administration table */
		printf("<BR><CENTER><TABLE BORDER=0 CELLSPACING=0 CELLPADDING=1"
			"BGCOLOR=\"#eeeeee\"><TR><TD ALIGN=CENTER>[ <SMALL>");
		
		/* close thread */
		if(fl & 0x0001) {
			printf("<A HREF=\"%s?close=%ld\"><font color=\"#AF0000\">%s</font></A>",
				MY_CGI_URL, root, MESSAGEMAIN_moderate_close_thread);
			g = 1;
		}

		/* open thread */
		if(fl & 0x0002) {
			if(g) printf(DESIGN_BUTTONS_DIVIDER);
			printf("<A HREF=\"%s?unclose=%ld\"><font color=\"#AF0000\">%s</font></A>",
				MY_CGI_URL, root, MESSAGEMAIN_moderate_unclose_thread);
			g = 1;
		}
		
		// change message 
		if(fl & 0x0004) {
			if(g) printf(DESIGN_BUTTONS_DIVIDER);
			printf("<A HREF=\"%s?changemsg=%ld\"><font color=\"#AF0000\">%s</font></A>",
				MY_CGI_URL, root, MESSAGEMAIN_moderate_change_message);
		}

		/* collapse thread */
		if(fl & 0x0020) {
			if(g) printf(DESIGN_BUTTONS_DIVIDER);
			printf("<A HREF=\"%s?roll=%ld\"><font color=\"#AF0000\">%s</font></A>",
				MY_CGI_URL, root, MESSAGEMAIN_moderate_roll);
		}
			
		/* uncollapse thread */
		if(fl & 0x0040) {
			if(g) printf(DESIGN_BUTTONS_DIVIDER);
			printf("<A HREF=\"%s?roll=%ld\"><font color=\"#AF0000\">%s</font></A>",
				MY_CGI_URL, root, MESSAGEMAIN_moderate_unroll);
		}

		/* hide thread */
		if(fl & 0x0008) {
			if(g) printf(DESIGN_BUTTONS_DIVIDER);
			printf("<A HREF=\"%s?hide=%ld\"><font color=\"#AF0000\">%s</font></A>",
				MY_CGI_URL, root, MESSAGEMAIN_moderate_hide_thread);
			g = 1;
		}
		
		/* unhide thread */
		if(fl & 0x0010) {
			if(g) printf(DESIGN_BUTTONS_DIVIDER);
			printf("<A HREF=\"%s?unhide=%ld\"><font color=\"#AF0000\">%s</font></A>",
				MY_CGI_URL, root, MESSAGEMAIN_moderate_unhide_thread);
			g =1;
		}

		/* delete thread */
		if(fl & 0x0080) {
			if(g) printf(DESIGN_BUTTONS_DIVIDER);
			printf("<A HREF=\"%s?delmsg=%d\"><font color=\"#FF1000\">%s</font></A>",
				MY_CGI_URL, root, MESSAGEMAIN_moderate_delete_thread);
		}

		printf("</SMALL> ]</TD></TR></TABLE></CENTER>");
	}
	return 0;
}

/* print FAQ-Help page */
void PrintFAQForm()
{
	size_t readed;
	char *buf;
	FILE *f;

	/* print faq file */
	if((buf = (char*)malloc(MAX_HTML_FILE_SIZE)) == NULL) printhtmlerror();
	
	if((f = fopen(F_FAQ_HELP, FILE_ACCESS_MODES_R)) == NULL) printhtmlerror();
 	//printhtmlerror();	
	if((readed = fread(buf, 1, MAX_HTML_FILE_SIZE, f)) == 0) printhtmlerror();
	if(fwrite(buf, 1, readed, stdout) != readed) printhtmlerror();
	fclose(f);

	free(buf);
}

void PrintSearchForm(char *s, DB_Base *db, int start = 0)
{
	printf("<FORM METHOD=POST ACTION=\"%s?search=action\">",
		MY_CGI_URL);
	
	printf("<CENTER><P><B>%s</B><BR><P>", MESSAGEHEAD_search);

	if(start) {
		FILE *f;
		char LastMsgStr[500];
		DWORD LastMsg = 0, LastDate = 0;
		f = fopen(F_SEARCH_LASTINDEX, FILE_ACCESS_MODES_R);
		if(f != NULL) {
			fscanf(f, "%d %d", &LastMsg, &LastDate);
			fclose(f);
		}
		if(LastMsg != 0) {
			SMessage mes;
			if(ReadDBMessage(db->TranslateMsgIndex(LastMsg), &mes)) {
				char s[200];
				ConvertTime(mes.Date, s);
				sprintf(LastMsgStr, "%d (%s)", LastMsg, s);
			}
			else strcpy(LastMsgStr, MESSAGEMAIN_search_indexerror);
		}
		else {
			strcpy(LastMsgStr, MESSAGEMAIN_search_notindexed);
		}
		printf("<P>%s<P>%s : %s<P>", MESSAGEMAIN_search_howtouse, MESSAGEMAIN_search_lastindexed, LastMsgStr);
	}

	printf("<P><EM>%s</EM><BR>", MESSAGEMAIN_search_searchmsg);
	printf("<INPUT TYPE=RADIO NAME=sel VALUE=1 CHECKED>%s <FONT FACE=\"Courier\">"
		"<INPUT TYPE=TEXT NAME=\"find\" SIZE=45 VALUE=\"%s\"></FONT>", 
			MESSAGEMAIN_search_containing, s);

	
	printf("<P><INPUT TYPE=SUBMIT VALUE=\"%s\"></CENTER></FORM>",
		MESSAGEHEAD_search);
}

#if STABLE_TITLE == 0
void Tittle_cat(char *s)
{
	// set title
	ConfTitle = (char*)realloc(ConfTitle, strlen(ConfTitle) + strlen(TITLE_divider) + strlen(s) + 1);
	strcat(ConfTitle, TITLE_divider);
	strcat(ConfTitle, s);
}
#else
#define Tittle_cat(x) {}
#endif

/* print create or edit(delete) profile form (depend of flags)
 * if flags == 1 - Edit profile, otherwise create profile
 */
void PrintEditProfileForm(SProfile_UserInfo *ui, SProfile_FullUserInfo *fui, DWORD flags)
{
	char str1[20], str2[20], str3[20], str4[20], str5[20];
	printf("<FORM METHOD=POST ACTION=\"%s?register=action\">",
		MY_CGI_URL);

	if(ULogin.LU.ID[0] == 0) {
		printf(DESIGN_BEGIN_REGISTER_OPEN "<TD COLSPAN=2 ALIGN=CENTER>"
			"<BIG>%s</BIG></TD></TR>", MESSAGEMAIN_register_intro);
	}
	else
		printf(DESIGN_BEGIN_REGISTER_OPEN "<TD COLSPAN=2 ALIGN=CENTER>"
			"<BIG>%s</BIG></TD></TR>", MESSAGEMAIN_register_chg_prof_intro);

	printf("<TR><TD COLSPAN=2><HR ALIGN=CENTER WIDTH=80%% NOSHADE></TD></TR><TR><TD ALIGN=RIGHT>%s </TD>",
			MESSAGEMAIN_register_login);

	if( (ULogin.LU.ID[0] == 0) || (ULogin.LU.ID[0] != 0 && ((ULogin.pui->right & USERRIGHT_SUPERUSER) != 0)) ) {
		printf("<TD><INPUT TYPE=TEXT NAME=\"login\" SIZE=35 MAXLENGTH=%d VALUE=\"%s\"></TD></TR>",
			AUTHOR_NAME_LENGTH - 1, ui->username);
	}
	else {
		printf("<TD>%s</TD></TR>", ui->username);
	}

	if( ULogin.LU.ID[0] != 0 && ((ULogin.LU.right & USERRIGHT_ALT_DISPLAY_NAME) != 0) ) {
		printf("<TR><TD ALIGN=RIGHT>%s </TD><TD><INPUT TYPE=TEXT NAME=\"dispnm\" SIZE=35 MAXLENGTH=%d VALUE=\"%s\"></TD></TR>",
			MESSAGEMAIN_register_displayname, PROFILES_MAX_ALT_DISPLAY_NAME - 1, ui->altdisplayname);
	}

	if((flags & 0x01) == 0) {
		printf("<TR><TD COLSPAN=2 ALIGN=CENTER><STRONG>" MESSAGEMAIN_register_oldpass_req "</STRONG></TR>" \
			   "<TR><TD ALIGN=RIGHT>%s </TD><TD><INPUT TYPE=PASSWORD NAME=\"opswd\" SIZE=35 " \
			   "MAXLENGTH=%d VALUE=\"\"></TD></TR>",
				MESSAGEMAIN_register_oldpassword, PROFILES_MAX_PASSWORD_LENGTH - 1);
	}

	if(ui->Flags & PROFILES_FLAG_VISIBLE_EMAIL)
		strcpy(str1, RADIO_CHECKED);
	else *str1 = 0;

	printf("<TR><TD COLSPAN=2 ALIGN=CENTER><STRONG>" MESSAGEMAIN_register_if_want_change "</STRONG></TR>" \
"<TR><TD ALIGN=RIGHT>%s </TD><TD><INPUT TYPE=PASSWORD NAME=\"pswd1\" SIZE=35 MAXLENGTH=%d VALUE=\"%s\"></TD></TR>" \
"<TR><TD ALIGN=RIGHT>%s </TD><TD><INPUT TYPE=PASSWORD NAME=\"pswd2\" SIZE=35 MAXLENGTH=%d VALUE=\"%s\"></TD></TR>" \
"<TR><TD ALIGN=RIGHT>%s </TD><TD><INPUT TYPE=TEXT NAME=\"name\" SIZE=35 MAXLENGTH=%d VALUE=\"%s\"></TD></TR>" \
"<TR><TD COLSPAN=2 ALIGN=CENTER><STRONG>" MESSAGEMAIN_register_validemail_req "</STRONG></TR>" \
"<TR><TD ALIGN=RIGHT>%s </TD><TD><INPUT TYPE=TEXT NAME=\"email\" SIZE=35 MAXLENGTH=%d VALUE=\"%s\"></TD></TR>" \
"<TR><TD COLSPAN=2 ALIGN=CENTER><STRONG>%s</STRONG><INPUT TYPE=CHECKBOX NAME=\"pem\" VALUE=1%s></TD></TR>" \
"<TR><TD ALIGN=RIGHT>%s </TD><TD><INPUT TYPE=TEXT NAME=\"hpage\" SIZE=35 MAXLENGTH=%d VALUE=\"%s\"></TD></TR>" \
"<TR><TD ALIGN=RIGHT>%s </TD><TD><INPUT TYPE=TEXT NAME=\"icq\" SIZE=15 MAXLENGTH=%d VALUE=\"%s\"></TD></TR>",
		  MESSAGEMAIN_register_password1, PROFILES_MAX_PASSWORD_LENGTH - 1, "",
		  MESSAGEMAIN_register_password2, PROFILES_MAX_PASSWORD_LENGTH - 1, "",
		  MESSAGEMAIN_register_full_name, PROFILES_FULL_USERINFO_MAX_NAME - 1, fui->FullName,
		  MESSAGEMAIN_register_email, PROFILES_FULL_USERINFO_MAX_EMAIL - 1, fui->Email,
		  MESSAGEMAIN_register_email_pub, str1, MESSAGEMAIN_register_homepage,
		  PROFILES_FULL_USERINFO_MAX_HOMEPAGE - 1, fui->HomePage,
		  MESSAGEMAIN_register_icq, PROFILES_MAX_ICQ_LEN - 1, ui->icqnumber
		  );

	if((ui->Flags & PROFILES_FLAG_INVISIBLE) == 0)
		strcpy(str1, RADIO_CHECKED);
	else *str1 = 0;

	if((ui->Flags & PROFILES_FLAG_PERSMSGDISABLED) != 0)
		strcpy(str2, RADIO_CHECKED);
	else *str2 = 0;

	if((ui->Flags & PROFILES_FLAG_PERSMSGTOEMAIL) != 0)
		strcpy(str3, RADIO_CHECKED);
	else *str3 = 0;

	if((ui->Flags & PROFILES_FLAG_ALWAYS_EMAIL_ACKN) != 0)
		strcpy(str4, RADIO_CHECKED);
	else *str4 = 0;

    if((ui->Flags & PROFILES_FLAG_VIEW_SETTINGS) != 0)
        strcpy(str5, RADIO_CHECKED);
    else *str5 = 0;
			
	fui->SelectedUsers[PROFILES_FULL_USERINFO_MAX_SELECTEDUSR - 1] = 0;	// FIX
   
	printf(
"<TR><TD COLSPAN=2 ALIGN=CENTER><STRONG>%s</STRONG><BR><TEXTAREA COLS=60 ROWS=3 NAME=\"about\" WRAP=VIRTUAL>%s</TEXTAREA></TD></TR>" \
"<TR><TD COLSPAN=2 ALIGN=CENTER><STRONG>%s</STRONG><BR><TEXTAREA COLS=60 ROWS=3 NAME=\"sign\" WRAP=VIRTUAL>%s</TEXTAREA></TD></TR>" \
"<TR><TD COLSPAN=2 ALIGN=CENTER><STRONG>%s</STRONG><BR><TEXTAREA COLS=30 ROWS=4 NAME=\"susr\" WRAP=VIRTUAL>%s</TEXTAREA></TD></TR>" \
"<TR><TD COLSPAN=2 ALIGN=CENTER><STRONG>%s</STRONG><INPUT TYPE=CHECKBOX NAME=\"vprf\" VALUE=1%s></TD></TR>" \
"<TR><TD COLSPAN=2 ALIGN=CENTER><STRONG>%s</STRONG><INPUT TYPE=CHECKBOX NAME=\"apem\" VALUE=1%s></TD></TR>"	\
"<TR><TD COLSPAN=2 ALIGN=CENTER><STRONG>%s</STRONG><INPUT TYPE=CHECKBOX NAME=\"pdis\" VALUE=1%s></TD></TR>" \
"<TR><TD COLSPAN=2 ALIGN=CENTER><STRONG>%s</STRONG><INPUT TYPE=CHECKBOX NAME=\"peml\" VALUE=1%s></TD></TR>" \
"<TR><TD COLSPAN=2 ALIGN=CENTER><STRONG>%s</STRONG><INPUT TYPE=CHECKBOX NAME=\"vprs\" VALUE=1%s></TD></TR>",
			MESSAGEMAIN_register_about, fui->AboutUser,
			MESSAGEMAIN_register_signature, fui->Signature,
			MESSAGEMAIN_register_selectedusers, fui->SelectedUsers,
			MESSAGEMAIN_register_private_prof, str1,
			MESSAGEMAIN_register_always_emlackn, str4,
			MESSAGEMAIN_register_pmsg_disable, str2,
			MESSAGEMAIN_register_pmsg_email, str3,
			MESSAGEMAIN_register_view_saving, str5
		);

	printf("<TR><TD COLSPAN=2></TR><TR><TD COLSPAN=2 ALIGN=CENTER><B><FONT COLOR=RED>" \
		   MESSAGEMAIN_register_req_fields "</FONT><B></TR><TR><TD COLSPAN=2>" \
		   "<HR ALIGN=CENTER WIDTH=80%% NOSHADE></TR><TR><TD COLSPAN=2 ALIGN=CENTER>");

	/* print buttons */
	if(flags & 0x01)
		printf("<INPUT TYPE=SUBMIT NAME=\"register\" VALUE=\"%s\">&nbsp;", MESSAGEMAIN_register_register);

	if(flags & 0x02)
		printf("<INPUT TYPE=SUBMIT NAME=\"register\" VALUE=\"%s\">&nbsp;",  MESSAGEMAIN_register_edit);

	if(flags & 0x04)
		printf(
		"<INPUT TYPE=SUBMIT NAME=\"register\" VALUE=\"" MESSAGEMAIN_register_delete "\">" 
		"<INPUT TYPE=CHECKBOX NAME=\"" CONFIRM_DELETE_CHECKBOX_TEXT "\" VALUE=\"true\">" 
		MESSAGEMAIN_register_confirm_delete);

	printf("</TD></TR>" DESIGN_END_REGISTER_CLOSE "</FORM>");
}


void PrintSessionsList(DWORD Uid)
{
	char **buf = NULL;
	char name[1000];
	DWORD sc = 0, i;
	DWORD seqid[2], userid;
	if(ULogin.GenerateListSessionForUser(&buf, &sc, Uid)){

		if(sc){
			printf(DESIGN_BEGIN_USERINFO_INTRO_OPEN);
			for(i = 0; i < sc; i++) {
				unsigned char *seqip = (unsigned char*)(buf[i]+4);
				seqid[0] = *((DWORD*)(buf[i]+8));
				seqid[1] = *((DWORD*)(buf[i]+12));
				userid = *((DWORD*)(buf[i]+16));
				time_t seqtime = *((DWORD*)(buf[i]));
				char *seqdate;
				if( seqtime > time(NULL)) seqdate = (char*)ConvertFullTime(seqtime-USER_SESSION_LIVE_TIME);
				else seqdate = (char*)ConvertFullTime(seqtime);

				printf("<TR><TD ALIGN=RIGHT>%ld. "MESSAGEMAIN_session_ip"</TD><TD"
					" ALIGN=LEFT><STRONG>%u.%u.%u.%u</STRONG> %s</TD></TR>",
					i+1, seqip[0] & 0xff, seqip[1] & 0xff, seqip[2] & 0xff, seqip[3] & 0xff, 
					userid & SEQUENCE_IP_CHECK_DISABLED ? MESSAGEMAIN_session_ip_nocheck : MESSAGEMAIN_session_ip_check );
					 
				printf("<TR><TD ALIGN=RIGHT>"MESSAGEMAIN_session_date"</TD><TD ALIGN=LEFT>"
					" <STRONG>%s</STRONG></TD></TR>", seqdate);
				printf("<TR><TD ALIGN=RIGHT>"MESSAGEMAIN_session_state"</TD><TD ALIGN=LEFT><STRONG>");

				if( seqtime > time(NULL) ) {
					printf(MESSAGEMAIN_session_state_active);

					if( (ULogin.LU.right & USERRIGHT_SUPERUSER) != 0  || ( ULogin.LU.UniqID == Uid && ( ULogin.LU.right & USERRIGTH_PROFILE_MODIFY) != 0 ) )
						printf(" [<a href=\"%s?clsession1=%ld&clsession2=%ld\">"MESSAGEMAIN_session_state_toclose"</a>]", MY_CGI_URL, seqid[0], seqid[1]);
				}
				else  printf(MESSAGEMAIN_session_state_closed);
				printf("</STRONG></TD></TR><TR><TD><BR></TD></TR>");
				free(buf[i]);
			}
			printf(DESIGN_END_USERINFO_INTRO_CLOSE);
		}
		else printf(DESIGN_BEGIN_USERINFO_INTRO_OPEN "<TR><TD ALIGN=CENTER><B>"
			MESSAGEMAIN_session_no "</B></TD></TR>" DESIGN_END_USERINFO_INTRO_CLOSE);
	}
	else printhtmlerror();
	if(buf) free(buf);
}
																																																																																																																																									
int PrintAboutUserInfo(char *name)
{
	char *nickname;
	CProfiles *mprf = new CProfiles();
	if(mprf->errnum != PROFILE_RETURN_ALLOK) {
#if ENABLE_LOG >= 1
		print2log("error working with profiles database (init)");
#endif
		return 0;
	}
	SProfile_FullUserInfo fui;
	SProfile_UserInfo ui;

	nickname = FilterHTMLTags(name, 1000);

	if(mprf->GetUserByName(name, &ui, &fui, NULL) != PROFILE_RETURN_ALLOK)
	{
		delete mprf;
		printf(MESSAGEMAIN_profview_no_user, nickname);

		return 1;
	}

/**********/
	if(strcmp(name, "www") == 0) print2log("Profile www was accessed by %s", getenv("REMOTE_ADDR"));
/**********/

	printf("<P></P>" DESIGN_BEGIN_USERINFO_INTRO_OPEN
		"<TD COLSPAN=2><BIG>%s %s</BIG></TD></TR>", MESSAGEMAIN_profview_intro, nickname);
	
	printf("<TR><TD COLSPAN=2><HR ALIGN=CENTER WIDTH=80%% NOSHADE></TR>");

	printf("<TR><TD ALIGN=RIGHT>%s</TD><TD><STRONG>%s</STRONG><SMALL>", MESSAGEMAIN_profview_login, nickname );
	

	if(ULogin.LU.UniqID == ui.UniqID){
		printf(" <A HREF=\"?register=form\">(%s)</A>", MESSAGEMAIN_profview_editinfo);
	}
	
	if(ULogin.LU.ID[0] && ULogin.LU.UniqID != ui.UniqID){
		printf(" <A HREF=\"?persmsgform=%s\">(%s)</A>", CodeHttpString(name), MESSAGEMAIN_profview_postpersmsg);
	}

	printf("</SMALL></TD></TR>");

	if((ui.Flags & PROFILES_FLAG_ALT_DISPLAY_NAME) != 0) {
		char *st;
		if(!PrepareTextForPrint(ui.altdisplayname, &st, ui.secheader/*security*/,
			MESSAGE_ENABLED_TAGS | BOARDTAGS_PURL_ENABLE))
		{
			st = (char*)malloc(1000);
			strcpy(st, ui.altdisplayname);
		}
		printf("<TR><TD ALIGN=RIGHT>%s</TD><TD><STRONG>%s</STRONG></TD></TR>",
				MESSAGEMAIN_profview_altname, st);
		free(st);
	}

	if(((ui.Flags & PROFILES_FLAG_INVISIBLE) == 0) || (ULogin.LU.right & USERRIGHT_SUPERUSER) ||
		(ULogin.LU.UniqID == ui.UniqID) )
	{
		printf("<TR><TD ALIGN=RIGHT>%s</TD><TD><STRONG>%s</STRONG></TD></TR>" \
			"<TR><TD ALIGN=RIGHT>%s</TD><TD><STRONG><A HREF=\"%s\">%s</A></STRONG></TD></TR>",
			MESSAGEMAIN_profview_fullname,	fui.FullName,
			MESSAGEMAIN_profview_homepage,	fui.HomePage, fui.HomePage);
		
		/* if invisible mail - allow view only for same user or superuser */
		if((ui.Flags & PROFILES_FLAG_VISIBLE_EMAIL) || (ULogin.LU.right & USERRIGHT_SUPERUSER) ||
			(ULogin.LU.UniqID == ui.UniqID)) {
			printf("<TR><TD ALIGN=RIGHT>%s</TD><TD><STRONG><A HREF=mailto:%s>%s</A></STRONG></TD></TR>",
				MESSAGEMAIN_profview_email, fui.Email, fui.Email);
		}
		else printf("<TR><TD ALIGN=RIGHT>%s</TD><TD><STRONG><FONT COLOR=#0000F0>%s</FONT></STRONG></TD></TR>",
			MESSAGEMAIN_profview_email, MESSAGEMAIN_profview_privacy_inf);
		
		/* possible error (with malloc) here */
		char *regdate, *logdate = (char*)malloc(255), *ustatus = (char*)malloc(255);
		/*************************************/
		if(!ui.LoginDate) strcpy(logdate, "Never logged in");
		else strcpy(logdate, (char*)ConvertFullTime(ui.LoginDate));
		regdate = (char*)ConvertFullTime(fui.CreateDate);

		// set up ustatus
		if((ui.right & USERRIGHT_SUPERUSER) && ((ULogin.LU.right & USERRIGHT_SUPERUSER) || (strcmp(name, "www") /*&& strcmp(name, "Jul'etka") */))) {
			strcpy(ustatus, MESSAGEMAIN_profview_u_moderator);
		}
		else {
			strcpy(ustatus, MESSAGEMAIN_profview_u_user);
		}

		//	check for too high value in status
		if(ui.Status >= USER_STATUS_COUNT) ui.Status = USER_STATUS_COUNT - 1;	// maximum status

		//strcat(ustatus, " ( ");
		//strcat(ustatus, UserStatus_List[ui.Status]);
		//strcat(ustatus, " )");

		ui.icqnumber[15] = 0;	//	FIX

		char icqpic[1000];
		icqpic[0] = 0;

		if(strlen(ui.icqnumber) > 0) {
			sprintf(icqpic, "<TR><TD ALIGN=RIGHT>%s</TD><TD><STRONG>" \
				"<IMG src=\"http://online.mirabilis.com/scripts/online.dll?icq=%s&img=5\">%s</STRONG></TD></TR><BR>",
				MESSAGEMAIN_profview_user_icq, ui.icqnumber, ui.icqnumber);
		}

		printf("%s<TR><TD ALIGN=RIGHT>%s</TD><TD><STRONG>%s</STRONG></TD></TR>" \
"<TR><TD ALIGN=RIGHT>%s</TD><TD><STRONG>%ld</STRONG></TD></TR>" \
"<TR><TD ALIGN=RIGHT>%s</TD><TD><STRONG>%s</STRONG></TD></TR>" \
"<TR><TD ALIGN=RIGHT>%s</TD><TD><STRONG>%s</STRONG></TD></TR>" \
"<TR><TD ALIGN=RIGHT>%s</TD><TD><STRONG>%s</STRONG></TD></TR>",
			   icqpic,
			   MESSAGEMAIN_profview_user_status,	ustatus,
			   MESSAGEMAIN_profview_postcount,		ui.postcount,
			   MESSAGEMAIN_profview_reg_date,		regdate,
			   MESSAGEMAIN_profview_login_date,		logdate,
			   MESSAGEMAIN_profview_about_user,		fui.AboutUser);

		if((ULogin.LU.right & USERRIGHT_SUPERUSER) || (ULogin.LU.UniqID == ui.UniqID) ) {
			char hname[10000];
			hostent *he;
			unsigned char *aa = (unsigned char *)(&ui.lastIP);
			sprintf(hname, "%u.%u.%u.%u", aa[0] & 0xff, aa[1] & 0xff, aa[2] & 0xff, aa[3] & 0xff);
			if((he = gethostbyaddr((char*)(&ui.lastIP), 4, AF_INET)) != NULL) {
				// prevent saving bad hostname
				if(strlen(he->h_name) > 0) {
					char tmp[1000];

					strcpy(tmp, hname);
					strncpy(hname, he->h_name, 9999);
					strcat(hname, " (");
					strcat(hname, tmp);
					strcat(hname, ")");
				}
			}
			// only for admin :)
			if((ULogin.LU.right & USERRIGHT_SUPERUSER) != 0)
				printf("<TR><TD ALIGN=RIGHT>%s</TD><TD><STRONG>%d</STRONG></TD></TR>", 
					MESSAGEMAIN_profview_refreshcnt, ui.RefreshCount);
			printf("<TR><TD ALIGN=RIGHT>%s</TD><TD><STRONG>%s</STRONG></TD></TR>",
				MESSAGEMAIN_profview_lastip, hname);
			printf("<TR><TD ALIGN=RIGHT>%s</TD><TD><STRONG>%d(%d)</STRONG></TD></TR>",
			        MESSAGEMAIN_profview_persmsgcnt, ui.persmescnt, ui.readpersmescnt);
		}

		free(logdate);
		free(ustatus);
	}
	else {
		printf("<TR><TD COLSPAN=2><BIG>%s</BIG></TD></TR><BR>", MESSAGEMAIN_profview_privacy_prof);
	}
	printf(DESIGN_END_USERINFO_INTRO_CLOSE);


	printf("<BR>");

  if((ULogin.LU.right & USERRIGHT_SUPERUSER) || (ULogin.LU.UniqID == ui.UniqID) ) {
	PrintSessionsList(ui.UniqID);

  }

	//
	//	if admin print profile modification form
	//
	if((ULogin.LU.right & USERRIGHT_SUPERUSER)) {
		char sel[100];
		BYTE i;
		printf("<BR><CENTER><FORM METHOD=POST ACTION=\"%s?changeusr=action\"> User Status: ", MY_CGI_URL);

		printf("<SELECT NAME=\"ustat\">");
		for(i = 0; i < USER_STATUS_COUNT; i++) {

			if(i == ui.Status) strcpy(sel, LISTBOX_SELECTED);
			else strcpy(sel, "");

			printf("<OPTION VALUE=\"%d\"%s>%s", i, sel, UserStatus_List[i]);
		}
		printf("</SELECT>");

		printf("<BR>" MESSAGEMAIN_profview_sechdr "<INPUT TYPE=TEXT SIZE=3 NAME=\"sechdr\" VALUE=\"%d\">", (DWORD)ui.secheader);
		printf("<BR>" MESSAGEMAIN_profview_secbdy "<INPUT TYPE=TEXT SIZE=3 NAME=\"secbdy\" VALUE=\"%d\">", (DWORD)ui.secur);
		printf("<INPUT TYPE=HIDDEN SIZE=0 NAME=\"name\" VALUE=\"%s\">", nickname);

		// user rights here
		puts("<BR><P><TABLE BORDER=1 CELLSPACING=0 CELLPADDING=6 BGCOLOR=#FFFFFF><TR><TH ALIGN=RIGHT>");
		for(i = 0; i < USERRIGHT_COUNT; i++)
		{
			if( (ui.right & (1<<i)) != 0)
				strcpy(sel, RADIO_CHECKED);
			else sel[0] = 0;
			printf("%s <INPUT TYPE=CHECKBOX NAME=\"right%d\"%s><BR>",
				UserRight_List[i], i, sel);
		}
		puts("</TH></TR></TABLE>");

		printf("<BR><BR><INPUT TYPE=SUBMIT NAME=\"update\" VALUE=\"%s\">", MESSAGEMAIN_register_edit);

		printf("</FORM></CENTER>");
	}

	free(nickname);
	delete mprf;

	return 1;
}

// string compare
static int cmp_name(const void *p1, const void *p2)
{
	char upper[2][AUTHOR_NAME_LENGTH];
	strcpy(upper[0], (*(char **)p1) + 20);
	strcpy(upper[1], (*(char **)p2) + 20);
	for(int i = 0; i < 2; i++) 
		toupperstr(upper[i]);
	
	return strcmp(upper[0], upper[1]);
}

// by last ip
static int cmp_ip(const void *p1, const void *p2)
{
	return int( ntohl((*((DWORD**)p1))[0]) - ntohl((*((DWORD**)p2))[0]) );
}

// by postcount
static int cmp_postcount(const void *p1, const void *p2)
{
	return int( (*((DWORD**)p2))[1] - (*((DWORD**)p1))[1] );
}

// by refresh count
static int cmp_refreshcount(const void *p1, const void *p2)
{
	return int( (*((DWORD**)p2))[3] - (*((DWORD**)p1))[3] );
}

// by last login date
static int cmp_date(const void *p1, const void *p2)
{
	return int( (*((DWORD**)p2))[2] - (*((DWORD**)p1))[2] );
}

// by security right
static int cmp_right(const void *p1, const void *p2)
{
	return int( (*((DWORD**)p1))[4] - (*((DWORD**)p2))[4] );
}

void PrintUserList(DB_Base *dbb, int code)
{
	char **buf = NULL;
	char name[1000];
	DWORD uc = 0, i;
	CProfiles uprof;

	if(!uprof.GenerateUserList(&buf, &uc))
		printhtmlerror();

	// Print header of user list
	printf("<CENTER><P><B>%s</B><BR>%s%d<P>", MESSAGEHEAD_userlist, MESSAGEMAIN_total_user_count, uc);

	switch(code) {
		case 1:
			qsort((void *)buf, uc, sizeof(char*), cmp_name);
			break;
		case 2:
			qsort((void *)buf, uc, sizeof(char*), cmp_postcount);
			break;
		case 3:
			qsort((void *)buf, uc, sizeof(char*), cmp_ip);
			break;
		case 4:
			qsort((void *)buf, uc, sizeof(char*), cmp_date);
			break;
		case 5:
			qsort((void *)buf, uc, sizeof(char*), cmp_refreshcount);
			break;
		case 6:
			qsort((void *)buf, uc, sizeof(char*), cmp_right);
			break;
	}

	// print sort link bar
	if((ULogin.LU.right & USERRIGHT_SUPERUSER)) {
		printf("<B>%s</B>", MESSAGEMAIN_userlist_sortby);
		if(code != 1) printf("<A HREF=\"%s?userlist=1\">%s</A> | ", MY_CGI_URL, MESSAGEMAIN_userlist_sortbyname);
		else printf("<B>%s</B> | ", MESSAGEMAIN_userlist_sortbyname);
		if(code != 2) printf("<A HREF=\"%s?userlist=2\">%s</A> | ", MY_CGI_URL, MESSAGEMAIN_userlist_sortbypcnt);
		else printf("<B>%s</B> | ", MESSAGEMAIN_userlist_sortbypcnt);
		if(code != 3) printf("<A HREF=\"%s?userlist=3\">%s</A> | ", MY_CGI_URL, MESSAGEMAIN_userlist_sortbyhost);
		else printf("<B>%s</B> | ", MESSAGEMAIN_userlist_sortbyhost);
		if(code != 4) printf("<A HREF=\"%s?userlist=4\">%s</A> | ", MY_CGI_URL, MESSAGEMAIN_userlist_sortbydate);
		else printf("<B>%s</B> | ", MESSAGEMAIN_userlist_sortbydate);
		if(code != 5) printf("<A HREF=\"%s?userlist=5\">%s</A> | ", MY_CGI_URL, MESSAGEMAIN_userlist_sortbyrefresh);
		else printf("<B>%s</B> | ", MESSAGEMAIN_userlist_sortbyrefresh);
		if(code != 6) printf("<A HREF=\"%s?userlist=6\">%s</A><BR><BR>", MY_CGI_URL, MESSAGEMAIN_userlist_sortbyright);
		else printf("<B>%s</B><BR><BR>", MESSAGEMAIN_userlist_sortbyright);
	}

	DWORD oldval;
	if(uc) {
		unsigned char *aa = (unsigned char*)buf[0];
		switch(code) {
			case 2:
				oldval = *((DWORD*)(buf[0] + 4));
				printf("<B>(%lu)</B><BR>", *((DWORD*)(buf[0] + 4)));
				break;
			case 3:
				oldval = *((DWORD*)(buf[0]));
				printf("<B>(%u.%u.%u.%u)</B><BR>", aa[0] & 0xff, aa[1] & 0xff, aa[2] & 0xff, aa[3] & 0xff);
				break;
			case 4:
				// not used yet
				break;
			case 5:
				oldval = *((DWORD*)(buf[0] + 12));
				printf("<B>(%lu)</B><BR>", *((DWORD*)(buf[0] + 12)));
				break;
			case 6:
				oldval = *((DWORD*)(buf[0] + 16));
				printf("<B>(%08x)</B><BR>", *((DWORD*)(buf[0] + 16)));
				break;
		}
	}
	int cc = 0;
	// print begin
	for(i = 0; i < uc; i++) {
		switch(code) {
			case 2:
				if(oldval != *((DWORD*)(buf[i] + 4))) {
					printf("<BR><BR><B>(%lu)</B><BR>", *((DWORD*)(buf[i] + 4)));
					oldval = *((DWORD*)(buf[i] + 4));
					cc = 0;
				}
				break;
			case 3:
				if(oldval != *((DWORD*)(buf[i]))) {
					unsigned char *aa = (unsigned char*)buf[i];
					printf("<BR><BR><B>(%u.%u.%u.%u)</B><BR>", aa[0] & 0xff, aa[1] & 0xff, aa[2] & 0xff, aa[3] & 0xff);
					oldval = *((DWORD*)(buf[i]));
					cc = 0;
				}
				break;
			case 4:
				break;
			case 5:
				if(oldval != *((DWORD*)(buf[i] + 12))) {
					printf("<BR><BR><B>(%lu)</B><BR>", *((DWORD*)(buf[i] + 12)));
					oldval = *((DWORD*)(buf[i] + 12));
					cc = 0;
				}
				break;
			case 6:
				if(oldval != *((DWORD*)(buf[i] + 16))) {
					printf("<BR><BR><B>(%08x)</B><BR>", *((DWORD*)(buf[i] + 16)));
					oldval = *((DWORD*)(buf[i] + 16));
					cc = 0;
				}
				break;
		}
		if((cc % 10) != 0) printf(" | ");
		if(((cc % 10) == 0) && cc != 0) {
			// print end and begin
			printf("</CENTER><BR><CENTER>");
		}
		cc++;

		dbb->Profile_UserName(buf[i] + 20, name, 1);
		printf("%s", name);
		free(buf[i]);
	}
	if(buf) free(buf);
	printf("</CENTER>");
}

/* create or update user profile
 * if op == 1 - create
 * if op == 2 - update
 * if op == 3 - delete
 */
int CheckAndCreateProfile(SProfile_UserInfo *ui, SProfile_FullUserInfo *fui, char *p2, char *oldp, int op, char *deal)
{
	CProfiles *uprof;
	DWORD err = 0, needregisteraltnick = 0;

	/* password check */
	if((ULogin.LU.right & USERRIGHT_SUPERUSER) == 0) {
		/* old password */
		if(op != 1 && (ULogin.LU.ID[0] == 0 || (strcmp(oldp, ULogin.pui->password) != 0) ||
			(strcmp(ULogin.pui->username, ui->username) != 0)))
			return PROFILE_CHK_ERROR_INVALID_PASSWORD;
	}
	/* password and confirm password */
	if(op != 3 && (strcmp(ui->password, p2) != 0 || (strlen(p2) < PROFILES_MIN_PASSWORD_LENGTH) && strlen(p2) != 0))
		return PROFILE_CHK_ERROR_INVALID_PASSWORD_REP;
	
	if(strlen(p2) == 0) {
		if(ULogin.LU.ID[0] != 0) strcpy(ui->password, ULogin.pui->password);
		else return PROFILE_CHK_ERROR_INVALID_PASSWORD_REP;
	}

	// ************ delete ************
	if(op == 3) {
		if((ULogin.LU.ID[0] == 0 || strcmp(ULogin.pui->username, ui->username) != 0 ||
			(ULogin.LU.right & USERRIGTH_PROFILE_MODIFY) == 0) && (!(ULogin.LU.right & USERRIGHT_SUPERUSER)))
			return PROFILE_CHK_ERROR_CANNOT_DELETE_USR;

		uprof = new CProfiles();

		SProfile_UserInfo nui;
		err = uprof->GetUserByName(ui->username, &nui, fui, NULL);
		if(err == PROFILE_RETURN_ALLOK) {
			// check for special admitions
			if(nui.altdisplayname[0] != 0 && (nui.Flags & PROFILES_FLAG_ALT_DISPLAY_NAME) ) {
				// need to delete alternative spelling too
				ui->UniqID = nui.UniqID;
				needregisteraltnick = 1;
			}

			// deleting user
			err = uprof->DeleteUser(ui->username);
			// check result of operation
			if(err == PROFILE_RETURN_ALLOK) {
				if(ULogin.LU.UniqID == nui.UniqID)
					ULogin.CloseSession(ULogin.LU.ID); // user deleted itself
				else ULogin.ForceCloseSessionForUser(nui.UniqID); // superuser delete smb.
			}
		}
		goto cleanup_and_parseerror;
	}

	/* Now we know that it can't be user deletion, so check common parameters */

	if(fui->Signature[0] != 0) {
		ui->Flags = ui->Flags | PROFILES_FLAG_HAVE_SIGNATURE;
	}

	if(ui->altdisplayname[0] != 0 && /*security check*/ (ULogin.LU.ID[0] != 0 && (ULogin.LU.right & USERRIGHT_ALT_DISPLAY_NAME) != 0 ) ) {
		ui->Flags |= PROFILES_FLAG_ALT_DISPLAY_NAME;
		needregisteraltnick = 1;
	}
	else {
		ui->Flags &= (~PROFILES_FLAG_ALT_DISPLAY_NAME);
		if( (ULogin.LU.ID[0] != 0 && (ULogin.LU.right & USERRIGHT_ALT_DISPLAY_NAME) != 0 ) ) needregisteraltnick = 1;
	}

	/* common email check (if requred vaild email) */
	if(IsMailCorrect(fui->Email) == 0)
		return PROFILE_CHK_ERROR_INVALID_EMAIL;

	uprof = new CProfiles();

	// ********** update **********
	if(op == 2) {
		err = uprof->ModifyUser(ui, fui, NULL);
		goto cleanup_and_parseerror;
	}

	// ********** create **********
	if(op == 1) {
		ui->lastIP = Nip;
		err = uprof->AddNewUser(ui, fui, NULL);
		goto cleanup_and_parseerror;
	}

	delete uprof;

	printhtmlerror();

cleanup_and_parseerror:

	delete uprof;

	switch(err) {
	case PROFILE_RETURN_ALLOK:
		{
			// Do post user creation/modificaton job
#if USER_ALT_NICK_SPELLING_SUPPORT
			if(op == 1 || op == 2 && needregisteraltnick) {
				if(ui->altdisplayname[0] != 0) {
					char *st;

					// parse tags
					if(!PrepareTextForPrint(ui->altdisplayname, &st, ui->secheader/*security*/,
							MESSAGE_ENABLED_TAGS | BOARDTAGS_PURL_ENABLE)) {
						st = (char*)malloc(1000);
						strcpy(st, ui->altdisplayname);
					}

					AltNames.AddAltName(ui->UniqID, ui->username, st);
					free(st);
				}
				else AltNames.DeleteAltName(ui->UniqID);
			}
			else if(op == 3 && needregisteraltnick) {
				AltNames.DeleteAltName(ui->UniqID);
			}
#endif
		}
		return PROFILE_CHK_ERROR_ALLOK;

	case PROFILE_RETURN_ALREADY_EXIST:
		return PROFILE_CHK_ERROR_ALREADY_EXIST;

	case PROFILE_RETURN_DB_ERROR:
#if ENABLE_LOG >= 1
		print2log("Profiles database error: DB ERROR, deal=%s", deal);
#endif 
		printhtmlerror();

	case PROFILE_RETURN_INVALID_FORMAT:
#if ENABLE_LOG >= 1
		print2log("Profiles database error: INVALID FORMAT, deal=%s", deal);
#endif
		printhtmlerror();

	case PROFILE_RETURN_INVALID_LOGIN:
		if(op == 1 || op == 2)
			return PROFILE_CHK_ERROR_INVALID_LOGIN_SPELL;
		return PROFILE_CHK_ERROR_CANNOT_DELETE_USR;

	case PROFILE_RETURN_INVALID_PASSWORD:
		return PROFILE_CHK_ERROR_INVALID_PASSWORD;

	case PROFILE_RETURN_PASSWORD_SHORT:
		return PROFILE_CHK_ERROR_SHORT_PASSWORD;

	case PROFILE_RETURN_UNKNOWN_ERROR:
	default:
#if ENABLE_LOG >= 1
		print2log("profiles database error : UNKNOWN, deal=%s", deal);
#endif 
		printhtmlerror();
	}
}

void PrintFavouritesList()
{	

	printf("<CENTER><P><B>%s</B><BR>", MESSAGEHEAD_favourites);
	//for( int i=0; i<20; i++){
	//			printf("ULogin.pui->favs[%ld]=%ld<br>",i, ULogin.pui->favs[i]);
	//}
	//DB.PrintHtmlMessageBufferByVI(ULogin.pui->favs, PROFILES_FAV_THREADS_COUNT);
	printf("</CENTER>");
}


/* complete operation with user profile and print 
 * result of execution
 * op == 1 create
 * op == 2 update
 * op == 3 delete
 */
void DoCheckAndCreateProfile(SProfile_UserInfo *ui, SProfile_FullUserInfo *fui, char *passwdconfirm, char *oldpasswd, int op, char *deal)
{
	int err;
	DWORD delme = 0;
	
	if(ULogin.LU.ID[0] != 0 && strcmp(ui->username, ULogin.pui->username) == 0 && op == 3) {
		delme = 1;
	}
	
	if(op == 1 ) {
		char *f = FilterWhitespaces(ui->username);
		memmove(ui->username, f, strlen(f) + 1);
	}

	print2log("DoCheckAndCreateProfile: user '%s', op=%d, (by %s)", ui->username, op, ULogin.LU.ID[0] != 0 ? ULogin.pui->username : "anonymous");
	err = CheckAndCreateProfile(ui, fui, passwdconfirm, oldpasswd, op, deal);

	if(err != PROFILE_CHK_ERROR_ALLOK)
		PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);

	char *tmp;

	switch(err) {
	case PROFILE_CHK_ERROR_ALLOK:
		if(op == 3) {
			PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE,
			MAINPAGE_INDEX);
			if(!delme)
				PrintBoardError(MESSAGEMAIN_register_delete_ex, MESSAGEMAIN_register_delete_ex2,
					HEADERSTRING_REFRESH_TO_MAIN_PAGE);
			else
				PrintBoardError(MESSAGEMAIN_register_delete_logoff, MESSAGEMAIN_register_delete_logoff2,
					HEADERSTRING_REFRESH_TO_MAIN_PAGE);
		}
		else {
			if(op == 1) {
				/* if session already opened - close it */
				if(ULogin.LU.ID[0] != 0)
					ULogin.CloseSession(ULogin.LU.ID);

				if(ULogin.OpenSession(ui->username, ui->password, NULL, Nip, 0) == 1) {
					// entered, set new cookie
					cookie_name = (char*)realloc(cookie_name, AUTHOR_NAME_LENGTH);
					strcpy(cookie_name, ui->username);
				}
				PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE,
					MAINPAGE_INDEX);
				PrintBoardError(MESSAGEMAIN_register_create_ex, MESSAGEMAIN_register_create_ex2,
					HEADERSTRING_REFRESH_TO_MAIN_PAGE);
			}
			else {
				PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE,
					MAINPAGE_INDEX);
				PrintBoardError(MESSAGEMAIN_register_edit_ex, MESSAGEMAIN_register_edit_ex2,
					HEADERSTRING_REFRESH_TO_MAIN_PAGE);
			}
		}

		PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE,
			MAINPAGE_INDEX);
		break;
	case PROFILE_CHK_ERROR_ALREADY_EXIST:
		PrintBoardError(MESSAGEMAIN_register_already_exit, MESSAGEMAIN_register_already_exit2, 0);
		break;
	case PROFILE_CHK_ERROR_NOT_EXIST:
		PrintBoardError(MESSAGEMAIN_register_invalid_psw, MESSAGEMAIN_register_invalid_psw2, 0);
		break;
	case PROFILE_CHK_ERROR_INVALID_LOGIN_SPELL:
		PrintBoardError(MESSAGEMAIN_register_invalid_lg_spell, MESSAGEMAIN_register_invalid_lg_spell2, 0);
		break;
	case PROFILE_CHK_ERROR_INVALID_PASSWORD:
		PrintBoardError(MESSAGEMAIN_register_invalid_psw, MESSAGEMAIN_register_invalid_psw2,
			HEADERSTRING_REFRESH_TO_MAIN_PAGE);
		break;
	case PROFILE_CHK_ERROR_INVALID_PASSWORD_REP:
		PrintBoardError(MESSAGEMAIN_register_invalid_n_psw, MESSAGEMAIN_register_invalid_n_psw2, 0);
		break;
	case PROFILE_CHK_ERROR_SHORT_PASSWORD:
		PrintBoardError(MESSAGEMAIN_register_invalid_n_psw, MESSAGEMAIN_register_invalid_n_psw2, 0);
		break;
	case PROFILE_CHK_ERROR_INVALID_EMAIL:
		PrintBoardError(MESSAGEMAIN_register_invalid_email, MESSAGEMAIN_register_invalid_email2, 0);
		break;
	case PROFILE_CHK_ERROR_CANNOT_DELETE_USR:
		/* possible BUG here */
		tmp = (char*)malloc(1000);
		/*********************/
		sprintf(tmp, MESSAGEMAIN_register_cannot_delete, ui->username);
		PrintBoardError(tmp, MESSAGEMAIN_register_cannot_delete2,
			HEADERSTRING_REFRESH_TO_MAIN_PAGE);
		free(tmp);
		break;
	default:
		printhtmlerror();
	}

	if(err != PROFILE_CHK_ERROR_ALLOK)
		PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
}

void printaccessdenied(char *deal)
{
	Tittle_cat(TITLE_Error);
	PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
#if ENABLE_LOG > 2
	print2log(LOG_UNKNOWN_URL, Cip, deal);
#endif
	PrintBoardError(MESSAGEMAIN_access_denied, MESSAGEMAIN_access_denied2, 0);
	PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
}

void printpassworderror(char *deal)
{
	/* incorrect username or password */
#if ENABLE_LOG > 1
	print2log(LOG_PSWDERROR, Cip, deal);
#endif

	Tittle_cat(TITLE_IncorrectPassword);
				
	/* incorrect login and/or password */
	PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
	PrintBoardError(MESSAGEMAIN_incorrectpwd, MESSAGEMAIN_incorrectpwd2, 0);
	PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
}

void printnomessage(char *deal)
{
	Tittle_cat(TITLE_Error);
#if ENABLE_LOG > 2
	print2log(LOG_UNKNOWN_URL, Cip, deal);
#endif
	PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
	PrintBoardError(MESSAGEMAIN_nonexistingmsg, MESSAGEMAIN_nonexistingmsg2, 0);
	PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
}

void printbadurl(char *deal)
{
	Tittle_cat(TITLE_Error);

#if ENABLE_LOG > 2
	print2log(LOG_UNKNOWN_URL, Cip, deal);
#endif
	PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
	PrintBoardError(MESSAGEMAIN_requesterror, MESSAGEMAIN_requesterror2, 0);
	PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
}

/* print message thread */
int PrintMessageThread(DB_Base *bd, DWORD root, DWORD Flag, DWORD Puser)
{
	// print admin toolbar
	PrintAdminToolbar(root, Flag, Puser);

	bd->DB_PrintMessageBody(root);
	bd->DB_PrintMessageThread(root);
	return 0;	
}

//rewritten for IIS compatibility
// Return not more than maxlen bytes (including '\0')
char* GetParams(DWORD maxlen)
{
	char* pCL = getenv("CONTENT_LENGTH");
	if(pCL != NULL)
	{
		int nCL = atoi(pCL);
		if(nCL > maxlen) nCL = maxlen;
		if(nCL > 0)
		{
			DWORD ret;
			char* szBuf = (char*)malloc(nCL + 3);
			if(szBuf == NULL) {
#if _DEBUG_ == 1
				print2log("GetParams::malloc - out of memory");
#endif
				return NULL;
			}
			if((ret = (DWORD)fread(szBuf, sizeof(char), nCL, stdin)) == 0) {
#if _DEBUG_ == 1
				print2log("GetParams::fread - failed");
#endif
				goto cleanup;	
			}
			szBuf[ret] = '&';	// patch string for our parser
			szBuf[ret+1] = '\0';
//			print2log("POST params = %s, readed = %d", szBuf, ret);
			return szBuf;
cleanup:	
			free(szBuf);
		}
	}
	else {
#if _DEBUG_ == 1
		print2log("No CONTENT_LENGTH env. value");
#endif
	}
	return NULL;
}

/* parse hex symbol to value */
char inline parsesym(char s)
{
	s = (char)toupper(s);
	if(s >= '0' && s <= '9') return (char)(s - '0');
	if(s >= 'A' && s <= 'F') return (char)(s - 'A' + 10);
	return 20;
}

/* find and return key [find] in string [par] between [find] and "&"
* par is limited to MAX_PARAMETERS_LENGTH - 1
*/
char* strget(char *par,const char *find, WORD maxl, char end, bool argparsing)
{
	bool bZend = false;
	char *res, *rres;
	char *s, *x;
	char a, b;
	if(par == NULL) return NULL;
	if(find == NULL) return NULL;
	if((s = strstr(par, find)) == NULL) return NULL;
	if(bZend = ((x = strchr(s, end)) == NULL))
		x = strchr(s, 0);
	else
		*x = 0; // temporary change '&' to '\0'
	rres = res = (char *)malloc(maxl + 1);
	s = s + strlen(find);
	if(argparsing) {
		while(*s != '\0' && res - rres < maxl) {
			if(*s == '%' && x > s + 2) {
				if((a = parsesym(*(s + 1))) == 20) {
					s++; continue; // ignore invalid %
				}
				if((b = parsesym(*(s + 2))) == 20) {
					s++; continue; // ignore invalid %
				}
				*res = (char)(a*16 + b);
				s+=2;
			}
			else if(*s == '+') *res = ' ';
			else *res = *s;
			res++;
			s++;
		}
		*res = 0;
		rres = (char*)realloc(rres, strlen(rres) + 1);
	}
	else {
		strncpy(rres, s, maxl);
		rres[maxl] = 0;	// fix for any string
	}
	if(!bZend)
		*x = end;
	return rres;
}

// get cookie information, if avaliable
void ParseCookie()
{
	char *c, *s, *ss, *t, *st;
	DWORD tmp;
	int tmp_int;
	
	GlobalNewSession = 1;
	currentft = 1;
	currentlt = 1;
	currentfm = 1;
	currentlm = 1;
	currentlann = 0;
	
	cookie_ss = CONFIGURE_SETTING_DEFAULT_ss;
	cookie_tt = CONFIGURE_SETTING_DEFAULT_tt;
	cookie_tv = CONFIGURE_SETTING_DEFAULT_tv;
	cookie_tc = CONFIGURE_SETTING_DEFAULT_tc;
	cookie_lsel = CONFIGURE_SETTING_DEFAULT_lsel;
	cookie_dsm = CONFIGURE_SETTING_DEFAULT_dsm;
	cookie_tz = DATETIME_DEFAULT_TIMEZONE;
	
#if	TOPICS_SYSTEM_SUPPORT
	cookie_topics = CONFIGURE_SETTING_DEFAULT_topics;
	topicsoverride = CONFIGURE_SETTING_DEFAULT_toverride;
#endif

	s = getenv("HTTP_COOKIE");
	//if(s) print2log(s);
	
	if(s != NULL) {
		c = (char*)malloc(strlen(s) + 1);
		strcpy(c, s);

		// After this strget() we will have all %XX parsed ! So we should
		// disable %XX parsing
		if((ss = strget(c, COOKIE_NAME_STRING, COOKIE_MAX_LENGTH, '&')) != NULL) {
			ss = (char*)realloc(ss, strlen(ss)+2);
			strcat(ss,"|");
			
			
			if(t = strget(ss, "name=", AUTHOR_NAME_LENGTH - 1, '|', 0)){
				cookie_name = t;
			}

			
			if(t = strget(ss, "seq=", 30, '|', 0)){
				cookie_seq = t;
			}

		
			
			if(t = strget(ss, "ed=", 20, '|', 0)) {
				RefreshDate = strtol(t, &st, 10);
				if((!((*t) != '\0' && *st == '\0')) || errno == ERANGE)
				{
					RefreshDate = 0;
				}
				free(t);
			}


			// read lsel (show type selection)
			if(t = strget(ss, "lsel=", 3, '|', 0)) {
				tmp = strtol(t, &st, 10);
				if(( (*t) != '\0' && *st == '\0') && errno != ERANGE && tmp <= 2 && tmp >= 1)
				{
					cookie_lsel = tmp;
				}
				free(t);
			}
		
			// read tc (thread count)
			if(t = strget(ss, "tc=", 12, '|', 0)) {
				tmp = strtol(t, &st, 10);
				if(((*t) != '\0' && *st == '\0') && errno != ERANGE && tmp > 0)
				{
					cookie_tc = tmp;
				}
				free(t);
			}

			// read tt (time type)
			if(t = strget(ss, "tt=", 3, '|', 0)) {
				tmp = strtol(t, &st, 10);
				if(((*t) != '\0' && *st == '\0') && errno != ERANGE && tmp <= 4 && tmp > 0)
				{
					cookie_tt = tmp;
				}
				free(t);
			}
				
			// read ss (style string)
			if(t = strget(ss, "ss=", 12, '|', 0)) {
				tmp = strtol(t, &st, 10);
				if(((*t) != '\0' && *st == '\0') && errno != ERANGE && tmp > 0 && tmp <= 4)
				{
					cookie_ss = tmp;
				}
				free(t);
			}
		
			// read tv (time value)
			if(t  = strget(ss, "tv=", 12, '|', 0)) {
				tmp = strtol(t, &st, 10);
				if(((*t) != '\0' && *st == '\0') && errno != ERANGE && tmp > 0)
				{
					cookie_tv = tmp;
				}
				free(t);
			}

			// read lt (last thread)
			if(t = strget(ss, "lt=", 12, '|', 0)){
				tmp = strtol(t, &st, 10);
				if(((*t) != '\0' && *st == '\0') && errno != ERANGE && tmp > 0)
				{
					currentlt = tmp;
				}
				free(t);
			}
					
			// read fm (first message)
			if(t = strget(ss, "ft=", 12, '|', 0)) {
				tmp = strtol(t, &st, 10);
				if(((*t) != '\0' && *st == '\0') && errno != ERANGE && tmp > 0)
				{
					currentft = tmp;
				}
				free(t);
			}
				
			// read lm (last message)
			if(t = strget(ss, "lm=", 12, '|', 0)){
				tmp = strtol(t, &st, 10);
				if(((*t) != '\0' && *st == '\0') && errno != ERANGE && tmp > 0)
				{
					currentlm = tmp;
				}
				free(t);
			}
			
			// read fm (first message)
			if(t = strget(ss, "fm=", 12, '|', 0)){
				tmp = strtol(t, &st, 10);
				if(((*t) != '\0' && *st == '\0') && errno != ERANGE && tmp > 0)
				{
					currentfm = tmp;
				}
				free(t);
			}
			
			// read dsm (globally disable smiles, picture, and 2-d link bar)
			if(t = strget(ss, "dsm=", 12, '|', 0)) {
				tmp = strtol(t, &st, 10);
				if(((*t) != '\0' && *st == '\0') && errno != ERANGE && tmp > 0)
				{
				cookie_dsm = tmp;
				}
				free(t);
			}
			
#if	TOPICS_SYSTEM_SUPPORT
			// read topics
			if(t = strget(ss, "topics=", 20, '|', 0)) {
				tmp = strtol(t, &st, 10);
				if(((*t) != '\0' && *st == '\0') && errno != ERANGE)
				{
				cookie_topics = tmp;
				}
				free(t);
			}

			// read topics override
			if(t = strget(ss, "tovr=", 12, '|', 0)) {
				tmp = strtol(t, &st, 10);
				if(((*t) != '\0' && *st == '\0') && errno != ERANGE)
				{
				topicsoverride = tmp;
				}
				free(t);
			}
#endif

			// read lann (last hided announce)
			if(t = strget(ss, "lann=", 12, '|', 0)) {
				tmp = strtol(t, &st, 10);
				if(((*t) != '\0' && *st == '\0') && errno != ERANGE && tmp >= 0)
				{
				currentlann = tmp;
				ReadLastAnnounceNumber(&tmp);
				if(currentlann > tmp) currentlann = tmp;
				}
				free(t);
			}
			
			// read timezone
			if(t = strget(ss, "tz=", 12, '|', 0) ) {
				tmp_int = strtol(t, &st, 10);
				if(((*t) != '\0' && *st == '\0') && errno != ERANGE && tmp_int >= -12 && tmp_int <= 12)
				{
					cookie_tz = tmp_int;
				}
				free(t);
			}

			free(ss);
	
		}
		
	
		if((ss = strget(c, COOKIE_SESSION_NAME, 12, '&')) && strcmp(ss, "on") == 0) {
			GlobalNewSession = 0;
			free(ss);
		}
			
		free(c);
	}

	if(!cookie_name){
		cookie_name = (char*)malloc(1);
		cookie_name[0] = 0;
	}
	
	if(!cookie_seq) {
		cookie_seq = (char*)malloc(1);
		cookie_seq[0] = 0;
	}
}

// calculate time limit for printing messages
void calc_print_time()
{
	if(currentlsel == 1) {
		// calculate time limit
		current_minprntime = time(NULL);
		switch(currenttt) {
		case 1:
			// hours
			current_minprntime = current_minprntime - 3600*currenttv;
			break;
		case 2:
			// days
			current_minprntime = current_minprntime - 3600*24*currenttv;
			break;
		case 3:
			// weeks
			current_minprntime = current_minprntime - 3600*24*7*currenttv;
			break;
		case 4:
			// months
			current_minprntime = current_minprntime - 3600*24*31*currenttv;
			break;
		}
	}
}

static void PrepareActionResult(int action, char **c_par1, char **c_par2)
{
	switch(action) {
	case MSG_CHK_ERROR_NONAME :
		*c_par1 = MESSAGEMAIN_add_no_name;
		*c_par2 = MESSAGEMAIN_add_no_name2;
		break;
	case MSG_CHK_ERROR_NOMSGHEADER:
		*c_par1 = MESSAGEMAIN_add_no_subject;
		*c_par2 = MESSAGEMAIN_add_no_subject2;
		break;
	case MSG_CHK_ERROR_NOMSGBODY:
		*c_par1 = MESSAGEMAIN_add_no_body;
		*c_par2 = MESSAGEMAIN_add_no_body2;
		break;
	case MSG_CHK_ERROR_BADSPELLING:
		*c_par1 = MESSAGEMAIN_add_spelling;
		*c_par2 = MESSAGEMAIN_add_spelling2;
		break;
#if BANNED_CHECK
	case MSG_CHK_ERROR_BANNED:
		*c_par1 = MESSAGEMAIN_add_banned;
		*c_par2 = MESSAGEMAIN_add_banned2;
		break;
#endif
	case MSG_CHK_ERROR_CLOSED:
		*c_par1 = MESSAGEMAIN_add_closed;
		*c_par2 = MESSAGEMAIN_add_closed2;
		break;
	case MSG_CHK_ERROR_INVALID_REPLY:
		*c_par1 = MESSAGEMAIN_add_invalid_reply;
		*c_par2 = MESSAGEMAIN_add_invalid_reply;
		break;
	case MSG_CHK_ERROR_INVALID_PASSW:
		*c_par1 = MESSAGEMAIN_incorrectpwd;
		*c_par2 = MESSAGEMAIN_incorrectpwd2;
		break;
	default:
		*c_par1 = MESSAGEMAIN_unknownerr;
		*c_par2 = MESSAGEMAIN_unknownerr2;
		break;
	}
}

int ConvertHex(char *s, char *res)
{
	int i = 0;
	if(!s) return 0;
	while(*s != 0 && *(s+1) != 0) {
#define FROM_HEX(x) ((x >= 'A' && x <= 'F') ? (x-'A' + 10) : \
	(x >= 'a' && x <= 'f') ? (x-'a' + 10) : (x - '0'))
		res[i] = FROM_HEX(*s)*16 + FROM_HEX(*(s+1));
		i++;
		s+=2;
	}
	return i;
}

int main()
{
	char *deal, *st, *mesb;
	char *par; // parameters string
	char *tmp;
	int initok = 0;
	DB_Base DB;
	SMessage mes;

#ifndef WIN32

	if(!isEnoughSpace()) {
		printf("Content-type: text/html\n\n"
		"Sorry guys, no space left for DB - wwwconf shutting down.");
		exit(1);
	}

#endif

#ifdef RT_REDIRECT
#define BADURL "/board/"
#define GOODURL "http://board.rt.mipt.ru/"
	if((st = getenv(REQUEST_URI)) != NULL)
	{
		deal = (char*)malloc(strlen(st) + 2);
		strcpy(deal, st);
		//fprintf(stderr,"req uri: %s\n",deal);
		if (strncmp(deal, BADURL, strlen(BADURL)) == 0 ) { 
			tmp = (char*)malloc(strlen(deal) + strlen(GOODURL) + 10);
			sprintf(tmp,"%s%s",GOODURL,deal + strlen(BADURL));
			//fprintf(stderr,"redir: %s\n",tmp);
			HttpRedirect(tmp);
			free(tmp);
			goto End_part;
		}	
		free(deal);
	}
#endif

#define UA_LINKS	"Links"
#define UA_LYNX		"Lynx"
#define UA_NN202	"Mozilla/2.02"
	// will we use <DIV> or <DL>?
	if( ((st = getenv("HTTP_USER_AGENT")) != NULL) && (*st != '\0') )
	{
		deal = (char*)malloc(strlen(st) + 2);
		strcpy(deal, st);
		if ( (strncmp(deal, UA_LINKS, strlen(UA_LINKS)) == 0) ||
			 (strncmp(deal, UA_LYNX,  strlen(UA_LYNX))  == 0) ||
			 (strncmp(deal, UA_NN202, strlen(UA_NN202)) == 0) ) {
			strcpy(DESIGN_open_dl, DESIGN_OP_DL);
			strcpy(DESIGN_open_dl_grey, DESIGN_OP_DL);
			strcpy(DESIGN_open_dl_white, DESIGN_OP_DL);
			strcpy(DESIGN_close_dl, DESIGN_CL_DL);
			strcpy(DESIGN_break, DESIGN_DD);
			strcpy(DESIGN_threads_divider, DESIGN_THREADS_DIVIDER_HR);
			initok = 1;
		}
		free(deal);
	}
	if (!initok) {
			strcpy(DESIGN_open_dl, DESIGN_OP_DIV);
			strcpy(DESIGN_open_dl_grey, DESIGN_OP_DIV_grey);
			strcpy(DESIGN_open_dl_white, DESIGN_OP_DIV_white);
			strcpy(DESIGN_close_dl, DESIGN_CL_DIV);
			strcpy(DESIGN_break, DESIGN_BR);
			strcpy(DESIGN_threads_divider, DESIGN_THREADS_DIVIDER_IMG);
	}

#if defined(WIN32) && defined(IP_TO_HOSTNAME_RESOLVE)

	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0 ) {
		// Tell the user that we could not find a usable
		// WinSock DLL, but here just do nothing and bailing out
		return -1;
	}

#endif

	/*
	// -----------------------------
	char *s = (char *)&mes;
	for(int i=0;i<sizeof(SMessage);i++) s[i]=0;
	s = (char *)&mbody;
	for(i=0;i<sizeof(SMessageBody);i++) s[i]=0;
	strcpy(mes.AuthorName,"www");
	strcpy(mes.MessageHeader,"1111111");
	strcpy(mes.HostName,"194.85.83.213");
	char sx[100] = "[i][code];))111 ;)serial[url=http:\\www.ru]serial[/url][code]";
	i = DB.DB_InsertMessage(&mes, 0, strlen(sx), sx, &mbody, 0xFFFFFFF, "www");
	goto End_part;
	// -----------------------------
	*/

#if USE_LOCALE
	/* set locale */
	setlocale(LC_ALL, LANGUAGE_LOCALE);
#endif

	/* get cookie string, if available, and parse it */
	ParseCookie();
	

#if STABLE_TITLE == 0
	// set default title
	ConfTitle = (char*)malloc(strlen(TITLE_WWWConfBegining) + 1);
	strcpy(ConfTitle, TITLE_WWWConfBegining);
#endif



	// get parameters with we have been run
	if((st = getenv(QUERY_STRING)) != NULL)
	{
		deal = (char*)malloc(strlen(st) + 2);
		strcpy(deal, st);
	}
	else deal = NULL;
	
	if(deal == NULL || (strcmp(deal,"") == 0))
	{
		deal = (char*)malloc(20);
		strcpy(deal,"index");
	}

	// detect IP
	if((tmp = getenv(REMOTE_ADDR)) != NULL)
	{
		Cip = (char*)malloc(strlen(tmp) + 1);
		strcpy(Cip, tmp);
	}
	else {
		Cip = (char*)malloc(strlen(TAG_IP_NOT_DETECTED) + 1);
		strcpy(Cip, TAG_IP_NOT_DETECTED);
	}

	// translate IP
	// if it fails, we will have Nip = 0
	char *tst, *tms;
	tst = Cip;
	{
		for(register DWORD i = 0; i < 4; i++)
		{
			if((tms = strchr(tst,'.')) != NULL || (tms = strchr(tst,'\0')) != NULL)
			{
				*tms = '\0';
				((char*)(&Nip))[i] = (unsigned char)atoi(tst);
				tst = tms + 1;
				if(i < 3) *tms = '.';
			}
			else break;
		}
	}
	if(Nip == 0) Nip = 1;

#if ACTIVITY_LOGGING_SUPPORT
	// user activity logging
	DWORD hostcnt, hitcnt;
	RegisterActivityFrom(Nip, hitcnt, hostcnt);
#endif

#if _DEBUG_ == 1
	//	print2log("Entering from : %s, deal=%s", Cip, deal);
#endif
	
	strcat(deal,"&");

	/************ get user info from session ************/
	{
		DWORD tmp[2];
		if(strlen(cookie_seq) == 16 &&
			ConvertHex(cookie_seq, (char*)&tmp) == 8)
		{
			tmp[0] = ntohl(tmp[0]);	// use network order due printf()
			tmp[1] = ntohl(tmp[1]);
			// if session code not zero let's open session
			if(tmp[0] != 0 && tmp[1] != 0)
			{
				// try to open sequence
				if(ULogin.CheckSession(tmp, Nip, 0))
					strcpy(cookie_name, ULogin.pui->username);
			}
		}
	}
	
	//	
	// checking settings were saved in profile or cookies and  restoring them.
	//

	if(ULogin.LU.ID[0] && (ULogin.pui->Flags & PROFILES_FLAG_VIEW_SETTINGS)  ) { 

		currentdsm = ULogin.pui->vs.dsm;
		currenttopics = ULogin.pui->vs.topics;
		currenttv = ULogin.pui->vs.tv;
		currenttc = ULogin.pui->vs.tc;
		currentss = ULogin.pui->vs.ss;
		currentlsel = ULogin.pui->vs.lsel;
		currenttt = ULogin.pui->vs.tt;
		currenttz = ULogin.pui->vs.tz;
	}
	else{
		currentlsel = cookie_lsel;
		currenttc = cookie_tc;
		currenttt = cookie_tt;
		currenttv = cookie_tv;
		currentss = cookie_ss;
		currentdsm = cookie_dsm;
		currenttopics = cookie_topics;
		currenttz = cookie_tz;
	}

	// caluclate minimal message print time
	calc_print_time();


	//security check
	if(ULogin.LU.ID[0] && (ULogin.LU.right & USERRIGHT_SUPERUSER) )
		print2log("Superuser: %s from %s - %s", ULogin.pui->username, Cip, deal);


	//==========================	
	// detecting user wishes %)
	//==========================
	
	
	
	if(strncmp(deal, "resetnew", 8) == 0) {
		// apply old last message value
		currentlm = currentfm;
		currentlt = currentft;
		cookie_lastenter =  time(NULL);
		// set new read time value
		PrintHTMLHeader(HEADERSTRING_REDIRECT_NOW | HEADERSTRING_NO_CACHE_THIS, MAINPAGE_INDEX);
		
		goto End_part;
	}

	if(strncmp(deal, "index", 5) == 0)
	{
		/* security check */
		if((ULogin.LU.right & USERRIGHT_VIEW_MESSAGE) == 0)
		{
			printaccessdenied(deal);
			goto End_part;
		}

		if((st = strget(deal,"index=", 16, '&')) != NULL)
		{
			int entok = 0;
			if(strcmp(st, "all") == 0) {
				topicsoverride = TOPICS_COUNT + 50;
				entok = 1;
			}
			else {
				errno = 0;
				char *ss;
				DWORD tmp = strtol(st, &ss, 10);
				if((!(*st != '\0' && *ss == '\0')) || errno == ERANGE || tmp > TOPICS_COUNT)
				{
					// just print common index
				}
				else {
					topicsoverride = tmp;
					entok = 1;
				}
			}
			if(entok) {
				PrintHTMLHeader(HEADERSTRING_REDIRECT_NOW | HEADERSTRING_NO_CACHE_THIS, MAINPAGE_INDEX);
				goto End_part;
			}
		}

#if TOPIC_SYSTEM_SUPPORT
#if STABLE_TITLE == 0
		// add current topic to index title
		if (topicsoverride > 0 && topicsoverride <= TOPICS_COUNT) {
			Tittle_cat(Topics_List[topicsoverride-1]);
		} else if (topicsoverride > (TOPICS_COUNT + 1)) {
			Tittle_cat(MESSAGEMAIN_WELCOME_ALLTOPICS);
		}
#endif
#endif


		//	Apply new last message and thread value
		DWORD readed = DB.VIndexCountInDB();
		DWORD mtc;
		//	Read main threads count
		if(!DB.ReadMainThreadCount(&mtc)) mtc=0;
		
		currentfm = readed;
		currentft = mtc;
		//
		//	Here we make a decision about new session of "+"
		//
		if(GlobalNewSession)
		{
			// apply old last message value
			currentlm = currentfm;
			currentlt = currentft;
			// set new read time value
			cookie_lastenter = time(NULL);
		}


		PrintHTMLHeader(HEADERSTRING_REG_USER_LIST | HEADERSTRING_POST_NEW_MESSAGE |
			HEADERSTRING_CONFIGURE | HEADERSTRING_WELCOME_INFO |
			HEADERSTRING_ENABLE_RESETNEW | HEADERSTRING_DISABLE_END_TABLE, MAINPAGE_INDEX);

		//	Prepare information about new message count and dispaly mode */
		char displaymode[500];		// display message mode
		char displaynewmsg[500];	// new message info
		char topicselect[2000];		// topic select: MAYBE BUG HERE IF TOO MANY TOPICS
		char privmesinfo[500];		// private message info
		char activityloginfo[500];	// user activity info
		topicselect[0] = 0;
		privmesinfo[0] = 0;
		activityloginfo[0] = 0;

		DWORD a = currentfm - currentlm;
		DWORD t = ((currentft - currentlt) >= 0) ? (currentft - currentlt) : 0;
		DWORD totalcount = DB.MessageCountInDB();
		if(a) sprintf(displaynewmsg, MESSAGEMAIN_WELCOME_NEWTHREADS, t, a, totalcount);
		else sprintf(displaynewmsg, MESSAGEMAIN_WELCOME_NONEWTHREADS, totalcount);

		// current settings in welcome message
		if(currentlsel == 1)
		{
			// hours
			sprintf(displaymode, MESSAGEMAIN_WELCOME_DISPLAYTIME, currenttv, MESSAGEHEAD_timetypes[currenttt-1]);
		}
		else
		{
			// threads
			sprintf(displaymode, MESSAGEMAIN_WELCOME_DISPLAYTHREADS, currenttc);
		}

#if	TOPICS_SYSTEM_SUPPORT
		{
		char tmp[500], sel[50], sel2[50];
		sel[0] = sel2[0] = 0;
		//	Prepare topic list
		if(topicsoverride == 0) strcpy(sel, LISTBOX_SELECTED);
		if(topicsoverride > TOPICS_COUNT) strcpy(sel2, LISTBOX_SELECTED);
		sprintf(topicselect, DESIGN_WELCOME_QUICKNAV, sel, MESSAGEMAIN_WELCOME_YOURSETTINGS, sel2, MESSAGEMAIN_WELCOME_ALLTOPICS);
		for(DWORD i = 0; i < TOPICS_COUNT; i++) {
			if(Topics_List_map[i] == (topicsoverride - 1)) strcpy(sel, LISTBOX_SELECTED);
			else sel[0] = 0; // ""
			sprintf(tmp, "<OPTION VALUE=\"?index=%d\"%s>%s</OPTION>\n", Topics_List_map[i]+1, sel, Topics_List[Topics_List_map[i]]);
			strcat(topicselect, tmp);
		}
		strcat(topicselect, "</SELECT>");
		}
#endif

		// moved to PrintTopStaticLinks //printf("<TR><TD class=cl>&nbsp;</TD></TR>");
		
		// print info about personal messages
		if(ULogin.LU.ID[0] != 0 && ULogin.pui->persmescnt - ULogin.pui->readpersmescnt > 0) {
			sprintf( privmesinfo, ", <A HREF=\"" MY_CGI_URL "?persmsg\" STYLE=\"text-decoration:underline;\"><FONT COLOR=RED>" MESSAGEMAIN_privatemsg_newmsgann " %ld " \
			MESSAGEMAIN_privatemsg_newmsgann1 "</FONT></A>", (ULogin.pui->persmescnt - ULogin.pui->readpersmescnt) );
		}

#if ACTIVITY_LOGGING_SUPPORT
		sprintf(activityloginfo, MESSAGEMAIN_ACTIVITY_STATVIEW, hitcnt, hostcnt);
#endif

		int formstarted = 0;
		if(ULogin.LU.ID[0] == 0) {
			printf(MESSAGEMAIN_WELCOME_START, PROFILES_MAX_USERNAME_LENGTH,
				FilterHTMLTags(cookie_name, 1000, 0),
				PROFILES_MAX_PASSWORD_LENGTH, displaynewmsg, displaymode,
				activityloginfo, topicselect);
			formstarted = 1;
		}
		else {
			char uname[1000];
			DB.Profile_UserName(ULogin.pui->username, uname, 1);
			printf(MESSAGEMAIN_WELCOME_LOGGEDSTART, uname, privmesinfo, displaynewmsg,
				displaymode, activityloginfo, topicselect);
		}


		
		// Announce going here
		{
			SGlobalAnnounce *ga;
			DWORD cnt, i;
			if(ReadGlobalAnnounces(0, &ga, &cnt) != ANNOUNCES_RETURN_OK) printhtmlerror();
			if(cnt) {
				char uname[1000], del[1000], date[100];
				int something_printed = 0;
				for(i = 0; i < cnt; i++) {
					if(ga[i].Number > currentlann) {
						DB.Profile_UserName(ga[i].From, uname, 1);

						char *st = FilterHTMLTags(ga[i].Announce, MAX_PARAMETERS_STRING - 1);
						char *st1 = NULL;
						DWORD retflg;
						if(FilterBoardTags(st, &st1, 0, MAX_PARAMETERS_STRING - 1, 
							MESSAGE_ENABLED_SMILES | MESSAGE_ENABLED_TAGS | BOARDTAGS_PURL_ENABLE |
							BOARDTAGS_EXPAND_ENTER, &retflg) == 0)
						{
							st1 = st;
							st = NULL;
						}

						if(((ULogin.LU.right & USERRIGHT_POST_GLOBAL_ANNOUNCE) != 0 && ga[i].UIdFrom ==
							ULogin.LU.UniqID) || (ULogin.LU.right & USERRIGHT_SUPERUSER) != 0)
						{
							sprintf(del, "<A HREF=\"" MY_CGI_URL "?ganndel=%d\" "
								"STYLE=\"text-decoration:underline;\">" MESSAGEMAIN_globann_delannounce
								"</A> <A HREF=\"" MY_CGI_URL "?globann=%d\" "
								"STYLE=\"text-decoration:underline;\">" MESSAGEMAIN_globann_updannounce
								"</A>", ga[i].Number, ga[i].Number);
					}	
						else del[0] = 0;

						ConvertTime(ga[i].Date, date);

						if(!something_printed) {
							if(!formstarted) printf("<BR>");
							else printf("</FORM>");
						}

						printf(DESIGN_GLOBALANN_FRAME, DESIGN_open_dl, st1, MESSAGEMAIN_globann_postedby,
							uname, date, del, DESIGN_close_dl);

						if(st) free(st);
						if(st1) free(st1);
						something_printed = 1;
					}
				}
				if(something_printed) {
					printf("<CENTER><A HREF=\"?rann=%d\" STYLE=\"text-decoration:underline;\" class=cl>" MESSAGEMAIN_globann_hidenewann "</A></CENTER><BR>", ga[cnt-1].Number);
				}
				else {
					// show all announces
					printf("<CENTER><A HREF=\"?rann=0\" STYLE=\"text-decoration:underline;\" class=cl>" MESSAGEMAIN_globann_showall "(%d)</A></CENTER><BR>", cnt);
					// gap between welcome header and messages
					if(formstarted) printf("</FORM>");
					else printf(DESIGN_INDEX_WELCOME_CLOSE);
				}
			}
			else {
				// gap between welcome header and messages
				if(formstarted) printf("</FORM>");
				else printf(DESIGN_INDEX_WELCOME_CLOSE);
			}
			if(ga) free(ga);
		} // of announces

		DWORD oldct = currenttopics;
		
		if(topicsoverride) {
			if(topicsoverride > TOPICS_COUNT) currenttopics = 0xffffffff;	// all
			else currenttopics = (1<<(topicsoverride-1));
		}
		DB.DB_PrintHtmlIndex(time(NULL), time(NULL), mtc);
		currenttopics = oldct;
		PrintBottomLines(HEADERSTRING_REG_USER_LIST | HEADERSTRING_POST_NEW_MESSAGE | HEADERSTRING_CONFIGURE | HEADERSTRING_ENABLE_RESETNEW, MAINPAGE_INDEX);
		goto End_part;
	}
	
	if(strncmp(deal, "read", 4) == 0)
	{
		/* security check */
		if((ULogin.LU.right & USERRIGHT_VIEW_MESSAGE) == 0)
		{
			printaccessdenied(deal);
			goto End_part;
		}

		if((st = strget(deal,"read=", 16, '&')) != NULL)
		{
			errno = 0;
			char *ss;
			DWORD tmp = strtol(st, &ss, 10);
			DWORD x;
			if((!(*st != '\0' && *ss == '\0')) || errno == ERANGE ||
				tmp < 1 || (x = DB.TranslateMsgIndex(tmp)) == NO_MESSAGE_CODE)
			{
				printnomessage(deal);
			}
			else
			{
				// read message
				if(!ReadDBMessage(x, &mes)) printhtmlerror();

				/* allow read invisible message only to SUPERUSER */
				if((mes.Flag & MESSAGE_IS_INVISIBLE) && ((ULogin.LU.right & USERRIGHT_SUPERUSER) == 0)) {
					printnomessage(deal);
				}
				else
				{
#if STABLE_TITLE == 0
					// change title
					char *an;
					DWORD xtmp;
					if(FilterBoardTags(mes.MessageHeader, &an, mes.SecHeader,
						MAX_PARAMETERS_STRING, MESSAGE_ENABLED_TAGS | BOARDTAGS_CUT_TAGS, &xtmp) != 1)
					{
						an = (char*)&(mes.MessageHeader);
					}
					if (mes.Topics < TOPICS_COUNT && mes.Topics  != 0 ){
						char *t;
						t = (char*)malloc(strlen(an) + strlen(TITLE_divider) + strlen(Topics_List[mes.Topics]) + 4);
						*t = 0;
						strcat(t, Topics_List[mes.Topics]);
						strcat(t, TITLE_divider);
						strcat(t, an);
						Tittle_cat(t);
						free(t);
					} else 
						Tittle_cat(an);

					if(an != (char*)&(mes.MessageHeader)) free(an);
#endif
					// tmpxx contains vindex of parent message if thread is rolled.
					// if some sub-thread is rolled, tmpxx contains vindex of MAIN parent of thread
					
					DWORD tmpxx;
					if (mes.Flag & MESSAGE_COLLAPSED_THREAD && mes.Level > 0){
						SMessage parmes;
						if(!ReadDBMessage(mes.ParentThread, &parmes)) printhtmlerror();
						tmpxx = parmes.ViIndex;
					}
					else tmpxx = tmp;

					PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_ENABLE_TO_MESSAGE |
						((currentdsm & 0x100) ? HEADERSTRING_ENABLE_REPLY_LINK : 0), tmpxx, tmp);
					
					PrintMessageThread(&DB, tmp, mes.Flag, mes.UniqUserID);

					/* allow post to closed message only to SUPERUSER and USER  */
					if( (((mes.Flag & MESSAGE_IS_CLOSED) == 0 &&
						(ULogin.LU.right & USERRIGHT_CREATE_MESSAGE) )  ||
						(ULogin.LU.right & USERRIGHT_SUPERUSER) != 0 ) && 
						((currentfm < MAX_DELTA_POST_MESSAGE) || 
						(tmp > (currentfm - MAX_DELTA_POST_MESSAGE)) || 
						(tmp == 0)) && ((currentdsm & 0x100) == 0))
					{
						strcpy(mes.AuthorName, cookie_name);
						mes.MessageHeader[0] = 0;
						mesb = (char*)malloc(1);
						*mesb = 0;
						PrintMessageForm(&mes, mesb, tmp, ACTION_BUTTON_POST | ACTION_BUTTON_PREVIEW | ACTION_BUTTON_FAKEREPLY);
						free(mesb);
					}

					PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_ENABLE_TO_MESSAGE, tmpxx, tmp);
				}
			}
			free(st);
		}
		else goto End_URLerror;
		goto End_part;
	}
	
	if(strncmp(deal, "form", 4) == 0)
	{
		DWORD repnum = 0;
		
		// read form= parameter (if reply form is required)
		if((st = strget(deal,"form=", 16, '&')) != NULL)
		{
			errno = 0;
			char *ss;
			repnum = strtol(st, &ss, 10);
			DWORD x;
			if((!(*st != '\0' && *ss == '\0')) || errno == ERANGE ||
				repnum < 1 || (x = DB.TranslateMsgIndex(repnum)) == NO_MESSAGE_CODE)
			{
				printnomessage(deal);
				free(st);
				goto End_part; 
			}
			free(st);
		}

		/* security check */
		if( (repnum && (ULogin.LU.right & USERRIGHT_CREATE_MESSAGE) == 0 ) ||
			( !repnum && (ULogin.LU.right & USERRIGHT_CREATE_MESSAGE_THREAD) == 0) ){
				printaccessdenied(deal);
				goto End_part;
		}
		
		if(!repnum) Tittle_cat(TITLE_Form);
		else Tittle_cat(TITLE_WriteReply);

		PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
		
		strcpy(mes.AuthorName, FilterHTMLTags(cookie_name, 1000, 0));
		mes.MessageHeader[0] = 0;
		mesb = (char*)malloc(1);
		*mesb = 0;
		PrintMessageForm(&mes, mesb, repnum,
			repnum ? ACTION_BUTTON_POST | ACTION_BUTTON_PREVIEW | ACTION_BUTTON_FAKEREPLY :
			ACTION_BUTTON_POST | ACTION_BUTTON_PREVIEW);
		PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
		goto End_part;
	}
	
	
	if(strncmp(deal, "xpost", 5) == 0) {
		char *ss = NULL;
		char *passw, *mb, *c_host;
		DWORD ROOT = 0;
		DWORD CFlags = 0, LogMeIn = 0;

		// read method GET post params
		if((ss = strget(deal, "xpost=", 16, '&')) != NULL ) {
			errno = 0;
			char *st;
			ROOT = strtol(ss, &st, 10);
			if((!(*ss != '\0' && *st == '\0')) || errno == ERANGE ||
				(ROOT != 0 && DB.TranslateMsgIndex(ROOT) == NO_MESSAGE_CODE)) {
				printnomessage(deal);
				goto End_part;
			}
			free(ss);
		}
		else {
			printbadurl(deal);
			goto End_part;
		}

		// Check security rights
		if(! ((ULogin.LU.right & USERRIGHT_CREATE_MESSAGE) && ROOT) &&
			!((ULogin.LU.right & USERRIGHT_CREATE_MESSAGE_THREAD) && ROOT == 0))
		{
			printaccessdenied(deal);
			goto End_part;
		}
			
			
		// make IP address
		if ((tmp = getenv("HTTP_X_FORWARDED_FOR")) != NULL) print2log("proxy %s - %s", tmp, deal);
		if(Nip != 1) {
		/*	// resolve
			char*tmp;
			// Exception! if forwareder for localhost or 127.0.0.1 - ignore forwarder
			if((tmp = getenv("HTTP_X_FORWARDED_FOR")) != NULL &&
				strcmp(tmp, "127.0.0.1") != 0 && strcmp(tmp, "localhost") != 0 &&
				strncmp(tmp, "192.168", 7) != 0 && strncmp(tmp, "10.", 3) != 0)
			{
				// TODO: more detailed log here
				print2log("proxy %s - %s", tmp, deal);
				
				// TODO: we need to resolve DNS here
				strncpy(mes.HostName, tmp, HOST_NAME_LENGTH - 1);
				mes.HostName[HOST_NAME_LENGTH - 1] = 0;
			} else
		*/ 
			if(!IP2HostName(Nip, mes.HostName, HOST_NAME_LENGTH - 1))
				strcpy(mes.HostName, Cip);
		}
		else strcpy(mes.HostName, TAG_IP_NOT_DETECTED);
		mes.IPAddr = Nip;

#if HTTP_REFERER_CHECK == 1
		char *useragent = getenv("HTTP_USER_AGENT");
		if(
			!useragent || 
			(strncmp(useragent, UA_LYNX, strlen(UA_LYNX)) &&
			strncmp(useragent, UA_LINKS, strlen(UA_LINKS)))
		) {
			char *tts = getenv("HTTP_REFERER");
			if(tts == NULL || strstr(tts, ALLOWED_HTTP_REFERER) == NULL) {
				// TODO: more detailed error here
				print2log("bad referer, tts='%s'", tts);
				printbadurl(deal);
				goto End_part;
			}
		}
#endif

		// get parameters
		par = GetParams(MAX_PARAMETERS_STRING);

		// read name
		st = strget(par,"name=",  AUTHOR_NAME_LENGTH - 1, '&');
		if(st == NULL) {
			strcpy(mes.AuthorName, "");
		}
		else {
			strncpy(mes.AuthorName, st, AUTHOR_NAME_LENGTH - 1);
			mes.AuthorName[AUTHOR_NAME_LENGTH - 1] = 0;
			free(st);
		}
		
		// read password
		passw = strget(par,"pswd=", PROFILES_MAX_PASSWORD_LENGTH - 1, '&');

		// read subject
		st = strget(par,"subject=", MESSAGE_HEADER_LENGTH - 1, '&');
		if(st == NULL) {
			strcpy(mes.MessageHeader, "");
		}
		else {
			strncpy(mes.MessageHeader, FilterWhitespaces(st), MESSAGE_HEADER_LENGTH - 1);
			mes.MessageHeader[MESSAGE_HEADER_LENGTH - 1] = 0;
			free(st);
		}

		// read host (for edit)
		c_host = strget(par,"host=", HOST_NAME_LENGTH - 1, '&');

		// read msg body
		mesb = strget(par,"body=", MAX_PARAMETERS_STRING, '&');
		// this is needed because of char #10 filtering.
		// in WIN32 printf() works incorrectly with it
#if defined(WIN32)
		FilterMessageForPreview(mesb, &mb);
		free(mesb);
		mesb = mb;
#endif
		
		// read dct (disable WWWConf Tags)
		st = strget(par,"dct=", 10, '&');
		if(st != NULL) {
			if(strcmp(st, "on") == 0) {
				CFlags = CFlags | MSG_CHK_DISABLE_WWWCONF_TAGS;
			}
			free(st);
		}
		
		// read dst (disable smile codes)
		st = strget(par,"dst=", 10, '&');
		if(st != NULL) {
			if(strcmp(st, "on") == 0) {
				CFlags = CFlags | MSG_CHK_DISABLE_SMILE_CODES;
			}
			free(st);
		}
		
		// read wen (acknol. by email)
		st = strget(par,"wen=", 10, '&');
		if(st != NULL) {
			// mail ackn. allowed ONLY for threads with ROOT == 0
			if(strcmp(st, "on") == 0 && ROOT == 0) {
				CFlags = CFlags | MSG_CHK_ENABLE_EMAIL_ACKNL;
			}
			free(st);
		}

		// read lmi (login me)
		st = strget(par,"lmi=", 10, '&');
		if(st != NULL) {
			if(strcmp(st, "on") == 0) {
				LogMeIn = 1;
			}
			free(st);
		}

		mes.Topics = 0;
#if TOPICS_SYSTEM_SUPPORT
		{
			char *ss;
			DWORD topicID;
			if((ss = strget(par, "topic=", 10, '&')) != NULL) {
				errno = 0;
				char *st;
				topicID = strtol(ss, &st, 10);
				if((!(*ss != '\0' && *st == '\0')) || errno == ERANGE || topicID > TOPICS_COUNT - 1)
				{
					// default topic
					mes.Topics = TOPICS_DEFAULT_SELECTED;
				}
				mes.Topics = topicID;
				free(ss);
			}
		}
#endif

		// get user action (post/edit/preview)
		int action = getAction(par);
		free(par);

		// init current error
		int cr = 0;
		switch(action) {
		case ACTION_POST:
			{
				//*************************************************
				//	post message
				//*************************************************

				// antispam
				if(CheckPostfromIPValidity(Nip, POST_TIME_LIMIT) == 0) {
#if ENABLE_LOG
					print2log(LOG_SPAM_TRY, Cip, deal);
#endif
					Tittle_cat(TITLE_Spamtry);

					PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
					PrintBoardError(MESSAGEMAIN_spamtry, MESSAGEMAIN_spamtry2, 0);
					PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
					if(passw) free(passw);
					goto End_part;
				}

				// do not allow posts to old threads
				if ( ROOT > 0 && ((DB.VIndexCountInDB() - ROOT) > MAX_DELTA_POST_MESSAGE) ) {
					PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP, ROOT);
					PrintBoardError(MESSAGEMAIN_add_closed, MESSAGEMAIN_add_closed2, 0);
					PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
					if(passw) free(passw);
					goto End_part;

				}
			
				mes.Date = time(NULL);	//	set current time
				mes.MDate = (time_t)0;	//	haven't modified
				char *banreason = NULL;
				if((cr = DB.DB_InsertMessage(&mes, ROOT, strlen(mesb), &mesb, CFlags, passw, &banreason)) != MSG_CHK_ERROR_PASSED)
				{
					char *c_ActionResult1, *c_ActionResult2;
					PrepareActionResult(cr, &c_ActionResult1, &c_ActionResult2);
					PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP, ROOT);
					PrintBoardError(c_ActionResult1, c_ActionResult2, 0);

					if(cr == MSG_CHK_ERROR_BANNED && banreason) {
						// print the reason if it exists
						printf(DESIGN_BAN_REASON_STYLE, MESSAGEMAIN_BANNED_REASON, banreason);
					}
					if(banreason) free(banreason);
				}
				else {
					//	Mark that IP as already posted
					CheckPostfromIPValidity(Nip, POST_TIME_LIMIT);

					//	posted, set new cookie
					cookie_name = (char*)realloc(cookie_name, AUTHOR_NAME_LENGTH);
					strcpy(cookie_name, mes.AuthorName);


					//
					//	Log in user if requested
					//
					if(LogMeIn && (passw != NULL && *passw != 0)) {
						LogMeIn = 0;
						/* if session already opened - close it */
						if(ULogin.LU.ID[0] != 0)
							ULogin.CloseSession(ULogin.LU.ID);

						if(ULogin.OpenSession(mes.AuthorName, passw, NULL, Nip, 0) == 1) {
							// entered, ok
							LogMeIn = 1;
						}
					}
					else LogMeIn = 0;

					//
					//	Check if message was posted to rolled thread than we should change ROOT to main root of thread
					//
					DWORD tmpxx;
					if((mes.Flag & MESSAGE_COLLAPSED_THREAD)) {
						SMessage parmes;
						if(!ReadDBMessage(mes.ParentThread, &parmes)) printhtmlerror();
						tmpxx = parmes.ViIndex;
					}
					else tmpxx = mes.ViIndex;

					PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP,
						tmpxx, ROOT);
					if(LogMeIn) {
						// if we have logged in also
						PrintBoardError(MESSAGEMAIN_add_ok_login, MESSAGEMAIN_add_ok2, HEADERSTRING_REFRESH_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_THREAD, mes.ViIndex);
					}
					else PrintBoardError(MESSAGEMAIN_add_ok, MESSAGEMAIN_add_ok2, HEADERSTRING_REFRESH_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_THREAD, mes.ViIndex);

					PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP, tmpxx, ROOT);

					if(passw != NULL) free(passw);

					goto End_part;
				}
			}
			break;
		case ACTION_PREVIEW:
			{
				//**********************************************
				//	preview message
				//**********************************************
				DWORD rf;
				SProfile_UserInfo UI;

				//
				//	set some fields of message
				//
				mes.ParentThread = NO_MESSAGE_CODE;
				mes.Date = time(NULL);
				mes.MDate = 0;
				mes.ViIndex = 0;

				if(ULogin.LU.ID[0] != 0) {
					memcpy(&UI, ULogin.pui, sizeof(UI));
					strcpy(mes.AuthorName, UI.username);
				}
				else {
					/* default user */
					UI.secur = DEFAULT_NOBODY_SECURITY_BYTE;
					UI.right = DEFAULT_NOBODY_RIGHT;
					UI.secheader = DEFAULT_NOBODY_HDR_SEC_BYTE;
					UI.UniqID = 0;
					UI.username[0] = 0;
				}

				if(UI.right & USERRIGTH_ALLOW_HTML) CFlags = CFlags | MSG_CHK_ALLOW_HTML;
				mes.Security = UI.secur;
				mes.SecHeader = UI.secheader;
				mes.UniqUserID = UI.UniqID;

				char *banreason = NULL;
				if((cr = CheckSpellingBan(&mes, &mesb, &banreason, CFlags, &rf)) != MSG_CHK_ERROR_PASSED)
				{
					char *c_ActionResult1, *c_ActionResult2;
					PrepareActionResult(cr, &c_ActionResult1, &c_ActionResult2);
					PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP, ROOT);
					PrintBoardError(c_ActionResult1, c_ActionResult2, 0);

					if(cr == MSG_CHK_ERROR_BANNED && banreason) {
						// print the reason if it exists
						printf(DESIGN_BAN_REASON_STYLE, MESSAGEMAIN_BANNED_REASON, banreason);
					}
				}
				if(banreason) free(banreason);

				mes.Flag = rf;
				PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);

				printf(DESIGN_PREVIEW_PREVIEWMESSAGE, MESSAGEHEAD_preview_preview_message);

				// fix message body size for print preview
				mes.msize = 1;	// any constant greater than zero

				DB.PrintHtmlMessageBody(&mes, mesb);

				printf(DESIGN_PREVIEW_CHANGEMESSAGE, MESSAGEHEAD_preview_change_message);

				PrintMessageForm(&mes, mesb, ROOT, ACTION_BUTTON_POST | ACTION_BUTTON_PREVIEW, CFlags);
				free(mesb);
			}
			break;
		case ACTION_EDIT:
			{
				SMessage msg;
				SProfile_UserInfo UI;
				free(st);

				//
				//	Read the message cause we'll need some parts of it
				//
				if(!ReadDBMessage(DB.TranslateMsgIndex(ROOT), &msg)) printhtmlerror();

				// we should leave current time intact and modify only modified
				msg.MDate = time(NULL);		// have been modified

				strcpy(msg.MessageHeader, mes.MessageHeader);
				strcpy(msg.AuthorName, mes.AuthorName);
				strcpy(msg.HostName, mes.HostName);
				if((ULogin.pui->right & USERRIGHT_SUPERUSER) && c_host && c_host[0] != 0) {
					strcpy(msg.HostName, c_host);
					msg.HostName[HOST_NAME_LENGTH - 1] = 0;
				}
				msg.IPAddr = Nip;

				// Check security rights
				if(ULogin.LU.ID[0] == 0 || (
					!(ULogin.pui->right & USERRIGHT_SUPERUSER) &&
					(ULogin.pui->UniqID != msg.UniqUserID)
					)
				) {
					if(passw) free(passw);
					printaccessdenied(deal);
					goto End_part;
				}
				if((ULogin.pui->right & USERRIGHT_SUPERUSER))
				{
					CProfiles *uprof = new CProfiles();
					if(uprof->errnum != PROFILE_RETURN_ALLOK) {
#if ENABLE_LOG >= 1
						print2log("Error working with profiles database (init)");
#endif
						printhtmlerror();
					}
					// if user does not exist - it should be unreg
					if(uprof->GetUserByName(mes.AuthorName, &UI, NULL, NULL) != PROFILE_RETURN_ALLOK)
						msg.UniqUserID = 0;
					delete uprof;
				}
				else {
					strcpy(msg.AuthorName, ULogin.pui->username);
				}

				char *banreason;
				if((cr = DB.DB_ChangeMessage(ROOT, &msg, // what message
					&mesb, strlen(mesb), CFlags, &banreason)) != MSG_CHK_ERROR_PASSED)
				{
					char *c_ActionResult1, *c_ActionResult2;
					PrepareActionResult(cr, &c_ActionResult1, &c_ActionResult2);
					PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP, ROOT);
					PrintBoardError(c_ActionResult1, c_ActionResult2, 0);

					if(cr == MSG_CHK_ERROR_BANNED && banreason) {
						// print the reason if it exists
						printf(DESIGN_BAN_REASON_STYLE, MESSAGEMAIN_BANNED_REASON, banreason);
					}
				}
				else {
					//	posted, set new cookie
					cookie_name = (char*)realloc(cookie_name, AUTHOR_NAME_LENGTH);
					strcpy(cookie_name, mes.AuthorName);
					
					PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP,
						ROOT);
					PrintBoardError(MESSAGEMAIN_add_ok, MESSAGEMAIN_add_ok2, HEADERSTRING_REFRESH_TO_MAIN_PAGE);
				}
			}
			break;
		default:
			{
#if ENABLE_LOG > 1
				print2log("Unknown parameter during message post, st = %s", st);
#endif
				if(st) free(st);
				if(passw) free(passw);
				goto End_URLerror;
			}
		}// switch(action)
		if(passw) free(passw);

		PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP, ROOT);
		goto End_part;
	}

	
	if(strncmp(deal, "configure", 9) == 0) {
		st = NULL;
		if((st = strget(deal,"configure=", 16, '&')) != NULL) {
			if(strcmp(st, "action") == 0) {
				free(st);
				// get parameters
				par = GetParams(MAX_PARAMETERS_STRING);

				currentss = CONFIGURE_SETTING_DEFAULT_ss;
				currenttt = CONFIGURE_SETTING_DEFAULT_tt;
				currenttv = CONFIGURE_SETTING_DEFAULT_tv;
				currenttc = CONFIGURE_SETTING_DEFAULT_tc;
				currentlsel = CONFIGURE_SETTING_DEFAULT_lsel;
				currentdsm = CONFIGURE_SETTING_DEFAULT_dsm;
				currenttz = DATETIME_DEFAULT_TIMEZONE;
				
				if(par != NULL) {
					char *st, *ss;
					DWORD tmp;

#define READ_PARAM_MASK(param, var, mask) {		\
	char *ss = strget(par, param, 20, '&');	\
	if(ss != NULL) {						\
		if(strcmp(ss, "1") == 0) {			\
			var |= mask;					\
		}									\
		else {								\
			var &= (~mask);					\
		}									\
		free(ss);							\
	}										\
	else var &= (~mask);					\
}

					// read disable smiles
					READ_PARAM_MASK("dsm=", currentdsm, 0x01);
					// read dup (disable upper picture)
					READ_PARAM_MASK("dup=", currentdsm, 0x02);
					// read dul (disable second link bar)
					READ_PARAM_MASK("dul=", currentdsm, 0x04);
					// read onh (disable own nick highlighing)
					READ_PARAM_MASK("onh=", currentdsm, 0x08);
					// read plu (enable + acting like an href)
					READ_PARAM_MASK("plu=", currentdsm, 0x10);
					// read host (disable host displaying)
					READ_PARAM_MASK("host=", currentdsm, 0x20);
					// read alt nick displaying
					READ_PARAM_MASK("nalt=", currentdsm, 0x40);
					// read signature disable
					READ_PARAM_MASK("dsig=", currentdsm, 0x80);
					// read show reply form
					READ_PARAM_MASK("shrp=", currentdsm, 0x100);

#define READ_PARAM_NUM(param, var, vardefault) {\
	char *ss = strget(par, param, 20, '&');		\
	if(ss != NULL) {							\
		DWORD tmp = strtol(ss, &st, 10);		\
		if(((!(*ss != '\0' && *st == '\0'))		\
			|| errno == ERANGE)) {				\
			var = vardefault;					\
		}										\
		else {									\
			var = tmp;							\
		}										\
		free(ss);								\
	}											\
	else var = vardefault;						\
}

					// read lsel (show type selection)
					READ_PARAM_NUM("lsel=", currentlsel, CONFIGURE_SETTING_DEFAULT_lsel);
					if(currentlsel != 1 && currentlsel != 2)
						currentlsel = CONFIGURE_SETTING_DEFAULT_lsel;
					// read tc (time count)
					READ_PARAM_NUM("tc=", currenttc, CONFIGURE_SETTING_DEFAULT_tc);
					if(currenttc <= 0)
						currenttc = CONFIGURE_SETTING_DEFAULT_tc;
					// read tv (thread value)
					READ_PARAM_NUM("tv=", currenttv, CONFIGURE_SETTING_DEFAULT_tv);
					if(currenttv <= 0)
						currenttv = CONFIGURE_SETTING_DEFAULT_tv;
					// read tt (read time type)
					READ_PARAM_NUM("tt=", currenttt, CONFIGURE_SETTING_DEFAULT_tt);
					if(currenttt < 1 || currenttt > 4)
						currenttt = CONFIGURE_SETTING_DEFAULT_tt;
					// read ss (read time type)
					READ_PARAM_NUM("ss=", currentss, CONFIGURE_SETTING_DEFAULT_ss);
					if(currentss < 1 || currentss > 4)
						currentss = CONFIGURE_SETTING_DEFAULT_ss;
					READ_PARAM_NUM("tz=", currenttz, DATETIME_DEFAULT_TIMEZONE);
					if(currenttz < -12 || currenttz > 12)
						currenttz = DATETIME_DEFAULT_TIMEZONE;

#if	TOPICS_SYSTEM_SUPPORT
					// read topics that should be displayed
					{
						currenttopics = 0;
						DWORD i;
						for(i = 0; i < TOPICS_COUNT; i++)
						{
							char st[30];
							sprintf(st, "topic%d=", i);
							if((ss = strget(par, st,  3, '&')) != NULL)
							{
								if(strcmp(ss, "on") == 0)
								{
									currenttopics |= (1<<i);
								}
								free(ss);
							}
						}
					}
#endif

					//
					// saving values - profile or cookie way
					//

					if((ULogin.LU.ID[0] != 0) && (ULogin.pui->Flags & PROFILES_FLAG_VIEW_SETTINGS) ){
							
						ULogin.pui->vs.dsm = currentdsm;
						ULogin.pui->vs.topics = currenttopics;
						ULogin.pui->vs.tv = currenttv;
						ULogin.pui->vs.tc = currenttc;
						ULogin.pui->vs.ss = currentss;
						ULogin.pui->vs.lsel = currentlsel;
						ULogin.pui->vs.tt = currenttt;
						ULogin.pui->vs.tz = currenttz;
				
						CProfiles *uprof;
						uprof = new CProfiles();
						uprof->ModifyUser(ULogin.pui, NULL, NULL);
						delete uprof;
					}
					else{
						// settings are not in profile. so new values should be in cookies
						cookie_lsel = currentlsel;
						cookie_tc = currenttc;
						cookie_tt = currenttt;
						cookie_tv = currenttv;
						cookie_ss = currentss;
						cookie_dsm = currentdsm;
						cookie_topics = currenttopics;
						cookie_tz = currenttz;
					 }
					
				}
				free(par);
				
				
				//
				// Redirect user to the index page
				//
				PrintHTMLHeader(HEADERSTRING_REDIRECT_NOW | HEADERSTRING_NO_CACHE_THIS, MAINPAGE_INDEX);

				goto End_part;
			}
			free(st);

			Tittle_cat(TITLE_Configure);

			/* print configuration form */
			PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
			PrintConfig();
			PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
			/****************************/
		}
		goto End_part;
	}

	if(strncmp(deal, "login", 5) == 0) {
		st = NULL;
		if((st = strget(deal,"login=", 16, '&')) != NULL) {
			
			if(strcmp(st, "action") == 0) {
				free(st);
				// check for User name and password
				// get parameters
				par = GetParams(MAX_PARAMETERS_STRING);

				/***************************************/
				int disableipcheck = 0;
				char *ipchk;
				if((ipchk = strget(par, "ipoff=", 10, '&')) != NULL) {
					if(strcmp(ipchk, "1") == 0) {
						disableipcheck = 1;
					}
					free(ipchk);
				}
				st = strget(par,"mname=", PROFILES_MAX_USERNAME_LENGTH - 1, '&');
				if(st != NULL) {
					char *ss;
					ss = strget(par,"mpswd=", PROFILES_MAX_PASSWORD_LENGTH - 1, '&');
					if(ss != NULL) {
						/* if session already opened - close it */
						if(ULogin.LU.ID[0] != 0)
							ULogin.CloseSession(ULogin.LU.ID);

						if(ULogin.OpenSession(st, ss, NULL, Nip, disableipcheck) == 1) {
							print2log("User '%s' was logged in (%s)", ULogin.pui->username, getenv(REMOTE_ADDR));

							//	Prepare conference login greetings
							char boardgreet[1000];
							char *greetnames[4] = {
								MESSAGEMAIN_login_helloday,
								MESSAGEMAIN_login_helloevn,
								MESSAGEMAIN_login_hellonight,
								MESSAGEMAIN_login_hellomor
							};
							int cur = 2;
							time_t tt = time(NULL);
							tm *x = localtime(&tt);
							if(x->tm_hour >= 6 && x->tm_hour < 10) cur = 3;
							if(x->tm_hour >= 10 && x->tm_hour < 18) cur = 0;
							if(x->tm_hour >= 18 && x->tm_hour < 22) cur = 1;
							sprintf(boardgreet, MESSAGEMAIN_login_ok, greetnames[cur], ULogin.pui->username);


							Tittle_cat(TITLE_Login);

							// entered, set new cookie
							cookie_name = (char*)realloc(cookie_name, AUTHOR_NAME_LENGTH);
							strcpy(cookie_name, st);


							PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_LOGIN | HEADERSTRING_REFRESH_TO_MAIN_PAGE, MAINPAGE_INDEX);
							PrintBoardError(boardgreet, MESSAGEMAIN_login_ok2, HEADERSTRING_REFRESH_TO_MAIN_PAGE);
							PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_LOGIN | HEADERSTRING_REFRESH_TO_MAIN_PAGE, MAINPAGE_INDEX);

							free(ss);
							free(st);
							goto End_part;
						}
						free(ss);
					}
					free(st);
				}

				printpassworderror(deal);
				goto End_part;
			}

			if(strcmp(st, "lostpasswform") == 0) {
				free(st);

				Tittle_cat(TITLE_LostPassword);

				PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_LOGIN, MAINPAGE_INDEX);
				printf(DESIGN_LOSTPASSW_HEADER, MESSAGEMAIN_lostpassw_header);
				PrintLostPasswordForm();
				PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_LOGIN, MAINPAGE_INDEX);

				goto End_part;
			}

			if(strcmp(st, "lostpasswaction") == 0) {
				free(st);

				// check for User name and email address
				// get parameters
				par = GetParams(MAX_PARAMETERS_STRING);

				/***************************************/
				st = strget(par,"mname=", PROFILES_MAX_USERNAME_LENGTH - 1, '&');
				if(st != NULL) {
					char *ss;
					ss = strget(par,"memail=", PROFILES_FULL_USERINFO_MAX_EMAIL - 1, '&');
					if(ss != NULL) {
						CProfiles uprof;
						SProfile_FullUserInfo Fui;
						SProfile_UserInfo ui;
						if(uprof.GetUserByName(st, &ui, &Fui, NULL) == PROFILE_RETURN_ALLOK) {
							if(strcmp(Fui.Email, ss) == 0) {
								//
								//	We should send password to the user
								//

								Tittle_cat(TITLE_PasswordSent);

								//
								//	Send email
								//
								{
									char subj[1000], bdy[10000];
																		
									sprintf(subj, MAILACKN_LOSTPASS_SUBJECT, st);
									
									sprintf(bdy, MAILACKN_LOSTPASS_BODY, st, ui.password);
									//print2log("will send message now %s", bdy);
									wcSendMail(Fui.Email, subj, bdy);
									
									print2log("Password was sent to %s", Fui.Email);
								}

								PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE, MAINPAGE_INDEX);
								PrintBoardError(MESSAGEMAIN_lostpassw_ok, MESSAGEMAIN_lostpassw_ok2, HEADERSTRING_REFRESH_TO_MAIN_PAGE);
								PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE, MAINPAGE_INDEX);

								free(ss);
								free(st);
								goto End_part;
							}
						}
					}
				}

				Tittle_cat(TITLE_LostPassword);

				PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_LOGIN, MAINPAGE_INDEX);
				printf(DESIGN_LOSTPASSW_HEADER, MESSAGEMAIN_lostpassw_hretry);
				PrintLostPasswordForm();
				PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_LOGIN, MAINPAGE_INDEX);

				goto End_part;
			}

			if(strcmp(st, "logoff") == 0) {
				free(st);

				if(ULogin.LU.ID[0] == 0) {
					/* not logged yet */
					PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE, MAINPAGE_INDEX);
					PrintBoardError(MESSAGEMAIN_logoff_not_logged_in, MESSAGEMAIN_logoff_not_logged_in2, 0);
					PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE, MAINPAGE_INDEX);
					goto End_part;
				}

				print2log("User '%s' was logged out (%s)", ULogin.pui->username, getenv(REMOTE_ADDR));

				/* close sequence */
				ULogin.CloseSession(ULogin.LU.ID);
				
				PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE, MAINPAGE_INDEX);
				
				PrintBoardError(MESSAGEMAIN_logoff_ok, MESSAGEMAIN_logoff_ok, HEADERSTRING_REFRESH_TO_MAIN_PAGE);
				
				PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE, MAINPAGE_INDEX);
				goto End_part;
			}
			free(st);
		}

		Tittle_cat(TITLE_Login);

		/******* print login form *******/
		PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_LOGIN, MAINPAGE_INDEX);
		printf(DESIGN_MODERATOR_ENTER_HEADER, MESSAGEMAIN_login_login_header);
		PrintLoginForm();
		PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_LOGIN, MAINPAGE_INDEX);
		/********************************/
		goto End_part;
	}
	
	if(strncmp(deal, "close", 5) == 0) {
		/* security check */
		if((ULogin.LU.right & USERRIGHT_CLOSE_MESSAGE) == 0) {
			printaccessdenied(deal);
			goto End_part;
		}

		if((st = strget(deal,"close=", 16, '&')) != NULL) {
			char *ss;
			DWORD midx;
			errno = 0;
			DWORD tmp = strtol(st, &ss, 10);
			if((!(*st != '\0' && *ss == '\0')) || errno == ERANGE ||
				tmp < 1 || ((midx = DB.TranslateMsgIndex(tmp)) == NO_MESSAGE_CODE) ) {
				printnomessage(deal);
			}
			else {
				/* Security check for own message or USERRIGHT_SUPERUSER */

				/******** read message ********/
				if(!ReadDBMessage(midx, &mes)) printhtmlerror();
				/* closing by author allowed in main thread only ! */
				if(ULogin.LU.ID[0] != 0 && ((mes.UniqUserID == ULogin.LU.UniqID && mes.Level == 0) || (ULogin.LU.right & USERRIGHT_SUPERUSER))) {

					Tittle_cat(TITLE_ClosingMessage);

					DB.DB_ChangeCloseThread(tmp, 1);
					print2log("Message %d (%s (by %s)) was closed by %s", tmp, mes.MessageHeader, mes.AuthorName, ULogin.pui->username);
					PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP,
						tmp);
					PrintBoardError(MESSAGEMAIN_threadwasclosed, MESSAGEMAIN_threadwasclosed2,
						HEADERSTRING_REFRESH_TO_MAIN_PAGE);
					PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP,
						tmp);
				}
				else printaccessdenied(deal);
			}
			free(st);
		}
		else goto End_URLerror;
		goto End_part;
	}
	
	
	if(strncmp(deal, "hide", 4) == 0) {
		/* security check */
		if((ULogin.LU.right & USERRIGHT_SUPERUSER) == 0) {
			printaccessdenied(deal);
			goto End_part;
		}

		if((st = strget(deal,"hide=", 16, '&')) != NULL) {
			errno = 0;
			char *ss;
			DWORD tmp = strtol(st, &ss, 10);
			if((!(*st != '\0' && *ss == '\0')) || errno == ERANGE ||
				tmp < 1 || DB.TranslateMsgIndex(tmp) == NO_MESSAGE_CODE) {
				printnomessage(deal);
			}
			else {
				DB.DB_ChangeInvisibilityThreadFlag(tmp, 1);
				if(!ReadDBMessage(DB.TranslateMsgIndex(tmp), &mes)) printhtmlerror();
				print2log("Message %d (%s (by %s)) was hided by %s", tmp, mes.MessageHeader, mes.AuthorName, ULogin.pui->username);

				Tittle_cat(TITLE_HidingMessage);

				PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP,
					tmp);
				PrintBoardError(MESSAGEMAIN_threadchangehided, MESSAGEMAIN_threadchangehided2,
					HEADERSTRING_REFRESH_TO_MAIN_PAGE);
				PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP,
					tmp);
			}
			free(st);
		}
		else goto End_URLerror;
		goto End_part;
	}
	
	
	if(strncmp(deal, "unhide", 6) == 0) {
		/* security check */
		if((ULogin.LU.right & USERRIGHT_SUPERUSER) == 0) {
			printaccessdenied(deal);
			goto End_part;
		}

		if((st = strget(deal,"unhide=", 16, '&')) != NULL) {
			errno = 0;
			char *ss;
			DWORD tmp = strtol(st, &ss, 10);
			if((!(*st != '\0' && *ss == '\0')) || errno == ERANGE ||
				tmp < 1 || DB.TranslateMsgIndex(tmp) == NO_MESSAGE_CODE) {
				printnomessage(deal);
			}
			else {
				Tittle_cat(TITLE_HidingMessage);

				DB.DB_ChangeInvisibilityThreadFlag(tmp, 0);
				if(!ReadDBMessage(DB.TranslateMsgIndex(tmp), &mes)) printhtmlerror();
				print2log("Message %d (%s (by %s)) was unhided by %s", tmp, mes.MessageHeader, mes.AuthorName, ULogin.pui->username);
				PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP,
					tmp);
				PrintBoardError(MESSAGEMAIN_threadchangehided, MESSAGEMAIN_threadchangehided2,
					HEADERSTRING_REFRESH_TO_MAIN_PAGE);
				PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP,
					tmp);
			}
			free(st);
		}
		else goto End_URLerror;
		goto End_part;
	}
	
	if(strncmp(deal, "unclose", 7) == 0) {
		/* security check */
		if((ULogin.LU.right & USERRIGHT_OPEN_MESSAGE) == 0) {
			printaccessdenied(deal);
			goto End_part;
		}

		if((st = strget(deal,"unclose=", 16, '&')) != NULL) {
			char *ss;
			DWORD midx;
			errno = 0;
			DWORD tmp = strtol(st, &ss, 10);
			if((!(*st != '\0' && *ss == '\0')) || errno == ERANGE ||
				tmp < 1 || ((midx = DB.TranslateMsgIndex(tmp)) == NO_MESSAGE_CODE) ) {
				printnomessage(deal);
			}
			else {
				/* Security check for own message or USERRIGHT_SUPERUSER */

				/******** read message ********/
				if(!ReadDBMessage(midx, &mes)) printhtmlerror();
				if(ULogin.LU.ID[0] != 0 && (mes.UniqUserID == ULogin.LU.UniqID || (ULogin.LU.right & USERRIGHT_SUPERUSER))) {

					Tittle_cat(TITLE_ClosingMessage);

					DB.DB_ChangeCloseThread(tmp, 0);
					if(!ReadDBMessage(DB.TranslateMsgIndex(tmp), &mes)) printhtmlerror();
					print2log("Message %d (%s (by %s)) was opened by %s", tmp, mes.MessageHeader, mes.AuthorName, ULogin.pui->username);
					PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP,
						tmp);
					PrintBoardError(MESSAGEMAIN_threadwasclosed, MESSAGEMAIN_threadwasclosed2,
						HEADERSTRING_REFRESH_TO_MAIN_PAGE);
					PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE |
						HEADERSTRING_DISABLE_FAQHELP, tmp);
				}
				else printaccessdenied(deal);
			}
			free(st);
		}
		else goto End_URLerror;
		goto End_part;
	}

	if(strncmp(deal, "roll", 4) == 0) {
		/* security check */
		if((ULogin.LU.right & USERRIGHT_SUPERUSER) == 0) {
			printaccessdenied(deal);
			goto End_part;
		}

		if((st = strget(deal,"roll=", 16, '&')) != NULL) {
			errno = 0;
			char *ss;
			DWORD tmp = strtol(st, &ss, 10);
			if((!(*st != '\0' && *ss == '\0')) || errno == ERANGE ||
				tmp < 1 || DB.TranslateMsgIndex(tmp) == NO_MESSAGE_CODE) {
				printnomessage(deal);
			}
			else {

				Tittle_cat(TITLE_RollMessage);

				DB.DB_ChangeRollThreadFlag(tmp);
				if(!ReadDBMessage(DB.TranslateMsgIndex(tmp), &mes)) printhtmlerror();
				print2log("Message %d (%s (by %s)) was (un)rolled by %s", tmp, mes.MessageHeader, mes.AuthorName, ULogin.pui->username);

				PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE,
					tmp);
				PrintBoardError(MESSAGEMAIN_threadrolled, MESSAGEMAIN_threadrolled2,
					HEADERSTRING_REFRESH_TO_MAIN_PAGE);
				PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE,
					tmp);
			}
			free(st);
		}
		else goto End_URLerror;
		goto End_part;
	}
	
	if(strncmp(deal, "delmsg", 6) == 0) {
		/* security check */
		if((ULogin.LU.right & USERRIGHT_SUPERUSER) == 0) {
			printaccessdenied(deal);
			goto End_part;
		}

		if((st = strget(deal,"delmsg=", 16, '&')) != NULL) {
			errno = 0;
			char *ss;
			DWORD tmp = strtol(st, &ss, 10);
			
			if((!(*st != '\0' && *ss == '\0')) || errno == ERANGE ||
				tmp < 1 || DB.TranslateMsgIndex(tmp) == NO_MESSAGE_CODE) {
				printnomessage(deal);
			}
			else {

				Tittle_cat(TITLE_DeletingMessage);

				if(!ReadDBMessage(DB.TranslateMsgIndex(tmp), &mes)) printhtmlerror();
				print2log("Message %d (%s (by %s)) was deleted by %s", tmp, mes.MessageHeader, mes.AuthorName, ULogin.pui->username);
				DB.DB_DeleteMessages(tmp);
				
				PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP, tmp);
				PrintBoardError(MESSAGEMAIN_threaddeleted, MESSAGEMAIN_threaddeleted2,
					HEADERSTRING_REFRESH_TO_MAIN_PAGE);
				PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP,
					tmp);
			}
			free(st);
		}
		else goto End_URLerror;
		goto End_part;
	}
	
	if(strncmp(deal, "changemsg", 9) == 0) {
		// precheck security
		if((ULogin.LU.right & USERRIGHT_MODIFY_MESSAGE) == 0) {
			printaccessdenied(deal);
			goto End_part;
		}

		if((st = strget(deal,"changemsg=", 16, '&')) != NULL) {
			errno = 0;
			char *ss;
			DWORD tmp = strtol(st, &ss, 10);
			DWORD midx;
			if( (!(*st != '\0' && *ss == '\0')) || errno == ERANGE ||
				tmp < 1 || ((midx = DB.TranslateMsgIndex(tmp)) == NO_MESSAGE_CODE)) {
				printnomessage(deal);
			}
			else {
				//
				//	read message
				//
				if(!ReadDBMessage(midx, &mes)) printhtmlerror();

				//
				//	security check
				//
				if(!( (ULogin.LU.right & USERRIGHT_SUPERUSER) || (	// admin ?
						((mes.Flag & MESSAGE_IS_INVISIBLE) == 0) && // not hided
						((mes.Flag & MESSAGE_IS_CLOSED) == 0) &&	// and not closed
						(ULogin.LU.right & USERRIGHT_MODIFY_MESSAGE) &&	// can modify?
						(mes.UniqUserID == ULogin.LU.UniqID) )	// message posted by this user
					))
				{
					Tittle_cat(TITLE_Error);
					printaccessdenied(deal);
				}
				else {
#if STABLE_TITLE == 0
					// set title - change title to change message
					ConfTitle = (char*)realloc(ConfTitle, strlen(ConfTitle) + strlen(TITLE_divider) + strlen(TITLE_ChangingMessage) + strlen(mes.MessageHeader) + 6);
					strcat(ConfTitle, TITLE_divider);
					strcat(ConfTitle, TITLE_ChangingMessage);
					strcat(ConfTitle, " - ");
					strcat(ConfTitle, mes.MessageHeader);
#endif
					PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, tmp);

					PrintMessageThread(&DB, tmp, mes.Flag, mes.UniqUserID);

					char *mesb = (char*)malloc(mes.msize + 1);
					mesb[0] = 0;

					//
					//	Read message body
					//
					if(!ReadDBMessageBody(mesb, mes.MIndex, mes.msize))
						printhtmlerrorat(LOG_UNABLETOLOCATEFILE, F_MSGBODY);

					PrintMessageForm(&mes, mesb, tmp, ACTION_BUTTON_EDIT);

					free(mesb);

					PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, tmp);
				}
			}
			free(st);
		}
		else goto End_URLerror;
		goto End_part;
	}
	
	if(strncmp(deal, "help", 4) == 0) {

		Tittle_cat(TITLE_HelpPage);

		PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP, MAINPAGE_INDEX);
		
		PrintFAQForm();
		
		PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP, MAINPAGE_INDEX);
		goto End_part;
	}

	if(strncmp(deal, "uinfo", 5) == 0) {
		char *name;
		if((name = strget(deal,"uinfo=", PROFILES_MAX_USERNAME_LENGTH, '&')) != NULL) {
#if STABLE_TITLE == 0
			ConfTitle = (char*)realloc(ConfTitle, strlen(ConfTitle) + 2*strlen(TITLE_divider) + strlen(TITLE_ProfileInfo) + strlen(name) + 1);
			strcat(ConfTitle, TITLE_divider);
			strcat(ConfTitle, TITLE_ProfileInfo);
			strcat(ConfTitle, TITLE_divider);
			strcat(ConfTitle, name);
#endif
			PrintHTMLHeader(HEADERSTRING_DISABLE_ALL, 0);
			
			PrintAboutUserInfo(name);
			
			PrintBottomLines(HEADERSTRING_DISABLE_ALL, 0);
			free(name);
		}
		else goto End_URLerror;
		goto End_part;
	}

	if(strncmp(deal, "searchword", 10) == 0) {
		/* security check */
		if((ULogin.LU.right & USERRIGHT_VIEW_MESSAGE) == 0) {
			printaccessdenied(deal);
			goto End_part;
		}

		char *ss;
		DWORD start = 0;
		if((ss = strget(deal,"searchword=", 255 - 1, '&')) != NULL) {
			if((st = strget(deal,"start=", 60, '&')) != NULL) {
				errno = 0;
				char *ss;
				start = strtol(st, &ss, 10);
				if( (!(*st != '\0' && *ss == '\0')) || errno == ERANGE || tmp == 0) {
					start = 1;
				}
				free(st);
			}
			else start = 1;

			if(strlen(ss) > 0) {
				Tittle_cat(TITLE_Search);

				PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_SEARCH, MAINPAGE_INDEX | HEADERSTRING_NO_CACHE_THIS);
				PrintSearchForm(ss, &DB);
				if(strlen(ss) >= SEARCHER_MIN_WORD) {
					CMessageSearcher *ms = new CMessageSearcher(SEARCHER_INDEX_CREATE_EXISTING);
					if(ms->errnum == SEARCHER_RETURN_ALLOK) {
						DWORD c;
						DWORD *vmsg = ms->SearchMessagesByPattern(ss, &c);
						printf(DESIGN_SEARCH_SEARCH_STR_WAS, MESSAGEMAIN_search_search_str, ms->srch_str);
						if(c != 0) {
							// print count of found messages
							printf(DESIGN_SEARCH_RESULT, MESSAGEMAIN_search_result1, c, MESSAGEMAIN_search_result2);
						}
						else {
							// Nothing have been found
							printf(DESIGN_SEARCH_NO_RESULT, MESSAGEMAIN_search_result1, MESSAGEMAIN_search_result_nothing);
						}

						//	Check and adjust start
						if(c <= (start-1)*SEARCH_MES_PER_PAGE_COUNT) {
							start = c/SEARCH_MES_PER_PAGE_COUNT + 1;
						}

						DWORD oldc = c;
						if(c > SEARCH_MES_PER_PAGE_COUNT) {
							char *wrd = CodeHttpString(ss, 0);
							if(wrd) {
								printf("<CENTER>" MESSAGEMAIN_search_result_pages);
								int max = (c/SEARCH_MES_PER_PAGE_COUNT) + 
									(((c % SEARCH_MES_PER_PAGE_COUNT) == 0)? 0: 1);
								for(int i = 0; i < max; i++) {
									if(i > 0 && (i % 20) == 0) printf("<BR>");
									if(i != start - 1) printf("&nbsp;<A HREF=\"?searchword=%s&start=%d\">%d</A>&nbsp;", wrd, i+1, i+1);
									else printf("<BOLD>&nbsp;%d&nbsp;</BOLD>", i+1);
								}
								printf("</CENTER>");
							}
						}

						if(c - (start-1)*SEARCH_MES_PER_PAGE_COUNT > SEARCH_MES_PER_PAGE_COUNT) c = SEARCH_MES_PER_PAGE_COUNT;
						else c = c - (start-1)*SEARCH_MES_PER_PAGE_COUNT;
						DB.PrintHtmlMessageBufferByVI(vmsg + (start-1)*SEARCH_MES_PER_PAGE_COUNT, c);
						free(vmsg);

						c = oldc;
						if(c > SEARCH_MES_PER_PAGE_COUNT) {
							char *wrd = CodeHttpString(ss, 0);
							if(wrd) {
								printf("<BR><CENTER>" MESSAGEMAIN_search_result_pages);
								int max = (c/SEARCH_MES_PER_PAGE_COUNT) + 
									(((c % SEARCH_MES_PER_PAGE_COUNT) == 0)? 0: 1);
								for(int i = 0; i < max; i++) {
									if(i > 0 && (i % 20 == 0)) printf("<BR>");
									if(i != start - 1) printf("&nbsp;<A HREF=\"?searchword=%s&start=%d\">%d</A>&nbsp;", wrd, i+1, i+1);
									else printf("<BOLD>&nbsp;%d&nbsp;</BOLD>", i+1);
								}
								printf("</CENTER>");
							}
						}
					}
					else {
						//	Write that searcher have not been configured properly
					}
					delete ms;
				}
				PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_SEARCH, MAINPAGE_INDEX);
				free(ss);
				goto End_part;
			}
		}

		Tittle_cat(TITLE_Search);

		PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_SEARCH, MAINPAGE_INDEX);
		
		PrintSearchForm("", &DB, 1);
		
		PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_SEARCH, MAINPAGE_INDEX);

		goto End_part;
	}

	if(strncmp(deal, "search", 6) == 0) {
		/* security check */
		if((ULogin.LU.right & USERRIGHT_VIEW_MESSAGE) == 0) {
			printaccessdenied(deal);
			goto End_part;
		}

		if((st = strget(deal,"search=", 60, '&')) != NULL) {
			if(strcmp(st, "action") == 0) {
				free(st);
				
				/* get "method post" parameters */
				par = GetParams(MAX_PARAMETERS_STRING);
				if(par != NULL) {
					char *ss;
					/* read search pattern */
					ss = strget(par, "find=", 255 - 1, '&');
					if(ss == NULL) {
						ss = (char*)malloc(1);
						ss[0] = 0;
					}

					Tittle_cat(TITLE_Search);

#if ENABLE_LOG == 2
					print2log("Search from %s, query=%s", getenv(REMOTE_ADDR), ss);
#endif
					PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_SEARCH, MAINPAGE_INDEX);
					PrintSearchForm(ss, &DB);
					if(strlen(ss) >= SEARCHER_MIN_WORD) {
						CMessageSearcher *ms = new CMessageSearcher(SEARCHER_INDEX_CREATE_EXISTING);
						if(ms->errnum == SEARCHER_RETURN_ALLOK) {
							DWORD c;
							DWORD *vmsg = ms->SearchMessagesByPattern(ss, &c);
							printf(DESIGN_SEARCH_SEARCH_STR_WAS, MESSAGEMAIN_search_search_str, ms->srch_str);
							if(c != 0) {
								// print count of found messages
								printf(DESIGN_SEARCH_RESULT, MESSAGEMAIN_search_result1, c, MESSAGEMAIN_search_result2);
							}
							else {
								// Nothing have been found
								printf(DESIGN_SEARCH_NO_RESULT, MESSAGEMAIN_search_result1, MESSAGEMAIN_search_result_nothing);
							}
							if(c > SEARCH_MES_PER_PAGE_COUNT) {
								char *wrd = CodeHttpString(ss, 0);
								if(wrd) {
									printf("<CENTER>" MESSAGEMAIN_search_result_pages);
									int max = (c/SEARCH_MES_PER_PAGE_COUNT) + 
										(((c % SEARCH_MES_PER_PAGE_COUNT) == 0)? 0: 1);
									for(int i = 0; i < max; i++) {
										if(i > 0 && (i % 20 == 0)) printf("<BR>");
										if(i != 0) printf("&nbsp;<A HREF=\"?searchword=%s&start=%d\">%d</A>&nbsp;", wrd, i+1, i+1);
										else printf("<BOLD>&nbsp;%d&nbsp;</BOLD>", i+1);
									}
									printf("</CENTER>");
								}
							}
							DWORD oldc = c;
							if(c > 0) {
								if( c > SEARCH_MES_PER_PAGE_COUNT) c = SEARCH_MES_PER_PAGE_COUNT;
								DB.PrintHtmlMessageBufferByVI(vmsg, c);
								free(vmsg);
							}
							c = oldc;
							if(c > SEARCH_MES_PER_PAGE_COUNT) {
								char *wrd = CodeHttpString(ss, 0);
								if(wrd) {
									printf("<BR><CENTER>" MESSAGEMAIN_search_result_pages);
									int max = (c/SEARCH_MES_PER_PAGE_COUNT) + 
										(((c % SEARCH_MES_PER_PAGE_COUNT) == 0)? 0: 1);
									for(int i = 0; i < max; i++) {
										if(i > 0 && (i % 20 == 0)) printf("<BR>");
										if(i != 0) printf("&nbsp;<A HREF=\"?searchword=%s&start=%d\">%d</A>&nbsp;", wrd, i+1, i+1);
										else printf("<BOLD>&nbsp;%d&nbsp;</BOLD>", i+1);
									}
									printf("</CENTER>");
								}
							}
						}
						else {
							//	Write that searcher have not been configured properly
						}
						delete ms;
					}
					PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_SEARCH, MAINPAGE_INDEX);
					free(ss);
					goto End_part;
				}
				else goto End_URLerror;
			}
			free(st);
		}

		Tittle_cat(TITLE_Search);

		PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_SEARCH, MAINPAGE_INDEX);
		
		PrintSearchForm("", &DB, 1);
		
		PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_SEARCH, MAINPAGE_INDEX);

		goto End_part;
	}

	if(strncmp(deal, "changeusr=", 8) == 0) {
		/* security check */
		if((ULogin.LU.right & USERRIGHT_SUPERUSER) == 0) {
			printaccessdenied(deal);
			goto End_part;
		}
		if((st = strget(deal,"changeusr=", 30, '&')) != NULL) {
			if(strcmp(st, "action") == 0) {
				free(st);

				//	here we do it :)
				//	get parameters
				par = GetParams(MAX_PARAMETERS_STRING);
				if(par !=NULL) {
					BYTE sechdr, secbdy, ustat;
					DWORD right = 0;
					char *name;

					/* security byte header */
					if((st = strget(par, "sechdr=", 10, '&')) != NULL) {
						errno = 0;
						char *ss;
						sechdr = (BYTE)strtol(st, &ss, 10);
						if( (!(*st != '\0' && *ss == '\0')) || errno == ERANGE) {
							sechdr = 255;
						}
						free(st);
					}
					else sechdr = 255;

					/* name */
					name = strget(par, "name=", PROFILES_MAX_USERNAME_LENGTH - 1, '&');

					/* security byte body */
					if((st = strget(par, "secbdy=", 10, '&')) != NULL) {
						errno = 0;
						char *ss;
						secbdy = (BYTE)strtol(st, &ss, 10);
						if( (!(*st != '\0' && *ss == '\0')) || errno == ERANGE) {
							secbdy = 255;
						}
						free(st);
					}
					else secbdy = 255;

					/* ustat */
					if((st = strget(par, "ustat=", 10, '&')) != NULL) {
						errno = 0;
						char *ss;
						ustat = (BYTE)strtol(st, &ss, 10);
						if( (!(*st != '\0' && *ss == '\0')) || errno == ERANGE || ustat >= USER_STATUS_COUNT) {
							ustat = 0;
						}
						free(st);
					}
					else ustat = 0;

					// read the right
					{
						DWORD i;
						char *ss;
						right = 0;
						for(i = 0; i < USERRIGHT_COUNT; i++)
						{
							char st[30];
							sprintf(st, "right%d=", i);
							if((ss = strget(par, st,  4, '&')) != NULL)
							{
								if(strcmp(ss, "on") == 0)
								{
									right |= (1<<i);
								}
								free(ss);
							}
						}
					}

					//
					//	Update the user
					//
					int updated = 0;
					if(name) {
						CProfiles *uprof;
						SProfile_UserInfo ui;
						SProfile_FullUserInfo fui;
						DWORD err = 0;
						DWORD idx;

						uprof = new CProfiles();
						err = uprof->GetUserByName(name, &ui, &fui, &idx);
						if(err == PROFILE_RETURN_ALLOK) {
							int altnupd = 0;
							// delete alt name if required
							if(((ui.right & USERRIGHT_SUPERUSER) != 0 ||
								(ui.right & USERRIGHT_ALT_DISPLAY_NAME) != 0) &&
							   ((right & USERRIGHT_ALT_DISPLAY_NAME) == 0 &&
								(right & USERRIGHT_SUPERUSER) == 0)
							  )
							{
								ui.Flags &= (~PROFILES_FLAG_ALT_DISPLAY_NAME);
								altnupd = 1;
							}

							ui.secur = secbdy;
							ui.secheader = sechdr;
							ui.Status = ustat;
							ui.right = right;

							if(uprof->SetUInfo(idx, &ui)) {
								if(altnupd) AltNames.DeleteAltName(ui.UniqID);
								updated = 1;
							}
						}
						if(uprof) delete uprof;
					}

					//
					//	Write complete message
					//
					if(updated) {
						Tittle_cat(TITLE_Registration);

						PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE,
							MAINPAGE_INDEX);
						PrintBoardError(MESSAGEMAIN_register_edit_ex, MESSAGEMAIN_register_edit_ex2,
							HEADERSTRING_REFRESH_TO_MAIN_PAGE);
						PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE,
							MAINPAGE_INDEX);

						// log this task
						print2log("User %s was updated by %s", name, ULogin.pui->username);
					}
					else {
						Tittle_cat(TITLE_Error);

						PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
						PrintBoardError(MESSAGEMAIN_register_edit_err, MESSAGEMAIN_register_edit_err2, 0);
						PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
					}

					free(par);
					goto End_part;
				}
				else goto End_URLerror;
			}
			free(st);
		}

		goto End_URLerror;
	}

	if(strncmp(deal, "register", 8) == 0) {
		/* security check */
		if((ULogin.LU.right & USERRIGTH_PROFILE_CREATE) == 0) {
			printaccessdenied(deal);
			goto End_part;
		}
		// We do not need to check security there, this action due to be done lately in DoCheckAndCreateProfile()
		if((st = strget(deal,"register=", 30, '&')) != NULL) {
			if(strcmp(st, "action") == 0) {
				free(st);
				SProfile_FullUserInfo fui;
				SProfile_UserInfo ui;

				/****** set default user creation parameters ******/
				ui.Flags = 0; // don't have picture or signature
				ui.right = DEFAULT_USER_RIGHT;
				ui.secur = DEFAULT_USER_SECURITY_BYTE;
				ui.secheader = DEFAULT_USER_HDR_SEC_BYTE;
				/**************************************************/

				/* get parameters */
				par = GetParams(MAX_PARAMETERS_STRING);
				if(par !=NULL) {
					char *ss, *passwdconfirm, *oldpasswd, *act, *mb;

					/* what we should do: edit, delete or create */
					act = strget(par, "register=", 255, '&');
					if(act == NULL) {
						// default action - register
						act = (char*)malloc(100);
						strcpy(act, MESSAGEMAIN_register_register);
					}

					/* read login name (username) and load user profile if update */
					ss = strget(par, "login=", PROFILES_MAX_USERNAME_LENGTH - 1, '&');
					if(ss != NULL) {
						strcpy(ui.username, ss);
						free(ss);
					}
					else {
						if(ULogin.LU.ID[0] != 0)
							strcpy(ui.username, ULogin.pui->username);
						else ui.username[0] = 0;
					}
					/* if edit - load current settings */
					if(strcmp(act, MESSAGEMAIN_register_edit) == 0 && ui.username[0] != 0) {
						CProfiles *cp = new CProfiles();
						cp->GetUserByName(ui.username, &ui, &fui, NULL);
						delete cp;
					}

					//	Read alternative display name for user
					ss = strget(par, "dispnm=", PROFILES_MAX_ALT_DISPLAY_NAME - 1, '&');
					if(ss != NULL) {
						strcpy(ui.altdisplayname, ss);
						free(ss);
					}
					else ui.altdisplayname[0] = 0;

					/* read password 1 */
					ss = strget(par, "pswd1=", PROFILES_MAX_PASSWORD_LENGTH - 1, '&');
					if(ss != NULL) {
						strcpy(ui.password, ss);
						free(ss);
					}
					else ui.password[0] = 0;
					
					/* read password 2 */
					passwdconfirm = strget(par, "pswd2=", PROFILES_MAX_PASSWORD_LENGTH - 1, '&');
					if(!passwdconfirm)
					{
						passwdconfirm = (char*)malloc(2);
						passwdconfirm[0] = 0;
					}
					
					/* read old password */
					oldpasswd = strget(par, "opswd=", PROFILES_MAX_PASSWORD_LENGTH - 1, '&');
					if(!oldpasswd)
					{
						oldpasswd = (char*)malloc(2);
						oldpasswd[0] = 0;
					}

					/* read full name */
					ss = strget(par, "name=", PROFILES_FULL_USERINFO_MAX_NAME - 1, '&');
					if(ss != NULL) {
						strcpy(fui.FullName, ss);
						free(ss);
					}
					else fui.FullName[0] = 0;
					
					/* read email */
					ss = strget(par, "email=", PROFILES_FULL_USERINFO_MAX_EMAIL - 1, '&');
					if(ss != NULL) {
						strcpy(fui.Email, ss);
						free(ss);
					}
					else fui.Email[0] = 0;

					/* read email */
					ss = strget(par, "icq=", PROFILES_MAX_ICQ_LEN - 1, '&');
					if(ss != NULL) {
						strcpy(ui.icqnumber, ss);
						free(ss);
					}
					else ui.icqnumber[0] = 0;

					/* read homepage address */
					ss = strget(par, "hpage=", PROFILES_FULL_USERINFO_MAX_HOMEPAGE - 1, '&');
					if(ss != NULL) {
						strcpy(fui.HomePage, ss);
						free(ss);
					}
					else fui.HomePage[0] = 0;
					
					/* read about */
					fui.AboutUser = strget(par, "about=", MAX_PARAMETERS_STRING - 1, '&');

					// this is needed because of char #10 filtering.
					// in WIN32 printf() works incorrectly with it
#if defined(WIN32)
					FilterMessageForPreview(fui.AboutUser, &mb);
					strcpy(fui.AboutUser, mb);
					free(mb);
#endif

					/* read signature */
					ss = strget(par, "sign=", PROFILES_MAX_SIGNATURE_LENGTH - 1, '&');
					if(ss != NULL) {
						// this is needed because of char #10 filtering.
						// in WIN32 printf() works incorrectly with it
#if defined(WIN32)
						FilterMessageForPreview(ss, &mb);
						strcpy(fui.Signature, mb);
						free(mb);
#else
						strcpy(fui.Signature, ss);
#endif
						free(ss);
					}
					else fui.Signature[0] = 0;

					//	read selected users
					ss = strget(par, "susr=", PROFILES_FULL_USERINFO_MAX_SELECTEDUSR - 1, '&');
					if(ss != NULL) {
						// this is needed because of char #10 filtering.
						// in WIN32 printf() works incorrectly with it
#if defined(WIN32)
						FilterMessageForPreview(ss, &mb);
						strcpy(fui.SelectedUsers, mb);
						free(mb);
#else
						strcpy(fui.SelectedUsers, ss);
#endif
						free(ss);
					}
					else fui.SelectedUsers[0] = 0;

					/* invisible profile ? */
					ss = strget(par, "vprf=", 10, '&');
					if(ss != NULL && strcmp(ss, "1") == 0) {
						ui.Flags = ui.Flags & (~PROFILES_FLAG_INVISIBLE);
					}
					else ui.Flags = ui.Flags | PROFILES_FLAG_INVISIBLE;
					if(ss != NULL) free(ss);

					/* always email ackn. for every post ? */
					ss = strget(par, "apem=", 10, '&');
					if(ss != NULL && strcmp(ss, "1") == 0) {
						ui.Flags = ui.Flags  | PROFILES_FLAG_ALWAYS_EMAIL_ACKN;
					}
					else ui.Flags = ui.Flags & (~PROFILES_FLAG_ALWAYS_EMAIL_ACKN);
					if(ss != NULL) free(ss);

					/* disabled private messages ? */
					ss = strget(par, "pdis=", 10, '&');
					if(ss != NULL && strcmp(ss, "1") == 0) {
						ui.Flags = ui.Flags | PROFILES_FLAG_PERSMSGDISABLED;
					}
					else ui.Flags = ui.Flags & (~PROFILES_FLAG_PERSMSGDISABLED);
					if(ss != NULL) free(ss);

					/* private message mail ackn ? */
					ss = strget(par, "peml=", 10, '&');
					if(ss != NULL && strcmp(ss, "1") == 0) {
						ui.Flags = ui.Flags | PROFILES_FLAG_PERSMSGTOEMAIL;
					}
					else ui.Flags = ui.Flags & (~PROFILES_FLAG_PERSMSGTOEMAIL);
					if(ss != NULL) free(ss);

					/* public email ? */
					ss = strget(par, "pem=", 10, '&');
					if(ss != NULL && strcmp(ss, "1") == 0) {
						ui.Flags = ui.Flags | PROFILES_FLAG_VISIBLE_EMAIL;
					}
					else ui.Flags = ui.Flags & (~PROFILES_FLAG_VISIBLE_EMAIL);
					if(ss != NULL) free(ss);
				
					/* save view settings to profile */
					ss = strget(par, "vprs=", 10, '&');
					if(ss != NULL && strcmp(ss, "1") == 0) {
						ui.Flags = ui.Flags | PROFILES_FLAG_VIEW_SETTINGS;
                    }
					else ui.Flags = ui.Flags & (~PROFILES_FLAG_VIEW_SETTINGS);
					if(ss != NULL) free(ss);
																									

					
					if(act != NULL && strcmp(act, MESSAGEMAIN_register_register) == 0) {

						Tittle_cat(TITLE_Registration);

						DoCheckAndCreateProfile(&ui, &fui, passwdconfirm, oldpasswd, 1, deal);
					}
					else if(act != NULL && strcmp(act, MESSAGEMAIN_register_edit) == 0) {

						Tittle_cat(TITLE_Registration);

						DoCheckAndCreateProfile(&ui, &fui, passwdconfirm, oldpasswd, 2, deal);
					}
					else
					if(act != NULL && strcmp(act, MESSAGEMAIN_register_delete) == 0) {

						char* delete_confirmed = strget(par, CONFIRM_DELETE_CHECKBOX_TEXT "=", 255, '&');
						if(!delete_confirmed || !strlen(delete_confirmed))
							goto End_URLerror;
						Tittle_cat(TITLE_Registration);

						DoCheckAndCreateProfile(&ui, &fui, passwdconfirm, oldpasswd, 3, deal);						
					}
					else {
						if(act != NULL) free(act);
						goto End_URLerror;
					}
					if(act != NULL) free(act);
					free(passwdconfirm);
					free(oldpasswd);
					goto End_part;
				}
				else goto End_URLerror;
			}
			free(st);
		}

		SProfile_FullUserInfo fui;
		SProfile_UserInfo ui;
		memset(&fui, 0, sizeof(fui));
		fui.AboutUser = (char*)malloc(1);
		fui.AboutUser[0] = 0;
		memset(&ui, 0, sizeof(ui));
		ui.Flags = USER_DEFAULT_PROFILE_CREATION_FLAGS;

		DWORD x = 0;
		if(ULogin.LU.ID[0] != 0 && (ULogin.LU.right & USERRIGHT_SUPERUSER)) x = 7;
		else if(ULogin.LU.ID[0] == 0) x = 1;
		else if(ULogin.LU.ID[0] != 0) x = 6;

		if(x & 0x02) {
			ULogin.uprof->GetUserByName(ULogin.pui->username, &ui, &fui, NULL);
		}
		else {
			strcpy(fui.HomePage, "http://");
		}

		Tittle_cat(TITLE_Registration);

		PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);

		PrintEditProfileForm(&ui, &fui, x);

		free(fui.AboutUser);

		PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
		goto End_part;
	}

#if	TOPICS_SYSTEM_SUPPORT
	if(strncmp(deal, "ChangeTopic", 11) == 0) {
		if((ULogin.LU.right & USERRIGHT_SUPERUSER) != 0) {
		char *sn;
		DWORD MsgNum = 0, Topic;
		if((sn = strget(deal, "ChangeTopic=", 255 - 1, '&')) != NULL) {
			errno = 0;
			int errok;
			char *ss;
			MsgNum = strtol(sn, &ss, 10);
			if( (!(*sn != '\0' && *ss == '\0')) || errno == ERANGE || MsgNum == 0 ||
				(MsgNum = DB.TranslateMsgIndex(MsgNum)) == NO_MESSAGE_CODE) {
				errok = 0;
			}
			else errok = 1;
			free(sn);
			if(errok && (st = strget(deal,"topic=", 60, '&')) != NULL) {
				errno = 0;
				char *ss;
				Topic = strtol(st, &ss, 10);
				if( (!(*st != '\0' && *ss == '\0')) || errno == ERANGE || Topic > TOPICS_COUNT) {
					errok = 0;
				}
				else errok = 1;
				free(st);

				//	Do real job (change the topic)
				SMessage mes;
				if(ReadDBMessage(MsgNum, &mes)) {
					DWORD oldTopic = mes.Topics;
					mes.Topics = Topic;
					if(WriteDBMessage(MsgNum, &mes)) {
						PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP, mes.ViIndex, mes.ViIndex);
						PrintBoardError(MESSAGEMAIN_messagechanged, MESSAGEMAIN_messagechanged2, HEADERSTRING_REFRESH_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_THREAD, mes.ViIndex);
						PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAQHELP, mes.ViIndex);
						print2log("Topic of message %d (%s (by %s)) was changed from [%s] to [%s] by %s", tmp, mes.MessageHeader, mes.AuthorName,
							Topics_List[oldTopic], Topics_List[Topic], ULogin.pui->username);
						goto End_part;
					}
				}
			}
		}

		//	request error
		else goto End_URLerror;
		}
	}
#endif

	if(strncmp(deal, "userlist", 8) == 0) {
		char *sn;
		DWORD code = 1; // by name
		if((sn = strget(deal, "userlist=", 255 - 1, '&')) != NULL) {
			errno = 0;
			DWORD retval;
			char *ss;
			retval = strtol(sn, &ss, 10);
			if( (!(*sn != '\0' && *ss == '\0')) || errno == ERANGE || retval == 0 || retval > 6) {

			}
			else code = retval;
		}

		// security check
		if((ULogin.LU.right & USERRIGHT_VIEW_MESSAGE) == 0) {
			printaccessdenied(deal);
			goto End_part;
		}
		if(code == 2 || code == 3 || code == 4 || code == 5 || code == 6) {
			// security check
			if((ULogin.LU.right & USERRIGHT_SUPERUSER) == 0) {
				printaccessdenied(deal);
				goto End_part;
			}
		}

		Tittle_cat(TITLE_UserList);

		PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);

		PrintUserList(&DB, code);

		PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);

		goto End_part;
	}

	if(strncmp(deal, "persmsgform", 11) == 0) {
		if(ULogin.LU.UniqID != 0) {
			// personal messages
			char *sn = strget(deal, "persmsgform=", 255 - 1, '&');
			if(sn) {
				char * f = FilterHTMLTags(sn, 255-1);
				free(sn);
				sn = f;
			}
			if(!sn) {
				sn = (char*)malloc(1000);
				strcpy(sn, "");
			}

			Tittle_cat(TITLE_AddPrivateMsg);

			PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);

			PrintPrivateMessageForm(sn, "");

			PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
			free(sn);
		}
		else {
			Tittle_cat(TITLE_Error);

			PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
			PrintBoardError(MESSAGEMAIN_privatemsg_denyunreg, MESSAGEMAIN_privatemsg_denyunreg2, 0);
			PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
		}
		goto End_part;
	}

	if(strncmp(deal, "persmsgpost", 11) == 0) {
		if(ULogin.LU.UniqID != 0) {
		// personal messages post or preview
		int bodyok = 0, nameok = 0, tolong = 0, allowpmsg = 1;
		
		/* get parameters */
		par = GetParams(MAX_PARAMETERS_STRING);
		if(par != NULL) {
			char *name, *body, *fbody, *todo;
			CProfiles prof;
			SProfile_UserInfo ui;
			SProfile_FullUserInfo fui;
			int preview = 0;

			todo = strget(par, "Post=", MAX_PARAMETERS_STRING - 1, '&');			

			if(todo != NULL && strcmp(todo, MESSAGEMAIN_privatemsg_prev_msg_btn) == 0) {
				preview = 1;
			}
			else if(todo != NULL && strcmp(todo, MESSAGEMAIN_privatemsg_send_msg_btn) == 0) {
				preview = 0;
			}
			else {
				free(par);
				printbadurl(deal);
				goto End_part;
			}

			free(todo);

			name = strget(par, "name=", PROFILES_MAX_USERNAME_LENGTH - 1, '&');
			body = strget(par, "body=", MAX_PARAMETERS_STRING - 1, '&');

			if(name && prof.GetUserByName(name, &ui, &fui, NULL) == PROFILE_RETURN_ALLOK) {
				if((ui.Flags & PROFILES_FLAG_PERSMSGDISABLED)) allowpmsg = 0;
				nameok = 1;
			}
			if(body && strlen(body) > 0) {
				DWORD retflg;
				if(FilterBoardTags(body, &fbody, 0, MAX_PARAMETERS_STRING - 1,
					MESSAGE_ENABLED_SMILES | MESSAGE_ENABLED_TAGS | BOARDTAGS_TAG_PREPARSE, &retflg) == 0) {
					/* if to long - ignore tags */
					
				}
				else {
					free(body);
					if(strcmp(fbody, " ") == 0) *fbody = 0;
					body = fbody;
				}
				if(strcmp(body, "") != 0) bodyok = 1;
				if(strlen(body) > PROFILE_PERSONAL_MESSAGE_LENGHT - 1) tolong = 1;
			}

			if(bodyok && nameok && (!tolong) && allowpmsg) {
				if(preview) {
					char tostr[2000];
					char *ss;

					Tittle_cat(TITLE_AddPrivateMsg);

					PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);

					printf(DESIGN_PREVIEW_PREVIEWMESSAGE "<BR>", MESSAGEHEAD_preview_preview_message);
					DB.Profile_UserName(name, tostr, 1);
					ss = ConvertFullTime(time(NULL));
					// print header
					printf("%s" MESSAGEMAIN_privatemsg_touser " %s, " MESSAGEMAIN_privatemsg_date " %s%s",
						DESIGN_open_dl, tostr, ss, DESIGN_close_dl);

					char *st = FilterHTMLTags(body, MAX_PARAMETERS_STRING - 1);
					char *st1 = NULL;
					DWORD retflg;
					if(FilterBoardTags(st, &st1, 0, MAX_PARAMETERS_STRING - 1, 
						MESSAGE_ENABLED_SMILES | MESSAGE_ENABLED_TAGS | BOARDTAGS_PURL_ENABLE |
						BOARDTAGS_EXPAND_ENTER, &retflg) == 0)
					{
						st1 = st;
						st = NULL;
					}
	
					// print message text
					printf(DESIGN_PRIVATEMSG_FRAME,	DESIGN_open_dl,
						DESIGN_PRIVATEMSG_FRAME_BGCL_OUT, st1, DESIGN_close_dl, DESIGN_break);

					PrintPrivateMessageForm(name, body);

					if(st) free(st);
					if(st1) free(st1);
				}
				else {
				if(prof.PostPersonalMessage(name, 0, body, ULogin.pui->username, ULogin.LU.UniqID) == PROFILE_RETURN_ALLOK) {

					Tittle_cat(TITLE_PrivateMsgWasPosted);

					PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE, MAINPAGE_INDEX);
					PrintBoardError(MESSAGEMAIN_privatemsg_msgwassent, MESSAGEMAIN_privatemsg_msgwassent2, HEADERSTRING_REFRESH_TO_MAIN_PAGE);

					// Send ackn. to recipient
					if((ui.Flags & PROFILES_FLAG_PERSMSGTOEMAIL)) {
						char *pb2;
						char subj[1000];
						char bdy[100000];

						sprintf(subj, MAILACKN_PRIVATEMSG_SUBJECT, ULogin.pui->username);

						if(!PrepareTextForPrint(body, &pb2, 0/*security*/, MESSAGE_ENABLED_TAGS | BOARDTAGS_EXPAND_ENTER | BOARDTAGS_PURL_ENABLE)) {
							pb2 = (char*)malloc(strlen(body) + 1);
							strcpy(pb2, body);
						}

						sprintf(bdy, MAILACKN_PRIVATEMSG_BODY, name, ULogin.pui->username, pb2, ULogin.pui->username);

						wcSendMail(fui.Email, subj, bdy);
						print2log("Private message mailackn was sent to %s (%s->%s)", fui.Email, ULogin.pui->username, name);

						free(pb2);
					}
				}
				else {
					Tittle_cat(TITLE_Error);

					PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
					PrintBoardError(MESSAGEMAIN_privatemsg_msgcantsend, MESSAGEMAIN_privatemsg_msgcantsend2, 0);
				}
				}

				free(body);
				free(name);
				if(nameok) free(fui.AboutUser);

				PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
				goto End_part;
			}
			else {
				Tittle_cat(TITLE_AddPrivateMsg);

				PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
			}
			if(nameok) free(fui.AboutUser);
			// print form and errors
			if(!allowpmsg) {
				printf("<P><CENTER><LI> <FONT COLOR=RED><BOLD>" MESSAGEMAIN_privatemsg_disable_pmsg "</BOLD></FONT></CENTER>");
			}
			else {
				if(!nameok)
					printf("<P><CENTER><LI> <FONT COLOR=RED>" MESSAGEMAIN_privatemsg_invalid_user "</FONT></CENTER>");
				if(!bodyok)
					printf("<P><CENTER><LI> <FONT COLOR=RED>" MESSAGEMAIN_privatemsg_invalid_body "</FONT></CENTER>");
				if(tolong)
					printf("<P><CENTER><LI> <FONT COLOR=RED>" MESSAGEMAIN_privatemsg_tolong_body "</FONT></CENTER>");
			}

			if(!name) {
				name = (char*)malloc(10);
				*name = 0;
			}
			if(!name) {
				body = (char*)malloc(10);
				*body = 0;
			}
			PrintPrivateMessageForm(name, body);

			PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
			free(body);
			free(name);
		}
		}
		else {
			Tittle_cat(TITLE_Error);

			PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
			PrintBoardError(MESSAGEMAIN_privatemsg_denyunreg, MESSAGEMAIN_privatemsg_denyunreg2, 0);
			PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
		}
		goto End_part;
	}

	if(strncmp(deal, "persmsg", 7) == 0) {
		if(ULogin.LU.UniqID != 0) {
		// personal messages
		char *sn;
		DWORD type = 0;
		if((sn = strget(deal, "persmsg=", 255 - 1, '&')) != NULL) {
			if(strcmp(sn, "all") == 0) {
				type = 1;
			}
			free(sn);
		}

		CProfiles prof;
		SPersonalMessage *msg, *frommsg;
		DWORD *tt, *ft;
		if(type) {
			tt = NULL;
			ft = NULL;
		}
		else {
			tt = (DWORD*)malloc(sizeof(DWORD));
			*tt = 10;
			ft = (DWORD*)malloc(sizeof(DWORD));
			*ft = 0;
		}
		// let's read to messages (maybe from too)
		if(prof.ReadPersonalMessages(NULL, ULogin.LU.SIndex, &msg, tt, &frommsg, ft) != PROFILE_RETURN_ALLOK)
			printhtmlerror();

		// let's get received message count
		DWORD cnt = 0, postedcnt = 0;
		if(msg) {
			while(msg[cnt].Prev != 0xffffffff) cnt++;
			cnt++;
		}

		if(ft) {
			if(cnt) {
				SPersonalMessage *msg1;
				time_t ld = msg[cnt-1].Date;

				if(prof.ReadPersonalMessagesByDate(NULL, ULogin.LU.SIndex, &msg1, 0, &frommsg, ld) != PROFILE_RETURN_ALLOK)
					printhtmlerror();

				// let's get posted message count
				if(frommsg) {
					while(frommsg[postedcnt].Prev != 0xffffffff) postedcnt++;
					postedcnt++;
				}
			}
			else {
				*ft = 10;
				// let's read to messages (maybe from too)
				if(prof.ReadPersonalMessages(NULL, ULogin.LU.SIndex, &msg, NULL, &frommsg, ft) != PROFILE_RETURN_ALLOK)
					printhtmlerror();
				// let's get posted message count
				if(frommsg) {
					while(frommsg[postedcnt].Prev != 0xffffffff) postedcnt++;
					postedcnt++;
				}
			}
		}
		else {
			// let's get posted message count
			if(frommsg) {
				while(frommsg[postedcnt].Prev != 0xffffffff) postedcnt++;
				postedcnt++;
			}
		}

		Tittle_cat(TITLE_PrivateMsg);

		PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_PRIVATEMSG, MAINPAGE_INDEX);

		char uuname[1000];
		DB.Profile_UserName(ULogin.pui->username, uuname, 1);

		printf("<BR><BR><CENTER><B>" MESSAGEMAIN_privatemsg_header " %s</B><BR>", uuname);
		if((ULogin.pui->Flags & PROFILES_FLAG_PERSMSGDISABLED))
			printf("<U>" MESSAGEMAIN_privatemsg_disabled "</U><BR>");

		if(cnt) printf(MESSAGEMAIN_privatemsg_newmsgcnt " %d, ",
			ULogin.pui->persmescnt - ULogin.pui->readpersmescnt);
		else printf(MESSAGEMAIN_privatemsg_nonewmsg ", ");
		printf(MESSAGEMAIN_privatemsg_allmsgcnt " %d, " MESSAGEMAIN_privatemsg_allmsgcnt1 " %d<BR>"
			"<A HREF=\"" MY_CGI_URL "?persmsgform\" STYLE=\"text-decoration:underline;\">" MESSAGEMAIN_privatemsg_writenewmsg
			"</A></CENTER><P><P>", ULogin.pui->persmescnt, ULogin.pui->postedmescnt);

		char tostr[1000], newm[100], *nickurl;
		char *ss;
		SPersonalMessage *pmsg;
		int i = 0;
		int j = 0;
		int received = 0;	// posted or received
		for(;;) {
			// check exit expression
			if(i == cnt && j == postedcnt) break;
			if(i == cnt) {
				pmsg = &(frommsg[j]);
				j++;
				received = 0;
			} else {
				if(j == postedcnt) {
					pmsg = &(msg[i]);
					i++;
					received = 1;
				}
				else {
					if(frommsg[j].Date > msg[i].Date) {
						pmsg = &(frommsg[j]);
						j++;
						received = 0;
					}
					else {
						pmsg = &(msg[i]);
						i++;
						received = 1;
					}
				} 
			}

			if(!received) {
				DB.Profile_UserName(pmsg->NameTo, tostr, 1);
			}
			else {
				DB.Profile_UserName(pmsg->NameFrom, tostr, 1);
			}

			ss = ConvertFullTime(pmsg->Date);

			if(received && i <= (ULogin.pui->persmescnt - ULogin.pui->readpersmescnt))
				strcpy(newm, MESSAGEMAIN_privatemsg_newmark);
			else strcpy(newm, "");

			if(!received) {
				// posted message
				printf("%s" MESSAGEMAIN_privatemsg_touser " %s, " MESSAGEMAIN_privatemsg_date " %s%s",
					DESIGN_open_dl, tostr, ss, DESIGN_close_dl);
			}
			else {
				nickurl = CodeHttpString(pmsg->NameFrom, 0);
				// received message
				if(nickurl) {
					printf("%s" MESSAGEMAIN_privatemsg_fromuser " %s, " MESSAGEMAIN_privatemsg_date
						" %s <A HREF=\"" MY_CGI_URL "?persmsgform=%s\" STYLE=\"text-decoration:underline;\">" MESSAGEMAIN_privatemsg_answer "</A> %s%s",
						DESIGN_open_dl, tostr, ss, nickurl, newm, DESIGN_close_dl);
				}
			}

			char *st = FilterHTMLTags(pmsg->Msg, MAX_PARAMETERS_STRING - 1);
			char *st1 = NULL;
			DWORD retflg;
			if(FilterBoardTags(st, &st1, 0, MAX_PARAMETERS_STRING - 1, 
				MESSAGE_ENABLED_SMILES | MESSAGE_ENABLED_TAGS | BOARDTAGS_PURL_ENABLE |
				BOARDTAGS_EXPAND_ENTER, &retflg) == 0)
			{
				st1 = st;
				st = NULL;
			}

			printf(DESIGN_PRIVATEMSG_FRAME,	DESIGN_open_dl,
				received ? DESIGN_PRIVATEMSG_FRAME_BGCL_IN : DESIGN_PRIVATEMSG_FRAME_BGCL_OUT, st1, DESIGN_close_dl, DESIGN_break);

			if(st) free(st);
			if(st1) free(st1);
		}

		ULogin.pui->readpersmescnt = ULogin.pui->persmescnt;
		prof.SetUInfo(ULogin.LU.SIndex, ULogin.pui);

		PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_PRIVATEMSG, MAINPAGE_INDEX);
		if(msg) free(msg);
		if(frommsg) free(frommsg);
		}
		else {
			Tittle_cat(TITLE_Error);

			PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
			PrintBoardError(MESSAGEMAIN_privatemsg_denyunreg, MESSAGEMAIN_privatemsg_denyunreg2, 0);
			PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
		}
		goto End_part;
	}

	if(strncmp(deal, "globann", 7) == 0) {
		if((ULogin.LU.right & USERRIGHT_POST_GLOBAL_ANNOUNCE) != 0) {
			// post global announce or global announce form
			char *sn;
			DWORD type = 0;
			if((sn = strget(deal, "globann=", 255 - 1, '&')) != NULL) {
				if(strcmp(sn, "post") == 0) {
					type = 1;
					free(sn);
				}
			}
			if(!type) {
				Tittle_cat(TITLE_PostGlobalAnnounce);

				char *ss, body[GLOBAL_ANNOUNCE_MAXSIZE];
				int cgann_num;
				cgann_num = strtol(sn, &ss, 10);
				if( (!(*sn != '\0' && *ss == '\0')) || errno == ERANGE) {
					cgann_num = 0;
				}
				free(sn);

				body[0] = 0;
				if(cgann_num != 0) {
					SGlobalAnnounce *ga;
					DWORD cnt, i;
					if(ReadGlobalAnnounces(0, &ga, &cnt) != ANNOUNCES_RETURN_OK) printhtmlerror();
					for(i = 0; i < cnt; i++) {
						if(ga[i].Number == cgann_num) {
							strcpy(body, ga[i].Announce);
						}
					}
				}

				// print form
				PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);

				PrintAnnounceForm(body, cgann_num);

				PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
				goto End_part;
			}
			else {
				char *body = NULL;
				int cgann_num = 0, preview = 0, refid = 0;

				// post
				/* get parameters */
				par = GetParams(MAX_PARAMETERS_STRING);
				if(par) {
					char *todo = strget(par, "Post=", MAX_PARAMETERS_STRING - 1, '&');

					// preview or post ?
					if(todo != NULL && strcmp(todo, MESSAGEMAIN_globann_prev_ann_btn) == 0) {
						preview = 1;
					}
					else if(todo != NULL && strcmp(todo, MESSAGEMAIN_globann_send_ann_btn) == 0) {
						preview = 0;
					}
					else {
						free(par);
						printbadurl(deal);
						goto End_part;
					}
					free(todo);

					char *sn = strget(par, "cgann=", 255 - 1, '&');
					char *refids = strget(par, "refid=", 100, '&');
					if(refids) {
						if(strcmp(refids, "1") == 0) refid = 1;
						free(refids);
					}
					body = strget(par, "body=", MAX_PARAMETERS_STRING - 1, '&');
					// translate to numeric format
					char *ss;
					if(sn) {
						cgann_num = strtol(sn, &ss, 10);
						if( (!(*sn != '\0' && *ss == '\0')) || errno == ERANGE) {
							cgann_num = 0;
						}
						free(sn);
					}
					free(par);
				}
				if(body && strlen(body) > 5) {
					if(strlen(body) >= GLOBAL_ANNOUNCE_MAXSIZE - 1) {

						Tittle_cat(TITLE_PostGlobalAnnounce);

						PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);

						printf("<P><CENTER><LI> <FONT COLOR=RED>" MESSAGEMAIN_globann_tolong "</FONT></CENTER>");

						PrintAnnounceForm(body, cgann_num);

						PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
					}
					else {
						if(preview) {
							char uname[1000];
							char *st;
							char date[1000];

							DB.Profile_UserName(ULogin.pui->username, uname, 1);
							PrepareTextForPrint(body, &st, 0, MESSAGE_ENABLED_SMILES | MESSAGE_ENABLED_TAGS |
								BOARDTAGS_PURL_ENABLE | BOARDTAGS_EXPAND_ENTER);
							ConvertTime(time(NULL), date);

							Tittle_cat(TITLE_PostGlobalAnnounce);

							PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);

							printf("<BR><BR><CENTER><B>%s</B></CENTER>", MESSAGEMAIN_globann_preview_hdr);

							printf(DESIGN_GLOBALANN_FRAME, DESIGN_open_dl, st, MESSAGEMAIN_globann_postedby,
								uname, date, "", DESIGN_close_dl);

							PrintAnnounceForm(body, cgann_num);

							PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);

							if(st) free(st);
						}
						else {
							if( (cgann_num && (!refid)) ? UpdateGlobalAnnounce(cgann_num, ULogin.pui->username,
								ULogin.pui->UniqID, body, 0, 0,
								ANNOUNCES_UPDATE_OPT_USER | ANNOUNCES_UPDATE_OPT_TIME |
								ANNOUNCES_UPDATE_OPT_TTL | ANNOUNCES_UPDATE_OPT_FLAGS) !=
								ANNOUNCES_RETURN_OK
								: PostGlobalAnnounce(ULogin.pui->username, ULogin.pui->UniqID, body,
								0, 0) != ANNOUNCES_RETURN_OK )
							{
								Tittle_cat(TITLE_Error);

								PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
								PrintBoardError(MESSAGEMAIN_globann_anncantsend, MESSAGEMAIN_globann_anncantsend2, 0);
								PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
							}
							else {
								Tittle_cat(TITLE_GlobalAnnWasPosed);

								if(cgann_num && refid) {
									DeleteGlobalAnnounce(cgann_num, 0);
									print2log("Announce %d was deleted during update", cgann_num);
								}

								if(!cgann_num) {
									PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE, MAINPAGE_INDEX);
									PrintBoardError(MESSAGEMAIN_globann_annwassent, MESSAGEMAIN_globann_annwassent2, HEADERSTRING_REFRESH_TO_MAIN_PAGE);
									PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE, MAINPAGE_INDEX);
								}
								else {
									PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE, MAINPAGE_INDEX);
									PrintBoardError(MESSAGEMAIN_globann_annwasupdated, MESSAGEMAIN_globann_annwasupdated2, HEADERSTRING_REFRESH_TO_MAIN_PAGE);
									PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE, MAINPAGE_INDEX);
								}

								print2log("Global announce (%s) was posted by %s", body, ULogin.pui->username);
							}
						}
					}
				}
				else if(body) {
					Tittle_cat(TITLE_PostGlobalAnnounce);

					PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);

					printf("<P><CENTER><LI> <FONT COLOR=RED>" MESSAGEMAIN_globann_toshort "</FONT></CENTER>");

					if(body) PrintAnnounceForm(body, cgann_num);
					else PrintAnnounceForm("", cgann_num);

					PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
				}
				else {
					// invalid request
					printbadurl(deal);
				}
				goto End_part;
			}
		}
		else {
			printaccessdenied(deal);
			goto End_part;
		}
	}

	if(strncmp(deal, "ganndel", 7) == 0) {
		char *sn;
		if((ULogin.LU.right & USERRIGHT_POST_GLOBAL_ANNOUNCE) != 0) {
			DWORD MsgNum = 0;
			if((sn = strget(deal, "ganndel=", 255 - 1, '&')) != NULL) {
				errno = 0;
				int errok;
				char *ss;
				MsgNum = strtol(sn, &ss, 10);
				if( (!(*sn != '\0' && *ss == '\0')) || errno == ERANGE) {
					errok = 0;
				}
				else errok = 1;
				free(sn);

				if(errok) {
					if(DeleteGlobalAnnounce(MsgNum, ((ULogin.LU.right & USERRIGHT_SUPERUSER) != 0) ? 0 : ULogin.LU.UniqID) == ANNOUNCES_RETURN_OK) {
						Tittle_cat(TITLE_GlobalAnnWasDeleted);

						print2log("Global Announce %d was deleted by user %s", MsgNum, ULogin.pui->username);
						PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
						PrintBoardError(MESSAGEMAIN_globann_wasdeleted, MESSAGEMAIN_globann_wasdeleted2, 0);
						PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
					}
					else {
						Tittle_cat(TITLE_Error);

						print2log("Global Announce %d cannot be deleted by user %s", MsgNum, ULogin.pui->username);
						PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
						PrintBoardError(MESSAGEMAIN_globann_cannotdel, MESSAGEMAIN_globann_cannotdel2, 0);
						PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
					}
				}
				else {
					Tittle_cat(TITLE_Error);

					PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
					PrintBoardError(MESSAGEMAIN_globann_invalidnum, MESSAGEMAIN_globann_invalidnum2, 0);
					PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
				}
			}
			goto End_part;
		}
	}

	if(strncmp(deal, "rann", 4) == 0) {
		char *sn;
		if((ULogin.LU.right & USERRIGHT_VIEW_MESSAGE) != 0) {
			DWORD MsgNum = 0;
			if((sn = strget(deal, "rann=", 255 - 1, '&')) != NULL) {
				errno = 0;
				int errok;
				char *ss;
				MsgNum = strtol(sn, &ss, 10);
				if( (!(*sn != '\0' && *ss == '\0')) || errno == ERANGE) {
					errok = 0;
				}
				else errok = 1;
				free(sn);

				if(!errok) goto End_URLerror;

				currentlann = MsgNum;
				// to main page at once
				PrintHTMLHeader(HEADERSTRING_REDIRECT_NOW | HEADERSTRING_NO_CACHE_THIS, MAINPAGE_INDEX);
				goto End_part;
			}
		}
	}

#ifdef USER_FAVOURITES_SUPPORT
	if(strncmp(deal, "favs", 4) == 0) {
		int num;
		if(ULogin.LU.UniqID != 0) {
			CProfiles prof;
			//favourites
			// security check
			if((ULogin.LU.right & USERRIGHT_VIEW_MESSAGE) == 0) {
				printaccessdenied(deal);
				goto End_part;
			}
			Tittle_cat(TITLE_FavouritesPage);
			PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAVOURITES, MAINPAGE_INDEX);
			printf("<P><CENTER><P><B>%s</B><BR></CENTER>", MESSAGEHEAD_favourites);

			int updated;
			if( (num = DB.PrintandCheckMessageFavsExistandInv(ULogin.pui,
				ULogin.LU.right & USERRIGHT_SUPERUSER, &updated)) == 0)
				printf("<P><CENTER><B>" MESSAGEMAIN_favourites_listclear "</B></CENTER><P>");
			if(updated) prof.SetUInfo(ULogin.LU.SIndex, ULogin.pui);
			PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_DISABLE_FAVOURITES, MAINPAGE_INDEX);
			goto End_part;
		}
		else {
			Tittle_cat(TITLE_Error);
			PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
			PrintBoardError(MESSAGEMAIN_favourites_denyunreg, MESSAGEMAIN_favourites_denyunreg2, 0);
			PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
		}
		goto End_part;
	}
	
	if(strncmp(deal, "favadd", 6) == 0) {
		if(ULogin.LU.UniqID != 0){
			if((ULogin.LU.right & USERRIGHT_VIEW_MESSAGE) == 0) {
				printaccessdenied(deal);
				goto End_part;
			}
			if((st = strget(deal, "favadd=", 255 - 1, '&')) != NULL) {
				errno = 0;
				DWORD addmsg;
				char *ss;
				addmsg = strtol(st, &ss, 10);
				if( (!(*st != '\0' && *ss == '\0')) || errno == ERANGE  || addmsg < 1){
					free (st);
					 goto End_URLerror;
				}
				free (st);
				DWORD msg;
				if( ( msg=DB.TranslateMsgIndex(addmsg)) == NO_MESSAGE_CODE){
					printnomessage(deal);
					goto End_part;
				}
				if(!ReadDBMessage(msg, &mes)) printhtmlerror();
				/* allow read invisible message only to SUPERUSER */
				if((mes.Flag & MESSAGE_IS_INVISIBLE) && ((ULogin.LU.right & USERRIGHT_SUPERUSER) == 0)) {
					printnomessage(deal);
					goto End_part;
				}
#if USER_FAVOURITES_SUPPORT == 2
				if( mes.ParentThread != 0){
						Tittle_cat(TITLE_FavouritesPageAdd);
						PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
						PrintBoardError(MESSAGEMAIN_favourites_addno, MESSAGEMAIN_favourites_addnoparent, 0);
						PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
						goto End_part;
				}
#endif
				CProfiles prof;
				int result = prof.CheckandAddFavsList(ULogin.LU.SIndex, addmsg, 1);
				switch(result) {
					case PROFILE_RETURN_ALLOK:
						Tittle_cat(TITLE_FavouritesPageAdd);
						PrintHTMLHeader(HEADERSTRING_DISABLE_ALL, 0);
						PrintBoardError(MESSAGEMAIN_favourites_added, MESSAGEMAIN_favourites_added2, 0);
						PrintBottomLines(HEADERSTRING_DISABLE_ALL, 0);
						goto End_part;
					case PROFILE_RETURN_ALREADY_EXIST:
						Tittle_cat(TITLE_FavouritesPageAdd);
						PrintHTMLHeader(HEADERSTRING_DISABLE_ALL, 0);
						PrintBoardError(MESSAGEMAIN_favourites_addno, MESSAGEMAIN_favourites_addexist, 0);
						PrintBottomLines(HEADERSTRING_DISABLE_ALL, 0);
						goto End_part;
					case PROFILE_RETURN_UNKNOWN_ERROR:
						Tittle_cat(TITLE_FavouritesPageAdd);
						PrintHTMLHeader(HEADERSTRING_DISABLE_ALL, 0);
						PrintBoardError(MESSAGEMAIN_favourites_addno, MESSAGEMAIN_favourites_addnoplace, 0);
						PrintBottomLines(HEADERSTRING_DISABLE_ALL, 0);
						goto End_part;
					default:
						printhtmlerror();
						goto End_part;
					
				}
			}
		}
		else {
			Tittle_cat(TITLE_Error);
			PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
			PrintBoardError(MESSAGEMAIN_favourites_denyunreg, MESSAGEMAIN_favourites_denyunreg2, 0);
			PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
		}
		goto End_part;
	}

	if(strncmp(deal, "favdel", 6) == 0) {
		if(ULogin.LU.UniqID != 0){
			if((ULogin.LU.right & USERRIGHT_VIEW_MESSAGE) == 0) {
				printaccessdenied(deal);
				goto End_part;
			}
			if((st = strget(deal, "favdel=", 255 - 1, '&')) != NULL) {
				errno = 0;
				DWORD delmsg;
				char *ss;
				delmsg = strtol(st, &ss, 10);
				if( (!(*st != '\0' && *ss == '\0')) || errno == ERANGE  || delmsg == 0){
					free (st);
					 goto End_URLerror;
				}
				free (st);
				DWORD msg;
				if( (msg = DB.TranslateMsgIndex(delmsg)) == NO_MESSAGE_CODE){
					printnomessage(deal);
					goto End_part;
				}
				if(!ReadDBMessage(msg, &mes)) printhtmlerror();
				/* allow read invisible message only to SUPERUSER */
				if((mes.Flag & MESSAGE_IS_INVISIBLE) && ((ULogin.LU.right & USERRIGHT_SUPERUSER) == 0)) {
					printnomessage(deal);
					goto End_part;
				}
				CProfiles prof;
				int result = prof.DelFavsList(ULogin.LU.SIndex, delmsg);
				switch(result) {
					case PROFILE_RETURN_ALLOK:
						Tittle_cat(TITLE_FavouritesPageDel);
						PrintHTMLHeader(HEADERSTRING_DISABLE_ALL, 0);
						PrintBoardError(MESSAGEMAIN_favourites_deleted, MESSAGEMAIN_favourites_deleted2, 0);
						PrintBottomLines(HEADERSTRING_DISABLE_ALL, 0);
						goto End_part;
					case PROFILE_RETURN_UNKNOWN_ERROR:
						Tittle_cat(TITLE_FavouritesPageDel);
						PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
						PrintBoardError(MESSAGEMAIN_favourites_delno,  MESSAGEMAIN_favourites_delnoexist, 0);
						PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
						goto End_part;
					default:
						printhtmlerror();
						goto End_part;
					
				}
			}
		}
		else {
			Tittle_cat(TITLE_Error);
			PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
			PrintBoardError(MESSAGEMAIN_favourites_denyunreg, MESSAGEMAIN_favourites_denyunreg2, 0);
			PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
		}
		goto End_part;
	}
#endif

#ifdef CLEANUP_IDLE_USERS

#define DAY (60*60*24)
#define YEAR (DAY*365)
	if(strncmp(deal, "cluserlist", 10) == 0) {
		bool fDelete = false;
		if(!(ULogin.LU.right & USERRIGHT_SUPERUSER)) {
			printaccessdenied(deal);
			goto End_part;
		}
		if((st = strget(deal,"cluserlist=", 14, '&')) != NULL) {
			if(strcmp(st, "yes") == 0) {
				free(st);
				fDelete = true;
			}
		}
		time_t tn = time(NULL);
		DWORD i = 0, ii=0;
		char **buf = NULL;
		char name[1000];
		DWORD uc = 0;
		CUserLogin ULogin;
		CProfiles uprof;
		Tittle_cat(TITLE_UserList);
        PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);

		printf("<BR><BR><center>");
		printf(
			"<font color=\"red\">%s</font><br>\n",
			fDelete ?
			"��������� ������������ ���� ������� �������:" : 
		"������ ������������� ���������� ��������:"
		);
		if(!uprof.GenerateUserList(&buf, &uc)) {
			printf("error generating list");
		} else {
			qsort((void*)buf, uc, sizeof(char*), cmp_name);
			for(i = 0; i < uc; i++) {
		    		DWORD 
					PostCount = *((DWORD*)(buf[i] + 4)),
					RefreshCount = *((DWORD*)(buf[i] + 12)),
					UserRight =  *((DWORD*)(buf[i] + 16)),
					LoginDate = *((DWORD*)(buf[i] + 8)),
					activity = PostCount + RefreshCount;
				char *username = buf[i] + 20;
				int
					idletime = tn - LoginDate;
				bool
					fSuperuser = UserRight & USERRIGHT_SUPERUSER,
					fAged1 = idletime > YEAR,
				 	fAged2 = idletime > YEAR / 2 &&	activity < 100,
					fAged3 = idletime > YEAR / 4 && activity < 10;
				if(!fSuperuser && (fAged1 || fAged2 || fAged3)) {	
					if((ii % 10) != 0) printf(" | ");
					if(((ii % 10) == 0) && ii != 0) printf("<br>");
					DB.Profile_UserName(buf[i] + 20, name, 1, 1);
					printf("%s", name);
					if(fDelete) {
						DWORD err = uprof.DeleteUser(username);
						if(err == PROFILE_RETURN_ALLOK) {
							printf("!");
						} else { 
							printf("���������� ������� '%s' !!!", name);
							goto End_part;
						}
					}
					ii++;
				}// if(!fSuperuser && (fAged1 || fAged2 || fAged3)
				free(buf[i]);
			} //for(;i<uc;)
			if(fDelete) {
				printf(
					"<br /> <b>������� %d �� %d �������������</b> \n", 
					ii, 
					uc);
			} else {
				printf(
					"<br /> <b>����� ������� <fonc color=red>%d</font> �� %d �������������</b> \n",
					ii, 
					uc);
				printf(
					"<br /><a href=\"" MY_CGI_URL "?cluserlist=yes\">"
					"<font color=\"red\">���������� ?</font></a>");
			}
		}//if(generate_list(..))
		PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, MAINPAGE_INDEX);
		goto End_part;
	}//if(strcmp(deal,"cluserlist")) 	
#endif //CLEANUP_IDLE_USERS


	if(strncmp(deal, "banlist", 7) == 0) {
		
		/* security check */
		if((ULogin.LU.right & USERRIGHT_SUPERUSER) == 0) {
			printaccessdenied(deal);
			goto End_part;
		}
		if((st = strget(deal,"banlist=", 30, '&')) != NULL) {
			if(strcmp(st, "save") == 0) {
				// read ban list
				par = GetParams(MAX_PARAMETERS_STRING);
				char *ban_list;
				ban_list = strget(par,"ban_list=", MAX_PARAMETERS_STRING, '&');
				// this is needed because of char #10 filtering.
				// in WIN32 printf() works incorrectly with it
#if defined(WIN32)
				char *mb;
				FilterMessageForPreview(ban_list, &mb);
				free(ban_list);
				ban_list = mb;
#endif
				
				// check ban_list is empty
				if(ban_list == NULL || *ban_list == 0) {
					Tittle_cat(TITLE_BanSave);
					PrintHTMLHeader(HEADERSTRING_DISABLE_ALL, 0);
					PrintBoardError(MESSAGEMAIN_ban_no_save, MESSAGEMAIN_ban_empty, 0);
					PrintBottomLines(HEADERSTRING_DISABLE_ALL, 0);
					goto End_part;
			
				}
				
				WCFILE *BAN_FILE;
				if ((BAN_FILE = wcfopen(F_BANNEDIP, FILE_ACCESS_MODES_RW)) == NULL) printhtmlerror();
				lock_file(BAN_FILE);
				
				if ( !fCheckedWrite(ban_list, strlen(ban_list), BAN_FILE)  )  {
					unlock_file(BAN_FILE);
					printhtmlerror();
				}


#ifdef WIN32	
				wctruncate(BAN_FILE, strlen(ban_list));
#else
				truncate(F_BANNEDIP, strlen(ban_list));
#endif

				unlock_file(BAN_FILE);
				wcfclose(BAN_FILE);
				
				Tittle_cat(TITLE_BanSave);
				PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE, 0);
				PrintBoardError(MESSAGEMAIN_ban_save, MESSAGEMAIN_ban_save2, 0);
				PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE | HEADERSTRING_REFRESH_TO_MAIN_PAGE, 0);

				print2log("Banlist update by %s from %s", ULogin.pui->username, Cip);
			
			}	
		}
		else{

			PrintHTMLHeader(HEADERSTRING_RETURN_TO_MAIN_PAGE, 0);
			PrintBanList();
			PrintBottomLines(HEADERSTRING_RETURN_TO_MAIN_PAGE, 0);

			print2log("Banlist view by %s from %s", ULogin.pui->username, Cip);
		}
		goto End_part;
	}
	
	
	
	if(strncmp(deal, "clsession1", 9) == 0) {
		if(ULogin.LU.UniqID != 0) {
		DWORD closeseq[2];
		if((st= strget(deal, "clsession1=", 255 - 1, '&')) != NULL) {
			errno = 0;
			char *ss;
			 closeseq[0] = strtol(st, &ss, 10);
			if( (!(*st != '\0' && *ss == '\0')) || errno == ERANGE ||  closeseq[0] == 0 ) {
				free(st);
				goto End_URLerror;
			}
		}
		free(st);
		if((st= strget(deal, "clsession2=", 255 - 1, '&')) != NULL) {
			errno = 0;
			char *ss;
			 closeseq[1] = strtol(st, &ss, 10);
			if( (!(*st != '\0' && *ss == '\0')) || errno == ERANGE ||  closeseq[1] == 0 ) {
				free(st);
				goto End_URLerror;
			}
		}
		free(st);

		// checking session exists and user have suff. rights
		// superuser knows if session does not exits
		// user can receive only access deny message (session bf aware)

		if((ULogin.LU.right & USERRIGHT_SUPERUSER) == 0){
			if( (ULogin.LU.right & USERRIGTH_PROFILE_MODIFY) == 0 || ULogin.CheckSession(closeseq, 0, ULogin.LU.UniqID) != 1) {
				printaccessdenied(deal);
				goto End_part;
			}
		}
		else {
			if( ULogin.CheckSession(closeseq, 0, 0) != 1) {
				Tittle_cat(TITLE_ClSession);
				PrintHTMLHeader(HEADERSTRING_DISABLE_ALL | HEADERSTRING_REFRESH_TO_MAIN_PAGE, 0);
				PrintBoardError(MESSAGEMAIN_session_closed_no, MESSAGEMAIN_session_check_failed, 0);
				PrintBottomLines(HEADERSTRING_DISABLE_ALL | HEADERSTRING_REFRESH_TO_MAIN_PAGE, 0);
				goto End_part;
			}
		}

		// force closing session
		if(ULogin.ForceCloseSessionBySeq(closeseq)){
			Tittle_cat(TITLE_ClSession);
			PrintHTMLHeader(HEADERSTRING_DISABLE_ALL | HEADERSTRING_REFRESH_TO_MAIN_PAGE, 0);
			PrintBoardError(MESSAGEMAIN_session_closed_ok, MESSAGEMAIN_session_closed_ok2, 0);
			PrintBottomLines(HEADERSTRING_DISABLE_ALL | HEADERSTRING_REFRESH_TO_MAIN_PAGE, 0);
			goto End_part;
		}
		else {
			Tittle_cat(TITLE_ClSession);
			PrintHTMLHeader(HEADERSTRING_DISABLE_ALL | HEADERSTRING_REFRESH_TO_MAIN_PAGE, 0);
			PrintBoardError(MESSAGEMAIN_session_closed_no, MESSAGEMAIN_session_close_failed, 0);
			PrintBottomLines(HEADERSTRING_DISABLE_ALL | HEADERSTRING_REFRESH_TO_MAIN_PAGE, 0);
			goto End_part;
		}
		}
		goto End_URLerror;
		
	}
	

End_URLerror:
	printbadurl(deal);

End_part:

#if _DEBUG_ == 1
	//print2log("Exit success");
#ifdef WIN32
#ifdef _DEBUG
	free(Cip);
	free(deal);
	free(ConfTitle);
	_CrtDumpMemoryLeaks();
#endif
#endif // WIN32
#endif // _DEBUG_

	return EXIT_SUCCESS;
}
