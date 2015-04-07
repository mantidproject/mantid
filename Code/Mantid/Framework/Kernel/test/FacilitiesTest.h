#ifndef MANTID_FACILITIESTEST_H_
#define MANTID_FACILITIESTEST_H_

#include <cxxtest/TestSuite.h>
#include <fstream>
#include <string>

#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>

using namespace Mantid::Kernel;

class FacilitiesTest : public CxxTest::TestSuite
{
public:
  void test_throws_on_missing_facility_name()
  {
    const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<facilities>"
      "  <facility zeropadding=\"5\" FileExtensions=\".nxs,.raw,.sav,.n*,.s*\">"
      "  </facility>"
      "</facilities>";

    TS_ASSERT_THROWS( getFacility(xmlStr), std::runtime_error );
  }

  void test_throws_if_no_file_extensions()
  {
    const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<facilities>"
      "  <facility name=\"MyFacility\">"
      "  </facility>"
      "</facilities>";

    TS_ASSERT_THROWS( getFacility(xmlStr), std::runtime_error );
  }

  void test_throws_if_no_instruments()
  {
    const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<facilities>"
      "  <facility name=\"MyFacility\" FileExtensions=\".xyz\">"
      "  </facility>"
      "</facilities>";

    TS_ASSERT_THROWS( getFacility(xmlStr), std::runtime_error );
  }

  void test_minimal()
  {
    const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<facilities>"
      "  <facility name=\"MyFacility\" FileExtensions=\".xyz\">"
      "    <instrument name=\"AnInst\">"
      "      <technique>Measuring Stuff</technique>"
      "    </instrument>"
      "  </facility>"
      "</facilities>";

    FacilityInfo* fac = NULL;
    TS_ASSERT_THROWS_NOTHING( fac = getFacility(xmlStr) );

    // Check that the few required things are set and that everything else has its default value
    TS_ASSERT_EQUALS( fac->name(),"MyFacility" );
    TS_ASSERT_EQUALS( fac->zeroPadding(), 0 );
    TS_ASSERT( fac->delimiter().empty() )
    const std::vector<std::string> exts = fac->extensions();
    TS_ASSERT_EQUALS( fac->extensions().size(), 1 );
    TS_ASSERT_EQUALS( fac->extensions()[0],".xyz" );
    TS_ASSERT_EQUALS( fac->preferredExtension(), ".xyz" );
    TS_ASSERT( fac->archiveSearch().empty() );
    TS_ASSERT( fac->liveListener().empty() );
    TS_ASSERT_EQUALS( fac->instruments().size(), 1 );
    TS_ASSERT_EQUALS( fac->instruments().front().name(), "AnInst" );
    TS_ASSERT_EQUALS( fac->instruments("Measuring Stuff").front().name(), "AnInst" );
    TS_ASSERT( fac->instruments("Nonsense").empty() );
    TS_ASSERT_EQUALS( fac->instrument("AnInst").name(), "AnInst" );
    TS_ASSERT_THROWS( fac->instrument("NoInst"), Exception::NotFoundError );

    delete fac;
  }

