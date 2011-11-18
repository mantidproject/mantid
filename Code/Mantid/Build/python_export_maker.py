"""Creates a simple template for exporting a class to Python
"""
import optparse
import os

def write_export_file(exportpath, headerpath, classname):
    """Write the export cpp file
    """
    headerfile = os.path.basename(headerpath)
    relativeheader = headerpath[headerpath.index('MantidKernel'):]
    namespace = os.path.dirname(relativeheader).lstrip('Mantid')
    
    cppcode = """#include "%(header)s"
#include <boost/python/class.hpp>

using Mantid::%(namespace)s::%(class)s;
using boost::python::class_;
using boost::python::no_init;

void export_%(class)s()
{
  class_<%(class)s,boost::noncopyable>("%(class)s", no_init)
    ;
}

"""
    cppfile = file(exportpath, 'w')
    cppfile.write(cppcode % {'header':relativeheader, 'namespace':namespace,'class':classname})
    
def create_modulepath(frameworkdir, submodule):
    """Creates a path to the requested submodule
    """
    return os.path.join(frameworkdir, 'PythonInterface', 'mantid',submodule)
    
def create_exportpath(frameworkdir, headerfile, submodule):
    exportdir = os.path.join(create_modulepath(frameworkdir, submodule), 'src')
    if (not os.path.exists(exportdir)) or (not os.path.isdir(exportdir)):
        raise RuntimeError("Invalid path to export file: '%s'. Check the sub_module is correct" % exportdir)
    classname = os.path.splitext(headerfile)[0]
    return os.path.join(exportdir, classname + '.cpp'), classname


def check_frameworkpath(path, parser):
    """Check the given path exists and looks like the Framework directory
    """
    if not os.path.exists(path):
        raise parser.error("Framework directory does not exist: '%s'" % path)
    if not os.path.exists(os.path.join(path, 'API')):
        raise parser.error("Given framework directory does not have the correct layout for the framework directory: '%s'")
    
def main():
    """Main function
    """
    parser = optparse.OptionParser(usage="usage: %prog [options] sub_module headerfile_path ", description="Creates a simple template for exporting a class to Python")
    parser.add_option("--overwrite", "-o", dest='overwrite', action='store_true', default=False, help='If the file already exists, overwrite it')
    (options, args) = parser.parse_args()

    # Header file
    if len(args) != 2:
        parser.error("Incorrect number of arguments")
    submodule = args[0]
    headerpath = args[1]
    if not os.path.exists(headerpath):
        parser.error("Invalid header path")
    # Options
    overwrite = options.overwrite
    # Check directory
    frameworkdir = os.path.join(os.getcwd(), '../Framework')
    # Does this look like the framework
    check_frameworkpath(frameworkdir, parser)
    
    # Export file path
    headerfile = os.path.basename(headerpath)
    exportpath, classname = create_exportpath(frameworkdir, headerfile, submodule)
    if os.path.exists(exportpath) and overwrite is False:
        parser.error("Export file '%s' already exists, use the --overwrite option to overwrite the file." % exportpath)

    # Generate the file
    write_export_file(exportpath, headerpath, classname)
    #
    print 'Generated export file "%s"' % exportpath
    print
    print "  ** Add this to the EXPORT_FILES variable in '%s'" % os.path.join(create_modulepath(frameworkdir, submodule), 'CMakeLists.txt')        

if __name__ == '__main__':
    main()
    
