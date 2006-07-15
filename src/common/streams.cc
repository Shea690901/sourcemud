/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "common/streams.h"
#include "mud/parse.h"
#include "mud/room.h"

StreamParse::StreamParse(String s_text)
	: text(s_text), argv(), names()
{}

StreamParse::StreamParse(String s_text, String s_name, ParseValue s_value)
	: text(s_text), argv(1), names(1)
{
	names[0] = s_name;
	argv[0] = s_value;
}

StreamParse::StreamParse(String s_text, String s_name1, ParseValue s_value1, String s_name2, ParseValue s_value2)
	: text(s_text), argv(2), names(2)
{
	names[0] = s_name1;
	argv[0] = s_value1;
	names[1] = s_name2;
	argv[1] = s_value2;
}

StreamParse::StreamParse(String s_text, String s_name1, ParseValue s_value1, String s_name2, ParseValue s_value2, String s_name3, ParseValue s_value3)
	: text(s_text), argv(3), names(3)
{
	names[0] = s_name1;
	argv[0] = s_value1;
	names[1] = s_name2;
	argv[1] = s_value2;
	names[2] = s_name3;
	argv[2] = s_value3;
}

StreamParse&
StreamParse::add (String s_name, ParseValue s_value)
{
	names.push_back(s_name);
	argv.push_back(s_value);
	return *this;
}

StreamParse&
StreamParse::add (ParseValue s_value)
{
	names.push_back(String());
	argv.push_back(s_value);
	return *this;
}