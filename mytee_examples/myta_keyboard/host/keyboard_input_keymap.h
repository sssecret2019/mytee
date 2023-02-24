const char *ev_key_code_keymap[] = {
"KEY_RESERVED",
"KEY_ESC",
"KEY_1",
"KEY_2",
"KEY_3",
"KEY_4",
"KEY_5",
"KEY_6",
"KEY_7",
"KEY_8",
"KEY_9",
"KEY_0",
"KEY_MINUS",
"KEY_EQUAL",
"KEY_BACKSPACE",
"KEY_TAB",
"KEY_Q",
"KEY_W",
"KEY_E",
"KEY_R",
"KEY_T",
"KEY_Y",
"KEY_U",
"KEY_I",
"KEY_O",
"KEY_P",
"KEY_LEFTBRACE",
"KEY_RIGHTBRACE",
"KEY_ENTER",
"KEY_LEFTCTRL",
"KEY_A",
"KEY_S",
"KEY_D",
"KEY_F",
"KEY_G",
"KEY_H",
"KEY_J",
"KEY_K",
"KEY_L",
"KEY_SEMICOLON",
"KEY_APOSTROPHE",
"KEY_GRAVE",
"KEY_LEFTSHIFT",
"KEY_BACKSLASH",
"KEY_Z",
"KEY_X",
"KEY_C",
"KEY_V",
"KEY_B",
"KEY_N",
"KEY_M",
"KEY_COMMA",
"KEY_DOT",
"KEY_SLASH",
"KEY_RIGHTSHIFT",
"KEY_KPASTERISK",
"KEY_LEFTALT",
"KEY_SPACE",
"KEY_CAPSLOCK",
"KEY_F1",
"KEY_F2",
"KEY_F3",
"KEY_F4",
"KEY_F5",
"KEY_F6",
"KEY_F7",
"KEY_F8",
"KEY_F9",
"KEY_F10",
"KEY_NUMLOCK",
"KEY_SCROLLLOCK",
"KEY_KP7",
"KEY_KP8",
"KEY_KP9",
"KEY_KPMINUS",
"KEY_KP4",
"KEY_KP5",
"KEY_KP6",
"KEY_KPPLUS",
"KEY_KP1",
"KEY_KP2",
"KEY_KP3",
"KEY_KP0",
"KEY_KPDOT",
"NONE",
"KEY_ZENKAKUHANKAKU",
"KEY_102ND",
"KEY_F11",
"KEY_F12",
"KEY_RO",
"KEY_KATAKANA",
"KEY_HIRAGANA",
"KEY_HENKAN",
"KEY_KATAKANAHIRAGANA",
"KEY_MUHENKAN",
"KEY_KPJPCOMMA",
"KEY_KPENTER",
"KEY_RIGHTCTRL",
"KEY_KPSLASH",
"KEY_SYSRQ",
"KEY_RIGHTALT",
"KEY_LINEFEED",
"KEY_HOME",
"KEY_UP",
"KEY_PAGEUP",
"KEY_LEFT",
"KEY_RIGHT",
"KEY_END",
"KEY_DOWN",
"KEY_PAGEDOWN",
"KEY_INSERT",
"KEY_DELETE",
"KEY_MACRO",
"KEY_MUTE",
"KEY_VOLUMEDOWN",
"KEY_VOLUMEUP",
"KEY_POWER",
"KEY_KPEQUAL",
"KEY_KPPLUSMINUS",
"KEY_PAUSE",
"KEY_SCALE",
"KEY_KPCOMMA",
"KEY_HANGEUL",
"KEY_HANGUEL",
"KEY_HANJA",
"KEY_YEN",
"KEY_LEFTMETA",
"KEY_RIGHTMETA",
"KEY_COMPOSE",
"KEY_STOP",
"KEY_AGAIN",
"KEY_PROPS",
"KEY_UNDO",
"KEY_FRONT",
"KEY_COPY",
"KEY_OPEN",
"KEY_PASTE",
"KEY_FIND",
"KEY_CUT",
"KEY_HELP",
"KEY_MENU",
"KEY_CALC",
"KEY_SETUP",
"KEY_SLEEP",
"KEY_WAKEUP",
"KEY_FILE",
"KEY_SENDFILE",
"KEY_DELETEFILE",
"KEY_XFER",
"KEY_PROG1",
"KEY_PROG2",
"KEY_WWW",
"KEY_MSDOS",
"KEY_COFFEE",
"KEY_SCREENLOCK",
"KEY_ROTATE_DISPLAY",
"KEY_DIRECTION",
"KEY_CYCLEWINDOWS",
"KEY_MAIL",
"KEY_BOOKMARKS",
"KEY_COMPUTER",
"KEY_BACK",
"KEY_FORWARD",
"KEY_CLOSECD",
"KEY_EJECTCD",
"KEY_EJECTCLOSECD",
"KEY_NEXTSONG",
"KEY_PLAYPAUSE",
"KEY_PREVIOUSSONG",
"KEY_STOPCD",
"KEY_RECORD",
"KEY_REWIND",
"KEY_PHONE",
"KEY_ISO",
"KEY_CONFIG",
"KEY_HOMEPAGE",
"KEY_REFRESH",
"KEY_EXIT",
"KEY_MOVE",
"KEY_EDIT",
"KEY_SCROLLUP",
"KEY_SCROLLDOWN",
"KEY_KPLEFTPAREN",
"KEY_KPRIGHTPAREN",
"KEY_NEW",
"KEY_REDO",
"KEY_F13",
"KEY_F14",
"KEY_F15",
"KEY_F16",
"KEY_F17",
"KEY_F18",
"KEY_F19",
"KEY_F20",
"KEY_F21",
"KEY_F22",
"KEY_F23",
"KEY_F24",
"KEY_PLAYCD",
"KEY_PAUSECD",
"KEY_PROG3",
"KEY_PROG4",
"KEY_DASHBOARD",
"KEY_SUSPEND",
"KEY_CLOSE",
"KEY_PLAY",
"KEY_FASTFORWARD",
"KEY_BASSBOOST",
"KEY_PRINT",
"KEY_HP",
"KEY_CAMERA",
"KEY_SOUND",
"KEY_QUESTION",
"KEY_EMAIL",
"KEY_CHAT",
"KEY_SEARCH",
"KEY_CONNECT",
"KEY_FINANCE",
"KEY_SPORT",
"KEY_SHOP",
"KEY_ALTERASE",
"KEY_CANCEL",
"KEY_BRIGHTNESSDOWN",
"KEY_BRIGHTNESSUP",
"KEY_MEDIA",
"KEY_SWITCHVIDEOMODE",
"KEY_KBDILLUMTOGGLE",
"KEY_KBDILLUMDOWN",
"KEY_KBDILLUMUP",
"KEY_SEND",
"KEY_REPLY",
"KEY_FORWARDMAIL",
"KEY_SAVE",
"KEY_DOCUMENTS",
"KEY_BATTERY",
"KEY_BLUETOOTH",
"KEY_WLAN",
"KEY_UWB",
"KEY_UNKNOWN",
"KEY_VIDEO_NEXT",
"KEY_VIDEO_PREV",
"KEY_BRIGHTNESS_CYCLE",
"KEY_BRIGHTNESS_AUTO",
"KEY_BRIGHTNESS_ZERO",
"KEY_DISPLAY_OFF",
"KEY_WWAN",
"KEY_WIMAX",
"KEY_RFKILL",
"KEY_MICMUTE",
0};


