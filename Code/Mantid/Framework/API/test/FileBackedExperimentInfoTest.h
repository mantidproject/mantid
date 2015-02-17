#ifndef MANTID_API_FILEBACKEDEXPERIMENTINFOTEST_H_
#define MANTID_API_FILEBACKEDEXPERIMENTINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FileBackedExperimentInfo.h"

using Mantid::API::FileBackedExperimentInfo;
using namespace Mantid::API;

class FileBackedExperimentInfoTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FileBackedExperimentInfoTest *createSuite() { return new FileBackedExperimentInfoTest(); }
  static void destroySuite( FileBackedExperimentInfoTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_API_FILEBACKEDEXPERIMENTINFOTEST_H_ */