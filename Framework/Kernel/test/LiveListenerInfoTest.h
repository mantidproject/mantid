// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef LIVELISTENERINFOTEST_H_
#define LIVELISTENERINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/LiveListenerInfo.h"


#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>

using namespace Mantid::Kernel;

class LiveListenerInfoTest : public CxxTest::TestSuite {
public:
  void test_xml_throws_no_connection() {
    TS_ASSERT_THROWS_NOTHING(createMinimalFacility("<livedata />"));
  }

  void test_xml_empty_connection() {
    const std::string xml = "<livedata>"
                            "<connection />"
                            "</livedata>";

    std::unique_ptr<FacilityInfo> fac;
    TS_ASSERT_THROWS_NOTHING(fac = createMinimalFacility(xml));

    InstrumentInfo inst = fac->instruments().front();
    LiveListenerInfo info = inst.liveListenerInfo();

    TS_ASSERT_EQUALS(info.name(), "");
    TS_ASSERT_EQUALS(info.address(), "");
    TS_ASSERT_EQUALS(info.listener(), "");
  }

  void test_xml_single_connection() {
    const std::string xml = "<livedata>"
                            "<connection name='n' address='a' listener='l' />"
                            "</livedata>";

    std::unique_ptr<FacilityInfo> fac;
    TS_ASSERT_THROWS_NOTHING(fac = createMinimalFacility(xml));

    InstrumentInfo inst = fac->instruments().front();
    LiveListenerInfo info = inst.liveListenerInfo();

    TS_ASSERT_EQUALS(info.name(), "n");
    TS_ASSERT_EQUALS(info.address(), "a");
    TS_ASSERT_EQUALS(info.listener(), "l");
  }

  void test_xml_two_connections() {
    const std::string xml = "<livedata>"
                            "<connection name='n1' address='a' listener='l' />"
                            "<connection name='n2' address='A' listener='L' />"
                            "</livedata>";

    std::unique_ptr<FacilityInfo> fac;
    TS_ASSERT_THROWS_NOTHING(fac = createMinimalFacility(xml));

    InstrumentInfo inst = fac->instruments().front();
    LiveListenerInfo info;

    info = inst.liveListenerInfo();
    TS_ASSERT_EQUALS(info.name(), "n1");
    TS_ASSERT_EQUALS(info.address(), "a");
    TS_ASSERT_EQUALS(info.listener(), "l");

    info = inst.liveListenerInfo("n1");
    TS_ASSERT_EQUALS(info.name(), "n1");
    TS_ASSERT_EQUALS(info.address(), "a");
    TS_ASSERT_EQUALS(info.listener(), "l");

    info = inst.liveListenerInfo("n2");
    TS_ASSERT_EQUALS(info.name(), "n2");
    TS_ASSERT_EQUALS(info.address(), "A");
    TS_ASSERT_EQUALS(info.listener(), "L");

    TS_ASSERT_THROWS(inst.liveListenerInfo("n3"), const std::runtime_error &);
  }

  void test_xml_two_connections_default() {
    const std::string xml = "<livedata default='n2'>"
                            "<connection name='n1' address='a' listener='l' />"
                            "<connection name='n2' address='A' listener='L' />"
                            "</livedata>";

    std::unique_ptr<FacilityInfo> fac = nullptr;
    TS_ASSERT_THROWS_NOTHING(fac = createMinimalFacility(xml));

    InstrumentInfo inst = fac->instruments().front();
    LiveListenerInfo info;

    info = inst.liveListenerInfo();
    TS_ASSERT_EQUALS(info.name(), "n2");
    TS_ASSERT_EQUALS(info.address(), "A");
    TS_ASSERT_EQUALS(info.listener(), "L");

    info = inst.liveListenerInfo("n1");
    TS_ASSERT_EQUALS(info.name(), "n1");
    TS_ASSERT_EQUALS(info.address(), "a");
    TS_ASSERT_EQUALS(info.listener(), "l");

    info = inst.liveListenerInfo("n2");
    TS_ASSERT_EQUALS(info.name(), "n2");
    TS_ASSERT_EQUALS(info.address(), "A");
    TS_ASSERT_EQUALS(info.listener(), "L");

    TS_ASSERT_THROWS(inst.liveListenerInfo("n3"), const std::runtime_error &);
  }

  void test_manual_construction() {
    TS_ASSERT_THROWS_NOTHING(LiveListenerInfo{});

    LiveListenerInfo info;
    TS_ASSERT_EQUALS(info.name(), "");
    TS_ASSERT_EQUALS(info.address(), "");
    TS_ASSERT_EQUALS(info.listener(), "");

    TS_ASSERT_THROWS_NOTHING(info = LiveListenerInfo("l"));
    TS_ASSERT_EQUALS(info.name(), "");
    TS_ASSERT_EQUALS(info.address(), "");
    TS_ASSERT_EQUALS(info.listener(), "l");

    TS_ASSERT_THROWS_NOTHING(info = LiveListenerInfo("l", "a"));
    TS_ASSERT_EQUALS(info.name(), "");
    TS_ASSERT_EQUALS(info.address(), "a");
    TS_ASSERT_EQUALS(info.listener(), "l");

    TS_ASSERT_THROWS_NOTHING(info = LiveListenerInfo("l", "a", "n"));
    TS_ASSERT_EQUALS(info.name(), "n");
    TS_ASSERT_EQUALS(info.address(), "a");
    TS_ASSERT_EQUALS(info.listener(), "l");
  }

  void test_equality() {
    LiveListenerInfo info1("l", "a", "n");
    LiveListenerInfo info2 = info1;

    TS_ASSERT_EQUALS(info1, info2);
    TS_ASSERT_EQUALS(info1.name(), info2.name());
    TS_ASSERT_EQUALS(info1.address(), info2.address());
    TS_ASSERT_EQUALS(info1.listener(), info2.listener());

    LiveListenerInfo info3;
    TS_ASSERT_DIFFERS(info1, info3);
  }

private:
  std::unique_ptr<FacilityInfo>
  createMinimalFacility(const std::string &livedataXml) {
    const std::string xmlStr =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<facilities>"
        "  <facility name=\"MyFacility\" FileExtensions=\".xyz\">"
        "    <instrument name=\"INST\">"
        "      <technique>Technique</technique>" +
        livedataXml +
        "    </instrument>"
        "  </facility>"
        "</facilities>";

    return createFacility(xmlStr);
  }

  std::unique_ptr<FacilityInfo> createFacility(const std::string &xml) {
    Poco::XML::DOMParser parser;
    Poco::AutoPtr<Poco::XML::Document> pDoc = parser.parseString(xml);
    Poco::XML::Element *pRootElem = pDoc->documentElement();
    Poco::XML::Element *elem = pRootElem->getChildElement("facility");

    return std::make_unique<FacilityInfo>(elem);
  }
};

#endif // LIVELISTENERINFOTEST_H_
