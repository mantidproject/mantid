#ifndef COMPUTERESOURCEINFOTEST_H_
#define COMPUTERESOURCEINFOTEST_H_

#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/XML/XMLException.h>

using namespace Mantid::Kernel;

class ComputeResourceInfoTest : public CxxTest::TestSuite {
public:
  void test_allMissing() {
    FacilityInfo *fac = NULL;
    TS_ASSERT_THROWS_NOTHING(fac =
                                 createCRInfoInMinimalFacility(simpleInstStr));
    TS_ASSERT(fac);
    std::vector<ComputeResourceInfo> cri;
    TS_ASSERT_THROWS_NOTHING(cri = fac->computeResInfos());
    TS_ASSERT_EQUALS(cri.size(), 0);

    delete fac;
    fac = NULL;
    TS_ASSERT_THROWS(fac = createCRInfoInMinimalFacility(
                         "<computeResource fooAtt=\"barVal\"/>"),
                     std::runtime_error);
    TS_ASSERT(!fac);
    delete fac;
  }

  void test_noURLTag() {
    const std::string crTxt = "<computeResource name=\"foo\">"
                              "<u>" +
                              fermiURL + "</u>"
                                         "</computeResource>";
    FacilityInfo *fac = NULL;
    TS_ASSERT_THROWS(fac = createCRInfoInMinimalFacility(crTxt),
                     std::runtime_error);
    TS_ASSERT(!fac);
    delete fac;
  }

  void test_wrongXML() {
    const std::string crTxt = "<computeResource name=\"foo\">"
                              "<u_foo>" +
                              fermiURL + "</u_bar>"
                                         "</compResource>";
    FacilityInfo *fac = NULL;
    TS_ASSERT_THROWS(fac = createCRInfoInMinimalFacility(crTxt),
                     Poco::XML::XMLException);
    TS_ASSERT(!fac);
    delete fac;
  }

  void test_normalFermi() {
    const std::string fermi = "<computeResource name=\"" + fermiName +
                              "\">"
                              "<baseURL>" +
                              fermiURL + "</baseURL>"
                                         "</computeResource>";

    FacilityInfo *fac = NULL;
    TS_ASSERT_THROWS_NOTHING(fac = createCRInfoInMinimalFacility(fermi));
    TS_ASSERT(fac);
    TS_ASSERT_EQUALS(fac->name(), this->testFacilityName);

    std::vector<ComputeResourceInfo> cri;
    TS_ASSERT_THROWS_NOTHING(cri = fac->computeResInfos());
    TS_ASSERT_EQUALS(cri.size(), 1);

    ComputeResourceInfo cr = fac->computeResInfos().front();
    TS_ASSERT_THROWS(ComputeResourceInfo fail = fac->computeResource(scarfName),
                     Mantid::Kernel::Exception::NotFoundError);
    ComputeResourceInfo cr2 = fac->computeResource(fermiName);
    TS_ASSERT_EQUALS(cr, cr2);
    TS_ASSERT_EQUALS(cr, cri.front());
    TS_ASSERT_EQUALS(cr2, cri.front());
    TS_ASSERT_EQUALS(cr.name(), fermiName);
    TS_ASSERT_EQUALS(cr2.name(), fermiName);
    TS_ASSERT_EQUALS(cr.baseURL(), fermiURL);
    TS_ASSERT_EQUALS(cr2.baseURL(), fermiURL);
    TS_ASSERT_EQUALS(cr.remoteJobManagerType(), defaultType);
    TS_ASSERT_EQUALS(cr2.remoteJobManagerType(), defaultType);
    TS_ASSERT_EQUALS(cr.facility().name(), fac->name());
    TS_ASSERT_EQUALS(cr2.facility().name(), fac->name());
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

    TS_ASSERT(!fac);
    delete fac;
  }

  void test_normalSCARF() {
    const std::string scarf = "<computeResource name=\"" + scarfName +
                              "\" JobManagerType=\"" + scarfType + "\">"
                                                                   "<baseURL>" +
                              scarfURL + "</baseURL>"
                                         "</computeResource>";

    FacilityInfo *fac = NULL;
    TS_ASSERT_THROWS_NOTHING(fac = createCRInfoInMinimalFacility(scarf));
    TS_ASSERT(fac);
    TS_ASSERT_EQUALS(fac->name(), this->testFacilityName);
    std::vector<ComputeResourceInfo> cri;
    TS_ASSERT_THROWS_NOTHING(cri = fac->computeResInfos());
    TS_ASSERT_EQUALS(cri.size(), 1);

    ComputeResourceInfo cr = fac->computeResInfos().front();
    TS_ASSERT_THROWS(ComputeResourceInfo fail = fac->computeResource("inexistent!"),
                     Mantid::Kernel::Exception::NotFoundError);
    ComputeResourceInfo cr2 = fac->computeResource(scarfName);
    TS_ASSERT_EQUALS(cr, cr2);
    TS_ASSERT_EQUALS(cri.front(), cr);
    TS_ASSERT_EQUALS(cri.front(), cr2);
    TS_ASSERT_EQUALS(scarfName, cr.name());
    TS_ASSERT_EQUALS(scarfName, cr2.name());
    TS_ASSERT_EQUALS(scarfURL, cr.baseURL());
    TS_ASSERT_EQUALS(scarfURL, cr2.baseURL());
    TS_ASSERT_EQUALS(scarfType, cr.remoteJobManagerType());
    TS_ASSERT_EQUALS(scarfType, cr2.remoteJobManagerType());
    TS_ASSERT_EQUALS(fac->name(), cr.facility().name());
    TS_ASSERT_EQUALS(fac->name(), cr2.facility().name());
    delete fac;
  }

