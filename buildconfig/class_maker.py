#!/usr/bin/env python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
""" Utility for generating a class file, header, and test file """
import argparse
import datetime
from cmakelists_utils import *

VERSION = "1.0"


def get_year():
    """returns the current year"""
    return datetime.datetime.now().year


def write_header(subproject, classname, filename, args):
    """Write a class header file"""
    print("Writing header file to", filename)
    f = open(filename, "w")

    subproject_upper = subproject.upper()

    # Create an Algorithm header; will not use it if not an algo
    algorithm_header = """
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
"""

    alg_class_declare = " : public API::Algorithm"
    alg_include = '#include "MantidAPI/Algorithm.h"'

    if not args.alg:
        algorithm_header = ""
        alg_class_declare = ""
        alg_include = ""

    # The full text
    s = r"""// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; {year} ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

{alg_include}
#include "Mantid{subproject}/DllConfig.h"

namespace Mantid {{
namespace {subproject} {{

/** {classname} : TODO: DESCRIPTION
 */
class MANTID_{subproject_upper}_DLL {classname}{alg_class_declare} {{
public:{algorithm_header}}};

}} // namespace {subproject}
}} // namespace Mantid
""".format(
        subproject=subproject,
        alg_include=alg_include,
        classname=classname,
        year=get_year(),
        subproject_upper=subproject_upper,
        alg_class_declare=alg_class_declare,
        algorithm_header=algorithm_header,
    )

    f.write(s)
    f.close()


def write_source(subproject, classname, filename, args):
    """Write a class source file"""
    print("Writing source file to", filename)
    f = open(filename, "w")

    algorithm_top = """using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM({})
""".format(
        classname
    )
    algorithm_source = """
//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string {algname}::name() const {{ return "{algname}"; }}

/// Algorithm's version for identification. @see Algorithm::version
int {algname}::version() const {{ return 1; }}

/// Algorithm's category for identification. @see Algorithm::category
const std::string {algname}::category() const {{ return "TODO: FILL IN A CATEGORY"; }}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string {algname}::summary() const {{ return "TODO: FILL IN A SUMMARY"; }}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void {algname}::init() {{
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("InputWorkspace", "", Direction::Input),
                  "An input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void {algname}::exec() {{
  // TODO Auto-generated execute stub
}}
""".format(
        algname=classname
    )

    if not args.alg:
        algorithm_top = ""
        algorithm_source = ""

    # ------- Now the normal class text ------------------------------
    s = """// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; {year} ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "Mantid{subproject}/{subfolder}{classname}.h"

namespace Mantid {{
namespace {subproject} {{
{algorithm_top}{algorithm_source}
}} // namespace {subproject}
}} // namespace Mantid
""".format(
        year=get_year(),
        subproject=subproject,
        subfolder=args.subfolder,
        classname=classname,
        algorithm_top=algorithm_top,
        algorithm_source=algorithm_source,
    )
    f.write(s)
    f.close()


def write_test(subproject, classname, filename, args):
    """Write a class test file"""
    print("Writing test file to", filename)
    f = open(filename, "w")

    algorithm_test = """
  void test_Init() {{
    {algname} alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }}

  void test_exec() {{
    // Create test input if necessary by filling in appropriate code below.
    // Consider using MantidFrameworkTestHelpers/WorkspaceCreationHelper.h
    // MatrixWorkspace_sptr inputWS =

    {algname} alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from the algorithm. The type here will probably need to change. It should
    // be the type using in declareProperty for the "OutputWorkspace" type.
    // We can't use auto as it's an implicit conversion.
    Workspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_FAIL("TODO: Check the results and remove this line");
  }}
""".format(
        algname=classname
    )

    if not args.alg:
        algorithm_test = ""

    s = """// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; {year} ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "Mantid{subproject}/{subfolder}{classname}.h"

using Mantid::{subproject}::{classname};

class {classname}Test : public CxxTest::TestSuite {{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static {classname}Test *createSuite() {{ return new {classname}Test(); }}
  static void destroySuite({classname}Test *suite) {{ delete suite; }}
{algorithm_test}
  void test_Something() {{ TS_FAIL("You forgot to write a test!"); }}
}};
""".format(
        year=get_year(), subproject=subproject, subfolder=args.subfolder, classname=classname, algorithm_test=algorithm_test
    )
    f.write(s)
    f.close()


