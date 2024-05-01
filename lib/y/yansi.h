/**
 * @header	yansi.h
 * @abstract	ANSI display.
 * @author	Amaury Bouchard <amaury@amaury.net>
 */
#pragma once

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif /* __cplusplus || c_plusplus */

/** @define YANSI_CLEAR Sequence to clear the terminal. */
#define YANSI_CLEAR	"\033[H\033[J"
/** @define YANSI_RESET End an ANSI sequence. */
#define YANSI_RESET	"\x1b[0m"

/** @define YANSI_BOLD Start a bold string. */
#define YANSI_BOLD	"\x1b[1m"
/** @define YANSI_END_BOLD End a bold string. */
#define YANSI_END_BOLD	"\x1b[22m"

/** @define YANSI_FAINT Start a faint text. */
#define YANSI_FAINT	"\x1b[2m"
/** @define YANSI_END_FAINT End a bold string. */
#define YANSI_END_FAINT	"\x1b[22m"

/** @define YANSI_ITALIC Start an italic text. */
#define YANSI_ITALIC		"\x1b[3m"
/** @define YANSI_END_ITALIC End an italic string. */
#define YANSI_END_ITALIC	"\x1b[23m"

/** @define YANSI_UNDERLINE Start an underlined text. */
#define YANSI_UNDERLINE		"\x1b[4m"
/** @define YANSI_END_UNDERLINE End an italic string. */
#define YANSI_END_UNDERLINE	"\x1b[24m"

/** @define YANSI_BLINK Start a blinking text. */
#define YANSI_BLINK	"\x1b[5m"
/** @define YANSI_END_BLINK End a blinking text. */
#define YANSI_END_BLINK	"\x1b[25m"

/** @define YANSI_NEGATIVE Start an inversed video text. */
#define YANSI_NEGATIVE		"\x1b[7m"
/* @define YANSI_END_NEGATIVE End an inversed video text. */
#define YANSI_END_NEGATIVE	"\x1b[27m"

/** @define YANSI_STRIKEOUT Start a striked out text. */
#define YANSI_STRIKEOUT		"\x1b[9m"
/** @define YANSI_END_STRIKEOUT End a striked out text. */
#define YANSI_END_STRIKEOUT	"\x1b[29m"

/** @define YANSI_END_COLOR End a color definition. */
#define YANSI_END_COLOR		"\x1b[39m"
/** @define YANSI_LIGHT_BLACK . */
#define YANSI_LIGHT_BLACK	"\x1b[38;5;0m"
/** @define YANSI_RED . */
#define YANSI_RED		"\x1b[38;5;1m"
/** @define YANSI_GREEN . */
#define YANSI_GREEN		"\x1b[38;5;2m"
/** @define YANSI_YELLOW . */
#define YANSI_YELLOW		"\x1b[38;5;3m"
/** @define YANSI_BLUE . */
#define YANSI_BLUE		"\x1b[38;5;4m"
/** @define YANSI_PURPLE . */
#define YANSI_PURPLE		"\x1b[38;5;5m"
/** @define YANSI_TEAL . */
#define YANSI_TEAL		"\x1b[38;5;6m"
/** @define YANSI_SILVER . */
#define YANSI_SILVER		"\x1b[38;5;7m"
/** @define YANSI_GRAY . */
#define YANSI_GRAY		"\x1b[38;5;8m"
/** @define YANSI_GREY . */
#define YANSI_GREY		"\x1b[38;5;8m"
/** @define YANSI_LIGHT_RED . */
#define YANSI_LIGHT_RED		"\x1b[38;5;9m"
/** @define YANSI_LIME . */
#define YANSI_LIME		"\x1b[38;5;10m"
/** @define YANSI_LIGHT_YELLOW . */
#define YANSI_LIGHT_YELLOW	"\x1b[38;5;11m"
/** @define YANSI_GOLD . */
#define YANSI_GOLD		"\x1b[38;5;11m"
/** @define YANSI_LIGHT_BLUE . */
#define YANSI_LIGHT_BLUE	"\x1b[38;5;12m"
/** @define YANSI_MAGENTA . */
#define YANSI_MAGENTA		"\x1b[38;5;13m"
/** @define YANSI_CYAN . */
#define YANSI_CYAN		"\x1b[38;5;14m"
/** @define YANSI_WHITE . */
#define YANSI_WHITE		"\x1b[38;5;15m"
/** @define YANSI_BLACK . */
#define YANSI_BLACK		"\x1b[38;5;16m"

