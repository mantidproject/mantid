#ifndef INSTRUMENTINFOTEST_H_
#define INSTRUMENTINFOTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/FacilityInfo.h"
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/AutoPtr.h>

using namespace Mantid::Kernel;

class InstrumentInfoTest : public CxxTest::TestSuite
{
public:
  void test_throws_if_unnamed_instrument()
  {
    TS_ASSERT_THROWS( createInstInfoInMinimalFacility("<instrument />"), std::runtime_error );
  }
  
  void test_throws_if_no_techinque_given()
  {
    TS_ASSERT_THROWS( createInstInfoInMinimalFacility("<instrument name=\"inst\"/>"), std::runtime_error );
  }

  void test_mostly_default_instrument()
  {
    const std::string instStr = "<instrument name=\"AnInst\">"
                                "  <technique>Measuring Stuff</technique>"
                                "</instrument>";

    FacilityInfo * fac = NULL;
    TS_ASSERT_THROWS_NOTHING( fac = createInstInfoInMinimalFacility(instStr) );
    
    InstrumentInfo inst = fac->instruments().front();

    TS_ASSERT_EQUALS( inst.name(), "AnInst" );
    TS_ASSERT_EQUALS( inst.shortName(), "AnInst" );
    TS_ASSERT_EQUALS( inst.zeroPadding(), 0 );
    TS_ASSERT( inst.delimiter().empty() );
    TS_ASSERT( inst.liveListener().empty() );
    TS_ASSERT( inst.liveDataAddress().empty() );
    TS_ASSERT_EQUALS( inst.techniques().size(), 1);
    TS_ASSERT_EQUALS( *inst.techniques().begin(), "Measuring Stuff" );
    TS_ASSERT_EQUALS( &inst.facility(), fac );

    delete fac;
  }
  
  void test_picks_up_facilityinfo_defaults()
  {
    const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<facilities>"
      "  <facility name=\"MyFacility\" zeropadding=\"99\" delimiter=\"!\" FileExtensions=\".xyz\">"
      "    <livedata listener=\"I'm listening\" />"
      "    <instrument name=\"AnInst\">"
      "      <livedata address=\"127.0.0.1:99\" />"
      "      <technique>Measuring Stuff</technique>"
      "    </instrument>"
      "  </facility>"
      "</facilities>";

    FacilityInfo * fac = NULL;
    TS_ASSERT_THROWS_NOTHING( fac = createFacility(xmlStr) );

    InstrumentInfo inst = fac->instruments().front();

    TS_ASSERT_EQUALS( inst.zeroPadding(), 99 );
    TS_ASSERT_EQUALS( inst.delimiter(), "!" );
    TS_ASSERT_EQUALS( inst.liveListener(), "I'm listening" );

    delete fac;
  }
  
  void test_instrument_values_override_facilityinfo_defaults()
  {
    const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<facilities>"
      "  <facility name=\"MyFacility\" zeropadding=\"99\" delimiter=\"!\" FileExtensions=\".xyz\">"
      "    <livedata listener=\"I'm listening\" />"
      "    <instrument name=\"AnInst\" zeropadding=\"66\" delimiter=\"?\" >"
      "      <livedata listener=\"pardon\" />"
      "      <technique>Measuring Stuff</technique>"
      "    </instrument>"
      "  </facility>"
      "</facilities>";

    FacilityInfo * fac = NULL;
    TS_ASSERT_THROWS_NOTHING( fac = createFacility(xmlStr) );

    InstrumentInfo inst = fac->instruments().front();

    TS_ASSERT_EQUALS( inst.zeroPadding(), 66 );
    TS_ASSERT_EQUALS( inst.delimiter(), "?" );
    TS_ASSERT_EQUALS( inst.liveListener(), "pardon" );

    delete fac;
  }

  void test_setting_all_aspects_of_instrument()
  {
    const std::string instStr =
      "<instrument name=\"MyInst\" shortname=\"mine\" zeropadding=\"8\" delimiter=\":\" >"
      "  <livedata listener=\"AListener\" address=\"myinst.facility.gov:99\" />"
      "  <technique>Measuring Stuff</technique>"
      "  <technique>Doing Stuff</technique>"
      "</instrument>";

    FacilityInfo * fac = NULL;
    TS_ASSERT_THROWS_NOTHING( fac = createInstInfoInMinimalFacility(instStr) );
    
    InstrumentInfo inst = fac->instruments().front();

    TS_ASSERT_EQUALS( inst.name(), "MyInst" );
    TS_ASSERT_EQUALS( inst.shortName(), "mine" );
    TS_ASSERT_EQUALS( inst.zeroPadding(), 8 );
    TS_ASSERT_EQUALS( inst.delimiter(), ":" );
    TS_ASSERT_EQUALS( inst.liveListener(), "AListener" );
    TS_ASSERT_EQUALS( inst.liveDataAddress(), "myinst.facility.gov:99" );
    auto techniques = inst.techniques();
    auto tech_it = techniques.begin();
    TS_ASSERT_EQUALS( techniques.size(), 2);
    TS_ASSERT_EQUALS( *tech_it, "Doing Stuff" );
    TS_ASSERT_EQUALS( *++tech_it, "Measuring Stuff" );
    TS_ASSERT_EQUALS( &inst.facility(), fac );

    std::stringstream ss;
    ss << inst;
    TS_ASSERT_EQUALS( ss.str(), "MyInst" );

    delete fac;
  }
  
  void test_equality_operator()
  {
    const std::string instStr = "<instrument name=\"AnInst\">"
                                "  <technique>Measuring Stuff</technique>"
                                "</instrument>"
                                "<instrument name=\"AnInst\" shortname=\"inst\">"
                                "  <livedata listener=\"AListener\" address=\"127.0.0.1:99\" />"
                                "  <technique>Doing Stuff</technique>"
                                "</instrument>"
                                "<instrument name=\"AnInst\" shortname=\"inst\" zeropadding=\"8\" delimiter=\":\">"
                                "  <technique>Measuring Stuff</technique>"
                                "  <technique>Doing Stuff</technique>"
                                "</instrument>";

    FacilityInfo * fac = NULL;
    TS_ASSERT_THROWS_NOTHING( fac = createInstInfoInMinimalFacility(instStr) );

    TS_ASSERT( fac->instruments()[0] == fac->instruments()[0] );
    TS_ASSERT( !(fac->instruments()[0] == fac->instruments()[1]) );
    TS_ASSERT( !(fac->instruments()[0] == fac->instruments()[2]) );
    TS_ASSERT( fac->instruments()[1] == fac->instruments()[2] );
    TS_ASSERT( fac->instruments()[2] == fac->instruments()[1] );

    delete fac;
  }

private:

  FacilityInfo * createInstInfoInMinimalFacility(const std::string& instStr)
  {
    const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<facilities>"
      "  <facility name=\"MyFacility\" FileExtensions=\".xyz\">"
      + instStr +
      "  </facility>"
      "</facilities>";

    return createFacility(xmlStr);
  }

  FacilityInfo * createFacility(const std::string& xml)
  {
    Poco::XML::DOMParser parser;
    Poco::AutoPtr<Poco::XML::Document> pDoc = parser.parseString(xml);
    Poco::XML::Element* pRootElem = pDoc->documentElement();
    Poco::XML::Element* elem = pRootElem->getChildElement("facility");

    return new FacilityInfo(elem);
  }

};
#endif /*MANTID_FACILITIESTEST_H_*/
