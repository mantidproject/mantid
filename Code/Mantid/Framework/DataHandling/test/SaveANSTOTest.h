#ifndef SAVEANSTOTEST_H_
#define SAVEANSTOTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/SaveANSTO.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FrameworkManager.h"
#include <fstream>
#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class SaveANSTOTest : public CxxTest::TestSuite
{

public:

  static SaveANSTOTest *createSuite() { return new SaveANSTOTest(); }
  static void destroySuite(SaveANSTOTest *suite) { delete suite; }

  SaveANSTOTest()
  {

  }
  ~SaveANSTOTest()
  {
    FrameworkManager::Instance().deleteWorkspace("SaveANSTOWS");
  }

  void testExec()
  {
  }
};


#endif /*SAVEANSTOTEST_H_*/
