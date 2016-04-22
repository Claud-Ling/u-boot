#!/bin/bash


ECHO_RED() {
	echo -e -n "\033[31m${1}\033[0m"
}
ECHO_GREEN() {
	echo -e -n "\033[32m${1}\033[0m"
}
ECHO_YELLOW() {
	echo -e -n "\033[33m${1}\033[0m"
}
ECHO_BLUE() {
	echo -e -n "\033[34m${1}\033[0m"
}


#
# check to see if we have codesourcery cross compiler (arm-none-linux-gnueabi-)
#
cross_tool=0
arm-none-linux-gnueabi-gcc -v 2> /dev/null

if [ $? -ne 0 ]; then
	arm-linux-gnueabihf-gcc -v 2> /dev/null
	if [ $? -ne 0 ]; then
		ECHO_RED "Please add the path of a cross compiler to your enviroment PATH\n"
		exit $cross_tool
	else
		ECHO_GREEN "Linaro toolchain is detected, do you want to use this? (Y/N)\n"
		read linaro
		if [ "$linaro" == "y" ] || [ "$linaro" == "Y" ]; then
			cross_tool=2
		else
			ECHO_RED "Please add the path of a cross compiler to PATH.\n"
			exit $cross_tool
		fi
	fi
else
	ECHO_GREEN "CodeSourcery toolchain is detected, do you want to use this? (Y/N)\n"
	read codesourcery
	if [ "$codesourcery" == "y" ] || [ "$codesourcery" == "Y" ]; then
		cross_tool=1
	else
		# let's try out linaro
		arm-linux-gnueabihf-gcc -v 2> /dev/null
		if [ $? -ne 0 ]; then
			ECHO_RED "Please add the path of a cross compiler to your enviroment PATH\n"
			exit $cross_tool
		else
			ECHO_GREEN "Linaro toolchain is detected, do you want to use this? (Y/N)\n"
			read linaro
			if [ "$linaro" == "y" ] || [ "$linaro" == "Y" ]; then
				cross_tool=2
			else
				ECHO_RED "Please add the path of a cross compiler to PATH.\n"
				exit $cross_tool
			fi
		fi
	fi
fi


#
# check CPU key package
#
required_envvars=(
	SMP8XXX_OEM_NAME
	SMP8XXX_KEY_DOMAIN
	SMP8XXX_CHIP_REV
	SMP8XXX_STAGE1_CERTID
	SMP8XXX_CPU_CERTID
	SMP8XXX_XXENV_CERTID
)

for var in ${required_envvars[@]}; do
	eval val=\$$var
	if [ -z "$val" ]; then
		ECHO_YELLOW "!!! WARNING !!! CPU key package not found\n"
        ECHO_YELLOW "You cannot build a xload image without CPU key package\n"
		ECHO_YELLOW "Please run \"source [CPU kEY PKG ROOT]/CPU_KEYS_xload3.env\"\n"
		exit $cross_tool
	fi
done

exit $cross_tool
