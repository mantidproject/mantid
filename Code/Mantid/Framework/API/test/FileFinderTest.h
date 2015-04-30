#ifndef FILEFINDERTEST_H_
#define FILEFINDERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/FileFinder.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"

#include <Poco/Path.h>
#include <Poco/File.h>
#include <boost/lexical_cast.hpp>

#include <stdio.h>
#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

class FileFinderTest: public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor (& destructor) isn't called when running other tests
  static FileFinderTest *createSuite() { return new FileFinderTest(); }
  static void destroySuite( FileFinderTest *suite ) { delete suite; }

  FileFinderTest() :
    m_facFile("./FileFinderTest_Facilities.xml")
  {
    if (m_facFile.exists())
      m_facFile.remove();

    const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<facilities>"
      "  <facility name=\"ISIS\" zeropadding=\"5\" FileExtensions=\".nxs,.raw,.sav,.n*,.s*\">"
      "    <archive>"
      "      <archiveSearch plugin=\"ISISDataSearch\" />"
      "    </archive>"
      "    <instrument name=\"HRPD\" shortname=\"HRP\">"
      "      <technique>Powder Diffraction</technique>"
      "    </instrument>"
      "    <instrument name=\"ABCD\" shortname=\"ABC\" >"
      "      <zeropadding size=\"8\"/>"
      "      <technique>Powder Diffraction</technique>"
      "    </instrument>"
      "    <instrument name=\"EFG2H\" shortname=\"EFG2H\">"
      "      <zeropadding size=\"8\"/>"
      "      <technique>Powder Diffraction</technique>"
      "    </instrument>"
      "    <instrument name=\"CRISP\" shortname=\"CSP\">"
      "      <technique>Technique</technique>"
      "    </instrument>"
      "    <instrument name=\"MUSR\">"
      "      <zeropadding size=\"8\"/>"
      "      <technique>Powder Diffraction</technique>"
      "    </instrument>"
      "    <instrument name=\"LOQ\">"
      "     <zeropadding size=\"5\"/>"
      "     <technique>Small Angle Scattering</technique>"
      "    </instrument>"
      "    <instrument name=\"OFFSPEC\">"
      "      <zeropadding size=\"8\"/>"
      "      <technique>Reflectometer</technique>"
      "    </instrument>"
      "    <instrument name=\"SANS2D\">"
      "      <zeropadding size=\"8\"/>"
      "      <technique>Small Angle Scattering</technique>"
      "    </instrument>"
      "  </facility>"
      "  <facility name=\"SNS\" delimiter=\"_\" FileExtensions=\"_event.nxs,.nxs,.dat\">"
      "    <archive>"
      "      <archiveSearch plugin=\"SNSDataSearch\" />"
      "    </archive>"
      "    <instrument name=\"SEQUOIA\" shortname=\"SEQ\">"
      "      <technique>Inelastic Spectroscopy</technique>"
      "    </instrument>"
      "    <instrument name=\"CNCS\" shortname=\"CNCS\">"
      "      <technique>Inelastic Spectroscopy</technique>"
      "    </instrument>"
      "    <instrument name=\"REF_L\" shortname=\"REF_L\">"
      "      <technique>Reflectometer</technique>"
      "    </instrument>"
      "    <instrument name=\"POWGEN\" shortname=\"PG3\">"
      "      <technique>Reflectometer</technique>"
      "    </instrument>"
      "  </facility>"
      "  <facility name=\"ILL\" delimiter=\"_\" FileExtensions=\".nxs,.dat\">"
	  "    <instrument name=\"IN5\" shortname=\"IN5\">"
      "      <technique>Inelastic Spectroscopy</technique>"
      "    </instrument>"
	  "  </facility>"
      "</facilities>";

    std::ofstream fil(m_facFile.path().c_str());
    fil << xmlStr;
    fil.close();

    ConfigService::Instance().updateFacilities(m_facFile.path());

  }

  ~FileFinderTest()
  {
    m_facFile.remove();
  }

  void testGetFullPathWithFilename()
  {
    std::string path = FileFinder::Instance().getFullPath("CSP78173.raw");
    TS_ASSERT(!path.empty());
  }

  void testGetFullPathWithDirectoryFindsDirectoryPath()
  {
    // Use the Schema directory under instrument
    std::string path = FileFinder::Instance().getFullPath("Schema");
    TS_ASSERT(!path.empty());

    // Code has separate path for path relative to working directory so check that too
    std::string tempTestName("__FileFinderTestTempTestDir__");
    Poco::File tempTestDir(Poco::Path().resolve(tempTestName));
    tempTestDir.createDirectory();

    path = FileFinder::Instance().getFullPath(tempTestName);
    TS_ASSERT(!path.empty());

    tempTestDir.remove();

  }

  void testGetFullPathSkipsDirectoriesOnRequest()
  {
    // Use the Schema directory under instrument
    const bool ignoreDirs(true);
    std::string path = FileFinder::Instance().getFullPath("Schema", ignoreDirs);
    TSM_ASSERT("Expected an empty path when looking for a directory, instead I found " + path, path.empty());

    // Code has separate path for path relative to working directory so check that too
    std::string tempTestName("__FileFinderTestTempTestDir__");
    Poco::File tempTestDir(Poco::Path().resolve(tempTestName));
    tempTestDir.createDirectory();

    path = FileFinder::Instance().getFullPath(tempTestName, ignoreDirs);
    TSM_ASSERT("Expected an empty path when looking for a directory relative to current, instead I found " + path, path.empty());

    tempTestDir.remove();
  }

  void testMakeFileNameForISIS()
  {
    // Set the facility
    const FacilityInfo& facility = ConfigService::Instance().getFacility("ISIS");
    const InstrumentInfo& instrument = facility.instrument("HRPD");

    // Set the default instrument
    ConfigService::Instance().setString("default.instrument", instrument.shortName());

    std::string fName = FileFinder::Instance().makeFileName("123", instrument);
    TS_ASSERT_EQUALS(fName, "HRP00123");

    fName = FileFinder::Instance().makeFileName("ABC0123", instrument);
    TS_ASSERT_EQUALS(fName, "ABC00000123");

    fName = FileFinder::Instance().makeFileName("ABCD123", instrument);
    TS_ASSERT_EQUALS(fName, "ABC00000123");

    TS_ASSERT_THROWS(fName = FileFinder::Instance().makeFileName("ABCD", instrument), std::invalid_argument);
    TS_ASSERT_THROWS(fName = FileFinder::Instance().makeFileName("123456", instrument), std::invalid_argument);

    fName = FileFinder::Instance().makeFileName("0", instrument);
    TS_ASSERT_EQUALS(fName, "HRP00000");

    TS_ASSERT_EQUALS("EFG2H00000123", FileFinder::Instance().makeFileName("EFG2H123", instrument));

  }

  void testMakeFileNameForSNS()
  {
    // Set the facility
    const FacilityInfo& facility = ConfigService::Instance().getFacility("SNS");
    const InstrumentInfo& instrument = facility.instrument("CNCS");

    // Set the default instrument
    ConfigService::Instance().setString("default.instrument", instrument.shortName());

    // Check that we remove any leading zeros
    TS_ASSERT_EQUALS("CNCS_123", FileFinder::Instance().makeFileName("0123", instrument));

    // Test using long and short name
    TS_ASSERT_EQUALS("SEQ_21", FileFinder::Instance().makeFileName("SEQUOIA21", instrument));
    TS_ASSERT_EQUALS("SEQ_21", FileFinder::Instance().makeFileName("SEQ21", instrument));

    // Test for POWGEN with a trailing number in the instrument name.
    TS_ASSERT_EQUALS("PG3_333", FileFinder::Instance().makeFileName("PG3333", instrument));

    // Test for REF_L (to check that the extra _ doesn't upset anything)
    TS_ASSERT_EQUALS("REF_L_666", FileFinder::Instance().makeFileName("REF_L666", instrument));

  }

  void testGetInstrument()
  {
    std::string name; // place to put results
    ConfigService::Instance().setFacility("ISIS");
    ConfigService::Instance().setString("default.instrument", "HRPD");

    TS_ASSERT_EQUALS(FileFinder::Instance().getInstrument("").name(), "HRPD");
    TS_ASSERT_EQUALS(FileFinder::Instance().getInstrument("PG31234").name(), "POWGEN");
    TS_ASSERT_EQUALS(FileFinder::Instance().getInstrument("PG3_1234").name(), "POWGEN");
    TS_ASSERT_EQUALS(FileFinder::Instance().getInstrument("PG3_1234_event.nxs").name(), "POWGEN");
    TS_ASSERT_EQUALS(FileFinder::Instance().getInstrument("/home/user123/CNCS_234_neutron_event.dat").name(), "CNCS");
    TS_ASSERT_EQUALS(FileFinder::Instance().getInstrument("REF_L1234").name(), "REF_L");
    TS_ASSERT_EQUALS(FileFinder::Instance().getInstrument("REF_L_1234").name(), "REF_L");
    TS_ASSERT_EQUALS(FileFinder::Instance().getInstrument("REF_L_1234.nxs.h5").name(), "REF_L");
    TS_ASSERT_EQUALS(FileFinder::Instance().getInstrument("LOQ16613.n001").name(), "LOQ");
    TS_ASSERT_EQUALS(FileFinder::Instance().getInstrument("LOQ16613.s01").name(), "LOQ");
    TS_ASSERT_EQUALS(FileFinder::Instance().getInstrument("SANS2D00032676.nxs").name(), "SANS2D");
  }

  void testGetExtension()
  {
    std::vector<std::string> exts;
    exts.push_back("_event.nxs");
    exts.push_back(".nxs.h5");
    exts.push_back(".n*");

    TS_ASSERT_EQUALS(FileFinder::Instance().getExtension("", exts), "");
    TS_ASSERT_EQUALS(FileFinder::Instance().getExtension("PG31234", exts), "");
    TS_ASSERT_EQUALS(FileFinder::Instance().getExtension("PG3_1234", exts), "");
    TS_ASSERT_EQUALS(FileFinder::Instance().getExtension("PG3_1234_event.nxs", exts), "_event.nxs");
    TS_ASSERT_EQUALS(FileFinder::Instance().getExtension("/home/user123/CNCS_234_neutron_event.dat", exts),
                     ".dat"); // doesn't know about full extension
    TS_ASSERT_EQUALS(FileFinder::Instance().getExtension("REF_L1234", exts), "");
    TS_ASSERT_EQUALS(FileFinder::Instance().getExtension("REF_L_1234", exts), "");
    TS_ASSERT_EQUALS(FileFinder::Instance().getExtension("REF_L_1234.nxs.h5", exts), ".nxs.h5");
    TS_ASSERT_EQUALS(FileFinder::Instance().getExtension("LOQ16613.n001", exts), ".n001");
    TS_ASSERT_EQUALS(FileFinder::Instance().getExtension("LOQ16613.s01", exts), ".s01");
  }

  void testFindRunForSNS()
  {
    // Turn off the archive searching
    ConfigService::Instance().setString("datasearch.searcharchive", "Off");

    std::string path = FileFinder::Instance().findRun("CNCS7860");
    TS_ASSERT(path.find("CNCS_7860_event.nxs") != std::string::npos);
    Poco::File file(path);
    TS_ASSERT(file.exists());

  }

  void testFindRunForISIS()
  {
    // Set the facility
    ConfigService::Instance().setString("default.facility", "ISIS");

    ConfigService::Instance().setString("datasearch.searcharchive", "Off");
    std::string path = FileFinder::Instance().findRun("CSP78173");
    TS_ASSERT(path.find("CSP78173.raw") != std::string::npos);
    Poco::File file(path);
    TS_ASSERT(file.exists());
    path = FileFinder::Instance().findRun("CSP74683", std::vector<std::string>(1, ".s02"));
    TS_ASSERT(path.size() > 3);
    TS_ASSERT_EQUALS(path.substr(path.size() - 3), "s02");

    //ConfigService::Instance().setString("datasearch.searcharchive","On");
    //path = FileFinder::Instance().findRun("CSP77374");
    //std::cerr<<"Path: "<<path<<'\n';
    //path = FileFinder::Instance().findRun("CSP78174");
    //std::cerr<<"Path: "<<path<<'\n';
  }

  void testFindFiles()
  {
    ConfigService::Instance().setString("default.facility","ISIS");
    std::vector<std::string> files;
    TS_ASSERT_THROWS(files = FileFinder::Instance().findRuns("MUSR15189-n15193"), std::invalid_argument);
    TS_ASSERT_THROWS(files = FileFinder::Instance().findRuns("MUSR15189n-15193"), std::invalid_argument);
    TS_ASSERT_THROWS(files = FileFinder::Instance().findRuns("MUSR15189-15193n"), std::invalid_argument);
    TS_ASSERT_THROWS(files = FileFinder::Instance().findRuns("MUSR15189-151n93"), std::invalid_argument);
    TS_ASSERT_THROWS(files = FileFinder::Instance().findRuns("MUSR15n189-151n93"), Exception::NotFoundError);
    TS_ASSERT_THROWS_NOTHING(files = FileFinder::Instance().findRuns("MUSR15189-15193"));
    TS_ASSERT_EQUALS(files.size(), 5);
    std::vector<std::string>::iterator it = files.begin();

    for (; it != files.end(); ++it)
    {
      if (it != files.begin())
      {
        TS_ASSERT_DIFFERS(*it, *(it - 1));
      }

    }
  }

  void testFindAddFiles()
  {
    //create a test file to find
    Poco::File file("LOQ00111-add.raw");
    std::ofstream fil(file.path().c_str());
    fil << "dummy";
    fil.close();


    ConfigService::Instance().setString("default.facility","ISIS");
    std::vector<std::string> files = FileFinder::Instance().findRuns("LOQ111-add");
    TS_ASSERT_EQUALS(files.size(), 1);

    file.remove();
  }

  void testFindFileExt()
  {
    // Set the facility
    ConfigService::Instance().setString("default.facility", "ISIS");

    ConfigService::Instance().setString("datasearch.searcharchive", "Off");
    std::string path = FileFinder::Instance().findRun("CSP78173.raw");
    TS_ASSERT(path.find("CSP78173.raw") != std::string::npos);
    Poco::File file(path);
    TS_ASSERT(file.exists());

    path = FileFinder::Instance().findRun("OFFSPEC4622.log");
    // Per discussion with Martyn on Dec 6, 2012: we decided to update this test case.
    // *.log is not a valid extension for ISIS instruments. Since we modified the FileFinder to strip
    // the extension using the facility extension list rather than to strip the extension after the last dot,
    // the returned path should be empty now.
//    TS_ASSERT(path.empty() == true);
    TS_ASSERT(path.size() > 3);
    TS_ASSERT_EQUALS(path.substr(path.size() - 3), "log");
  }

  void testFindRunsDefaultInst()
  {
    ConfigService::Instance().setString("default.instrument", "MUSR");
    std::vector<std::string> paths = FileFinder::Instance().findRuns("15189-15190");
    TS_ASSERT( paths.size() == 2 );
  }

  // test to see if case sensitive on/off works
  void testFindFileCaseSensitive()
  {
    auto & fileFinder = FileFinder::Instance();
    const bool startingCaseOption = fileFinder.getCaseSensitive();

    // By default case sensitive is on
    fileFinder.setCaseSensitive(false);

    std::string path = fileFinder.findRun("CSp78173.Raw");
#ifdef _WIN32
    TS_ASSERT(path.find("CSp78173.Raw") != std::string::npos);
#else
    TS_ASSERT(path.find("CSP78173.raw") != std::string::npos);
#endif
    Poco::File file(path);
    TS_ASSERT(file.exists());
    std::string path2 = fileFinder.getFullPath("IDFs_for_UNiT_TESTiNG/IDF_for_UNiT_TESTiNG.xMl");
    Poco::File file2(path2);
    TS_ASSERT(file2.exists());

    // turn on case sensitive - this one should fail on none windows
    FileFinder::Instance().setCaseSensitive(true);
    std::string pathOn = fileFinder.findRun("CSp78173.Raw");
    Poco::File fileOn(pathOn);

    std::string pathOn2 = FileFinder::Instance().getFullPath("IDFs_for_UNiT_TESTiNG/IDF_for_UNiT_TESTiNG.xMl");
    Poco::File fileOn2(pathOn2);

    std::string pathOn3 = FileFinder::Instance().getFullPath("IDFs_for_UNIT_TESTING/IDF_for_UNiT_TESTiNG.xMl");
    Poco::File fileOn3(pathOn3);

    std::string pathOn4 = FileFinder::Instance().getFullPath("CSp78173.Raw");
    Poco::File fileOn4(pathOn4);


    // Refs #4916 -- The FileFinder findRun() method is revised to continue search using the facility supplied extensions
    // if the user supplied filename (containg extension) couldn't be found. Regardless of the platform, this test case
    // would be successful now.
    TS_ASSERT(fileOn.exists());
#ifdef _WIN32
    TS_ASSERT(fileOn2.exists()); 
    TS_ASSERT(fileOn3.exists()); 
    TS_ASSERT(fileOn4.exists());
#else
    TS_ASSERT_THROWS_ANYTHING(fileOn2.exists());
    TS_ASSERT_THROWS_ANYTHING(fileOn3.exists());
    TS_ASSERT_THROWS_ANYTHING(fileOn4.exists());
#endif

    fileFinder.setCaseSensitive(startingCaseOption);

  }

