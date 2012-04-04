#!/usr/bin/env python
""" Utility for generating a class file, header, and test file """

import sys
import os
try:
    import argparse
    useArgparse = True
except ImportError, e:
    import optparse # deprecated in v2.7
    useArgparse = False
import datetime
import re
import cmakelists_utils
from cmakelists_utils import *
import commands

VERSION = "1.0"

#======================================================================
def write_header(subproject, classname, filename, args):
    """Write a class header file"""
    print "Writing header file to %s" % filename
    f = open(filename, 'w')
    
    guard = "MANTID_%s_%s_H_" % (subproject.upper(), classname.upper())
    
    # Create an Algorithm header; will not use it if not an algo
    algorithm_header = """
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;

  private:
    virtual void initDocs();
    void init();
    void exec();

"""

    # ---- Find the author, default to blank string ----
    author = ""
    try:
        author = commands.getoutput('git config user.name')
    except:
        pass
        
    alg_class_declare = " : public API::Algorithm"
    alg_include = '#include "MantidAPI/Algorithm.h"'
    
    if not args.alg:
        algorithm_header = ""
        alg_class_declare = ""
        alg_include = ""
    
    # The full text
    s = """#ifndef %s
#define %s

#include "MantidKernel/System.h"
%s

namespace Mantid
{
namespace %s
{

  /** %s : TODO: DESCRIPTION
    
    @date %s

    Copyright &copy; %s ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport %s %s
  {
  public:
    %s();
    virtual ~%s();
    %s
  };


} // namespace %s
} // namespace Mantid

#endif  /* %s */""" % (guard, guard,
       alg_include, subproject, classname,
       datetime.datetime.now().date(),
       datetime.datetime.now().date().year, classname, alg_class_declare,
       classname, classname, algorithm_header, subproject, guard)

    f.write(s)
    f.close()





#======================================================================
def write_source(subproject, classname, filename, args):
    """Write a class source file"""
    print "Writing source file to %s" % filename
    f = open(filename, 'w')
    
    algorithm_top = """
  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(%s)
  
""" % (classname)

    algorithm_source = """
  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string %s::name() const { return "%s";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int %s::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string %s::category() const { return "General";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void %s::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void %s::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void %s::exec()
  {
    // TODO Auto-generated execute stub
  }

""" % (classname, classname, classname, classname, classname, classname, classname)   

    if not args.alg:
        algorithm_top = ""
        algorithm_source = ""
        s = ""
    else:
        s = """/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

"""

    # ------- Now the normal class text ------------------------------    
    s += """#include "Mantid%s/%s%s.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace %s
{
%s

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  %s::%s()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  %s::~%s()
  {
  }
  
%s

} // namespace Mantid
} // namespace %s""" % (
        subproject, args.subfolder, classname, subproject, algorithm_top,
        classname, classname, classname, classname, algorithm_source, subproject)
    f.write(s)
    f.close()


    
#======================================================================
def write_test(subproject, classname, filename, args):
    """Write a class test file"""
    print "Writing test file to %s" % filename
    f = open(filename, 'w')

    guard = "MANTID_%s_%sTEST_H_" % (subproject.upper(), classname.upper())
    algorithm_test = """
  void test_Init()
  {
    %s alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("%sTest_OutputWS");
  
    %s alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("REPLACE_PROPERTY_NAME_HERE!!!!", "value") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service. TODO: Change to your desired type
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;
    
    // TODO: Check the results
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  """ % (classname,classname,classname);
  
    if not args.alg:
        algorithm_test = ""
  
    s = """#ifndef %s
#define %s

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "Mantid%s/%s%s.h"

using namespace Mantid;
using namespace Mantid::%s;
using namespace Mantid::API;

class %sTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static %sTest *createSuite() { return new %sTest(); }
  static void destroySuite( %sTest *suite ) { delete suite; }

%s
  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* %s */""" % (
          guard, guard, subproject, args.subfolder, classname,
          subproject, classname, classname, classname, classname,
          algorithm_test, guard)
    f.write(s)
    f.close()
    
    
    
    