#define KEY_RESERVED		0
#define KEY_ESC			1
#define KEY_1			2
#define KEY_2			3
#define KEY_3			4
#define KEY_4			5
#define KEY_5			6
#define KEY_6			7
#define KEY_7			8
#define KEY_8			9
#define KEY_9			10
#define KEY_0			11
#define KEY_MINUS		12
#define KEY_EQUAL		13
#define KEY_BACKSPACE		14
#define KEY_TAB			15
#define KEY_Q			16
#define KEY_W			17
#define KEY_E			18
#define KEY_R			19
#define KEY_T			20
#define KEY_Y			21
#define KEY_U			22
#define KEY_I			23
#define KEY_O			24
#define KEY_P			25
#define KEY_LEFTBRACE		26
#define KEY_RIGHTBRACE		27
#define KEY_ENTER		28
#define KEY_LEFTCTRL		29
#define KEY_A			30
#define KEY_S			31
#define KEY_D			32
#define KEY_F			33
#define KEY_G			34
#define KEY_H			35
#define KEY_J			36
#define KEY_K			37
#define KEY_L			38
#define KEY_SEMICOLON		39
#define KEY_APOSTROPHE		40
#define KEY_GRAVE		41
#define KEY_LEFTSHIFT		42
#define KEY_BACKSLASH		43
#define KEY_Z			44
#define KEY_X			45
#define KEY_C			46
#define KEY_V			47
#define KEY_B			48
#define KEY_N			49
#define KEY_M			50
#define KEY_COMMA		51
#define KEY_DOT			52
#define KEY_SLASH		53
#define KEY_RIGHTSHIFT		54
#define KEY_KPASTERISK		55
#define KEY_LEFTALT		56
#define KEY_SPACE		57
#define KEY_CAPSLOCK		58
#define KEY_F1			59
#define KEY_F2			60
#define KEY_F3			61
#define KEY_F4			62
#define KEY_F5			63
#define KEY_F6			64
#define KEY_F7			65
#define KEY_F8			66
#define KEY_F9			67
#define KEY_F10			68
#define KEY_NUMLOCK		69
#define KEY_SCROLLLOCK		70
#define KEY_KP7			71
#define KEY_KP8			72
#define KEY_KP9			73
#define KEY_KPMINUS		74
#define KEY_KP4			75
#define KEY_KP5			76
#define KEY_KP6			77
#define KEY_KPPLUS		78
#define KEY_KP1			79
#define KEY_KP2			80
#define KEY_KP3			81
#define KEY_KP0			82
#define KEY_KPDOT		83

