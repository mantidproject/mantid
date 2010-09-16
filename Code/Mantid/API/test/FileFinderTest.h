#ifndef FILEFINDERTEST_H_
#define FILEFINDERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/FileFinder.h"
#include "MantidKernel/ConfigService.h"

#include "Poco/Path.h"
#include "Poco/File.h"

#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

class FileFinderTest : public CxxTest::TestSuite
{
 
public:
  
  FileFinderTest():m_facFile("./FileFinderTest_Facilities.xml")
  {
    if( m_facFile.exists() ) m_facFile.remove();

    const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<facilities>"
      "  <facility name=\"ISIS\" zeropadding=\"5\" FileExtensions=\".nxs,.raw,.sav,.n*,.s*\">"
      "    <archive>"
      "      <archiveSearch plugin=\"ISISDataSearch\" />"
      "    </archive>"
      "    <instrument name=\"HRPD\" shortname=\"HRP\">"
      "      <technique>Powder Diffraction</technique>"
      "    </instrument>"
      "    <instrument name=\"ABCD\" shortname=\"ABC\" zeropadding=\"8\">"
      "      <technique>Powder Diffraction</technique>"
      "    </instrument>"
      "    <instrument name=\"EFG2H\" shortname=\"EFG2H\" zeropadding=\"8\">"
      "      <technique>Powder Diffraction</technique>"
      "    </instrument>"
      "    <instrument name=\"CRISP\" shortname=\"CSP\">"
      "      <technique>Technique</technique>"
      "    </instrument>"
      "    <instrument name=\"MUSR\" zeropadding=\"8\">"
      "      <technique>Powder Diffraction</technique>"
      "    </instrument>"
      "  </facility>"
      "</facilities>";

    std::ofstream fil(m_facFile.path().c_str());
    fil << xmlStr;
    fil.close();

 }

  void setUp()
  {
    ConfigService::Instance().updateFacilities(m_facFile.path());
    ConfigService::Instance().setString("default.instrument","HRPD");
    ConfigService::Instance().setString("default.facility","ISIS");
  }

  ~FileFinderTest()
  {
    m_facFile.remove();
  }

  void testGetFullPath()
  {
    ConfigService::Instance().setString("datasearch.directories",
      "../../../../Test/AutoTestData;../../../../Test/");
    std::string path = FileFinder::Instance().getFullPath("CSP78173.raw");
    TS_ASSERT( !path.empty() );
  }

  void testMakeFileName()
  {
    std::string fName = FileFinder::Instance().makeFileName("123");
    TS_ASSERT_EQUALS(fName,"HRP00123");

    fName = FileFinder::Instance().makeFileName("ABC0123");
    TS_ASSERT_EQUALS(fName,"ABC00000123");

    fName = FileFinder::Instance().makeFileName("ABCD123");
    TS_ASSERT_EQUALS(fName,"ABC00000123");

    TS_ASSERT_THROWS(fName = FileFinder::Instance().makeFileName("ABCD"),std::invalid_argument);
    TS_ASSERT_THROWS(fName = FileFinder::Instance().makeFileName("123456"),std::invalid_argument);

    fName = FileFinder::Instance().makeFileName("0");
    TS_ASSERT_EQUALS(fName,"HRP00000");

    TS_ASSERT_EQUALS("EFG2H00000123", FileFinder::Instance().makeFileName("EFG2H123"));
  }

  void testFindRun()
  {
    ConfigService::Instance().setString("datasearch.searcharchive","Off");
    std::string path = FileFinder::Instance().findRun("CSP78173");
    TS_ASSERT(path.find("CSP78173.raw") != std::string::npos);
    Poco::File file(path);
    TS_ASSERT(file.exists());
    path = FileFinder::Instance().findRun("HRP37129");
    TS_ASSERT(path.size() > 3);
    TS_ASSERT_EQUALS(path.substr(path.size()-3),"S02");

    //ConfigService::Instance().setString("datasearch.searcharchive","On");
    //path = FileFinder::Instance().findRun("CSP77374");
    //std::cerr<<"Path: "<<path<<'\n';
    //path = FileFinder::Instance().findRun("CSP78174");
    //std::cerr<<"Path: "<<path<<'\n';
  }

  void testFindFiles()
  {
    std::vector<std::string> files = FileFinder::Instance().findRuns("MUSR15189-99");
    TS_ASSERT_EQUALS(files.size(),11);
    std::vector<std::string>::iterator it = files.begin();
    for(; it != files.end(); ++it)
    {
      if (it != files.begin())
      {
        TS_ASSERT_DIFFERS(*it,*(it-1));
      }
    }
  }

private:

  Poco::File m_facFile;
};



#endif /*FILEFINDERTEST_H_*/
