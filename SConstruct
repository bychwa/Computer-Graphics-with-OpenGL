# SConstruct - Build the labs under Linux
#

# Note: uses a modified version of scons. The plain scons from www.scons.org
# will not work.
#

from SCript.Environment import *;

import SCript.Stages;
import SCript.Beautify;
import SCript.Variants;

import SCript.Defaults.OSEnv;
import SCript.Defaults.Variants;

import SCript.Defaults.Tools.CXX;
import SCript.Defaults.Tools.Link;

# Default environment
env = env_create( "base" );

env_srcdir_set( env, "." );
env_blddir_set( env, "build" );

# Fixup for DevIL --  we need to define ILUT_USE_OPENGL
# (DevIL sucks somewhat. Defining this in code causes problems in Windows,
# not defining it causes problems in Linux).
env.AppendUnique( CPPDEFINES = ["ILUT_USE_OPENGL=1"] );

# Default configuration
from SCript.Stages import config;
from SCript.Config.Common.OpenGL import *;

from SCript.Config.Messages import conf_fatal;
from SCript.Config.Link import check_symbol_in_lib;
from SCript.Config.CXX import check_cxx_header, check_cxx_flag;

	# small helper...
from os.path import abspath;
def ensure_real_path(path): 
	if path: return abspath(str(env.Dir(path)));
	return None;

@config( env )
def configure_opengl( env, conf ):
	# Check for OpenGL and GLU
	# It's expected that these are provided by your system.
	configure_gl_opengl( env, conf, mandatory = 1 );
	configure_gl_glu( env, conf, mandatory = 1 );

	# Check for GLUT
	# Generally, your system should provide GLUT (possible through freeglut),
	# but check for local versions anyway
	gotGLUT = configure_gl_glut( env, conf );

	if not gotGLUT:
		gotGLUT = configure_gl_glut( env, conf, 
			cpppath = ensure_real_path("#linux/include"),
			libpath = ensure_real_path("#linux/lib64/glut"),
			forcepaths = True,
			withRPATH = True,
		);
		pass;

	if not gotGLUT:
		gotGLUT = configure_gl_glut( env, conf, 
			cpppath = ensure_real_path("#linux/include"),
			libpath = ensure_real_path("#linux/lib32/glut"),
			forcepaths = True,
			withRPATH = True,
		);
		pass;

	if not gotGLUT:
		gotGLUT = configure_gl_glut( env, conf, 
			cpppath = ensure_real_path("#inc"),
			libpath = ensure_real_path("#lib"),
			forcepaths = True,
			withRPATH = True,
		);
		pass;

	# Check for GLEW
	# Try those provided by the system first, otherwise try the ones
	# in linux/lib64, those in linux/lib32 and finally those in lib/
	gotGLEW = configure_gl_glew( env, conf );

	if not gotGLEW:
		gotGLEW = configure_gl_glew( env, conf, 
			cpppath = ensure_real_path("#linux/include"),
			libpath = ensure_real_path("#linux/lib64/glew"),
			forcepaths = True,
			withRPATH = True,
		);
		pass;

	if not gotGLEW:
		gotGLEW = configure_gl_glew( env, conf, 
			cpppath = ensure_real_path("#linux/include"),
			libpath = ensure_real_path("#linux/lib32/glew"),
			forcepaths = True,
			withRPATH = True,
		);
		pass;

	if not gotGLEW:
		gotGLEW = configure_gl_glew( env, conf, 
			cpppath = ensure_real_path("#inc"),
			libpath = ensure_real_path("#lib"),
			withRPATH = True,
			forcepaths = True,
		);
		pass;

	return;

@config( env )
def configure_devil( env, conf ):
	libWhere = [None, "#linux/lib64/devil", "#linux/lib32/devil", "lib"];
	incWhere = [None, "#linux/include", "#linux/include", "inc"];

	# check for IL, ILU, and ILUT
	gotIL = False;
	gotILU = False;
	gotILUT = False;

	for i in range(0, len(libWhere)):
		lib = ensure_real_path(libWhere[i]);
		inc = ensure_real_path(incWhere[i]);

		withRPATH = True;
		if not lib: withRPATH = False;

		if not gotIL and check_cxx_header( env, conf, "IL/il.h", cpppath = inc ):
			gotIL = check_symbol_in_lib( env, conf, "ilInit", "IL", 
				header = "IL/il.h",
				withRPATH = withRPATH,
				libpath = lib
			);
			pass;

		if not gotILU and check_cxx_header( env, conf, "IL/ilu.h", cpppath = inc ):
			gotILU = check_symbol_in_lib( env, conf, "iluInit", "ILU", 
				header = "IL/ilu.h",
				withRPATH = withRPATH,
				libpath = lib
			);
			pass;

		if not gotILUT and check_cxx_header( env, conf, "IL/ilut.h", cpppath = inc ):
			gotILUT = check_symbol_in_lib( env, conf, "ilutInit", "ILUT", 
				header = "IL/ilut.h",
				withRPATH = withRPATH,
				libpath = lib
			);
			pass;

		pass;

	# Did we get everything?
	if not gotIL: conf_fatal( "Missing DevIL IL library" );
	if not gotILU: conf_fatal( "Missing DevIL ILU library" );
	if not gotILUT: conf_fatal( "Missing DevIL ILUT library" );

	return;

@config( env )
def setup_paths( env, conf ):
	# linmath => vector and matrix classes
	check_cxx_header( env, conf, "float2.h", 
		cpppath = ensure_real_path("#linmath"),
		mandatory = 1 );

	# glutil => local GL utilities
	check_cxx_header( env, conf, "glutil.h", 
		cpppath = ensure_real_path("#glutil"),
		mandatory = 1 );
	return;

@config( env )
def linmath_cxx_fixes( env, conf ):
	# Avoid the "warning: type qualifiers ignored on function return type"
	# warnin-spam.
	check_cxx_flag( env, conf, "-Wno-ignored-qualifiers" );
	return;

@config( env )
def ilut_cxx_fixes( env, conf ):
	# Avoid the "warning: deprecated conversion from string constant to 'char*'"
	# Yes, it's illegal to cast a string literal to a non-const char*, but it
	# seems unlikely that ILUT is going to fix itself in any foreseeable future.
	check_cxx_flag( env, conf, "-Wno-write-strings" );

	# Avoid warnings about undefined macro symbols.
	check_cxx_flag( env, conf, "-Wno-undef" );

	return;

# Add subprojects
env_directory_add( env, "glutil", exportResultAs = "libGLUTIL" );
env_directory_add( env, "linmath", exportResultAs = "libLinmath" );

env_directory_add( env, "lab1" );
env_directory_add( env, "lab2-textures" );
env_directory_add( env, "lab3-camera" );
env_directory_add( env, "lab4-cubemapping" );
env_directory_add( env, "lab5-rendertotexture" );
env_directory_add( env, "lab6-shadowmaps" );
env_directory_add( env, "project" );

env_directory_add( env, "scenes" );

# EOF vim:syntax=python:foldmethod=marker:ts=4:noexpandtab
