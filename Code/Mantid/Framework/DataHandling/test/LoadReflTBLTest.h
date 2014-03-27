#ifndef LOADREFLTBLTEST_H_
#define LOADREFLTBLTEST_H_

#include "cxxtest/TestSuite.h"
#include "MantidDataHandling/LoadReflTBL.h"
#include "MantidDataHandling/SaveReflTBL.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/Workspace2D.h"
#include <Poco/File.h>
#include <fstream>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

class LoadReflTBLTest : public CxxTest::TestSuite
{
public:
  static LoadReflTBLTest *createSuite() { return new LoadReflTBLTest(); }
  static void destroySuite(LoadReflTBLTest *suite) { delete suite; }

  LoadReflTBLTest()
  {
    m_filename = "LoadReflTBLTest";
    m_ext = ".tbl";
  }

  ~LoadReflTBLTest()
  {
  }

  void testExec()
  {

  }

private:
  std::string m_filename;
  std::string m_ext;
};


#endif //LOADREFLTBLTEST_H_