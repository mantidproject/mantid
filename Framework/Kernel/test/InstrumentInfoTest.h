// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InstrumentInfo.h"

#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>

using namespace Mantid::Kernel;

class InstrumentInfoTest : public CxxTest::TestSuite {
public:
  void test_throws_if_unnamed_instrument() {
    TS_ASSERT_THROWS(createInstInfoInMinimalFacility("<instrument />"), const std::runtime_error &);
  }

  void test_throws_if_no_techinque_given() {
    TS_ASSERT_THROWS(createInstInfoInMinimalFacility("<instrument name=\"inst\"/>"), const std::runtime_error &);
  }

  void test_mostly_default_instrument() {
    const std::string instStr = "<instrument name=\"AnInst\">"
                                "  <technique>Measuring Stuff</technique>"
                                "</instrument>";

    FacilityInfo *fac = nullptr;
    TS_ASSERT_THROWS_NOTHING(fac = createInstInfoInMinimalFacility(instStr));

    InstrumentInfo inst = fac->instruments().front();

    TS_ASSERT_EQUALS(inst.name(), "AnInst");
    TS_ASSERT_EQUALS(inst.shortName(), "AnInst");
    TS_ASSERT_EQUALS(inst.zeroPadding(123), fac->zeroPadding());
    TS_ASSERT(inst.delimiter().empty());
    TS_ASSERT(inst.liveListener().empty());
    TS_ASSERT(inst.liveDataAddress().empty());
    TS_ASSERT_EQUALS(inst.techniques().size(), 1);
    TS_ASSERT_EQUALS(*inst.techniques().begin(), "Measuring Stuff");
    TS_ASSERT_EQUALS(&inst.facility(), fac);

    delete fac;
  }

  void test_picks_up_facilityinfo_defaults() {
    const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                               "<facilities>"
                               "  <facility name=\"MyFacility\" zeropadding=\"99\" delimiter=\"!\" "
                               "FileExtensions=\".xyz\">"
                               "    <instrument name=\"AnInst\">"
                               "      <technique>Measuring Stuff</technique>"
                               "    </instrument>"
                               "  </facility>"
                               "</facilities>";

    FacilityInfo *fac = nullptr;
    TS_ASSERT_THROWS_NOTHING(fac = createFacility(xmlStr));

    InstrumentInfo inst = fac->instruments().front();

    TS_ASSERT_EQUALS(inst.zeroPadding(123), 99);
    TS_ASSERT_EQUALS(inst.delimiter(), "!");

    delete fac;
  }

  void test_instrument_values_override_facilityinfo_defaults() {
    const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                               "<facilities>"
                               "  <facility name=\"MyFacility\" zeropadding=\"99\" delimiter=\"!\" "
                               "FileExtensions=\".xyz\">"
                               "    <instrument name=\"AnInst\" delimiter=\"?\" >"
                               "      <zeropadding size=\"66\"/>"
                               "      <technique>Measuring Stuff</technique>"
                               "    </instrument>"
                               "  </facility>"
                               "</facilities>";

    FacilityInfo *fac = nullptr;
    TS_ASSERT_THROWS_NOTHING(fac = createFacility(xmlStr));

    InstrumentInfo inst = fac->instruments().front();

    TS_ASSERT_EQUALS(inst.zeroPadding(123), 66);
    TS_ASSERT_EQUALS(inst.delimiter(), "?");

    delete fac;
  }

  void test_setting_all_aspects_of_instrument() {
    const std::string instStr = "<instrument name=\"MyInst\" shortname=\"mine\" delimiter=\":\" >"
                                "  <zeropadding size=\"8\"/>"
                                "  <livedata>"
                                "    <connection name=\"default\" "
                                "                listener=\"AListener\" "
                                "                address=\"myinst.facility.gov:99\" />"
                                "  </livedata>"
                                "  <technique>Measuring Stuff</technique>"
                                "  <technique>Doing Stuff</technique>"
                                "</instrument>";

    FacilityInfo *fac = nullptr;
    TS_ASSERT_THROWS_NOTHING(fac = createInstInfoInMinimalFacility(instStr));

    InstrumentInfo inst = fac->instruments().front();

    TS_ASSERT_EQUALS(inst.name(), "MyInst");
    TS_ASSERT_EQUALS(inst.shortName(), "mine");
    TS_ASSERT_EQUALS(inst.zeroPadding(123), 8);
    TS_ASSERT_EQUALS(inst.delimiter(), ":");
    TS_ASSERT_EQUALS(inst.liveListener(), "AListener");
    TS_ASSERT_EQUALS(inst.liveDataAddress(), "myinst.facility.gov:99");
    auto techniques = inst.techniques();
    auto tech_it_end = techniques.end();
    TS_ASSERT_EQUALS(techniques.size(), 2);
    TSM_ASSERT_DIFFERS("Techniques should contain 'Doing Stuff'", techniques.find("Doing Stuff"), tech_it_end)
    TSM_ASSERT_DIFFERS("Techniques should contain 'Measuring Stuff'", techniques.find("Measuring Stuff"), tech_it_end)
    TS_ASSERT_EQUALS(&inst.facility(), fac);

    std::stringstream ss;
    ss << inst;
    TS_ASSERT_EQUALS(ss.str(), "MyInst");

    delete fac;
  }

