// RTboard quick reply script
// Based on LJ Instant Comment (below)
// Version 0.2 2005-12-13
// Ported by demon
//
//
// LJ Instant Comment
// version 0.6
// 2005-08-23
// Copyright (c) 2005, Tim Babych
// Homepage: http://clear.com.ua/projects/firefox/instant_comment
// Released under the GPL license
// http://www.gnu.org/copyleft/gpl.html
//
// --------------------------------------------------------------------
//
// This is a Greasemonkey user script.
//
// To install, you need Greasemonkey: http://greasemonkey.mozdev.org/
// Then restart Firefox and revisit this script.
// Under Tools, there will be a new menu item to "Install User Script".
// Accept the default configuration and install.
//
// To uninstall, go to Tools/Manage User Scripts,
// select "Ctrl+Enter Submits", and click Uninstall.
//
// --------------------------------------------------------------------
//
// ==UserScript==
// @name          RtBoard Reply 
// @description	 Allows replying on rtboard
// @include      http://board.rt.mipt.ru/*
// @include      http://zlo.rt.mipt.ru/*
// ==/UserScript==

	// cyrillic quotes
	_smart_quotes_regexp = '$1\u00ab$2\u00bb$3'

	// latin quotes 
//	_smart_quotes_regexp = '$1\u201c$2\u201d$3'

//================================
//	WORKHORSES
//================================

// triggerd by clicking Instant Comment link
function want_reply(){
	


	dd = document.getElementById('quick_reply')
	
	if (dd.getAttribute('caller_entry_id') == this.id) // hide-unhide
		if (dd.style.display == 'block' ) {
			hide_div(dd)
			return
		} else
			dd.style.display = 'block'
	else {
		// insert it in da new place
		Xwidth = 350
		Xshift = 170
		Xcoord = findPosX(this)
		Xwindow = document.body.clientWidth

		dd.style.left = (Xcoord - Xshift) + "px"
		if(Xcoord < Xshift) dd.style.left = 0
		if(Xcoord + Xwidth - Xshift > Xwindow) dd.style.left = (Xwindow - Xwidth - 20) + "px"


		dd.style.top = (findPosY(this) + 20) + "px"
		dd.setAttribute('caller_entry_id', this.id)
		dd.firstChild.value = ''
		dd.style.display = 'block'
	} 
	
	// workaround near firefox 1.0 bug with focus()
	hScroll = window.pageXOffset; vScroll = window.pageYOffset
	dd.firstChild.focus()
	window.scrollTo(hScroll, vScroll)
}

// triggered when user presses Ctrl+Enter
function trigger_submit_on_ctrl_enter(e) {
	//hide on Esc
	if (e.keyCode==27) {
		hide_div(this.parentNode)
	}

	// not enter and (ctrl or alt)
	if (! (e.keyCode==13 && (e.ctrlKey || e.altKey))) 
		return


	qsubj = document.getElementById('quick_subj')
	msgsubj = zakavych(qsubj.value)
	msgsubj = EscapeToWin(msgsubj)
	qsubj.value = ''

	qbody = document.getElementById('quick_body')
	msgbody = zakavych(qbody.value)
	msgbody = EscapeToWin(msgbody)
	qbody.value = ''


	reply = document.getElementById(this.parentNode.getAttribute('caller_entry_id'))
	reply.style.border = "3px solid rgb(0,250,0)"
	reply.style.padding = "3px"
	reply.setAttribute('mycolor', 200)
	reply.setAttribute('step', -5)


	fading = function(){
		elem = reply
		mycolor = parseInt(elem.getAttribute('mycolor'))
		step = parseInt(elem.getAttribute('step'))
		if ( mycolor> 250)
			step -= 5
		if ( mycolor < 50)
			step += 5
		mycolor += step
		elem.style.borderColor = "rgb("+mycolor+", 250,"+mycolor+")"
		elem.setAttribute('mycolor', mycolor)
		reply.setAttribute('step', step)
		//GM_log("rgb("+mycolor+", 250,"+mycolor+")")
	}


	anim = window.setInterval(fading, 15)
	hide_div(this.parentNode)


	GM_xmlhttpRequest({
	    method: 'POST',
	    url: 'http://' + window.location.hostname + '/?xpost=' + reply.getAttribute('msgid'),
	    headers: {
		'User-agent': 'Mozilla/4.0 (compatible) Greasemonkey',
		'Content-type': 'application/x-www-form-urlencoded; charset=cp1251',
		'Referer' : 'http://' + window.location.hostname + '/?read=' + reply.getAttribute('msgid')
	    },
	    data: 'jpost=post&subject=' + msgsubj + '&body=' + msgbody ,
	    onload: function() {
			window.clearInterval(anim); 
			reply.style.borderColor="green"
		}
	});

}

// hide input div, return focus to page
function hide_div(div){
	div.style.display = 'none'
	div.firstChild.blur()
}


//================================
//	INIT
//================================

