#ifndef SAVEREFLTBLTEST_H_
#define SAVEREFLTBLTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/SaveReflTBL.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FrameworkManager.h"
#include <fstream>
#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class SaveReflTBLTest : public CxxTest::TestSuite
{

public:

  static SaveReflTBLTest *createSuite() { return new SaveReflTBLTest(); }
  static void destroySuite(SaveReflTBLTest *suite) { delete suite; }

  SaveReflTBLTest()
  {
  }
  ~SaveReflTBLTest()
  {
  }

  void testExec()
  {
  }
};


#endif /*SAVEREFLTBLTEST_H_*/
