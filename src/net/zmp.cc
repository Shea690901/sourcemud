/*
 * ZMP Example Implementation Library
 * Modified for Source MUD usage
 * http://www.sourcemud.org/zmp/
 */
 
/* Copyright (C) 2004	Sean Middleditch
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *	* Redistributions of source code must retain the above copyright notice,
 *		this list of conditions and the following disclaimer.
 *	* Redistributions in binary form must reproduce the above copyright
 *		notice, this list of conditions and the following disclaimer in the
 *		documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include <vector>

#include <stdlib.h>
#include <arpa/telnet.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>

#include "common/log.h"
#include "net/telnet.h"
#include "net/zmp.h"

SZMPManager ZMPManager;

// built-in handlers
namespace {
	void handle_zmp_ping (TelnetHandler* telnet, size_t argc, std::string argv[]);
	void handle_zmp_check (TelnetHandler* telnet, size_t argc, std::string argv[]);
	void handle_zmp_support (TelnetHandler* telnet, size_t argc, std::string argv[]);
	void handle_zmp_input (TelnetHandler* telnet, size_t argc, std::string argv[]);
}

// return 0 if not valid, or non-0 if valid
namespace {
	bool
	check_zmp_chunk(size_t size, const char* data)
	{
		// size must be at least two bytes
		if (size < 2)
			return false;

		// first byte must be printable ASCII
		if (!isprint(*data))
			return false;

		// last byte must be NUL
		if (data[size - 1] != '\0')
			return false;

		// good enough for us
		return true;
	}
}

// new zmp packed command
ZMPPack::ZMPPack (std::string command)
{
	add(command);
}

// add a string
ZMPPack&
ZMPPack::add (std::string command)
{
	args.push_back(command);

	return *this;
}

// add an 'int'
ZMPPack&
ZMPPack::add (long i)
{
	args.push_back(tostr(i));
	return *this;
}

// add a 'uint'
ZMPPack&
ZMPPack::add (ulong i)
{
	args.push_back(tostr(i));
	return *this;
}

SZMPManager::SZMPManager (void) : commands()
{
}

SZMPManager::~SZMPManager (void)
{
}

// initialize ZMP commands
int
SZMPManager::initialize (void)
{
	if (add(S("zmp.ping"), handle_zmp_ping))
		return -1;
	if (add(S("zmp.check"), handle_zmp_check))
		return -1;
	if (add(S("zmp.support"), handle_zmp_support))
		return -1;
	if (add(S("zmp.input"), handle_zmp_input))
		return -1;
	return 0;
}

// shutdown
void
SZMPManager::shutdown (void)
{
	commands.resize(0);
}

// register a new command
int
SZMPManager::add (std::string name, ZMPFunction func)
{
	// must have a name
	if (name.empty())
		return -1;

	// must have a func
	if (!func)
		return -1;

	// add command
	ZMPCommand command;
	command.name = name;
	command.function = func;
	command.wild = name[name.size()-1] == '.'; // ends in a . then its a wild-card match
	commands.push_back(command);

	return 0;
}

// find the request function; return NULL if not found
ZMPCommand*
SZMPManager::lookup(std::string name)
{
	// search list - easy enough
	for (ZMPCommandList::iterator i = commands.begin(); i != commands.end(); ++i) {
		if (i->wild && str_eq(i->name, name, i->name.size()))
			return &(*i);
		if (!i->wild && i->name == name)
			return &(*i);
	}
	// not found
	return NULL;
}

// match a package pattern; non-zero on match
bool
SZMPManager::match(std::string pattern)
{
	int package = 0; // are we looking for a package?

	// pattern must have a lengh
	if (pattern.empty())
		return false;

	// check if this is a package we're looking for
	if (pattern[pattern.size() - 1] == '.')
		package = 1; // yes, it is

	// search for match
	for (ZMPCommandList::iterator i = commands.begin(); i != commands.end(); ++i) {
		// package match?
		if (package && str_eq(i->name, pattern, pattern.size()))
			return true; // found match
		else if (!package && i->name == pattern)
			return true; // found match
		else if (i->wild && str_eq(i->name, pattern, i->name.size()))
			return true; // found match
	}

	// no match
	return false;
}

// handle an ZMP command - size is size of chunk, data is chunk
void
TelnetHandler::process_zmp(size_t size, char* data)
{
	const size_t argv_size = 20; // argv[] element size
	std::string argv[argv_size]; // arg list
	size_t argc; // number of args
	char* cptr; // for searching
	ZMPCommand* command;
	
	// check the data chunk is valid
	if (!check_zmp_chunk(size, data))
		return;
	
	// add command to argv
	argv[0] = std::string(data);
	argc = 1;

	// find the command
	command = ZMPManager.lookup(argv[0]);
	if (command == NULL) {
		// command not found
		return;
	}
	
	cptr = data; // init searching
	
	// parse loop - keep going as long as we have room in argv
	while (argc < argv_size) {
		// find NUL
		while (*cptr != '\0')
			++cptr;
	
		// is this NUL the last byte?
		if ((size_t)(cptr - data) == size - 1)
			break;
	
		// an argument follows
		++cptr; // move past the NUL byte
		argv[argc++] = std::string(cptr); // get argument
	}
		
	// invoke the proper command handler
	command->function(this, argc, argv);
}

// send an zmp command
void
TelnetHandler::send_zmp(size_t argc, const std::string argv[])
{
	// check for ZMP support
	if (!has_zmp())
		return;

	// must have at least one arg
	if (!argc)
		return;

	// telnet codes
	static char start_sb[3] = { IAC, SB, 93 }; // begin; 93 is ZMP code
	static char end_sb[2] = { IAC, SE }; // end request
	static char double_iac[2] = { IAC, IAC }; // for IAC escaping

	size_t i; // current argument index
	const char* start; // for handling argument chunks
	const char* cptr; // for searching

	// send request start
	add_output(start_sb, 3);
	
	// loop through argv[], which has argc elements
	for (i = 0; i < argc; ++i) {
		// to handle escaping, we will send this in chunks
	
		start = argv[i].c_str(); // string section we are working on now
	
		// loop finding any IAC bytes
		while ((cptr = strchr(start, IAC)) != NULL) {
			// send the bytes from start until cptr
			add_output(start, cptr - start);
			// send the double IAC bytes
			add_output(double_iac, 2);
			// the byte _following_ the IAC is the new start
			start = cptr + 1;
		}
	
		/* send the rest of the argument - send one extra byte past
			 the remainder length, so we get the NUL byte in the string,
			 which we need to send for the ZMP specification. */
		add_output(start, strlen(start) + 1);
	}

	// send request end
	add_output(end_sb, 2);
}

