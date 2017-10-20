resume_check()
{
	[ "${BTC_RESUME}" != "yesplease" ] && return 1
	__RESUME_FILE="$DIR_LOG/.resume.$(echo $1 | tr '[/]' '_')"
	[ -e "${__RESUME_FILE}" ]
}

resume_save()
{
	[ -n "${__RESUME_FILE}" ] && touch "${__RESUME_FILE}"
}

resume_clear()
{
	rm -f "$DIR_LOG"/.resume.*
}
