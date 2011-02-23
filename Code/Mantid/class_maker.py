#!/usr/bin/env python
""" Utility for generating a class file, header, and test file """

import sys
import os
import argparse
import datetime



#======================================================================
def write_header(subproject, classname, filename):
    """Write a class header file"""
    print "Writing header file to %s" % filename
    f = open(filename, 'w')
    
    guard = "MANTID_%s_%s_H_" % (subproject.upper(), classname.upper())
    
    s = """#ifndef %s
#define %s
    
#include "MantidKernel/System.h"


namespace Mantid
{
namespace %s
{

  /** %s : TODO: DESCRIPTION
   * 
   * @author:
   * @date: %s
   */
  class %s
  {
  public:
    %s();
    ~%s();
  };


} // namespace Mantid
} // namespace %s

#endif  /* %s */
""" % (guard, guard, subproject, classname, datetime.datetime.now(), classname,classname,classname, subproject, guard)
    f.write(s)
    f.close()
    




#======================================================================
def write_source(subproject, classname, filename):
    """Write a class source file"""
    print "Writing source file to %s" % filename
    f = open(filename, 'w')
    s = """#include "Mantid%s/%s.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace %s
{

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

} // namespace Mantid
} // namespace %s

""" % (subproject, classname, subproject, classname,classname,classname,classname, subproject)
    f.write(s)
    f.close()


    
#======================================================================
def write_test(subproject, classname, filename):
    """Write a class test file"""
    print "Writing test file to %s" % filename
    f = open(filename, 'w')

    guard = "MANTID_%s_%sTEST_H_" % (subproject.upper(), classname.upper())

    s = """#ifndef %s
#define %s

#include <cxxtest/TestSuite.h>
#include <MantidKernel/Timer.h>
#include <MantidKernel/System.h>
#include <iostream>
#include <iomanip>

#include <Mantid%s/%s.h>

using namespace Mantid::%s;

class %sTest : public CxxTest::TestSuite
{
public:

  void test()
  {
  }


};


#endif /* %s */

""" % (guard,guard, subproject, classname,subproject, classname, guard)
    f.write(s)
    f.close()
    

#======================================================================
def generate(subproject, classname, overwrite):
    # Directory at base of subproject
    basedir = os.path.join(os.path.curdir, "Framework/"+subproject)
    
    headerfile = os.path.join(basedir, "inc/Mantid"+subproject+"/"+classname+".h")
    sourcefile = os.path.join(basedir, "src/"+classname+".cpp")
    testfile = os.path.join(basedir, "test/"+classname+"Test.h")
    
    if not overwrite and (os.path.exists(headerfile) or os.path.exists(headerfile) or os.path.exists(headerfile)):
        raise BaseException("Error! One or more of the class files already exist. Use --force to overwrite.")
      
    write_header(subproject, classname, headerfile)
    write_source(subproject, classname, sourcefile)
    write_test(subproject, classname, testfile)
    
    print "\n\n   Don't forget to add your class to CMakeLists.txt:"
    print 
    print "\tsrc/%s.cpp" % (classname)
    print "\tinc/Mantid%s/%s.cpp" % (subproject, classname)
    print "\ttest/%sTest.h" % (classname)


#======================================================================
if __name__ == "__main__":
    generate("Kernel", "ThreadPoolRunnable", True)