def write_rst(subproject, classname, filename, args):
    """Write an algorithm rst documentation file"""
    print("Writing rst file to", filename)
    f = open(filename, "w")

    s = """
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

TODO: Enter a full rst-markup description of your algorithm here.


Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - {algname}**

.. testcode:: {algname}Example

   # Create a host workspace
   ws = CreateWorkspace(DataX=range(0,3), DataY=(0,2))
   or
   ws = CreateSampleWorkspace()

   wsOut = {algname}()

   # Print the result
   print "The output workspace has %%i spectra" %% wsOut.getNumberHistograms()

Output:

.. testoutput:: {algname}Example

  The output workspace has ?? spectra

.. categories::

.. sourcelink::

""".format(
        algname=classname
    )

    f.write(s)
    f.close()


def generate(subproject, classname, overwrite, args):
    # Directory at base of subproject
    basedir, header_folder = find_basedir(args.project, subproject)

    headerfile = os.path.join(basedir, "inc", header_folder, args.subfolder + classname + ".h")
    sourcefile = os.path.join(basedir, "src", args.subfolder + classname + ".cpp")
    testfile = os.path.join(basedir, "test", classname + "Test.h")
    # up two from the subproject basedir and then docs\source\algorithms
    mantiddir = os.path.dirname(os.path.dirname(basedir))
    rstfile = os.path.join(mantiddir, "docs", "source", "algorithms", classname + "-v1.rst")

    if args.header and not overwrite and os.path.exists(headerfile):
        print("\nError! Header file {} already exists. Use --force to overwrite.\n".format(headerfile))
        return
    if args.cpp and not overwrite and os.path.exists(sourcefile):
        print("\nError! Source file {} already exists. Use --force to overwrite.\n".format(sourcefile))
        return
    if args.test and not overwrite and os.path.exists(testfile):
        print("\nError! Test file {} already exists. Use --force to overwrite.\n".format(testfile))
        return
    if args.rst and args.alg and not overwrite and os.path.exists(rstfile):
        print("\nError! Rst documentation file {} already exists. Use --force to overwrite.\n".format(rstfile))
        return

    print()
    if args.header:
        write_header(subproject, classname, headerfile, args)
    if args.cpp:
        write_source(subproject, classname, sourcefile, args)
    if args.test:
        write_test(subproject, classname, testfile, args)
    if args.rst and args.alg:
        write_rst(subproject, classname, rstfile, args)

    # Insert into the cmake list if required
    if args.cmake:
        add_to_cmake(subproject, classname, args, args.subfolder)
        print("\n   Files were added to Framework/{}/CMakeLists.txt !".format(subproject))
    print()

    if args.alg:
        print("Note: if this is a WorkflowAlgorithm, please subclass DataProcessorAlgorithm")
        print("Note: if this algorithm operates on a WorkspaceGroup, please override processGroups()")
        print()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Utility to create Mantid class files: header, source and test. " "version " + VERSION)
    parser.add_argument("subproject", metavar="SUBPROJECT", type=str, help="The subproject under Framework/; e.g. Kernel")
    parser.add_argument("classname", metavar="CLASSNAME", type=str, help="Name of the class to create")
    parser.add_argument(
        "--force",
        dest="force",
        action="store_const",
        const=True,
        default=False,
        help="Force overwriting existing files. Use with caution!",
    )
    parser.add_argument("--no-header", dest="header", action="store_const", const=False, default=True, help="Don't create the header file")
    parser.add_argument("--no-test", dest="test", action="store_const", const=False, default=True, help="Don't create the test file")
    parser.add_argument("--no-cpp", dest="cpp", action="store_const", const=False, default=True, help="Don't create the cpp file")
    parser.add_argument("--no-rst", dest="rst", action="store_const", const=False, default=True, help="Don't create the rst file")
    parser.add_argument("--no-cmake", dest="cmake", action="store_const", const=False, default=True, help="Don't modify cmake files")
    parser.add_argument(
        "--alg",
        dest="alg",
        action="store_const",
        const=True,
        default=False,
        help="Create an Algorithm stub. This adds some methods common to algorithms.",
    )
    parser.add_argument(
        "--subfolder",
        dest="subfolder",
        default="",
        help="Put the source under a subfolder below the main part of the project, e.g. " "Geometry/Instrument.",
    )
    parser.add_argument(
        "--project", dest="project", default="Framework", help="The project in which this goes. Default: Framework. Can be MantidQt"
    )

    args = parser.parse_args()
    subproject = args.subproject
    classname = args.classname
    overwrite = args.force

    # Make sure the subfolders end with a /
    if args.subfolder != "":
        if args.subfolder[-1:] != "/":
            args.subfolder += "/"

    generate(subproject, classname, overwrite, args)
