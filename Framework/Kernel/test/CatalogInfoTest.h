#ifndef MANTID_CATALOGINFOTEST_H_
#define MANTID_CATALOGINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/CatalogInfo.h"
#include "MantidKernel/ConfigService.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>

using namespace Mantid::Kernel;

class CatalogInfoTest : public CxxTest::TestSuite {
public:
  /// Tests that the related attributes get set when object is created.
  void testConstructCatalogInfo() {
    const std::string facilitiesXml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<facilities>"
        "<facility name=\"ISIS\">"
        "<catalog name=\"ICat3Catalog\">"
        "<soapendpoint "
        "url=\"https://facilities01.esc.rl.ac.uk:443/ICATService/ICAT\"></"
        "soapendpoint>"
        "<filelocation>"
        "<prefix regex=\"\\\\\\\\isis\\\\inst\\$\\\\Instruments\\$\"></prefix>"
        "<windows replacement=\"\"></windows>"
        "<linux replacement=\"/archive\"></linux>"
        "<mac replacement=\"/archive\"></mac>"
        "</filelocation>"
        "</catalog>"
        "</facility>"
        "</facilities>";

    CatalogInfo *catalogInfo = NULL;
    TS_ASSERT_THROWS_NOTHING(catalogInfo = getCatalogFromXML(facilitiesXml));

    TS_ASSERT_EQUALS(catalogInfo->catalogName(), "ICat3Catalog");
    TS_ASSERT_EQUALS(catalogInfo->soapEndPoint(),
                     "https://facilities01.esc.rl.ac.uk:443/ICATService/ICAT");
    // The regex needs escaped in order to work correctly. The output should be
    // same as Facilities.xml (\\\\isis\\inst\$\\Instruments\$)
    TS_ASSERT_EQUALS(catalogInfo->catalogPrefix(),
                     "\\\\\\\\isis\\\\inst\\$\\\\Instruments\\$");
    TS_ASSERT_EQUALS(catalogInfo->windowsPrefix(), "");
    TS_ASSERT_EQUALS(catalogInfo->macPrefix(), "/archive");
    TS_ASSERT_EQUALS(catalogInfo->linuxPrefix(), "/archive");

    delete catalogInfo;
  }

  /// Test transformation of possible combinations of archive paths.
  /// Ensures the path is transformed into the correct OS dependent path.
  void testTransformPath() {
    const std::string facilitiesXml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<facilities>"
        "<facility name=\"ISIS\">"
        "<catalog name=\"ICat3Catalog\">"
        "<soapendpoint "
        "url=\"https://facilities01.esc.rl.ac.uk:443/"
        "ICATService/ICAT\"></soapendpoint>"
        "<filelocation>"
        "<prefix "
        "regex=\"\\\\\\\\isis\\\\inst\\$\\\\Instruments\\$\"></"
        "prefix>" // Same regex from facilities.xml, lots of "\"
                  // character escaping.
        "<windows replacement=\"\"></windows>"
        "<linux replacement=\"/archive\"></linux>"
        "<mac replacement=\"/archive\"></mac>"
        "</filelocation>"
        "</catalog>"
        "</facility>"
        "</facilities>";

    CatalogInfo *catalogInfo = NULL;
    TS_ASSERT_THROWS_NOTHING(catalogInfo = getCatalogFromXML(facilitiesXml));

    // Set the paths to test against.
    std::string linuxPrefixPath =
        "/archive/NDXSANDALS/Instrument/data/cycle_05_3/ALF06716.LOG";
    std::string macPrefixPath =
        "/archive/NDXSANDALS/Instrument/data/cycle_05_3/ALF06716.LOG";
    std::string winPrefixPath =
        "\\NDXSANDALS\\Instrument\\data\\cycle_05_3\\ALF06716.LOG";
    std::string winDefaultPath = "\\\\isis\\inst$\\Instruments$"
                                 "\\NDXSANDALS\\Instrument\\data\\cycle_05_"
                                 "3\\ALF06716.LOG";

    // Perform the transformation of each path prior to assertions for
    // re-usability of code.
    std::string transformLin =
        catalogInfo->transformArchivePath(linuxPrefixPath);
    std::string transformMac = catalogInfo->transformArchivePath(macPrefixPath);
    std::string transformWin =
        catalogInfo->transformArchivePath(winDefaultPath);

// Test each variation
#ifdef __linux__
    TS_ASSERT_EQUALS(linuxPrefixPath, transformMac);
    TS_ASSERT_EQUALS(linuxPrefixPath, transformWin);
    TS_ASSERT_EQUALS(linuxPrefixPath, transformLin);
#elif __APPLE__
    TS_ASSERT_EQUALS(macPrefixPath, transformMac);
    TS_ASSERT_EQUALS(macPrefixPath, transformWin);
    TS_ASSERT_EQUALS(macPrefixPath, transformLin);
#elif _WIN32
    TS_ASSERT_EQUALS(winDefaultPath, transformWin);
    TS_ASSERT_EQUALS(winPrefixPath, transformMac);
    TS_ASSERT_EQUALS(winPrefixPath, transformLin);
#endif

    delete catalogInfo;
  }

  /// Parse the XML string and create a catalog Object.
  CatalogInfo *getCatalogFromXML(const std::string &xmlStr) const {
    Poco::XML::DOMParser parser;
    Poco::AutoPtr<Poco::XML::Document> documentParser =
        parser.parseString(xmlStr);
    TS_ASSERT(documentParser);

    Poco::XML::Element *rootElement = documentParser->documentElement();
    Poco::AutoPtr<Poco::XML::NodeList> elementTag =
        rootElement->getElementsByTagName("facility");
    TS_ASSERT(elementTag->length() > 0);

    Poco::XML::Element *element =
        dynamic_cast<Poco::XML::Element *>(elementTag->item(0));
    TS_ASSERT(element);

    return (new CatalogInfo(element));
  }
};

#endif /*MANTID_CATALOGINFOTEST_H_*/
