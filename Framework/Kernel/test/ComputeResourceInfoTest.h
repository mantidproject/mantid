// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef COMPUTERESOURCEINFOTEST_H_
#define COMPUTERESOURCEINFOTEST_H_

#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/XML/XMLException.h>
#include <boost/make_shared.hpp>

using namespace Mantid::Kernel;

class ComputeResourceInfoTest : public CxxTest::TestSuite {
public:
  void test_allMissing() {
    boost::shared_ptr<FacilityInfo> fac;
    TS_ASSERT_THROWS_NOTHING(fac =
                                 createCRInfoInMinimalFacility(simpleInstStr));
    TS_ASSERT(fac);
    std::vector<ComputeResourceInfo> cri;
    TS_ASSERT_THROWS_NOTHING(cri = fac->computeResInfos());
    TS_ASSERT_EQUALS(cri.size(), 0);

    boost::shared_ptr<FacilityInfo> another;
    TS_ASSERT_THROWS(another = createCRInfoInMinimalFacility(
                         "<computeResource fooAtt=\"barVal\"/>"),
                     const std::runtime_error &);
    TS_ASSERT(!another);
  }

  void test_noURLTag() {
    const std::string crTxt = "<computeResource name=\"foo\">"
                              "<u>" +
                              fermiURL +
                              "</u>"
                              "</computeResource>";
    boost::shared_ptr<FacilityInfo> fac;
    TS_ASSERT_THROWS(fac = createCRInfoInMinimalFacility(crTxt),
                     const std::runtime_error &);
    TS_ASSERT(!fac);
  }

  void test_wrongXML() {
    const std::string crTxt = "<computeResource name=\"foo\">"
                              "<u_foo>" +
                              fermiURL +
                              "</u_bar>"
                              "</compResource>";
    boost::shared_ptr<FacilityInfo> fac;
    TS_ASSERT_THROWS(fac = createCRInfoInMinimalFacility(crTxt),
                     const Poco::XML::XMLException &);
    TS_ASSERT(!fac);
  }

  void test_normalFermi() {
    const std::string fermi = "<computeResource name=\"" + fermiName +
                              "\">"
                              "<baseURL>" +
                              fermiURL +
                              "</baseURL>"
                              "</computeResource>";

    boost::shared_ptr<FacilityInfo> fac;
    TS_ASSERT_THROWS_NOTHING(fac = createCRInfoInMinimalFacility(fermi));
    TS_ASSERT(fac);
    TS_ASSERT_EQUALS(fac->name(), this->testFacilityName);

    std::vector<ComputeResourceInfo> cri;
    TS_ASSERT_THROWS_NOTHING(cri = fac->computeResInfos());
    TS_ASSERT_EQUALS(cri.size(), 1);

    ComputeResourceInfo cr = fac->computeResInfos().front();
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
    const std::string fermi = "<computeResource name=\"" + fermiName +
                              "\">"
                              "<URL>" +
                              fermiURL +
                              "</URL>"
                              "</computeResource>";

    boost::shared_ptr<FacilityInfo> fac;
    TS_ASSERT_THROWS(fac = createCRInfoInMinimalFacility(fermi),
                     const std::runtime_error &);

    TS_ASSERT(!fac);
  }

  void test_equals() {
    const std::string otherName = "other";
    const std::string otherURL = "www.example.com/foo/baz";
    const std::string thirdName = "third";
    const std::string rep = "<computeResource name=\"" + fermiName +
                            "\">"
                            "<baseURL>" +
                            fermiURL +
                            "</baseURL>"
                            "</computeResource>"

                            "<computeResource name=\"" +
                            otherName +
                            "\">"
                            "<baseURL>" +
                            otherURL +
                            "</baseURL>"
                            "</computeResource>"

                            "<computeResource name=\"" +
                            thirdName +
                            "\">"
                            "<baseURL>" +
                            fermiURL +
                            "</baseURL>"
                            "</computeResource>"

                            "<computeResource name=\"" +
                            fermiName +
                            "\">"
                            "<baseURL>" +
                            fermiURL +
                            "</baseURL>"
                            "</computeResource>";

    boost::shared_ptr<FacilityInfo> fac;
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
  }

private:
  /// make a minimal facilities file/xml string includin the compute resource
  /// passed
  boost::shared_ptr<FacilityInfo>
  createCRInfoInMinimalFacility(const std::string &crStr) {
    const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                               "<facilities>"
                               "  <facility name=\"" +
                               testFacilityName +
                               R"(" FileExtensions=".xyz">)" + simpleInstStr +
                               crStr +
                               "  </facility>"
                               "</facilities>";

    return createFacility(xmlStr);
  }

  boost::shared_ptr<FacilityInfo> createFacility(const std::string &xml) {
    Poco::XML::DOMParser parser;
    Poco::AutoPtr<Poco::XML::Document> pDoc = parser.parseString(xml);
    Poco::XML::Element *pRootElem = pDoc->documentElement();
    Poco::XML::Element *elem = pRootElem->getChildElement("facility");

    return boost::make_shared<FacilityInfo>(elem);
  }

private:
  // a minimal instrument to create a facility info
  static const std::string simpleInstStr;

  // default remote job manager type
  static const std::string defaultType;

  static const std::string testFacilityName;

  static const std::string fermiName;
  static const std::string fermiURL;
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

#endif // COMPUTERESOURCEINFOTEST_H_
