#ifndef MANTID_API_FILEBACKEDEXPERIMENTINFOTEST_H_
#define MANTID_API_FILEBACKEDEXPERIMENTINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FileBackedExperimentInfo.h"
#include "MantidTestHelpers/NexusTestHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"


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


  void test_toString_method_returns_same_as_parent()
  {
    // Create a helpful file

    NexusTestHelper nexusTestHelper(true);
    nexusTestHelper.createFile("ExperimentInfoTest1.nxs");
    std::string groupName = "experiment0";
    ExperimentInfo ws;

    boost::shared_ptr<Instrument> inst1(new Instrument());
    inst1->setName("GEM");
    inst1->setFilename("GEM_Definition.xml");
    inst1->setXmlText("");
    //boost::shared_ptr<ParameterMap> paramMap = inst1->getParameterMap();
    //paramMap->addParameterFilename("refl_fake.cal");

    ws.setInstrument(inst1);

    ws.saveExperimentInfoNexus(nexusTestHelper.file);

    // Load the file using filebacked experiment info

    FileBackedExperimentInfo fileBackedWS(nexusTestHelper.file, groupName);
    std::string fileBackedParameterStr;
    nexusTestHelper.reopenFile();
    fileBackedWS.loadExperimentInfoNexus(nexusTestHelper.file, fileBackedParameterStr);

    // Load the file using standard experiment info

    ExperimentInfo standardWS;
    std::string standardParameterStr;
    nexusTestHelper.reopenFile();
    standardWS.loadExperimentInfoNexus(nexusTestHelper.file, standardParameterStr);

    // Check results do not differ
    TS_ASSERT_EQUALS(standardWS.toString(), fileBackedWS.toString());
  }


};


#endif /* MANTID_API_FILEBACKEDEXPERIMENTINFOTEST_H_ */