  void test_multiple_zeropadding() {
    const std::string instStr = "<instrument name=\"MyInst\" shortname=\"mine\" delimiter=\":\" >"
                                "  <technique>Measuring Stuff</technique>"
                                "  <zeropadding size=\"8\"/>"
                                "  <zeropadding size=\"15\" startRunNumber=\"123\" "
                                "prefix=\"MyNewInstrument\"/>"
                                "  <zeropadding size=\"20\" startRunNumber=\"321\"/>"
                                "</instrument>";

    FacilityInfo *fac = nullptr;
    TS_ASSERT_THROWS_NOTHING(fac = createInstInfoInMinimalFacility(instStr));

    InstrumentInfo inst = fac->instruments().front();

    TS_ASSERT_EQUALS(inst.zeroPadding(0), 8);
    TS_ASSERT_EQUALS(inst.zeroPadding(12), 8);
    TS_ASSERT_EQUALS(inst.zeroPadding(122), 8);
    TS_ASSERT_EQUALS(inst.zeroPadding(123), 15);
    TS_ASSERT_EQUALS(inst.zeroPadding(130), 15);
    TS_ASSERT_EQUALS(inst.zeroPadding(320), 15);
    TS_ASSERT_EQUALS(inst.zeroPadding(321), 20);
    TS_ASSERT_EQUALS(inst.zeroPadding(32100), 20);

    TS_ASSERT_EQUALS(inst.filePrefix(0), "mine");
    TS_ASSERT_EQUALS(inst.filePrefix(12), "mine");
    TS_ASSERT_EQUALS(inst.filePrefix(122), "mine");
    TS_ASSERT_EQUALS(inst.filePrefix(123), "MyNewInstrument");
    TS_ASSERT_EQUALS(inst.filePrefix(130), "MyNewInstrument");
    TS_ASSERT_EQUALS(inst.filePrefix(320), "MyNewInstrument");
    TS_ASSERT_EQUALS(inst.filePrefix(321), "mine");
    TS_ASSERT_EQUALS(inst.filePrefix(32100), "mine");

    delete fac;
  }

  void test_error_in_multiple_zeropadding() {
    const std::string instStr = "<instrument name=\"MyInst\" shortname=\"mine\" delimiter=\":\" >"
                                "  <technique>Measuring Stuff</technique>"
                                "  <zeropadding size=\"8\"/>"
                                "  <zeropadding size=\"15\"/>"
                                "</instrument>";

    TS_ASSERT_THROWS(createInstInfoInMinimalFacility(instStr), const std::runtime_error &);
  }

  void test_error_1_in_multiple_zeropadding() {
    const std::string instStr = "<instrument name=\"MyInst\" shortname=\"mine\" delimiter=\":\" >"
                                "  <technique>Measuring Stuff</technique>"
                                "  <zeropadding size=\"nan\"/>"
                                "</instrument>";

    TS_ASSERT_THROWS(createInstInfoInMinimalFacility(instStr), const std::runtime_error &);
  }

  void test_error_2_in_multiple_zeropadding() {
    const std::string instStr = "<instrument name=\"MyInst\" shortname=\"mine\" delimiter=\":\" >"
                                "  <technique>Measuring Stuff</technique>"
                                "  <zeropadding size=\"8\" startRunNumber=\"nan\"/>"
                                "</instrument>";

    TS_ASSERT_THROWS(createInstInfoInMinimalFacility(instStr), const std::runtime_error &);
  }

  void test_error_3_in_multiple_zeropadding() {
    const std::string instStr = "<instrument name=\"MyInst\" shortname=\"mine\" delimiter=\":\" >"
                                "  <technique>Measuring Stuff</technique>"
                                "  <zeropadding startRunNumber=\"333\"/>"
                                "</instrument>";

    TS_ASSERT_THROWS(createInstInfoInMinimalFacility(instStr), const std::runtime_error &);
  }

  void test_equality_operator() {
    const std::string instStr = "<instrument name=\"AnInst\">"
                                "  <technique>Measuring Stuff</technique>"
                                "</instrument>"
                                "<instrument name=\"AnInst\" shortname=\"inst\">"
                                "  <livedata listener=\"AListener\" address=\"127.0.0.1:99\" />"
                                "  <technique>Doing Stuff</technique>"
                                "</instrument>"
                                "<instrument name=\"AnInst\" shortname=\"inst\" zeropadding=\"8\" "
                                "delimiter=\":\">"
                                "  <technique>Measuring Stuff</technique>"
                                "  <technique>Doing Stuff</technique>"
                                "</instrument>";

    FacilityInfo *fac = nullptr;
    TS_ASSERT_THROWS_NOTHING(fac = createInstInfoInMinimalFacility(instStr));

    TS_ASSERT(fac->instruments()[0] == fac->instruments()[0]);
    TS_ASSERT(!(fac->instruments()[0] == fac->instruments()[1]));
    TS_ASSERT(!(fac->instruments()[0] == fac->instruments()[2]));
    TS_ASSERT(fac->instruments()[1] == fac->instruments()[2]);
    TS_ASSERT(fac->instruments()[2] == fac->instruments()[1]);

    delete fac;
  }

private:
  FacilityInfo *createInstInfoInMinimalFacility(const std::string &instStr) {
    const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                               "<facilities>"
                               "  <facility name=\"MyFacility\" FileExtensions=\".xyz\">" +
                               instStr +
                               "  </facility>"
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
};
