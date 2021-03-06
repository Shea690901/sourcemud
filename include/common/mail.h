/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef __SOURCEMUD_MAIL_H__
#define __SOURCEMUD_MAIL_H__

#include "common/strbuf.h"

#ifdef HAVE_SENDMAIL

class MailMessage
{
private:
	std::string to;
	std::string subject;
	StringBuffer body;
	struct Header {
		std::string name;
		std::string value;
	};
	std::vector<Header> headers;

public:
	MailMessage(const std::string& s_to, const std::string& s_subj, const std::string& s_body) :
			to(s_to), subject(s_subj), body(), headers() { body << s_body; }
	MailMessage(const std::string& s_to, const std::string& s_subj) :
			to(s_to), subject(s_subj), body(), headers() {}

	void append(const std::string& data);

	void header(const std::string& name, const std::string& value);

	int send() const;
};

#endif // HAVE_SENDMAIL

#endif