private:

  Poco::File m_facFile;
};

class FileFinderTestPerformance : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FileFinderTestPerformance *createSuite() { return new FileFinderTestPerformance(); }
  static void destroySuite( FileFinderTestPerformance *suite ) { delete suite; }

  FileFinderTestPerformance() :
    m_oldDataSearchDirectories(), m_dirPath("_FileFinderTestPerformanceDummyData"),
      // Keeping these as low as possible so as to keep the time of the test down, but users with 70,000+ files
      // in a single folder looking for a range of hundreds of files are not unheard of.
      m_filesInDir(10000), m_filesToFind(100)
  {
    // Create some dummy TOSCA run files to use.
    Poco::File dir(m_dirPath);
    dir.createDirectories();
    
    for( size_t run = 0; run < m_filesInDir; ++run )
    {
      std::stringstream filename;
      generateFileName(filename, run);

      {
        // Hoping for some speed increase from this (assuming no RVO optimisation 
        // when using filename.str().c_str() ...):
        const std::string& tmp = filename.str();
        const char* cstr = tmp.c_str();
        
        // Shun use of Poco, which is slower.
        FILE * pFile;
        pFile = fopen (cstr , "w");
        if (pFile != NULL)
        {
          fclose (pFile);
        }
      }
    }
    
    // Set TOSCA as default instrument.
    Mantid::Kernel::ConfigService::Instance().setString("default.instrument", "TSC");
    
    // Add dummy directory to search path, saving old search paths to be put back later.
    Poco::Path path(dir.path());
    path = path.makeAbsolute();
    m_oldDataSearchDirectories = Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories");
    Mantid::Kernel::ConfigService::Instance().setString("datasearch.directories", path.toString());
  }

  ~FileFinderTestPerformance()
  {
    // Put back the old search paths.
    Mantid::Kernel::ConfigService::Instance().setString("datasearch.directories", m_oldDataSearchDirectories);

    // Destroy dummy folder and files.  
    // Use Poco here so removing works on multiple platforms. Recursive .remove(true) also means we dont have to generate
    // the filenames for a second time.
    Poco::File dir(m_dirPath);
    dir.remove(true);
  }

  void test_largeDirectoryOfFiles()
  {
    auto & fileFinder = FileFinder::Instance();
    const bool startingCaseOption = fileFinder.getCaseSensitive();

    // By default case sensitive is on
    fileFinder.setCaseSensitive(false);

    std::vector<std::string> files;
    std::stringstream range;
    range << (m_filesInDir - m_filesToFind) << "-" << (m_filesInDir - 1);
    TS_ASSERT_THROWS_NOTHING( files = fileFinder.findRuns(range.str().c_str()) );
    TS_ASSERT( files.size() == m_filesToFind );

    fileFinder.setCaseSensitive(startingCaseOption);
  }

  void test_manyMissingFilesWithLargeDirectory()
  {
    auto & fileFinder = FileFinder::Instance();
    const bool startingCaseOption = fileFinder.getCaseSensitive();


    // This test essentially covers the case where a user types an erroneous range of runs into an MWRunFiles widget.
    // If they have accidentally typed in an extremely large range (most of which dont exist) then it is important
    // that this fact is realised as early as possible, and the user is not punished by either having to wait or just
    // restart Mantid.  Here, we guard against any change in FileFinder that could reintroduce this problem.
    std::vector<std::string> files;
    std::stringstream range;
    std::string startOfRange = boost::lexical_cast<std::string>(m_filesInDir - 10);
    std::string accidentalEndOfRange = "99999";
    range << startOfRange << "-" << accidentalEndOfRange;
    TS_ASSERT_THROWS( files = fileFinder.findRuns(range.str().c_str()), Mantid::Kernel::Exception::NotFoundError );


    fileFinder.setCaseSensitive(startingCaseOption);
  }
  
