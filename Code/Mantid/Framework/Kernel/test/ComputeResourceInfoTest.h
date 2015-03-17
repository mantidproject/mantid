#ifndef COMPUTERESOURCEINFOTEST_H_
#define COMPUTERESOURCEINFOTEST_H_

#include "MantidKernel/FacilityInfo.h"

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMParser.h>

using namespace Mantid::Kernel;

class ComputeResourceInfoTest : public CxxTest::TestSuite {
public:
  void test_noComputeResource() {
    FacilityInfo *fac = NULL;
    TS_ASSERT_THROWS_NOTHING(fac = createCRInfoInMinimalFacility(""));

    TS_ASSERT_THROWS_NOTHING(
        fac = createCRInfoInMinimalFacility("<computeResource />"));

    ComputeResourceInfo cr = fac->computeResInfos().front();
  }

  void test_normalFermi() {
    const std::string fermi = "<computeResource name=\"" + fermiName +
                              "\">"
                              "<baseURL>" +
                              fermiURL + "</baseURL>"
                                         "</computeResource>";

    FacilityInfo *fac = NULL;
    TS_ASSERT_THROWS_NOTHING(fac = createCRInfoInMinimalFacility(fermi));

    ComputeResourceInfo cr = fac->computeResInfos().front();
  }

  void test_brokenFermi() {
    // wrong 'baseURL' tag
    const std::string fermi = "<computeResource name=\"" + fermiName + "\">"
                                                                       "<URL>" +
                              fermiURL + "</URL>"
                                         "</computeResource>";

    FacilityInfo *fac = NULL;
    TS_ASSERT_THROWS(fac = createCRInfoInMinimalFacility(fermi),
                     std::runtime_error);

    ComputeResourceInfo cr = fac->computeResInfos().front();
  }

  void test_normalSCARF() {
    const std::string type = "SCARFLSFJobManager";
    const std::string scarf = "<computeResource name=\"" + scarfName +
                              "\" JobManagerType=\"" + type + "\">"
                                                              "<baseURL>" +
                              scarfURL + "</baseURL>"
                                    "</computeResource>";

    FacilityInfo *fac = NULL;
    TS_ASSERT_THROWS_NOTHING(fac = createCRInfoInMinimalFacility(scarf));

    ComputeResourceInfo cr = fac->computeResInfos().front();
  }

  void test_brokenSCARF() {}

private:
  /// make a minimal facilities file/xml string includin the compute resource
  /// passed
  FacilityInfo *createCRInfoInMinimalFacility(const std::string &crStr) {
    const std::string xmlStr =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<facilities>"
        "  <facility name=\"ATestFacility\" FileExtensions=\".xyz\">" +
        crStr + "  </facility>"
                "</facilities>";

    return createFacility(xmlStr);
  }

  FacilityInfo *createFacility(const std::string &xml) {
    Poco::XML::DOMParser parser;
    Poco::AutoPtr<Poco::XML::Document> pDoc = parser.parseString(xml);
    Poco::XML::Element *pRootElem = pDoc->documentElement();
    Poco::XML::Element *elem = pRootElem->getChildElement("facility");

    return new FacilityInfo(elem);
  }

private:
  static const std::string fermiName;
  static const std::string fermiURL;
  static const std::string scarfName;
  static const std::string scarfURL;
};

const std::string ComputeResourceInfoTest::fermiURL =
    "https://fermi.ornl.gov/MantidRemote";
const std::string ComputeResourceInfoTest::fermiName = "Fermi";
const std::string ComputeResourceInfoTest::scarfURL =
    "https://portal.scarf.rl.ac.uk";
const std::string ComputeResourceInfoTest::scarfName = "SCARF@STFC";

#endif // COMPUTERESOURCEINFOTEST_H_