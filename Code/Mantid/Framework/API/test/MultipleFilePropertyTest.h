#ifndef MANTID_API_MULTIPLEFILEPROPERTYTEST_H_
#define MANTID_API_MULTIPLEFILEPROPERTYTEST_H_

#include "MantidAPI/MultipleFileProperty.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/ConfigService.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <Poco/Path.h>
#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::API;

class MultipleFilePropertyTest : public CxxTest::TestSuite
{
private:
  std::string m_multiFileLoading;
public: 

  void setUp()
  {
    // Make sure that multi file loading is enabled for each test.
    m_multiFileLoading = Kernel::ConfigService::Instance().getString("loading.multifile");
    Kernel::ConfigService::Instance().setString("loading.multifile", "On");
  }

  void tearDown()
  {
    // Replace user's preference after the test has run.
    Kernel::ConfigService::Instance().setString("loading.multifile", m_multiFileLoading);
  }

  void test_setValue()
  {
    MultipleFileProperty p("Filename");
    // REF_L example is important since the instrument has no zero padding value.
    p.setValue("REF_L_32035.nxs, CSP78173.raw");
    std::vector<std::vector<std::string> > filenames = p();
    TS_ASSERT_EQUALS( filenames.size(), 2);
    TSM_ASSERT( "Files with no path are found using ConfigService paths", Poco::Path(filenames[0][0]).isAbsolute() );
    TSM_ASSERT( "Files with no path are found using ConfigService paths", Poco::Path(filenames[1][0]).isAbsolute() );
  }
  
  void test_setValue2()
  {
    // Here we try to load some files, some of them to be added together.  We should end up with a vector of vectors as follows:
    //
    // | [dir]MUSR00015189.nxs | [dir]MUSR00015190.nxs | [dir]MUSR00015189.nxs | [dir]MUSR00015191.nxs |
    //                                                 | [dir]MUSR00015190.nxs |
    //                                                 | [dir]MUSR00015191.nxs |

    MultipleFileProperty p("Filename");
    p.setValue("MUSR15189:15190,15189-15191,15191.nxs");
    std::vector<std::vector<std::string> > fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0].size(), 1);
    TS_ASSERT_EQUALS(fileNames[1].size(), 1);
    TS_ASSERT_EQUALS(fileNames[2].size(), 3);
    TS_ASSERT_EQUALS(fileNames[3].size(), 1);
  }

  void test_failsOnComplexAddition()
  {
    MultipleFileProperty p("Filename");
    p.setValue("MUSR15189:15190+MUSR15189");
    std::vector<std::vector<std::string>> fileNames = p();
    TS_ASSERT_EQUALS(fileNames.size(), 0);
  }

  void test_failsOnBadlyFormedFilename()
  {
    MultipleFileProperty p("Filename");
    p.setValue("MUSR15189,,MUSR15189");
    std::vector<std::vector<std::string>> fileNames = p();
    TS_ASSERT_EQUALS(fileNames.size(), 0);
  }

  void test_multiFileSwitchedOff()
  {
    Kernel::ConfigService::Instance().setString("loading.multifile", "Off");

    std::string filename = "_MultipleFilePropertyTest_tempFileWithA+AndA,InTheName.txt";

    Poco::File temp(filename);
    temp.createFile();

    MultipleFileProperty p("Filename");
    p.setValue(filename);
    std::vector<std::vector<std::string>> fileNames = p();
    TS_ASSERT_EQUALS(fileNames.size(), 1);
  }

  void test_folderWithWhitespace()
  {
    std::string dirPath = "_MultipleFilePropertyTestDummyFolder WithWhiteSpace";
    std::string filename = "TSC99999.raw";
    std::string oldDataSearchDirectories = "";
       
    // Create a dummy folder with whitespace to use.
    Poco::File dir(dirPath);
    dir.createDirectories();
    
    Poco::File file(dirPath + "/" + filename);
    file.createFile();
    
    // Add dummy directory to search path, saving old search paths to be put back later.
    Poco::Path path(dir.path());
    path = path.makeAbsolute();
    oldDataSearchDirectories = Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories");
    Mantid::Kernel::ConfigService::Instance().setString("datasearch.directories", oldDataSearchDirectories + "; " + path.toString());
    
    MultipleFileProperty p("Filename");
    p.setValue(filename);
    std::vector<std::vector<std::string> > fileNames = p();

    TS_ASSERT_EQUALS(fileNames.size(), 1);
    TS_ASSERT_EQUALS(fileNames[0].size(), 1);

    // Put back the old search paths.
    Mantid::Kernel::ConfigService::Instance().setString("datasearch.directories", oldDataSearchDirectories);

    // Destroy dummy folder and any files it contains.  
    dir.remove(true);
  }
};


#endif /* MANTID_API_MULTIPLEFILEPROPERTYTEST_H_ */

