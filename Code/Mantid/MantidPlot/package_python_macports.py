#!/usr/bin/env python
"""
Script to pack the python libraries
to the dmg package created with clang+macports
"""
import sys, shutil, os
from distutils.spawn import find_executable

# path to copy the libraries
#OUTPUT_PATH = "/Users/ganeva/build/jcns-mantid/build/bin/MantidPlot.app/Contents/MacOS"
OUTPUT_PATH = os.path.abspath("Contents/MacOS")

# list of the python libraries to copy
PYTHON_LIBRARIES = ['sphinx', 'sphinx_bootstrap_theme', 'IPython', 'zmq', 'pygments', \
                '_markerlib', 'backports', 'certifi', 'tornado', 'markupsafe', \
                'jinja2', 'psutil', 'nxs']

# path to the nxs
# by default the nexus library installs it here
sys.path.append('/opt/local//lib/python2.7/site-packages/')


def copy_directory(src, dest):
    """
    Copies recursively directory src to dest
    overwrites dest if the directory dest already exists
    """
    if os.path.exists(dest):
        shutil.rmtree(dest)
    try:
        shutil.copytree(src, dest)
    # Directories are the same
    except shutil.Error as error:
        print 'Directory not copied. Error: %s' % error
    # Any error saying that the directory doesn't exist
    except OSError as error:
        print 'Directory not copied. Error: %s' % error

def copy_file(src, dest):
    """
    Copies the file src to destination dst
    dest must be a full file name
    """
    try:
        shutil.copyfile(src, dest)
    # file already exists
    except shutil.Error as error:
        print 'File is not copied. Error: %s' % error
    # Any error saying that the directory doesn't exist
    except OSError as error:
        print 'File is not copied. Error: %s' % error


if __name__ == '__main__':
    # copy the python libraries
    for lib in PYTHON_LIBRARIES:
        try:
            module = map(__import__, [lib])
        except ImportError as detail:
            print "Cannot import library ", lib
            print "Reason: ", detail
        else:
            copy_directory(module[0].__path__[0], os.path.join(OUTPUT_PATH, lib))

    # copy ipython (although I do not understand why)
    # find ipython executable
    IPYTHON_EXECUTABLE = find_executable('ipython')
    if not IPYTHON_EXECUTABLE:
        print "Cannot find ipython executable"
    else:
        # create bin folder
        BIN_DIRECTORY_NAME = os.path.join(OUTPUT_PATH, 'bin')
        try:
            if not os.path.exists(BIN_DIRECTORY_NAME):
                os.mkdir(BIN_DIRECTORY_NAME)
        except OSError as error:
            print "Cannot create directory %s. Error: %s." % (BIN_DIRECTORY_NAME, error)
        else:
            # copy ipython executable
            copy_file(IPYTHON_EXECUTABLE, os.path.join(BIN_DIRECTORY_NAME, 'ipython'))

    # find and copy pyparsing
    try:
        import pyparsing
    except ImportError as detail:
        print "Cannot import pyparsing. Error: ", detail
    else:
        copy_file(pyparsing.__file__, os.path.join(OUTPUT_PATH, 'pyparsing.pyc'))
        copy_file(os.path.splitext(pyparsing.__file__)[0]+'.py', \
            os.path.join(OUTPUT_PATH, 'pyparsing.py'))

    # find and copy readline
    try:
        import readline
        import readline_path
    except ImportError as detail:
        print "Cannot import readline. Error: ", detail
    else:
        copy_file(readline.__file__, os.path.join(OUTPUT_PATH, os.path.split(readline.__file__)[1]))
        copy_file(readline_path.__file__, os.path.join(OUTPUT_PATH, \
            os.path.split(readline_path.__file__)[1]))
        copy_file(os.path.splitext(readline_path.__file__)[0]+'.py', \
            os.path.join(OUTPUT_PATH, 'readline_path.py'))