#define KEY_ZENKAKUHANKAKU	85
#define KEY_102ND		86
#define KEY_F11			87
#define KEY_F12			88
#define KEY_RO			89
#define KEY_KATAKANA		90
#define KEY_HIRAGANA		91
#define KEY_HENKAN		92
#define KEY_KATAKANAHIRAGANA	93
#define KEY_MUHENKAN		94
#define KEY_KPJPCOMMA		95
#define KEY_KPENTER		96
#define KEY_RIGHTCTRL		97
#define KEY_KPSLASH		98
#define KEY_SYSRQ		99
#define KEY_RIGHTALT		100
#define KEY_LINEFEED		101
#define KEY_HOME		102
#define KEY_UP			103
#define KEY_PAGEUP		104
#define KEY_LEFT		105
#define KEY_RIGHT		106
#define KEY_END			107
#define KEY_DOWN		108
#define KEY_PAGEDOWN		109
#define KEY_INSERT		110
#define KEY_DELETE		111
#define KEY_MACRO		112
#define KEY_MUTE		113
#define KEY_VOLUMEDOWN		114
#define KEY_VOLUMEUP		115
#define KEY_POWER		116	/* SC System Power Down */
#define KEY_KPEQUAL		117
#define KEY_KPPLUSMINUS		118
#define KEY_PAUSE		119
#define KEY_SCALE		120	/* AL Compiz Scale (Expose) */

#define KEY_KPCOMMA		121
#define KEY_HANGEUL		122
#define KEY_HANGUEL		KEY_HANGEUL
#define KEY_HANJA		123
#define KEY_YEN			124
#define KEY_LEFTMETA		125
#define KEY_RIGHTMETA		126
#define KEY_COMPOSE		127

#define KEY_STOP		128	/* AC Stop */
#define KEY_AGAIN		129
#define KEY_PROPS		130	/* AC Properties */
#define KEY_UNDO		131	/* AC Undo */
#define KEY_FRONT		132
#define KEY_COPY		133	/* AC Copy */
#define KEY_OPEN		134	/* AC Open */
#define KEY_PASTE		135	/* AC Paste */
#define KEY_FIND		136	/* AC Search */
#define KEY_CUT			137	/* AC Cut */
#define KEY_HELP		138	/* AL Integrated Help Center */
#define KEY_MENU		139	/* Menu (show menu) */
#define KEY_CALC		140	/* AL Calculator */
#define KEY_SETUP		141
#define KEY_SLEEP		142	/* SC System Sleep */
#define KEY_WAKEUP		143	/* System Wake Up */
#define KEY_FILE		144	/* AL Local Machine Browser */
#define KEY_SENDFILE		145
#define KEY_DELETEFILE		146
#define KEY_XFER		147
#define KEY_PROG1		148
#define KEY_PROG2		149
#define KEY_WWW			150	/* AL Internet Browser */
#define KEY_MSDOS		151
#define KEY_COFFEE		152	/* AL Terminal Lock/Screensaver */
#define KEY_SCREENLOCK		KEY_COFFEE
#define KEY_ROTATE_DISPLAY	153	/* Display orientation for e.g. tablets */
#define KEY_DIRECTION		KEY_ROTATE_DISPLAY
#define KEY_CYCLEWINDOWS	154
#define KEY_MAIL		155
#define KEY_BOOKMARKS		156	/* AC Bookmarks */
#define KEY_COMPUTER		157
#define KEY_BACK		158	/* AC Back */
#define KEY_FORWARD		159	/* AC Forward */
#define KEY_CLOSECD		160
#define KEY_EJECTCD		161
#define KEY_EJECTCLOSECD	162
#define KEY_NEXTSONG		163
#define KEY_PLAYPAUSE		164
#define KEY_PREVIOUSSONG	165
#define KEY_STOPCD		166
#define KEY_RECORD		167
#define KEY_REWIND		168
#define KEY_PHONE		169	/* Media Select Telephone */
#define KEY_ISO			170
#define KEY_CONFIG		171	/* AL Consumer Control Configuration */
#define KEY_HOMEPAGE		172	/* AC Home */
#define KEY_REFRESH		173	/* AC Refresh */
#define KEY_EXIT		174	/* AC Exit */
#define KEY_MOVE		175
#define KEY_EDIT		176
#define KEY_SCROLLUP		177
#define KEY_SCROLLDOWN		178
#define KEY_KPLEFTPAREN		179
#define KEY_KPRIGHTPAREN	180
#define KEY_NEW			181	/* AC New */
#define KEY_REDO		182	/* AC Redo/Repeat */

