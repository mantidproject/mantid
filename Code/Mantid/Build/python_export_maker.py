"""Creates a simple template for exporting a class to Python
"""
import optparse
import os
import re

def get_exportfile(headerfile):
    """
    For the given header path create a path to the corresponding 
    export file in the PythonInterface
    """
    # We need to find the submodule from the Mantid package
    submodule = get_submodule(headerfile)
    frameworkdir = get_frameworkdir(headerfile)
    exportpath = os.path.join(frameworkdir, 'PythonInterface', 'mantid', submodule, 'src')
    exportfile = os.path.join(exportpath, os.path.basename(headerfile).replace('.h','.cpp'))
    return exportfile

def get_submodule(headerfile):
    """
    Returns the corresponding Python submodule for the header
    """
    return get_namespace(headerfile).lower()

def get_namespace(headerfile):
    """
    Returns the Mantid namespace that the header resides in
    """
    matches = re.match(r".*inc/Mantid([a-z,A-z]*)/.*\.h", headerfile)
    if matches:
        namespace = matches.group(1)
    else:
        raise RuntimeError("Unknown header path style. Cannot extract Mantid namespace name.")
    return namespace

def get_frameworkdir(headerfile):
    """
    Returns the Framework directory
    """
    if 'Framework' in headerfile:
        matches = re.match(r"(.*Framework/).*\.h", headerfile)
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
    matches = re.match(r".*inc/(Mantid[a-z,A-z]*/.*\.h)", headerfile)
    if matches:
        return matches.group(1)
    else:
        raise RuntimeError("Unable to determine include path from given header")

def get_modulepath(frameworkdir, submodule):
    """Creates a path to the requested submodule
    """
    return os.path.join(frameworkdir, 'PythonInterface', 'mantid',submodule)


def write_export_file(headerfile, overwrite):
    """Write the export cpp file
    """
    # Where are we writing the output
    exportfile = get_exportfile(headerfile)
    print 'Writing export to \"%s\" '% exportfile
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

    print 'Generated export file "%s"' % exportfile
    print
    print "  ** Add this to the EXPORT_FILES variable in '%s'" % \
       os.path.join(get_modulepath(get_frameworkdir(headerfile), get_submodule(headerfile)), 'CMakeLists.txt')        

    return exportfile

def main():
    """Main function
    """
    parser = optparse.OptionParser(usage="usage: %prog [options] headerfile ", description="Creates a simple template for exporting a class to Python")
    parser.add_option("--overwrite", "-o", dest='overwrite', action='store_true', default=False, help='If the file already exists, overwrite it')
    (options, args) = parser.parse_args()
    if len(args) != 1:
        parser.error("Incorrect number of arguments")
    
    headerfile = args[0]
    if not os.path.exists(headerfile):
        parser.error("Invalid header path")

    # Options
    overwrite = options.overwrite
    
    # Generate
    write_export_file(os.path.abspath(headerfile), overwrite)

if __name__ == '__main__':
    main()
    
