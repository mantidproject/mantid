#!/usr/bin/env python
VERSION = "1.0"

import os
import sys

FRONTMATTER = """// If you get the message  "This application has failed to start because MSVCR80.dll was not found. Re-installing the application may fix this problem."
// when running to run this main.cpp in debug mode then try to uncomment the line below (see also http://blogs.msdn.com/dsvc/archive/2008/08/07/part-2-troubleshooting-vc-side-by-side-problems.aspx for more details)
//#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.VC80.CRT' version='8.0.50608.0' processorArchitecture='X86' publicKeyToken='1fc8b3b9a1e18e3b' \"")

#include <iostream>
#include <iomanip>
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/Memory.h"
#include "MantidKernel/Timer.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

void printMem(MemoryStats &mem, Timer &timer)
{
  mem.update();
  std::cout << "*** at " << timer << ": mem "<< mem << std::endl;
}

int main()
{
  FrameworkManagerImpl& fm = FrameworkManager::Instance();
  IAlgorithm *alg;
  MemoryStats mem;
  mem.ignoreFields(MEMORY_STATS_IGNORE_SYSTEM);
  Timer k_timer;
  printMem(mem, k_timer);

  // ADD YOUR CODE HERE
"""

BACKMATTER = """
  printMem(mem, k_timer);
  std::cout << "total time ";
  fm.clear();
  exit(0);

}
"""

class Writer:
    def __init__(self, filename=None, overwrite=False, debug=False):
        if debug:
            self.__handle = sys.stdout
        else:
            filename = os.path.abspath(filename)
            if os.path.exists(filename) and not overwrite:
                raise RuntimeError("To overwrite file use '--overwrite' flag")
            print "Creating file '%s'" % filename
            self.__handle = open(filename, 'w')

    def write(self):
        self.__handle.write(FRONTMATTER)
        self.__handle.write(BACKMATTER)

if __name__ == "__main__":
    import optparse
    info=[]
    parser = optparse.OptionParser("usage: %prog [options] <outfile>",
                                   None, optparse.Option, VERSION, 'error',
                                   " ".join(info))
    parser.add_option("-d", "--debug", dest="debug", action="store_true",
                      help="print the file to stdout rather than write it to disk")
    parser.add_option("", "--overwrite", dest="overwrite", action="store_true",
                      help="Whether or not to overwrite existing main.cpp file. Default=False.")
    parser.set_defaults(overwrite=False, debug=False)
    (options, args) = parser.parse_args()
    if len(args) <= 0:
        parser.error("Must specify an output file")
    if len(args) > 1:
        parser.error("Can only produce a single ouput file")
    filename = args[0]
    if not filename.endswith(".cpp"):
        parser.error("Filename '%s' must end with '.cpp'" % filename)

    writer = Writer(filename, options.overwrite, options.debug)
    writer.write()