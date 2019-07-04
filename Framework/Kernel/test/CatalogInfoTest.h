// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CATALOGINFOTEST_H_
#define MANTID_CATALOGINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/CatalogInfo.h"
#include "MantidKernel/ConfigService.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <memory>

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

    std::unique_ptr<CatalogInfo> catalogInfo = nullptr;
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
        "<windows "
        "replacement=\"\\\\\\\\isis\\.cclrc\\.ac\\.uk\\\\inst\\$"
        "\\\\Instruments\\$\"></windows>"
        "<linux replacement=\"/archive\"></linux>"
        "<mac replacement=\"/archive\"></mac>"
        "</filelocation>"
        "</catalog>"
        "</facility>"
        "</facilities>";

    std::unique_ptr<CatalogInfo> catalogInfo = nullptr;
    TS_ASSERT_THROWS_NOTHING(catalogInfo = getCatalogFromXML(facilitiesXml));

    // Set the paths to test against.
    std::string linuxPrefixPath =
        "/archive/NDXSANDALS/Instrument/data/cycle_05_3/ALF06716.LOG";
    std::string macPrefixPath =
        "/archive/NDXSANDALS/Instrument/data/cycle_05_3/ALF06716.LOG";
    std::string winPrefixPath =
        R"(\\isis.cclrc.ac.uk\inst$\Instruments$\NDXSANDALS\Instrument\data\cycle_05_3\ALF06716.LOG)";
    std::string winDefaultPath =
        R"(\\isis\inst$\Instruments$\NDXSANDALS\Instrument\data\cycle_05_3\ALF06716.LOG)";

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
    TS_ASSERT_EQUALS(winPrefixPath, transformWin);
    TS_ASSERT_EQUALS(winPrefixPath, transformMac);
    TS_ASSERT_EQUALS(winPrefixPath, transformLin);
#endif
  }

  /// Parse the XML string and create a catalog Object.
  std::unique_ptr<CatalogInfo>
  getCatalogFromXML(const std::string &xmlStr) const {
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

    return (std::make_unique<CatalogInfo>(element));
  }

  void testCopyConstructor() {
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

    std::unique_ptr<CatalogInfo> original(getCatalogFromXML(facilitiesXml));

    CatalogInfo copy = *original;

    TS_ASSERT_EQUALS(original->catalogName(), copy.catalogName());
    TS_ASSERT_EQUALS(original->catalogPrefix(), copy.catalogPrefix());
    TS_ASSERT_EQUALS(original->externalDownloadURL(),
                     copy.externalDownloadURL());
    TS_ASSERT_EQUALS(original->linuxPrefix(), copy.linuxPrefix());
    TS_ASSERT_EQUALS(original->macPrefix(), copy.macPrefix());
    TS_ASSERT_EQUALS(original->windowsPrefix(), copy.windowsPrefix());
    TS_ASSERT_EQUALS(original->soapEndPoint(), copy.soapEndPoint());
  }
};

#endif /*MANTID_CATALOGINFOTEST_H_*/
