/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef __AWEMUD_MAIL_H__
#define __AWEMUD_MAIL_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SENDMAIL

#include <vector>
#include "common/string.h"

class MailMessage : public GC {
	private:
	String to;
	String subject;
	String body;
	struct Header {
		String name;
		String value;
	};
	GCType::vector<Header> headers;

	public:
	MailMessage (String s_to, String s_subj, String s_body) :
		to(s_to), subject(s_subj), body(s_body), headers() {}
	MailMessage (String s_to, String s_subj) :
		to(s_to), subject(s_subj), body(), headers() {}

	void append (String data);

	void header (String name, String value);

	int send (void) const;
};

#endif // HAVE_SENDMAIL

#endif
