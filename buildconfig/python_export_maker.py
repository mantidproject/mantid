#! /usr/bin/env python
"""Creates a simple template for exporting a class to Python
"""
import optparse
import os
import re
import sys

def get_exportfile(headerfile):
    """
    For the given header path create a path to the corresponding
    export file in the PythonInterface
    """
    # We need to find the submodule from the Mantid package
    submodule = get_submodule(headerfile)
    frameworkdir = get_frameworkdir(headerfile)
    exportpath = os.path.join(frameworkdir, 'PythonInterface', 'mantid', submodule, 'src', 'Exports')
    exportfile = os.path.join(exportpath, os.path.basename(headerfile).replace('.h','.cpp'))
    return exportfile

def get_unittest_file(headerfile):
    """
    For the given header path create a path to the corresponding
    Python unit test file
    """
    frameworkdir = get_frameworkdir(headerfile)
    submodule = get_submodule(headerfile)
    testpath = os.path.join(frameworkdir, 'PythonInterface', 'test',
                            'python','mantid',submodule)
    return os.path.join(testpath, os.path.basename(headerfile).replace('.h','Test.py'))

def get_submodule(headerfile):
    """
    Returns the corresponding Python submodule for the header
    """
    return get_namespace(headerfile).lower()

def get_namespace(headerfile):
    """
    Returns the Mantid namespace that the header resides in
    """
    matches = re.match(r".*inc(/|\\)Mantid(\w+)(/|\\).*\.h", headerfile)
    if matches:
        namespace = matches.group(2)
    else:
        raise RuntimeError("Unknown header path style. Cannot extract Mantid namespace name.")
    return namespace

def get_frameworkdir(headerfile):
    """
    Returns the Framework directory
    """
    if 'Framework' in headerfile:
        matches = re.match(r"(.*Framework(/|\\)).*\.h", headerfile)
        if matches:
            frameworkdir = matches.group(1)
        else:
            raise RuntimeError("Unknown header path style. Cannot extract Framework directory.")
    else:
        raise RuntimeError("Script must be run from outside the Framework directory")
    return frameworkdir

def get_classname(headerfile):
    """
    Returns the classname from a given file. At the moment it just uses the file name
    """
    filename = os.path.basename(headerfile)
    return os.path.splitext(filename)[0]

def get_include(headerfile):
    """
    Returns the relative include path for inclusion in the code
    """
    matches = re.match(r".*inc(/|\\)(Mantid[a-z,A-z]*(/|\\).*\.h)", headerfile)
    if matches:
        includefile = matches.group(2)
    else:
        raise RuntimeError("Unable to determine include path from given header")
    # Make sure the include only has forward slases
    includefile = includefile.replace("\\", "/")
    return includefile
def get_modulepath(frameworkdir, submodule):
    """Creates a path to the requested submodule
    """
    return os.path.join(frameworkdir, 'PythonInterface', 'mantid',submodule)

def write_export_file(headerfile, overwrite):
    """Write the export cpp file
    """
    # Where are we writing the output
    exportfile = get_exportfile(headerfile)
    print 'Writing export file \"%s\" '% os.path.basename(exportfile)
    if os.path.exists(exportfile) and not overwrite:
        raise RuntimeError("Export file '%s' already exists, use the --overwrite option to overwrite the file." % exportfile)

    namespace = get_namespace(headerfile)
    classname = get_classname(headerfile)
    include = get_include(headerfile)

    cppcode = """#include "%(header)s"
#include <boost/python/class.hpp>

using Mantid::%(namespace)s::%(class)s;
using namespace boost::python;

void export_%(class)s()
{
  class_<%(class)s,boost::noncopyable>("%(class)s", no_init)
    ;
}

"""
    cppfile = file(exportfile, 'w')
    cppfile.write(cppcode % {'header':include, 'namespace':namespace,'class':classname})
    cppfile.close()

    print 'Generated export file "%s"' % os.path.basename(exportfile)
    print
    print "  ** Add this to the EXPORT_FILES variable in '%s'" % \
       os.path.join(get_modulepath(get_frameworkdir(headerfile), get_submodule(headerfile)), 'CMakeLists.txt')

    return exportfile

def write_unittest(headerfile, overwrite):
    """
       Write a unit test for the given header
    """
    filename = get_unittest_file(headerfile)
    print 'Writing unit test \"%s\" '% os.path.basename(filename)
    if os.path.exists(filename) and not overwrite:
        raise RuntimeError("A unit test file '%s' already exists, use the --overwrite-test option to overwrite the file." % filename)

    classname = get_classname(headerfile)
    pytest = \
"""import unittest
from mantid import %(classname)s

class %(classname)sTest(unittest.TestCase):

    def test_something(self):
        self.fail("Test something")

if __name__ == '__main__':
    unittest.main()
"""
    unittest = open(filename, 'w')
    unittest.write(pytest % {'classname':classname})
    unittest.close()

    print 'Generated unit test file "%s"' % os.path.basename(filename)
    print
    print "  ** Add this to the TEST_PY_FILES variable in '%s'" % \
        os.path.join('PythonInterface', 'CMakeLists.txt')

def main():
    """Main function
    """
    parser = optparse.OptionParser(usage="usage: %prog [options] headerfile ", description="Creates a simple template for exporting a class to Python along with a unit test")
    parser.add_option("--overwrite", "-o", dest='overwrite', action='store_true', default=False, help='If the file already exists, overwrite it')
    parser.add_option("--export-only", "-e", dest='export_only', action='store_true', default=False, help='If True only attempt to generate the export file')
    parser.add_option("--test-only", "-t", dest='test_only', action='store_true', default=False, help='If True only attempt to generate the unit test')
    parser.add_option("--overwrite-test", "-w", dest='overwrite_test', action='store_true', default=False, help='If true overwrite a test file if it exists')

    (options, args) = parser.parse_args()
    if len(args) != 1:
        parser.error("Incorrect number of arguments")

    headerfile = args[0]
    if not os.path.exists(headerfile):
        parser.error("Invalid header path")

    # Sanity
    if options.export_only and options.test_only:
        parser.error("The export_only and test_only options are mutually exclusive, please specify only one.")

    # Generate
    if not options.test_only:
        write_export_file(os.path.abspath(headerfile), options.overwrite)

    # Unit test
    if not options.export_only:
        write_unittest(os.path.abspath(headerfile), options.overwrite_test)

if __name__ == '__main__':
    main()

