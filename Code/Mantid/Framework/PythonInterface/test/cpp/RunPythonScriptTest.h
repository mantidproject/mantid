#ifndef MANTID_PYTHONINTERFACE_RUNPYTHONSCRIPTTEST_H_
#define MANTID_PYTHONINTERFACE_RUNPYTHONSCRIPTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidPythonInterface/api/Algorithms/RunPythonScript.h"

/**
 * This is a very minimal test just to check the properties.
 * 
 * A proper check requires a running python environment and some
 * systems have issues with trying to initialize a Python environment in the
 * C++ tests.
 * For an in-depth test see PythonInterface/test/python/mantid/api/RunPythonScriptTest.py
 * 
 */

class RunPythonScriptTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RunPythonScriptTest *createSuite() { return new RunPythonScriptTest(); }
  static void destroySuite( RunPythonScriptTest *suite ) { delete suite; }

  void testInit()
  {
    using Mantid::PythonInterface::RunPythonScript;

    RunPythonScript alg;
    alg.initialize();
   
    TS_ASSERT_EQUALS(3, alg.getProperties().size());

    TS_ASSERT(alg.existsProperty("InputWorkspace"));
    TS_ASSERT(alg.existsProperty("Code"));
    TS_ASSERT(alg.existsProperty("OutputWorkspace"));
  }

};


#endif /* MANTID_PYTHONINTERFACE_RUNPYTHONSCRIPTTEST_H_ */
