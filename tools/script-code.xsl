<?xml version="1.0"?>
<xsl:stylesheet version="1.1" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="text"/>
  <xsl:template match="/script-interface">
    <!-- header -->
<xsl:text><![CDATA[
// Generated by script-code.xsl
// DO NOT EDIT

#line 10 "tools/script-code.xsl"

#include <fnmatch.h>
#include <dirent.h>

#include "common/gcbase.h"
#include "common/awestr.h"
#include "mud/bindings.h"
#include "mud/settings.h"
#include "mud/command.h"
#include "common/log.h"
#include "mud/fileobj.h"
#include "scriptix/number.h"
#include "scriptix/array.h"
#include "scriptix/system.h"
#include "scriptix/struct.h"
#include "scriptix/type.h"
#include "scriptix/vimpl.h"

#define SCRIPTIX_MAX_ARGV 32

using namespace Scriptix;
using namespace std;

// Global awescript
SScriptBindings ScriptBindings;

namespace {
	// find a list of files matching a particular pattern in a particular directory
	StringList
	find_files (StringArg path, StringArg pattern)
	{
		DIR *dir;
		dirent *dent;
		StringList files;
		
		// open dir
		dir = opendir (path);
		if (dir == NULL) {
			Log::Error << "Failed opening '" << path << "'";
			return files;
		}

		// read entries
		while ((dent = readdir (dir)) != NULL) {
			// if it matches, add to list
			if (!fnmatch (pattern.c_str(), dent->d_name, 0))
				files.push_back(path + "/" + dent->d_name);
		}

		// all done
		closedir(dir);
		return files;
	}

	void
	append_unique (StringList& base, const StringList& append)
	{
		for (StringList::const_iterator i = append.begin(); i != append.end(); ++i)
			if (std::find(base.begin(), base.end(), *i) == base.end())
				base.push_back(*i);
	}

	class InitHandler : public Scriptix::CompilerHandler {
		public:
		GCType::vector<Scriptix::Function*> init;

		virtual bool handle_function (Scriptix::Function* function, bool is_public) {
			if (strcmp(function->id.name(), "init") == 0) {
				init.push_back(function);
			}
			return true;
		}
	};

	class CatchHandler : public Scriptix::CompilerHandler {
		public:
		Scriptix::Function* function;

		CatchHandler () : function(NULL) {}

		virtual bool handle_function (Scriptix::Function* function, bool is_public) {
			CatchHandler::function = function;
			return true;
		}
	};
}

int
SScriptBindings::initialize()
{
	require(ScriptManager);

	// initialize interface
	bind();

	// load scripts
	StringList scripts;

	// read file listing scripts to load
	String path = SettingsManager.get_scripts_path();
	File::Reader reader;
	if (reader.open(path + "/load")) {
		Log::Error << "Failed to open '" << path << "/load'";
		return -1;
	}

	FO_READ_BEGIN
		FO_ATTR("script")
			scripts.push_back(path + "/" + node.get_data());
		FO_ATTR("pattern")
			append_unique(scripts, find_files(path, node.get_data()));
	FO_READ_ERROR
		return -1;
	FO_READ_END

	reader.close();

	// do loading
	InitHandler init;
	for (StringList::iterator i = scripts.begin(); i != scripts.end(); i ++) {
		if (ScriptManager.load_file(i->c_str(), &init) != SXE_OK) {
			Log::Error << "Failed to load script";
			return -1;
		}
	}

	// run all init functions
	for (GCType::vector<Function*>::iterator i = init.init.begin(); i != init.init.end(); ++i) {
		Scriptix::ScriptManager.invoke(*i, 0, NULL, NULL);
	}

	return 0;
}

void
SScriptBindings::shutdown()
{
}

// struct wrapper
//  wrap a class or struct that won't be made into its own manageable
//  type, like GameTime
#define SCRIPT_WRAP(TYPE,NAME) \
class NAME : public Scriptix::Native \
{ \
	private: \
	TYPE our_data; \
	public: \
	inline NAME (const Scriptix::TypeInfo* type) : Scriptix::Native(type), our_data() {} \
	inline NAME (const Scriptix::TypeInfo* type, const TYPE& s_data) : Scriptix::Native(type), our_data(s_data) {} \
	inline NAME (const TYPE& s_data) : Scriptix::Native(AweMUD_ ## NAME ## Type), our_data(s_data) {} \
	inline NAME& operator = (const TYPE& s_data) { our_data = s_data; return *this; } \
	inline TYPE& data() { return our_data; } \
	inline const TYPE& data () const { return our_data; } \
	protected: \
	inline bool equal (const Scriptix::Value& other) const \
		{ if (other.is_a(AweMUD_ ## NAME ## Type)) return our_data == ((NAME*)other.get())->our_data; \
		else return false; } \
};

namespace Scriptix { extern const TypeDef Struct_Type; }

// ------------- BEGIN GENERATED BINDINGS ---------------

]]></xsl:text>
    <xsl:value-of select="header"/>

    <!-- generate global static class type holders -->
    <xsl:for-each select="type">
      const Scriptix::TypeInfo* AweMUD_<xsl:value-of select="name"/>Type = NULL;
      <xsl:if test="cppwrap">
        namespace { SCRIPT_WRAP(<xsl:value-of select="cppwrap"/>, <xsl:value-of select="name"/>) }
      </xsl:if>
    </xsl:for-each>

    <!-- private namespace -->
    <xsl:text>namespace {</xsl:text>

    <!-- generate iterators -->
    <xsl:for-each select="function[not(code) or string-length(code)!=0]">
      <xsl:if test="string(return/type)='Iterator'">
        namespace _Iterators {
          class Func_<xsl:value-of select="name"/>_Iter : public Scriptix::Iterator{
            public:
            <xsl:value-of select="iterdata"/>

            protected:
            virtual bool next (Scriptix::Value&amp; _iterval) {
              <xsl:value-of select="iternext"/>
            }

            public:
            Func_<xsl:value-of select="name"/>_Iter () : Scriptix::Iterator() {
              <xsl:value-of select="iternew"/>
            }
            virtual ~Func_<xsl:value-of select="name"/>_Iter () {
              <xsl:value-of select="iterdel"/>
            }
          };
        }
      </xsl:if>
    </xsl:for-each>
    <xsl:for-each select="type[not(doconly)]">
      <xsl:if test="new">
        namespace _New {
          Scriptix::Value
          create_<xsl:value-of select="name"/>_Type (const Scriptix::TypeInfo* _type) {
            <xsl:value-of select="new"/>
          }
        }
      </xsl:if>
      <xsl:for-each select="method[not(code) or string-length(code)!=0]">
        <xsl:if test="string(return/type)='Iterator'">
          namespace _Iterators {
            class Class_<xsl:value-of select="../name"/>_Method_<xsl:value-of select="name"/>_Iter : public Scriptix::Iterator{
              public:
              <xsl:value-of select="iterdata"/>
  
              protected:
              virtual bool next (Scriptix::Value&amp; _iterval) {
                <xsl:value-of select="iternext"/>
              }

              public:
              Class_<xsl:value-of select="../name"/>_Method_<xsl:value-of select="name"/>_Iter () : Scriptix::Iterator() {
                <xsl:value-of select="iternew"/>
              }
              virtual ~Class_<xsl:value-of select="../name"/>_Method_<xsl:value-of select="name"/>_Iter () {
                <xsl:value-of select="iterdel"/>
              }
            };
          }
        </xsl:if>
      </xsl:for-each>
    </xsl:for-each>

    <!-- functions -->
    <xsl:for-each select="function[not(code) or string-length(code)!=0]">
    Scriptix::Value
      _inter_function_<xsl:value-of select="name"/> (size_t _argc, Scriptix::Value _argv[]) {
      <xsl:apply-templates select="arg" mode="define"/>
      <xsl:apply-templates select="." mode="define-ret"/>
      <xsl:variable name="arg-offset">0</xsl:variable>
      <xsl:call-template name="args-get"/>
      <xsl:if test="string(return/type)='Iterator'">
        typedef _Iterators::Func_<xsl:value-of select="name"/>_Iter _iterator;
      </xsl:if>
      <xsl:apply-templates select="." mode="invoke"/>
      <xsl:apply-templates select="." mode="do-ret"/>
      }
    </xsl:for-each>
    
    <!-- class attrs/methods -->
    <xsl:for-each select="type[not(doconly)]">
      <!-- methods -->
      <xsl:for-each select="method[not(code) or string-length(code)!=0]">
      Scriptix::Value
        _inter_method_<xsl:value-of select="../name"/>_<xsl:value-of select="name"/> (size_t _argc, Scriptix::Value _argv[]) {
        <xsl:apply-templates select=".." mode="cppname"/>* _self = (<xsl:apply-templates select=".." mode="cppname"/>*)_argv[0].get();
        <xsl:apply-templates select="arg" mode="define"/>
        <xsl:apply-templates select="." mode="define-ret"/>
        <xsl:call-template name="args-get">
          <xsl:with-param name="arg-offset">1</xsl:with-param>
        </xsl:call-template>
        <xsl:if test="string(return/type)='Iterator'">
          typedef _Iterators::Class_<xsl:value-of select="../name"/>_Method_<xsl:value-of select="name"/>_Iter _iterator;
        </xsl:if>
        <xsl:apply-templates select="." mode="invoke"/>
        <xsl:apply-templates select="." mode="do-ret"/>
        }
      </xsl:for-each>
    </xsl:for-each>

    <!-- end private namespace -->
    <xsl:text>}</xsl:text>

    <!-- Type defs -->
    <xsl:for-each select="type[not(doconly)]">
      SX_BEGINMETHODS(<xsl:value-of select="name"/>)
      <xsl:for-each select="method[not(code) or string-length(code)!=0]">
        SX_DEFMETHOD(_inter_method_<xsl:value-of select="../name"/>_<xsl:value-of select="name"/>, "<xsl:value-of select="name"/>", <xsl:value-of select="count(arg)"/>, 0)
      </xsl:for-each>
      SX_ENDMETHODS
      SX_TYPEIMPL(
        <xsl:value-of select="name"/>,
        "<xsl:value-of select="name"/>",
        <xsl:choose>
          <xsl:when test="parent"><xsl:value-of select="parent"/></xsl:when>
          <xsl:otherwise>Scriptix::Struct</xsl:otherwise>
        </xsl:choose>,
        <xsl:choose>
          <xsl:when test="new">_New::create_<xsl:value-of select="name"/>_Type</xsl:when>
          <xsl:otherwise>SX_TYPECREATENONE(<xsl:apply-templates select="." mode="cppname"/>)</xsl:otherwise>
        </xsl:choose>
      )
    </xsl:for-each>
      
    void
    SScriptBindings::bind() {
    <xsl:for-each select="function[not(code) or string-length(code)!=0]">
      Scriptix::ScriptManager.add_function(new Scriptix::Function(Scriptix::Atom("<xsl:value-of select="name"/>"), <xsl:value-of select="count(arg)"/>, _inter_function_<xsl:value-of select="name"/>));
    </xsl:for-each>

    <xsl:for-each select="type[not(doconly)]">
      AweMUD_<xsl:value-of select="name"/>Type = Scriptix::ScriptManager.add_type(&amp;<xsl:value-of select="name"/>_Type);
    </xsl:for-each>

    <xsl:for-each select="global-group/global">
      Scriptix::ScriptManager.add_global(Scriptix::Atom("<xsl:value-of select="name"/>"), <xsl:choose><xsl:when test="expr"><xsl:value-of select="expr"/></xsl:when><xsl:otherwise><xsl:value-of select="name"/></xsl:otherwise></xsl:choose>);
    </xsl:for-each>
}

    <!-- footer -->
    <xsl:value-of select="footer"/>

    <xsl:text>/* eof */
</xsl:text>
  </xsl:template>

  <!-- argument definition -->
  <xsl:template match="arg" mode="define">
    <xsl:choose>
      <xsl:when test="string(type)='Integer'">
        long _arg_<xsl:value-of select="name"/> = 0;
      </xsl:when>
      <xsl:when test="string(type)='Boolean'">
        long _arg_<xsl:value-of select="name"/> = 0;
      </xsl:when>
      <xsl:when test="string(type)='String'">
        String _arg_<xsl:value-of select="name"/>;
      </xsl:when>
      <xsl:when test="string(type)='Array'">
        Scriptix::Array* _arg_<xsl:value-of select="name"/>;
      </xsl:when>
      <xsl:when test="string(type)='Function'">
        Scriptix::Function* _arg_<xsl:value-of select="name"/> = NULL;
      </xsl:when>
      <xsl:when test="string(type)='Mixed'">
        Scriptix::Value _arg_<xsl:value-of select="name"/>;
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates select="." mode="cppname"/>* _arg_<xsl:value-of select="name"/> = NULL;
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- argument get-->
  <xsl:template name="args-get">
    <xsl:param name="arg-offset">0</xsl:param>
    <xsl:for-each select="arg">
      <xsl:if test="nullok">if (!_argv[<xsl:value-of select="position()-1"/> + <xsl:value-of select="$arg-offset" />].is_nil()) {</xsl:if>
      <xsl:choose>
        <xsl:when test="string(type)='Integer'">
          _arg_<xsl:value-of select="name"/> = Scriptix::Number::to_int(_argv[<xsl:value-of select="position()-1"/> + <xsl:value-of select="$arg-offset" />]);
        </xsl:when>
        <xsl:when test="string(type)='Boolean'">
          _arg_<xsl:value-of select="name"/> = Scriptix::Number::to_int(_argv[<xsl:value-of select="position()-1"/> + <xsl:value-of select="$arg-offset" />]);
        </xsl:when>
        <xsl:when test="string(type)='String'">
          if (!_argv[<xsl:value-of select="position()-1"/> + <xsl:value-of select="$arg-offset" />].is_string()) {
            Scriptix::ScriptManager.raise_error(Scriptix::SXE_BADTYPE, "Argument '<xsl:value-of select="name"/>' is not a string: %s", _argv[<xsl:value-of select="position()-1"/> + <xsl:value-of select="$arg-offset" />].get_type()->get_name().name().c_str());
            return Scriptix::Nil;
          } else {
            _arg_<xsl:value-of select="name"/> = _argv[<xsl:value-of select="position()-1"/> + <xsl:value-of select="$arg-offset" />].get_string();
          }
        </xsl:when>
        <xsl:when test="string(type)='Array'">
          if (!_argv[<xsl:value-of select="position()-1"/> + <xsl:value-of select="$arg-offset" />].is_array()) {
            Scriptix::ScriptManager.raise_error(Scriptix::SXE_BADTYPE, "Argument '<xsl:value-of select="name"/>' is not an array");
            return Scriptix::Nil;
          } else {
            _arg_<xsl:value-of select="name"/> = (Scriptix::Array*)_argv[<xsl:value-of select="position()-1"/> + <xsl:value-of select="$arg-offset" />].get();
          }
        </xsl:when>
        <xsl:when test="string(type)='Function'">
          if (!_argv[<xsl:value-of select="position()-1"/> + <xsl:value-of select="$arg-offset" />].is_function()) {
            Scriptix::ScriptManager.raise_error(Scriptix::SXE_BADTYPE, "Argument '<xsl:value-of select="name"/>' is not invocable");
            return Scriptix::Nil;
          } else {
            _arg_<xsl:value-of select="name"/> = (Scriptix::Function*)_argv[<xsl:value-of select="position()-1"/> + <xsl:value-of select="$arg-offset" />].get();
          }
        </xsl:when>
        <xsl:when test="string(type)='Mixed'">
          _arg_<xsl:value-of select="name"/> = _argv[<xsl:value-of select="position()-1"/> + <xsl:value-of select="$arg-offset" />].get();
        </xsl:when>
        <xsl:otherwise>
          if (!_argv[<xsl:value-of select="position()-1"/> + <xsl:value-of select="$arg-offset" />].is_a(AweMUD_<xsl:value-of select="type"/>Type)) {
            Scriptix::ScriptManager.raise_error(Scriptix::SXE_BADTYPE, "Argument '<xsl:value-of select="name"/>' is not a <xsl:value-of select="type"/>");
            return Scriptix::Nil;
          }
          _arg_<xsl:value-of select="name"/> = (<xsl:apply-templates select="." mode="cppname"/>*)_argv[<xsl:value-of select="position()-1"/> + <xsl:value-of select="$arg-offset" />].get();
        </xsl:otherwise>
      </xsl:choose>
      <xsl:if test="nullok">}</xsl:if>
    </xsl:for-each>
  </xsl:template>

  <!-- argument list -->
  <xsl:template match="arg" mode="list">
    <xsl:if test="position()-1!=0">, </xsl:if>_arg_<xsl:value-of select="name"/>
  </xsl:template>

  <!-- type cppname -->
  <xsl:template match="type" mode="cppname">
    <xsl:choose><xsl:when test="cppname"><xsl:value-of select="cppname"/></xsl:when><xsl:otherwise><xsl:value-of select="name"/></xsl:otherwise></xsl:choose>
  </xsl:template>
  <xsl:template match="function|method" mode="cppname">
    <xsl:variable name="type"><xsl:value-of select="return/type"/></xsl:variable>
    <xsl:choose><xsl:when test="//type[child::name=$type and cppname]"><xsl:value-of select="//type[child::name=$type]/cppname"/></xsl:when><xsl:otherwise><xsl:value-of select="$type"/></xsl:otherwise></xsl:choose>
  </xsl:template>
  <xsl:template match="arg" mode="cppname">
    <xsl:variable name="type"><xsl:value-of select="type"/></xsl:variable>
    <xsl:choose><xsl:when test="//type[child::name=$type and cppname]"><xsl:value-of select="//type[child::name=$type]/cppname"/></xsl:when><xsl:otherwise><xsl:value-of select="$type"/></xsl:otherwise></xsl:choose>
  </xsl:template>

  <!-- function return -->
  <xsl:template match="function|method" mode="define-ret">
    <xsl:choose>
      <xsl:when test="string(return/type)='Integer'">
        long _return = 0;
      </xsl:when>
      <xsl:when test="string(return/type)='Boolean'">
        bool _return = false;
      </xsl:when>
      <xsl:when test="string(return/type)='String'">
        String _return;
      </xsl:when>
      <xsl:when test="string(return/type)='Array'">
        Scriptix::Array* _return = NULL;
      </xsl:when>
      <xsl:when test="string(return/type)='Mixed'">
        Scriptix::Value _return;
      </xsl:when>
      <xsl:when test="string(return/type)='Function'">
        Scriptix::Function* _return;
      </xsl:when>
      <xsl:when test="string(return/type)='Iterator'">
        <xsl:choose>
          <xsl:when test="name()='function'">
            _Iterators::Func_<xsl:value-of select="name"/>_Iter* _return = NULL;
          </xsl:when>
          <xsl:when test="name()='method'">
            _Iterators::Class_<xsl:value-of select="../name"/>_Method_<xsl:value-of select="name"/>_Iter* _return = NULL;
          </xsl:when>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="return/type">
        <xsl:apply-templates select="." mode="cppname"/>* _return = NULL;
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <!-- do function return -->
  <xsl:template match="function|method" mode="do-ret">
    <xsl:choose>
      <xsl:when test="string(return/type)='Integer'">
        return Scriptix::Number::create(_return);
      </xsl:when>
      <xsl:when test="string(return/type)='Boolean'">
        if (_return)
          return Scriptix::Number::create(1);
      </xsl:when>
      <xsl:when test="string(return/type)='String'">
        if (_return)
          return Scriptix::Value(_return);
      </xsl:when>
      <xsl:when test="string(return/type)='Mixed'">
        return Scriptix::Value(_return);
      </xsl:when>
      <xsl:when test="return/type">
        if (_return)
          return Scriptix::Value(_return);
      </xsl:when>
    </xsl:choose>
    return Scriptix::Nil;
  </xsl:template>

  <!-- invoke function -->
  <xsl:template match="function" mode="invoke">
    <xsl:choose>
      <xsl:when test="code">
        <xsl:apply-templates select="code"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:if test="function-available('saxon:line-number')" xmlns:saxon="http://icl.com/saxon">
    <xsl:text>
    //#line </xsl:text><xsl:value-of select="saxon:line-number()"/> <xsl:text> "src/xml/script-intr.xml"
    </xsl:text>
        </xsl:if>
        <xsl:if test="return/type">_return = </xsl:if>
        <xsl:choose>
          <xsl:when test="invoke"><xsl:value-of select="invoke" /></xsl:when>
          <xsl:otherwise><xsl:value-of select="name" /></xsl:otherwise>
        </xsl:choose>
        (<xsl:apply-templates select="arg" mode="list"/>);
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- invoke method -->
  <xsl:template match="method" mode="invoke">
    <xsl:choose>
      <xsl:when test="code">
        <xsl:apply-templates select="code"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:if test="function-available('saxon:line-number')" xmlns:saxon="http://icl.com/saxon">
    <xsl:text>
    //#line </xsl:text><xsl:value-of select="saxon:line-number()"/> <xsl:text> "src/xml/script-intr.xml"
    </xsl:text>
        </xsl:if>
        <xsl:if test="return/type">_return = </xsl:if>_self->
        <xsl:choose>
          <xsl:when test="invoke"><xsl:value-of select="invoke" /></xsl:when>
          <xsl:otherwise><xsl:value-of select="name" /></xsl:otherwise>
        </xsl:choose>
        (<xsl:apply-templates select="arg" mode="list"/>);
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- implement 'code' code -->
  <xsl:template match="code">
    <xsl:if test="function-available('saxon:line-number')" xmlns:saxon="http://icl.com/saxon">
<xsl:text>
//#line </xsl:text><xsl:value-of select="saxon:line-number()"/> <xsl:text> "src/xml/script-intr.xml"
</xsl:text>
    </xsl:if>
    <xsl:value-of select="."/>
  </xsl:template>
</xsl:stylesheet>
<!-- vim: set filetype=xml tabstop=2 shiftwidth=2 expandtab: -->