private:

  void generateFileName(std::stringstream & stream, size_t run)
  {
    // Alternate cases of instrument and extension.
    if( run % 4 == 0 )
    {
      stream << m_dirPath << "/TSC";
      pad(stream, run);
      stream << ".raw";
    }
    else if( run % 4 == 1 )
    {
      stream << m_dirPath << "/TSC";
      pad(stream, run);
      stream << ".RAW";
    }
    else if( run % 4 == 2 )
    {
      stream << m_dirPath << "/tsc";
      pad(stream, run);
      stream << ".RAW";
    }
    else
    {
      stream << m_dirPath << "/tsc";
      pad(stream, run);
      stream << ".raw";
    }
  }
  
  /**
   * Pad the given stringstream with zeros from a lookup, then add the run number.
   */
  void pad(std::stringstream & stream, size_t run)
  {
    if( run < 10 )
      stream << "0000" << run;
    else if( run < 100 )
      stream << "000" << run;
    else if( run < 1000 )
      stream << "00" << run;
    else if( run < 10000 )
      stream << "0" << run;
    else
      stream << run;
  }

  // Temp storage.
  std::string m_oldDataSearchDirectories;
  // Path to directory where dummy files will be created.
  std::string m_dirPath;
  // Number of dummy files to create.
  size_t m_filesInDir;
  // Number of files to find.
  size_t m_filesToFind;
};

#endif /*FILEFINDERTEST_H_*/