#define KEY_F13			183
#define KEY_F14			184
#define KEY_F15			185
#define KEY_F16			186
#define KEY_F17			187
#define KEY_F18			188
#define KEY_F19			189
#define KEY_F20			190
#define KEY_F21			191
#define KEY_F22			192
#define KEY_F23			193
#define KEY_F24			194

#define KEY_PLAYCD		200
#define KEY_PAUSECD		201
#define KEY_PROG3		202
#define KEY_PROG4		203
#define KEY_DASHBOARD		204	/* AL Dashboard */
#define KEY_SUSPEND		205
#define KEY_CLOSE		206	/* AC Close */
#define KEY_PLAY		207
#define KEY_FASTFORWARD		208
#define KEY_BASSBOOST		209
#define KEY_PRINT		210	/* AC Print */
#define KEY_HP			211
#define KEY_CAMERA		212
#define KEY_SOUND		213
#define KEY_QUESTION		214
#define KEY_EMAIL		215
#define KEY_CHAT		216
#define KEY_SEARCH		217
#define KEY_CONNECT		218
#define KEY_FINANCE		219	/* AL Checkbook/Finance */
#define KEY_SPORT		220
#define KEY_SHOP		221
#define KEY_ALTERASE		222
#define KEY_CANCEL		223	/* AC Cancel */
#define KEY_BRIGHTNESSDOWN	224
#define KEY_BRIGHTNESSUP	225
#define KEY_MEDIA		226

#define KEY_SWITCHVIDEOMODE	227	/* Cycle between available video
					   outputs (Monitor/LCD/TV-out/etc) */
#define KEY_KBDILLUMTOGGLE	228
#define KEY_KBDILLUMDOWN	229
#define KEY_KBDILLUMUP		230

#define KEY_SEND		231	/* AC Send */
#define KEY_REPLY		232	/* AC Reply */
#define KEY_FORWARDMAIL		233	/* AC Forward Msg */
#define KEY_SAVE		234	/* AC Save */
#define KEY_DOCUMENTS		235

#define KEY_BATTERY		236

#define KEY_BLUETOOTH		237
#define KEY_WLAN		238
#define KEY_UWB			239

#define KEY_UNKNOWN		240

#define KEY_VIDEO_NEXT		241	/* drive next video source */
#define KEY_VIDEO_PREV		242	/* drive previous video source */
#define KEY_BRIGHTNESS_CYCLE	243	/* brightness up, after max is min */
#define KEY_BRIGHTNESS_AUTO	244	/* Set Auto Brightness: manual
					  brightness control is off,
					  rely on ambient */
#define KEY_BRIGHTNESS_ZERO	KEY_BRIGHTNESS_AUTO
#define KEY_DISPLAY_OFF		245	/* display device to off state */

#define KEY_WWAN		246	/* Wireless WAN (LTE, UMTS, GSM, etc.) */
#define KEY_WIMAX		KEY_WWAN
#define KEY_RFKILL		247	/* Key that controls all radios */

#define KEY_MICMUTE		248	/* Mute / unmute the microphone */

/* Code 255 is reserved for special needs of AT keyboard driver */