#======================================================================
def generate(subproject, classname, overwrite, args):
    
    # Directory at base of subproject
    basedir, header_folder = find_basedir(args.project, subproject)
    
    headerfile = os.path.join(basedir, "inc", header_folder, args.subfolder + classname + ".h")
    sourcefile = os.path.join(basedir, "src", args.subfolder + classname + ".cpp")
    testfile = os.path.join(basedir, "test", classname + "Test.h")
    
    if args.header and not overwrite and os.path.exists(headerfile):
        print "\nError! Header file %s already exists. Use --force to overwrite.\n" % headerfile
        return
    if args.cpp and not overwrite and os.path.exists(sourcefile):
        print "\nError! Source file %s already exists. Use --force to overwrite.\n" % sourcefile
        return
    if args.test and not overwrite and os.path.exists(testfile):
        print "\nError! Test file %s already exists. Use --force to overwrite.\n" % testfile
        return
      
    print
    if args.header:
        write_header(subproject, classname, headerfile, args)
    if args.cpp:
        write_source(subproject, classname, sourcefile, args)
    if args.test:
        write_test(subproject, classname, testfile, args)
    
    # Insert into the cmake list
    add_to_cmake(subproject, classname, args, args.subfolder)
    
    print "\n   Files were added to Framework/%s/CMakeLists.txt !" % (subproject)
    print 
#    if not test_only:
#        print "\tsrc/%s.cpp" % (classname)
#        print "\tinc/Mantid%s/%s.h" % (subproject, classname)
#    print "\ttest/%sTest.h" % (classname)
#    print  



#======================================================================
if __name__ == "__main__":
    parser = None
    
    if useArgparse:
        parser = argparse.ArgumentParser(description='Utility to create Mantid class files: header, source and test. version ' + VERSION)
        parser.add_argument('subproject', metavar='SUBPROJECT', type=str,
                            help='The subproject under Framework/; e.g. Kernel')
        parser.add_argument('classname', metavar='CLASSNAME', type=str,
                            help='Name of the class to create')
        parser.add_argument('--force', dest='force', action='store_const',
                            const=True, default=False,
                            help='Force overwriting existing files. Use with caution!')
        parser.add_argument('--no-header', dest='header', action='store_const',
                            const=False, default=True,
                            help="Don't create the header file")
        parser.add_argument('--no-test', dest='test', action='store_const',
                            const=False, default=True,
                            help="Don't create the test file")
        parser.add_argument('--no-cpp', dest='cpp', action='store_const',
                            const=False, default=True,
                            help="Don't create the cpp file")
        parser.add_argument('--alg', dest='alg', action='store_const',
                            const=True, default=False,
                            help='Create an Algorithm stub. This adds some methods common to algorithms.')
        parser.add_argument('--subfolder', dest='subfolder', 
                            default="",
                            help='Put the source under a subfolder below the main part of the project, e.g. Geometry/Instrument.')
        parser.add_argument('--project', dest='project', 
                            default="Framework",
                            help='The project in which this goes. Default: Framework. Can be MantidQt, Vates')
    else:
        parser = optparse.OptionParser("Usage: %prog SUBPROJECT CLASSNAME [options]", None,
                                       optparse.Option, VERSION, 'error', 'Utility to create Mantid class files: header, source and test.')
        #parser.add_option('--subproject', metavar='SUBPROJECT', type=str,
        #                    help='The subproject under Framework/; e.g. Kernel')
        #parser.add_option('--classname', metavar='CLASSNAME', type=str,
        #                    help='Name of the class to create')
        parser.add_option('--force', dest='force', action='store_const',
                            const=True, default=False,
                            help='Force overwriting existing files. Use with caution!')
        parser.add_option('--no-header', dest='header', action='store_const',
                            const=False, default=True,
                            help="Don't create the header file")
        parser.add_option('--no-test', dest='test', action='store_const',
                            const=False, default=True,
                            help="Don't create the test file")
        parser.add_option('--no-cpp', dest='cpp', action='store_const',
                            const=False, default=True,
                            help="Don't create the cpp file")
        parser.add_option('--alg', dest='alg', action='store_const',
                            const=True, default=False,
                            help='Create an Algorithm stub. This adds some methods common to algorithms.')
        parser.add_option('--subfolder', dest='subfolder', 
                            default="",
                            help='Put the source under a subfolder below the main part of the project, e.g. Geometry/Instrument.')
        parser.add_option('--project', dest='project', 
                            default="Framework",
                            help='The project in which this goes. Default: Framework. Can be MantidQt, Vates')

    args = None
    if useArgparse:
        args = parser.parse_args()
    else:
        (options, myargs) = parser.parse_args()
        args = options
        args.subproject = myargs[0]
        args.classname = myargs[1]

    subproject = args.subproject
    classname = args.classname
    overwrite = args.force

    # Make sure the subfolders end with a /
    if args.subfolder != "":
        if args.subfolder[-1:] != "/":
            args.subfolder += "/"
    
    generate(subproject, classname, overwrite, args)
