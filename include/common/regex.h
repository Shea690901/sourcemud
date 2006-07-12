/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_COMMON_AWEREGEX_H
#define AWEMUD_COMMON_AWEREGEX_H 1

#include <sys/types.h>
#include <regex.h>

#include "common/gcbase.h"
#include "common/string.h"

class RegEx : public GCType::CleanupNonVirtual {
	public:
	RegEx (String pattern, bool nocase = false);
	~RegEx ();

	bool grep (String string);
	StringList match (String string);

	private:
	regex_t regex;
};

#endif // __AWEREGEX_H__
