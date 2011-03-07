#!/usr/bin/env python
""" Utility for generating a class file, header, and test file """

import sys
import os
import argparse
import datetime



#======================================================================
def write_header(subproject, classname, filename, algorithm):
    """Write a class header file"""
    print "Writing header file to %s" % filename
    f = open(filename, 'w')
    
    guard = "MANTID_%s_%s_H_" % (subproject.upper(), classname.upper())
    
    # Create an Algorithm header; will not use it if 
    algorithm_header = """
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "%s";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "General";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

""" % classname
        
    alg_class_declare = " : public API::Algorithm"
    alg_include = """#include "MantidAPI/Algorithm.h" """
    
    if not algorithm:
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
   * 
   * @author
   * @date %s
   */
  class DLLExport %s %s
  {
  public:
    %s();
    ~%s();
    %s
  };


} // namespace Mantid
} // namespace %s

#endif  /* %s */
""" % (guard, guard, alg_include, subproject, classname, datetime.datetime.now(), classname, alg_class_declare, classname,classname, algorithm_header, subproject, guard)
    f.write(s)
    f.close()





#======================================================================
def write_source(subproject, classname, filename, algorithm):
    """Write a class source file"""
    print "Writing source file to %s" % filename
    f = open(filename, 'w')
    
    algorithm_top = """
  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(%s)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;
""" % (classname)

    algorithm_source = """
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

"""% (classname, classname, classname)   

    if not algorithm:
        algorithm_top = ""
        algorithm_source = ""
        
    
    s = """#include "Mantid%s/%s.h"
#include "MantidKernel/System.h"

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
    // TODO Auto-generated constructor stub
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  %s::~%s()
  {
    // TODO Auto-generated destructor stub
  }
  
%s

} // namespace Mantid
} // namespace %s

""" % (subproject, classname, subproject, algorithm_top, classname,classname,classname,classname,  algorithm_source, subproject)
    f.write(s)
    f.close()


    
#======================================================================
def write_test(subproject, classname, filename, algorithm):
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
  """ % classname
  
    if not algorithm:
        algorithm_test = ""
  
    s = """#ifndef %s
#define %s

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "Mantid%s/%s.h"

using namespace Mantid::%s;

class %sTest : public CxxTest::TestSuite
{
public:
%s
  void test_Something()
  {
  }


};


#endif /* %s */

""" % (guard,guard, subproject, classname,subproject, classname, algorithm_test, guard)
    f.write(s)
    f.close()
    

#======================================================================
def generate(subproject, classname, overwrite, test_only, algorithm):
    # Directory at base of subproject
    basedir = os.path.join(os.path.curdir, "Framework/"+subproject)
    
    headerfile = os.path.join(basedir, "inc/Mantid"+subproject+"/"+classname+".h")
    sourcefile = os.path.join(basedir, "src/"+classname+".cpp")
    testfile = os.path.join(basedir, "test/"+classname+"Test.h")
    
    if not test_only and not overwrite and os.path.exists(headerfile):
        print "\nError! Header file %s already exists. Use --force to overwrite.\n" % headerfile
        return
    if not test_only and not overwrite and os.path.exists(sourcefile):
        print "\nError! Source file %s already exists. Use --force to overwrite.\n" % sourcefile
        return
    if not overwrite and os.path.exists(testfile):
        print "\nError! Test file %s already exists. Use --force to overwrite.\n" % testfile
        return
      
    print
    if not test_only:
        write_header(subproject, classname, headerfile, algorithm)
        write_source(subproject, classname, sourcefile, algorithm)
    write_test(subproject, classname, testfile, algorithm)
    
    print "\n   Don't forget to add your class to Framework/%s/CMakeLists.txt" % subproject
    print 
    if not test_only:
        print "\tsrc/%s.cpp" % (classname)
        print "\tinc/Mantid%s/%s.h" % (subproject, classname)
    print "\ttest/%sTest.h" % (classname)
    print  


#======================================================================
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Utility to create Mantid class files: header, source and test.')
    parser.add_argument('subproject', metavar='SUBPROJECT', type=str, 
                        help='The subproject under Framework/; e.g. Kernel')
    parser.add_argument('classname', metavar='CLASSNAME', type=str, 
                        help='Name of the class to create')
    parser.add_argument('--force', dest='force', action='store_const',
                        const=True, default=False,
                        help='Force overwriting existing files. Use with caution!')
    parser.add_argument('--test', dest='test', action='store_const',
                        const=True, default=False,
                        help='Create only the test file.')
    parser.add_argument('--alg', dest='alg', action='store_const',
                        const=True, default=False,
                        help='Create an Algorithm stub. This adds some methods common to algorithms.')
     
    args = parser.parse_args()
    subproject = args.subproject
    classname = args.classname
    overwrite = args.force
    test_only = args.test
    algorithm = args.alg
    
    generate(subproject, classname, overwrite, test_only, algorithm)
