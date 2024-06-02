#!/bin/sh

# autoextraction script for Arkiv.sh agent
# © 2024 Amaury Bouchard <amaury@amaury.net>

# ansi()
# Write ANSI-compatible statements.
# @param	string	Command:
#			- reset: Remove all decoration.
#			- bold:  Write text in bold.
#			- dim:   Write dim text.
#			- rev:   Write text in reverse video. Could take another parameter with the background color.
#			- under: Write underlined text.
#			- black, red, green, yellow, blue, magenta, cyan, white: Change the text color.
ansi() {
	if [ "$TERM" = "" ] || [ "$OPT_NOANSI" = "1" ]; then
		return
	fi
	if ! type tput > /dev/null; then
		return
	fi
	case "$1" in
		"up")		tput cuu 1
		;;
		"reset")	tput sgr0
		;;
		"bold")		tput bold
		;;
		"dim")		tput dim
		;;
		"rev")
			case "$2" in
				"black")	tput setab 0
				;;
				"red")		tput setab 1
				;;
				"green")	tput setab 2
				;;
				"yellow")	tput setab 3
				;;
				"blue")		tput setab 4
				;;
				"magenta")	tput setab 5
				;;
				"cyan")		tput setab 6
				;;
				"white")	tput setab 7
				;;
				*)		tput rev
			esac
		;;
		"under")	tput smul
		;;
		"black")	tput setaf 0
		;;
		"red")		tput setaf 1
		;;
		"green")	tput setaf 2
		;;
		"yellow")	tput setaf 3
		;;
		"blue")		tput setaf 4
		;;
		"magenta")	tput setaf 5
		;;
		"cyan")		tput setaf 6
		;;
		"white")	tput setaf 7
		;;
	esac
}

echo
echo "$(ansi rev blue)                                                 $(ansi reset)"
echo "$(ansi rev blue)               Arkiv agent install               $(ansi reset)"
echo "$(ansi rev blue)                                                 $(ansi reset)"
echo
echo

# check user
USER="$(id -un)"
if [ "$USER" != "root" ]; then
	echo "Arkiv agent installation must be done by the $(ansi yellow)root$(ansi reset) user."
	echo "$(ansi red)Abort$(ansi reset)"
	exit 1
fi

if [ -d "/opt/arkiv" ]; then
	echo "The directory $(ansi dim)/opt/arkiv$(ansi reset) already exists."
	read -p "Update it? [y/N] " ANSWER
	if [ "$ANSWER" != "y" ] && [ "$ANSWER" != "Y" ]; then
		echo "$(ansi red)Abort$(ansi reset)"
		exit 0
	fi
else
	echo "Arkiv agent will be installed in $(ansi dim)/opt/arkiv$(ansi reset)"
	read -p "Press ENTER when ready" ANSWER
	echo -n "$(ansi up)Create directories... "
	mkdir /opt/arkiv && mkdir /opt/arkiv/bin && mkdir /opt/arkiv/etc
	if [ $? -ne 0 ]; then
		echo "$(ansi red)Failed$(ansi reset)"
		echo "$(ansi red)Abort$(ansi reset)"
		exit 2
	fi
	echo "$(ansi green)Done$(ansi reset)"
fi
echo -n "Copy executables... "
cp ./arkiv_agent /opt/arkiv/bin/ && cp ./rclone /opt/arkiv/bin/
if [ $? -ne 0 ]; then
	echo "$(ansi red)Failed$(ansi reset)"
	echo "$(ansi red)Abort$(ansi reset)"
	exit 4
fi
echo "$(ansi green)Done$(ansi reset)"
echo
echo "The agent configuration will start now. Press CTRL-C to abort."
echo "You can launch it whenever you want with this command: $(ansi dim)/opt/arkiv/bin/arkiv_agent config$(ansi reset)"
echo
read -p "Press ENTER when ready" ANSWER
echo -n "$(ansi up)$(ansi up)"

/opt/arkiv/bin/arkiv_agent config