d = document.createElement('DIV')
d.id = "quick_reply"
d.setAttribute('caller_entry_id', false)
d.innerHTML = "<input name='quick_subj' id='quick_subj' maxlength='99' /><br />" +
		"<textarea name='quick_body' id='quick_body'></textarea><br />"+
		"<small>Ctrl+Enter to post</small>"

d.firstChild.addEventListener("keydown", trigger_submit_on_ctrl_enter, false);
document.body.appendChild(d)

document.getElementById('quick_body').addEventListener("keydown", trigger_submit_on_ctrl_enter, false);
//--------------------------------------------------------

get_itemid_regexp = /.*read=(\d+)/

var allReplies
allReplies = document.evaluate(
    "//a[contains(@href, '?read=')]",
    document,
    null,
    XPathResult.UNORDERED_NODE_SNAPSHOT_TYPE,
    null);
for (var i = 0; i < allReplies.snapshotLength; i++) {
	thisOne = allReplies.snapshotItem(i);
	
	params =  get_itemid_regexp.exec(thisOne.href)
	
	a = document.createElement('a')
	
	a.href = 'javascript:void(0)'
	a.addEventListener("click", want_reply, false);
	a.setAttribute('msgid', params[1])
	a.id = 'reply_'+params[1]
	linktxt = document.createTextNode('(r)')
	a.appendChild(linktxt)
	
	t = document.createTextNode(' -- ')

	thisOne.parentNode.insertBefore(t, thisOne.nextSibling);
	t.parentNode.insertBefore(a, t.nextSibling);
}
//-----------------------------------------------------------------------
addGlobalStyle(
"#quick_reply { position: absolute; display: none;	border: 1px solid #999;"+
"	background: #ececec; padding: 1px; text-align: center; z-index:99}"+
"#quick_reply textarea { width:350px; height:100px; min-height: 10px;"+
"	margin: 1px; border:1px solid #999; padding-left: 3px;	background: white; }"+
"#quick_reply textarea:focus { border:1px solid black; }"+
"#quick_reply small { color:#999; font: 10px Arial }"+
"#quick_reply input { width:350px;"+
"	margin: 1px; border:1px solid #999; padding-left: 3px;	background: white; }"+
"#quick_reply input:focus { border:1px solid black; }"

)
//-----------------------------------------------------------------------
get_user_from_cookie = /RTBB=name(\w+)|.*|seq=(\w+).*/
if (matches = get_user_from_cookie.exec(document.cookie)) 
	cookieuser = matches[1]
else
	cookieuser = false

//====================================
//	ROUTINES
//====================================

function findPosX(obj) {
	var curleft = 0;
	if (obj.offsetParent) {
		while (obj.offsetParent) {
			curleft += obj.offsetLeft
			obj = obj.offsetParent;
		}
	} 
	return curleft;
}

function findPosY(obj) {
	var curtop = 0;
	if (obj.offsetParent) {
		while (obj.offsetParent) {
			curtop += obj.offsetTop
			obj = obj.offsetParent;
		}
	} 
	return curtop;
}

function addGlobalStyle(css) {
    style = document.createElement('STYLE');
    style.type = 'text/css';
    style.innerHTML = css;
    document.body.appendChild(style);
}

function zakavych(text) {
	
	replacements = [
	

	
	];

	s = text
	for( i=0; i < replacements.length; i++) {
		s = s.replace(replacements[i][0], replacements[i][1])
	}
	
	return s
}

	var Letters=new Array('%C0','%C1','%C2','%C3','%C4','%C5','%C6','%C7','%C8','%C9','%CA','%CB','%CC','%CD','%CE','%CF',
					'%D0','%D1','%D2','%D3','%D4','%D5','%D6','%D7','%D8','%D9','%DA','%DB','%DC','%DD','%DE','%DF','%E0','%E1',
					'%E2','%E3','%E4','%E5','%E6','%E7','%E8','%E9','%EA','%EB','%EC','%ED','%EE','%EF','%F0','%F1','%F2','%F3',
					'%F4','%F5','%F6','%F7','%F8','%F9','%FA','%FB','%FC','%FD','%FE','%FF','%A8','%B8');


function EscapeToWin(AStr){

	var Result='';
	for(var i=0;i<AStr.length;i++)
		
		if(AStr.charCodeAt(i) >= 1040 && AStr.charCodeAt(i) <= 1103)
			Result+=Letters[AStr.charCodeAt(i)-0x0410];
		else if(AStr.charCodeAt(i)==1105)
			Result+=Letters[65];
		else if(AStr.charCodeAt(i)== 1025)
			Result+=Letters[64];
		else if(AStr.charAt(i)=='=')
			Result+='%3D';
		else if(AStr.charAt(i)=='&')
			Result+='%26';
		else if(AStr.charAt(i)=='\n')
			Result+='%0A%0D';
		else if(AStr.charAt(i)=='+')
			Result+='%2B';
		else if(AStr.charAt(i)=='%')
			Result+='%25';
		else
			Result+=AStr.charAt(i);
	return Result;
}//EscapeToWin