// add a zmp command (to insert mid-processing, basically for color - YUCJ)
void
TelnetHandler::add_zmp(size_t argc, std::string argv[])
{
	// check for ZMP support
	if (!has_zmp())
		return;

	// must have at least one arg
	if (!argc)
		return;

	// telnet codes
	static char start_sb[3] = { IAC, SB, 93 }; // begin; 93 is ZMP code
	static char end_sb[2] = { IAC, SE }; // end request
	static char double_iac[2] = { IAC, IAC }; // for IAC escaping

	size_t i; // current argument index
	const char* start; // for handling argument chunks
	const char* cptr; // for searching

	// send request start
	add_to_chunk(start_sb, 3);
	
	// loop through argv[], which has argc elements
	for (i = 0; i < argc; ++i) {
		// to handle escaping, we will send this in chunks
	
		start = argv[i].c_str(); // string section we are working on now
	
		// loop finding any IAC bytes
		while ((cptr = strchr(start, IAC)) != NULL) {
			// send the bytes from start until cptr
			add_to_chunk(start, cptr - start);
			// send the double IAC bytes
			add_to_chunk(double_iac, 2);
			// the byte _following_ the IAC is the new start
			start = cptr + 1;
		}
	
		/* send the rest of the argument - send one extra byte past
			 the remainder length, so we get the NUL byte in the string,
			 which we need to send for the ZMP specification. */
		add_to_chunk(start, strlen(start) + 1);
	}

	// send request end
	add_to_chunk(end_sb, 2);
}

// deal with ZMP support/no-support
void
TelnetHandler::zmp_support (std::string pkg, bool value)
{
	// color.define?
	if (str_eq(pkg, S("color.define"))) {
		io_flags.zmp_color = value;

		// init if true
		if (value) {
			std::string argv[4] = {S("color.define"), std::string(), std::string(), std::string()};
			for (int i = 1; i < NUM_CTYPES; ++i) {
				argv[1] = tostr(i);
				argv[2] = color_type_names[i];
				argv[3] = color_type_rgb[i];
				send_zmp(4, argv);
			}
		}
	}
}

// built-in handlers
namespace {
	// handle a zmp.ping command
	void
	handle_zmp_ping (TelnetHandler* telnet, size_t argc, std::string argv[])
	{
		// generate response
		char buffer[40];
		time_t t;
		time(&t);
		strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", gmtime(&t));
		buffer[sizeof(buffer) - 1] = 0;
		std::string response[2] = { S("zmp.time"), std::string(buffer) };
		telnet->send_zmp(2, response);
	}

	// handle a zmp.check command
	void
	handle_zmp_check (TelnetHandler* telnet, size_t argc, std::string argv[])
	{
		// valid args?
		if (argc != 2)
			return;

		// have we the argument?
		if (ZMPManager.match(argv[1])) {
			argv[0] = S("zmp.support");
			telnet->send_zmp(2, argv);
		// nope
		} else {
			argv[0] = S("zmp.no-support");
			telnet->send_zmp(2, argv);
		}
	}

	// handle a zmp.support command
	void
	handle_zmp_support (TelnetHandler* telnet, size_t argc, std::string argv[])
	{
		// valid args?
		if (argc != 2)
			return;

		// tell the user about it
		telnet->zmp_support(argv[1], true);
	}

	// handle a zmp.no-support command
	void
	handle_zmp_nosupport (TelnetHandler* telnet, size_t argc, std::string argv[])
	{
		// valid args?
		if (argc != 2)
			return;

		// tell the user about it
		telnet->zmp_support(argv[1], false);
	}

	// handle a zmp.input command
	void
	handle_zmp_input (TelnetHandler* telnet, size_t argc, std::string argv[])
	{
		// valid args
		if (argc != 2)
			return;

		// process input
		// FIXME: ugly hack!
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "%s", argv[1].c_str());
		telnet->process_command(buffer);
	}
}