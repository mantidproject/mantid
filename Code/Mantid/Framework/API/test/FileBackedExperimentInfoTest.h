#ifndef MANTID_API_FILEBACKEDEXPERIMENTINFOTEST_H_
#define MANTID_API_FILEBACKEDEXPERIMENTINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FileBackedExperimentInfo.h"
#include "MantidTestHelpers/NexusTestHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidAPI/FileFinder.h"

#include <nexus/NeXusFile.hpp>

using Mantid::API::FileBackedExperimentInfo;

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class FileBackedExperimentInfoTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FileBackedExperimentInfoTest *createSuite() { return new FileBackedExperimentInfoTest(); }
  static void destroySuite( FileBackedExperimentInfoTest *suite ) { delete suite; }

  void test_toString_method_returns_same_as_ExperimentInfo_class()
  {
    // Find an MD file to use, and MD file is fine
    std::string filename = FileFinder::Instance().getFullPath("TOPAZ_3680_5_sec_MDEW.nxs");

    // Filebacked ExperimentInfo
    //
    // Load the file we want to use
    ::NeXus::File *nexusFileBacked = new ::NeXus::File(filename, NXACC_READ);
    nexusFileBacked->openGroup("MDEventWorkspace", "NXentry");

    // Create the file backed experiment info, shouldn't be loaded yet
    FileBackedExperimentInfo fileBackedWS(nexusFileBacked, "experiment0");

    // Standard ExperimentInfo
    //
    // Load the file again, so what we did before does not affect it
    ::NeXus::File *nexusFile = new ::NeXus::File(filename, NXACC_READ);
    nexusFile->openGroup("MDEventWorkspace", "NXentry");

    // Actually do the loading here
    ExperimentInfo standardWS;
    std::string standardParameterStr;
    nexusFile->openGroup("experiment0", "NXgroup");
    standardWS.loadExperimentInfoNexus(nexusFile, standardParameterStr);

    TS_ASSERT_EQUALS(standardWS.toString(), fileBackedWS.toString());
  }

};


#endif /* MANTID_API_FILEBACKEDEXPERIMENTINFOTEST_H_ */
