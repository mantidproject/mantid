#!/usr/bin/env python
""" Utility for generating a class file, header, and test file """

import sys
import os
import argparse
import datetime
import re


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
""" % (guard, guard, alg_include, subproject, classname, datetime.datetime.now(), classname, alg_class_declare, classname, classname, algorithm_header, subproject, guard)
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

""" % (classname, classname, classname)   

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

""" % (subproject, classname, subproject, algorithm_top, classname, classname, classname, classname, algorithm_source, subproject)
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
  
  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("%sTest_OutputWS");
  
    %s alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("REPLACE_PROPERTY_NAME_HERE!!!!", "value") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() )
    
    // Retrieve the workspace from data service. TODO: Change to your desired type
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(ws);
    if (!ws) return;
    
    // TODO: Check the results
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  """ % (classname,classname,classname);
  
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
using namespace Mantid::API;

class %sTest : public CxxTest::TestSuite
{
public:
%s
  void test_Something()
  {
  }


};


#endif /* %s */

""" % (guard, guard, subproject, classname, subproject, classname, algorithm_test, guard)
    f.write(s)
    f.close()
    
    
#======================================================================================
def find_line_number(lines, searchfor, startat=0):
    """Look line-by-line in lines[] for a line that starts with searchfor. Return
    the line number in source where the line was found, and the padding (in spaces) before it"""
    count = 0
    done = False
    padding = ""
    for n in xrange(startat, len(lines)):
        line = lines[n]
        s = line.strip()
        if s.startswith(searchfor):
            # How much padding?
            padding = get_padding(line)
            return (n, padding)

    return (None, None)


#======================================================================================
def insert_lines(lines, insert_lineno, extra, padding):
    """Insert a text, split by lines, inside a list of 'lines', at index 'insert_lineno'
    Adds 'padding' to each line."""
    # split
    extra_lines = extra.split("\n");
    #Pad 
    for n in xrange(len(extra_lines)):
        extra_lines[n] = padding + extra_lines[n]
    return lines[0:insert_lineno] + extra_lines + lines[insert_lineno:]


#======================================================================================
def save_lines_to(lines, fullpath):
    """Save a list of strings to one file"""        
    # Join into one big string
    source = "\n".join(lines)
    # Save to disk
    f = open(fullpath, 'w')
    f.write(source)
    f.close()

#======================================================================================
def redo_cmake_section(lines, cmake_tag, add_this_line):
    """ Read the LINES of a file. Find "set ( cmake_tag",
    read all the lines to get all the files,
    add your new line,
    sort them,
    rewrite. Yay!"""

    search_for1 = "set ( %s" % cmake_tag
    search_for2 = "set (%s" % cmake_tag
    # List of files in the thingie
    files = []
    lines_before = []
    lines_after = []
    section_num = 0
    for line in lines:
        if line.strip().startswith(search_for1): section_num = 1
        if line.strip().startswith(search_for2): section_num = 1
        
        if section_num == 0:
            # These are the lines before
            lines_before.append(line)
        elif section_num == 1:
            #this is a line with the name of a file
            line = line.strip()
            # Take off the tag
            if line.startswith(search_for1): line = line[len(search_for1):].strip()
            if line.startswith(search_for2): line = line[len(search_for2):].strip()
            # Did we reach the last one?
            if line.endswith(")"): 
                section_num = 2
                line = line[0:len(line) - 1].strip()
                
            if len(line) > 0:
                files.append(line) 
        else:
            # These are lines after
            lines_after.append(line)
            
    # Add the new file to the list of files
    if len(add_this_line) > 0:
        files.append(add_this_line)
    # Use a set to keep only unique linese
    files = set(files)
    files = list(files)
    # Sort-em alphabetically
    files.sort()
        
    lines = lines_before
    lines.append("set ( %s" % cmake_tag)
    for file in files:
        lines.append("\t" + file)
    lines.append(")") # close the parentheses
    lines += lines_after
     
    return lines


#======================================================================
def add_to_cmake(subproject, classname, args):
    """ Add the class to the cmake list of the given class """
    cmake_path = os.path.join(os.path.curdir, "Framework/" + subproject + "/CMakeLists.txt")
    source = open(cmake_path).read()
    lines = source.split("\n");
    if args.header:
        lines = redo_cmake_section(lines, "INC_FILES", "inc/Mantid" + subproject + "/" + classname + ".h")
    if args.cpp:
        lines = redo_cmake_section(lines, "SRC_FILES", "src/" + classname + ".cpp")
    if args.test:
        lines = redo_cmake_section(lines, "TEST_FILES", "test/" + classname + "Test.h")
    
    f = open(cmake_path, 'w')
    text = "\n".join(lines) 
    f.write(text)
    f.close()

#======================================================================
def fix_cmake_format(subproject):
    """ Just fix the CMAKE format"""
    cmake_path = os.path.join(os.path.curdir, "Framework/" + subproject + "/CMakeLists.txt")
    source = open(cmake_path).read()
    lines = source.split("\n");
    lines = redo_cmake_section(lines, "SRC_FILES", "")
    lines = redo_cmake_section(lines, "INC_FILES", "")
    lines = redo_cmake_section(lines, "TEST_FILES", "")
    
    f = open(cmake_path, 'w')
    text = "\n".join(lines) 
    f.write(text)
    f.close()

#======================================================================
def fix_all_cmakes():
    """ Fix all cmake files """
    projects = ["Algorithms", "DataObjects", "MDAlgorithms", "PythonAPI", "API", 
                       "Geometry", "MDDataObjects", "CurveFitting", "ICat", "MDEvents", 
                       "DataHandling", "Kernel", "Nexus", "Crystal"]
    for proj in projects:
        fix_cmake_format(proj)
    
#======================================================================
def generate(subproject, classname, overwrite, args):
    # Directory at base of subproject
    basedir = os.path.join(os.path.curdir, "Framework/" + subproject)
    
    headerfile = os.path.join(basedir, "inc/Mantid" + subproject + "/" + classname + ".h")
    sourcefile = os.path.join(basedir, "src/" + classname + ".cpp")
    testfile = os.path.join(basedir, "test/" + classname + "Test.h")
    
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
        write_header(subproject, classname, headerfile, args.alg)
    if args.cpp:
        write_source(subproject, classname, sourcefile, args.alg)
    if args.test:
        write_test(subproject, classname, testfile, args.alg)
    
    # Insert into the cmake list
    add_to_cmake(subproject, classname, args)
    
    print "\n   Files were added to Framework/%s/CMakeLists.txt !" % subproject
    print 
#    if not test_only:
#        print "\tsrc/%s.cpp" % (classname)
#        print "\tinc/Mantid%s/%s.h" % (subproject, classname)
#    print "\ttest/%sTest.h" % (classname)
#    print  



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
     
    args = parser.parse_args()
    subproject = args.subproject
    classname = args.classname
    overwrite = args.force
    
    generate(subproject, classname, overwrite, args)
