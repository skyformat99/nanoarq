import os
import ycm_core
import ntpath

flags = [
    '-DARQ_ASSERTS_ENABLED=1',
    '-DARQ_COMPILE_CRC32=1',
    '-DARQ_USE_C_STDLIB=1',
    '-DARQ_LITTLE_ENDIAN_CPU=1',
    '-Wall',
    '-Wextra',
    '-Wunreachable-code',
    '-Wshadow',
    '-Wcast-align',
    '-Wcast-qual',
    '-Wstrict-aliasing',
    '-Wmissing-braces',
    '-I', '.',
    '-I', 'unit_tests',
    '-I', 'functional_tests',
    '-I', 'external/CppUTest/src/CppUTest_external/include'
]

cpp_flags = [ '-std=c++11', '-x', 'c++' ]
c_flags = [ '-std=c99', '-x', 'c' ]


def DirectoryOfThisScript():
  return os.path.dirname( os.path.abspath( __file__ ) )


def MakeRelativePathsInFlagsAbsolute( flags, working_directory ):
  if not working_directory:
    return list( flags )
  new_flags = []
  make_next_absolute = False
  path_flags = [ '-isystem', '-I', '-iquote', '--sysroot=' ]
  for flag in flags:
    new_flag = flag

    if make_next_absolute:
      make_next_absolute = False
      if not flag.startswith( '/' ):
        new_flag = os.path.join( working_directory, flag )

    for path_flag in path_flags:
      if flag == path_flag:
        make_next_absolute = True
        break

      if flag.startswith( path_flag ):
        path = flag[ len( path_flag ): ]
        new_flag = path_flag + os.path.join( working_directory, path )
        break

    if new_flag:
      new_flags.append( new_flag )
  return new_flags

def PathLeaf(path):
    head, tail = ntpath.split(path)
    return tail or ntpath.basename(head)

def FlagsForFile( filename, **kwargs ):
  relative_to = DirectoryOfThisScript()
  final_flags = MakeRelativePathsInFlagsAbsolute( flags, relative_to )

  if ( PathLeaf(filename) == 'arq.h' ):
      final_flags = final_flags + c_flags
  else:
      final_flags = final_flags + cpp_flags

  return {
    'flags': final_flags,
    'do_cache': True
  }
