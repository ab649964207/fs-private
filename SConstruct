
import fnmatch
import os
from util.build import *

# read variables from the cache, a user's custom.py file or command line arguments
vars = Variables(['variables.cache', 'custom.py'], ARGUMENTS)
vars.Add(BoolVariable('debug', 'Debug build', 'no'))
vars.Add(BoolVariable('edebug', 'Extreme debug', 'no'))
vars.Add(BoolVariable('use_ompl', 'Include OMPL driver support in library', 'yes'))
vars.Add(BoolVariable('use_gecode', 'Include Gecode solver dependencies', 'yes'))
vars.Add(BoolVariable('use_soplex', 'Include Soplex LP solver adapter and dependencies', 'yes'))
vars.Add(EnumVariable('default_compiler', 'Preferred compiler', 'clang++', allowed_values=('g++', 'clang++')))
# The LAPKT path can be optionally specified, otherwise we fetch it from the corresponding environment variable.
vars.Add(PathVariable('lapkt', 'Path where the LAPKT library is installed', os.getenv('LAPKT', ''), PathVariable.PathIsDir))

env = Environment(variables=vars, ENV=os.environ)
env['CXX'] = os.environ.get('CXX', env['default_compiler'])

if env['edebug']:
	build_dirname = '.build/edebug'
elif env['debug']:
	build_dirname = '.build/debug'
else:
	build_dirname = '.build/prod'
env.VariantDir(build_dirname, '.')

Help(vars.GenerateHelpText(env))
vars.Save('variables.cache', env)

env.Append(CCFLAGS = ['-Wall', '-pedantic', '-std=c++14' ])  # Flags common to all options

# Extreme debug implies normal debug as well
if env['debug'] or env['edebug']:
	env.Append(CCFLAGS = ['-g', '-DDEBUG' ])
	lib_name = 'fs-debug'
else:
	env.Append(CCFLAGS = ['-O3', '-DNDEBUG' ])
	lib_name = 'fs'

# Additionally, extreme debug implies a different name plus extra compilation flags
if env['edebug']:
	env.Append(CCFLAGS = ['-DEDEBUG'])
	lib_name = 'fs-edebug'


# Base include directories
include_paths = ['src', os.path.join(env['lapkt'], 'include')]
isystem_paths = []

# Gecode tweaks
isystem_paths += ['/usr/local/include'] # MRJ: This probably should be acquired from an environment variable

# include local by default
isystem_paths += [os.environ['HOME'] + '/local/include']

# Process modules and external dependencies
sources = []
Export('env', 'sources')
SConscript('modules/core')#, variant_dir=build_dirname, src_dir='.', duplicate = 0)
if env['use_ompl'] :
	SConscript('modules/ompl')
if env['use_gecode'] :
	SConscript('modules/gecode')
if env['use_soplex'] :
	SConscript('modules/soplex')

env.Append( CPPPATH = [ os.path.abspath(p) for p in include_paths ] )
env.Append( CCFLAGS = [ '-isystem' + os.path.abspath(p) for p in isystem_paths ] )

# Determine all the build files
build_files = [os.path.join(build_dirname, src) for src in sources]
shared_lib = env.SharedLibrary('lib/' + lib_name, build_files)
#static_lib = env.Library('lib/' + lib_name, build_files)

Default([shared_lib])
#Default([static_lib, shared_lib])