  void testFacilities()
  {
    const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<facilities>"
      "  <facility name=\"ISIS\" zeropadding=\"5\" delimiter=\"%\" FileExtensions=\".nxs,.raw,.sav,.n*,.s*\">"
      "    <archive>"
      "      <archiveSearch plugin=\"ADataSearch\" />"
      "      <archiveSearch plugin=\"BDataSearch\" />"
      "    </archive>"
      "    <instrument name=\"HRPD\" shortname=\"HRP\">"
      "      <technique>Powder Diffraction</technique>"
      "    </instrument>"
      "    <instrument name=\"WISH\" >"
      "      <zeropadding size=\"8\"/>"
      "      <zeropadding size=\"15\" startRunNumber=\"300\"/>"
      "      <technique>Powder Diffraction</technique>"
      "      <technique>Single Crystal Diffraction</technique>"
      "    </instrument>"
      "  </facility>"
      "</facilities>";

    FacilityInfo* fac = getFacility(xmlStr);

    TS_ASSERT(fac);

    TS_ASSERT_EQUALS(fac->name(),"ISIS");
    TS_ASSERT_EQUALS(fac->zeroPadding(),5);
    TS_ASSERT_EQUALS(fac->delimiter(), "%");
    const std::vector<std::string> exts = fac->extensions();
    TS_ASSERT_EQUALS(exts.size(), 5);
    TS_ASSERT_EQUALS(exts[0],".nxs");
    TS_ASSERT_EQUALS(exts[1],".raw");
    TS_ASSERT_EQUALS(exts[2],".sav");
    TS_ASSERT_EQUALS(exts[3],".n*");
    TS_ASSERT_EQUALS(exts[4],".s*");
    TS_ASSERT_EQUALS(fac->preferredExtension(),".nxs");

    TS_ASSERT_EQUALS(fac->archiveSearch().size(),2);
    std::vector<std::string>::const_iterator it = fac->archiveSearch().begin();
    TS_ASSERT_EQUALS(*it,"ADataSearch");
    TS_ASSERT_EQUALS(*++it,"BDataSearch");

    const std::vector<InstrumentInfo> instrums = fac->instruments();
    TS_ASSERT_EQUALS(instrums.size(),2);

    TS_ASSERT_THROWS_NOTHING(fac->instrument("HRPD"));
    InstrumentInfo instr = fac->instrument("HRP"); // Tests getting by short name
    TS_ASSERT_EQUALS(instr.name(),"HRPD");
    TS_ASSERT_EQUALS(instr.shortName(),"HRP");
    TS_ASSERT_EQUALS(instr.zeroPadding(123),5);

    TS_ASSERT_THROWS_NOTHING(fac->instrument("WISH"));
    instr = fac->instrument("WISH");
    TS_ASSERT_EQUALS(instr.name(),"WISH");
    TS_ASSERT_EQUALS(instr.shortName(),"WISH");
    TS_ASSERT_EQUALS(instr.zeroPadding(123),8);
    TS_ASSERT_EQUALS(instr.zeroPadding(301),15);

    const std::vector<InstrumentInfo> pwdInstr = fac->instruments("Powder Diffraction");
    TS_ASSERT_EQUALS(pwdInstr.size(),2);

    const std::vector<InstrumentInfo> crysInstr = fac->instruments("Single Crystal Diffraction");
    TS_ASSERT_EQUALS(crysInstr.size(),1);
    TS_ASSERT_EQUALS(fac->instruments("rubbish category").size(), 0);

    // Test default live listener is empty
    TS_ASSERT( fac->liveListener().empty() )

    delete fac;
  }

  void testConfigService()
  {
    TS_ASSERT_THROWS_NOTHING(ConfigService::Instance().getFacility("ISIS"));
  }

  void testDefaultInstrument()
  {
    ConfigService::Instance().setString("default.instrument","HRPD");
    InstrumentInfo instr = ConfigService::Instance().getInstrument();
    TS_ASSERT_EQUALS(instr.name(),"HRPD");
  }

  void testFacilitiesArchiveMissing()
  {
    const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<facilities>"
      "  <facility name=\"ISIS\" zeropadding=\"5\" FileExtensions=\".nxs,.raw,.sav,.n*,.s*\">"
      "    <instrument name=\"HRPD\" shortname=\"HRP\">"
      "      <technique>Powder Diffraction</technique>"
      "    </instrument>"
      "    <instrument name=\"WISH\" zeropadding=\"8\">"
      "      <technique>Powder Diffraction</technique>"
      "      <technique>Single Crystal Diffraction</technique>"
      "    </instrument>"
      "  </facility>"
      "</facilities>";
    
    FacilityInfo* fac = getFacility(xmlStr);

    TS_ASSERT(fac);

    TS_ASSERT_EQUALS(fac->name(),"ISIS");
    TS_ASSERT_EQUALS(fac->archiveSearch().size(),0);

    delete fac;
  }

  void testListener()
  {
    const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<facilities>"
      "  <facility name=\"TESTER\" FileExtensions=\"*.*\" >"
      "    <livedata listener=\"Listener1\" />"
      "    <instrument name=\"ABCD\" >"
      "      <livedata listener=\"Listener2\" />"
      "      <technique>None</technique>"
      "    </instrument>"
      "  </facility>"
      "</facilities>";

    FacilityInfo* fac = getFacility(xmlStr);
    TS_ASSERT(fac);
    TS_ASSERT_EQUALS( fac->liveListener(), "Listener1" );
    delete fac;
  }

private:

  FacilityInfo* getFacility(const std::string& xmlStr)const
  {
    Poco::XML::DOMParser parser;
    Poco::AutoPtr<Poco::XML::Document> pDoc = parser.parseString(xmlStr);
    TS_ASSERT(pDoc);

    Poco::XML::Element* pRootElem = pDoc->documentElement();
    Poco::AutoPtr<Poco::XML::NodeList> pNL_facility = pRootElem->getElementsByTagName("facility");
    size_t n = pNL_facility->length();

    TS_ASSERT(n > 0);

    Poco::XML::Element* elem = dynamic_cast<Poco::XML::Element*>(pNL_facility->item(0));
    TS_ASSERT(elem);

    FacilityInfo * facility = new FacilityInfo(elem);

    return facility;
  }

};

#endif /*MANTID_FACILITIESTEST_H_*/