  void test_brokenSCARF() {
    const std::string type = "SCARFLSFJobManager";
    const std::string err = "<computeResource foo=\"" + scarfName +
                            "\" JobManagerType=\"" + type + "\">"
                                                            "<URL>" +
                            scarfURL + "</URL>"
                                       "</computeResource>";
    FacilityInfo *fac = NULL;
    TS_ASSERT_THROWS(fac = createCRInfoInMinimalFacility(err),
                     std::runtime_error);
    TS_ASSERT(!fac);
    delete fac;
  }

  void test_equals() {
    const std::string otherName = "other";
    const std::string otherURL = "www.example.com/foo/baz";
    const std::string thirdName = "third";
    const std::string rep = "<computeResource name=\"" + fermiName +
                            "\">"
                            "<baseURL>" +
                            fermiURL + "</baseURL>"
                                       "</computeResource>"

                                       "<computeResource name=\"" +
                            otherName + "\">"
                                        "<baseURL>" +
                            otherURL + "</baseURL>"
                                       "</computeResource>"

                                       "<computeResource name=\"" +
                            thirdName + "\">"
                                        "<baseURL>" +
                            fermiURL + "</baseURL>"
                                       "</computeResource>"

                                       "<computeResource name=\"" +
                            fermiName + "\">"
                                        "<baseURL>" +
                            fermiURL + "</baseURL>"
                                       "</computeResource>";

    FacilityInfo *fac = NULL;
    TS_ASSERT_THROWS_NOTHING(fac = createCRInfoInMinimalFacility(rep));
    TS_ASSERT(fac);
    TS_ASSERT_EQUALS(fac->computeResources().size(), 3);
    TS_ASSERT_EQUALS(fac->computeResInfos().size(), 4);

    // compare names
    TS_ASSERT(fac->computeResources()[0] == fac->computeResources()[0]);
    TS_ASSERT(!(fac->computeResources()[0] == fac->computeResources()[1]));
    TS_ASSERT(!(fac->computeResources()[0] == fac->computeResources()[2]));
    TS_ASSERT(!(fac->computeResources()[1] == fac->computeResources()[2]));

    // compare full comp resource info
    TS_ASSERT(fac->computeResInfos()[0] == fac->computeResInfos()[0]);
    TS_ASSERT(!(fac->computeResInfos()[0] == fac->computeResInfos()[1]));
    TS_ASSERT(!(fac->computeResInfos()[0] == fac->computeResInfos()[2]));
    TS_ASSERT(!(fac->computeResInfos()[1] == fac->computeResInfos()[2]));
    TS_ASSERT(!(fac->computeResInfos()[2] == fac->computeResInfos()[3]));
    TS_ASSERT(fac->computeResInfos()[0] == fac->computeResInfos()[3]);

    // compare comp resource info retrieved by names
    TS_ASSERT(
        !(fac->computeResource(fermiName) == fac->computeResource(otherName)));
    TS_ASSERT(
        !(fac->computeResource(fermiName) == fac->computeResource(thirdName)));
    TS_ASSERT(
        !(fac->computeResource(otherName) == fac->computeResource(thirdName)));
    delete fac;
  }

private:
  /// make a minimal facilities file/xml string includin the compute resource
  /// passed
  FacilityInfo *createCRInfoInMinimalFacility(const std::string &crStr) {
    const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                               "<facilities>"
                               "  <facility name=\"" +
                               testFacilityName +
                               "\" FileExtensions=\".xyz\">" + simpleInstStr +
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
  // a minimal instrument to create a facility info
  static const std::string simpleInstStr;

  // default remote job manager type
  static const std::string defaultType;

  static const std::string testFacilityName;

  static const std::string fermiName;
  static const std::string fermiURL;
  static const std::string scarfName;
  static const std::string scarfURL;
  static const std::string scarfType;
};

const std::string ComputeResourceInfoTest::simpleInstStr =
    "<instrument name=\"AnInst\">"
    "  <technique>Measuring Stuff</technique>"
    "</instrument>";

const std::string ComputeResourceInfoTest::defaultType =
    "MantidWebServiceAPIJobManager";

const std::string ComputeResourceInfoTest::testFacilityName = "ATestFacility";

const std::string ComputeResourceInfoTest::fermiURL =
    "https://fermi.ornl.gov/MantidRemote";
const std::string ComputeResourceInfoTest::fermiName = "Fermi";
const std::string ComputeResourceInfoTest::scarfURL =
    "https://portal.scarf.rl.ac.uk";
const std::string ComputeResourceInfoTest::scarfName = "SCARF@STFC";
const std::string ComputeResourceInfoTest::scarfType = "SCARFLSFJobManager";

#endif // COMPUTERESOURCEINFOTEST_H_
