/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef AWEMUD_COMMON_MANIFEST_H
#define AWEMUD_COMMON_MANIFEST_H

#include "common/gcvector.h"
#include "common/string.h"

class ManifestFile
{
	public:
	ManifestFile (String s_path, String s_ext) : path(s_path), ext(s_ext) {}

	StringList get_files () const;
	bool has_file (String file) const;
	int remove_file (String file);
	int add_file (String file);

	private:
	String path;
	String ext;
};

#endif