/** @define YANSI_END_BGCOLOR End a background color definition. */
#define YANSI_END_BGCOLOR	"\x1b[49m"
/** @define YANSI_BG_LIGHT_BLACK . */
#define YANSI_BG_LIGHT_BLACK	"\x1b[48;5;0m"
/** @define YANSI_BG_RED . */
#define YANSI_BG_RED		"\x1b[48;5;1m"
/** @define YANSI_BG_GREEN . */
#define YANSI_BG_GREEN		"\x1b[48;5;2m"
/** @define YANSI_BG_YELLOW . */
#define YANSI_BG_YELLOW		"\x1b[48;5;3m"
/** @define YANSI_BG_BLUE . */
#define YANSI_BG_BLUE		"\x1b[48;5;4m"
/** @define YANSI_BG_PURPLE . */
#define YANSI_BG_PURPLE		"\x1b[48;5;5m"
/** @define YANSI_BG_TEAL . */
#define YANSI_BG_TEAL		"\x1b[48;5;6m"
/** @define YANSI_BG_SILVER . */
#define YANSI_BG_SILVER		"\x1b[48;5;7m"
/** @define YANSI_BG_GRAY . */
#define YANSI_BG_GRAY		"\x1b[48;5;8m"
/** @define YANSI_BG_GREY . */
#define YANSI_BG_GREY		"\x1b[48;5;8m"
/** @define YANSI_BG_LIGHT_RED . */
#define YANSI_BG_LIGHT_RED	"\x1b[48;5;9m"
/** @define YANSI_BG_LIME . */
#define YANSI_BG_LIME		"\x1b[48;5;10m"
/** @define YANSI_BG_LIGHT_YELLOW . */
#define YANSI_BG_LIGHT_YELLOW	"\x1b[48;5;11m"
/** @define YANSI_BG_GOLD . */
#define YANSI_BG_GOLD		"\x1b[48;5;11m"
/** @define YANSI_BG_LIGHT_BLUE . */
#define YANSI_BG_LIGHT_BLUE	"\x1b[48;5;12m"
/** @define YANSI_BG_MAGENTA . */
#define YANSI_BG_MAGENTA	"\x1b[48;5;13m"
/** @define YANSI_BG_CYAN . */
#define YANSI_BG_CYAN		"\x1b[48;5;14m"
/** @define YANSI_BG_WHITE . */
#define YANSI_BG_WHITE		"\x1b[48;5;15m"
/** @define YANSI_BG_BLACK . */
#define YANSI_BG_BLACK		"\x1b[48;5;16m"

/**
 * @define YANSI_LINK	Use to create a link. Must be used as the first parameter
 *			of printf() or sprintf() function.
 * @param	string	url	URL pointed by the link.
 * @param	string	title	Title of the link.
 */
#define YANSI_LINK		"\x1b]8;;%s\x1b\\%s\x1b]8;;\x1b\\"
/**
 * @define YANSI_LINK_STATIC	Use to create a link. Must be used with static strings.
 * @param	string	url	URL pointed by the link.
 * @param	string	title	Title of the link.
 */
#define YANSI_LINK_STATIC(url, title)	"\x1b]8;;" url "\x1b\\" title "\x1b]8;;\x1b\\"

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif /* __cplusplus || c_plusplus */

