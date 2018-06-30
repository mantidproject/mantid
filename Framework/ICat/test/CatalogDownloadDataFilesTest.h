#ifndef DOWNLOADDATAFILE_H_
#define DOWNLOADDATAFILE_H_

#include "ICatTestHelper.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidICat/CatalogDownloadDataFiles.h"
#include "MantidICat/CatalogGetDataFiles.h"
#include "MantidICat/CatalogLogin.h"
#include "MantidICat/CatalogSearch.h"
#include "MantidKernel/ConfigService.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <iomanip>

using namespace Mantid;
using namespace Mantid::ICat;
using namespace Mantid::API;
class CatalogDownloadDataFilesTest : public CxxTest::TestSuite {
public:
  /** Ping the  download.mantidproject.org and
   * skip all tests if internet/server is down.
   */
  bool skipTests() override {
#ifdef _WIN32
    // I don't know how to get exit status from windows.
    return false;
#else
    // Ping once, with a 1 second wait.
    std::string cmdstring = "ping download.mantidproject.org -c 1 -w 1";

    int status;
    status = system(cmdstring.c_str());
    if (status == -1) {
      // Some kind of system() failure
    } else
      // Get the exit code
      status = WEXITSTATUS(status);

    if (status != 0) {
      std::cout << "Skipping test since '" << cmdstring << "' FAILED!\n";
      return true;
    }
    return false;
#endif
  }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(downloadobj.initialize());
    TS_ASSERT(downloadobj.isInitialized());
  }
  void xtestDownLoadDataFile() {
    TS_ASSERT(ICatTestHelper::login());

    if (!searchobj.isInitialized())
      searchobj.initialize();
    searchobj.setPropertyValue("RunRange", "100-102");
    searchobj.setPropertyValue("Instrument", "HET");
    searchobj.setPropertyValue("OutputWorkspace", "investigations");

    TS_ASSERT_THROWS_NOTHING(searchobj.execute());
    TS_ASSERT(searchobj.isExecuted());

    if (!invstObj.isInitialized())
      invstObj.initialize();
    invstObj.setPropertyValue("InvestigationId", "13539191");
    invstObj.setPropertyValue("OutputWorkspace",
                              "investigation"); // selected invesigation
    //
    TS_ASSERT_THROWS_NOTHING(invstObj.execute());
    TS_ASSERT(invstObj.isExecuted());

    clock_t start = clock();
    if (!downloadobj.isInitialized())
      downloadobj.initialize();

    downloadobj.setPropertyValue("Filenames", "HET00097.RAW");

    TS_ASSERT_THROWS_NOTHING(downloadobj.execute());

    clock_t end = clock();
    float diff = float(end - start) / CLOCKS_PER_SEC;

    std::string filepath =
        Kernel::ConfigService::Instance().getString("defaultsave.directory");
    filepath += "download_time.txt";

    std::ofstream ofs(filepath.c_str(), std::ios_base::out);
    if (ofs.rdstate() & std::ios::failbit) {
      throw Mantid::Kernel::Exception::FileError("Error on creating File",
                                                 "download_time.txt");
    }
    ofs << "Time taken to  download files with investigation id 12576918 is "
        << std::fixed << std::setprecision(2) << diff << " seconds\n";

    ICatTestHelper::logout();

    TS_ASSERT(downloadobj.isExecuted());
    // delete the file after execution
    // remove("HET00097.RAW");

    AnalysisDataService::Instance().remove("investigations");
    AnalysisDataService::Instance().remove("investigation");
    // Clean up test files
    if (Poco::File(filepath).exists())
      Poco::File(filepath).remove();
  }

  void xtestDownLoadNexusFile() {
    TS_ASSERT(ICatTestHelper::login());

    if (!searchobj.isInitialized())
      searchobj.initialize();
    searchobj.setPropertyValue("RunRange", "17440-17556");
    searchobj.setPropertyValue("Instrument", "EMU");
    searchobj.setPropertyValue("OutputWorkspace", "investigations");

    TS_ASSERT_THROWS_NOTHING(searchobj.execute());
    TS_ASSERT(searchobj.isExecuted());

    if (!invstObj.isInitialized())
      invstObj.initialize();

    invstObj.setPropertyValue("InvestigationId", "24070400");

    invstObj.setPropertyValue("OutputWorkspace", "investigation");

    TS_ASSERT_THROWS_NOTHING(invstObj.execute());
    TS_ASSERT(invstObj.isExecuted());

    clock_t start = clock();
    if (!downloadobj.isInitialized())
      downloadobj.initialize();

    downloadobj.setPropertyValue("Filenames", "EMU00017452.nxs");
    TS_ASSERT_THROWS_NOTHING(downloadobj.execute());

    clock_t end = clock();
    float diff = float(end - start) / CLOCKS_PER_SEC;

    std::string filepath =
        Kernel::ConfigService::Instance().getString("defaultsave.directory");
    filepath += "download_time.txt";

    std::ofstream ofs(filepath.c_str(),
                      std::ios_base::out | std::ios_base::app);
    if (ofs.rdstate() & std::ios::failbit) {
      throw Mantid::Kernel::Exception::FileError("Error on creating File",
                                                 "download_time.txt");
    }
    ofs << "Time taken to download files with investigation id 24070400 is "
        << std::fixed << std::setprecision(2) << diff << " seconds\n";
    // ofs.close();

    ICatTestHelper::logout();

    TS_ASSERT(downloadobj.isExecuted());
    // delete the file after execution
    // remove("EMU00017452.nxs");
    AnalysisDataService::Instance().remove("investigations");
    AnalysisDataService::Instance().remove("investigation");
    // Clean up test files
    if (Poco::File(filepath).exists())
      Poco::File(filepath).remove();
  }

  void xtestDownLoadDataFile_Merlin() {
    TS_ASSERT(ICatTestHelper::login());

    if (!searchobj.isInitialized())
      searchobj.initialize();
    searchobj.setPropertyValue("RunRange", "600-601");
    searchobj.setPropertyValue("Instrument", "MERLIN");
    searchobj.setPropertyValue("OutputWorkspace", "investigations");

    TS_ASSERT_THROWS_NOTHING(searchobj.execute());
    TS_ASSERT(searchobj.isExecuted());

    if (!invstObj.isInitialized())
      invstObj.initialize();
    invstObj.setPropertyValue("InvestigationId", "24022007");
    invstObj.setPropertyValue("OutputWorkspace",
                              "investigation"); // selected invesigation
    //
    TS_ASSERT_THROWS_NOTHING(invstObj.execute());
    TS_ASSERT(invstObj.isExecuted());

    clock_t start = clock();
    if (!downloadobj.isInitialized())
      downloadobj.initialize();

    downloadobj.setPropertyValue("Filenames", "MER00599.raw");

    TS_ASSERT_THROWS_NOTHING(downloadobj.execute());

    clock_t end = clock();
    float diff = float(end - start) / CLOCKS_PER_SEC;

    std::string filepath =
        Kernel::ConfigService::Instance().getString("defaultsave.directory");
    filepath += "download_time.txt";

    std::ofstream ofs(filepath.c_str(),
                      std::ios_base::out | std::ios_base::app);
    if (ofs.rdstate() & std::ios::failbit) {
      throw Mantid::Kernel::Exception::FileError("Error on creating File",
                                                 "download_time.txt");
    }
    ofs << "Time taken to download files with investigation id 24022007 is "
        << std::fixed << std::setprecision(2) << diff << " seconds\n";

    ICatTestHelper::logout();

    TS_ASSERT(downloadobj.isExecuted());
    AnalysisDataService::Instance().remove("investigations");
    AnalysisDataService::Instance().remove("investigation");

    // Clean up test files
    if (Poco::File(filepath).exists())
      Poco::File(filepath).remove();
  }

  void xtestDownloaddataFile1() {
    std::string filepath =
        Kernel::ConfigService::Instance().getString("defaultsave.directory");
    filepath += "download_time.txt";
    std::ofstream ofs(filepath.c_str(),
                      std::ios_base::out | std::ios_base::app);
    if (ofs.rdstate() & std::ios::failbit) {
      throw Mantid::Kernel::Exception::FileError("Error on creating File",
                                                 "download_time.txt");
    }

    CatalogDownloadDataFiles downloadobj1;

    // As the algorithm now uses setProperty to allow us to save it to a
    // directory we must pass in the default for testing.
    std::string fName =
        Kernel::ConfigService::Instance().getString("defaultsave.directory");
    // Need to initialize the algorithm in order to set the "downloadPath"
    // property.
    if (!downloadobj1.isInitialized())
      downloadobj1.initialize();
    downloadobj1.setPropertyValue("DownloadPath", fName);

    clock_t start = clock();
    // this gets the main doc page, which should always be there
    // it's a wiki page so it can be relatively slow
    std::string fullPathDownloadedFile = downloadobj1.testDownload(
        "http://www.mantidproject.org/Documentation", "test.htm");
    clock_t end = clock();
    float diff = float(end - start) / CLOCKS_PER_SEC;

    ofs << "Time taken for http download from mantidwebserver over internet "
           "for a small file of size 1KB is "
        << std::fixed << std::setprecision(2) << diff << " seconds\n";

    ICatTestHelper::logout();

    // delete the file after execution
    remove("test.htm");

    // test if fullPathDownloadedFile ok
    Poco::Path defaultSaveDir(
        Kernel::ConfigService::Instance().getString("defaultsave.directory"));
    Poco::Path path(defaultSaveDir, "test.htm");
    TS_ASSERT(fullPathDownloadedFile == path.toString());
  }

private:
  CatalogSearch searchobj;
  CatalogGetDataFiles invstObj;
  CatalogDownloadDataFiles downloadobj;
};
#endif
