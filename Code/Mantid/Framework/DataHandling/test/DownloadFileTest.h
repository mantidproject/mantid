#ifndef MANTID_DATAHANDLING_DOWNLOADFILETEST_H_
#define MANTID_DATAHANDLING_DOWNLOADFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/DownloadFile.h"
#include "MantidKernel/InternetHelper.h"

#include <Poco/TemporaryFile.h>


#include <fstream>
#include <sstream>

using Mantid::Kernel::InternetHelper;
using Mantid::DataHandling::DownloadFile;
using namespace Mantid::Kernel;
using namespace Mantid::API;


namespace
{
  /**
   * Mock out the internet calls of this helper
   */
  class MockedInternetHelper : public InternetHelper
  {
  protected:
    virtual int sendHTTPSRequest(const std::string& url, 
                          std::ostream& responseStream,
                          const StringToStringMap& headers = StringToStringMap())
    {
      UNUSED_ARG(url);
      UNUSED_ARG(headers);
      responseStream << "HTTPS request succeeded";
      return 200;
    }
    virtual int sendHTTPRequest(const std::string& url, 
                              std::ostream& responseStream,
                              const StringToStringMap& headers = StringToStringMap())
    {
      UNUSED_ARG(url);
      UNUSED_ARG(headers);
      responseStream << "HTTP request succeeded";
      return 200;
    }
  };

    /**
   * Mock out the internet calls of this algorithm
   */
  class MockedDownloadFile : public DownloadFile
  {
  public:
    MockedDownloadFile()
    {
      delete m_internetHelper;
      m_internetHelper = new MockedInternetHelper();
    }
  };
}


class DownloadFileTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DownloadFileTest *createSuite() { return new DownloadFileTest(); }
  static void destroySuite( DownloadFileTest *suite ) { delete suite; }


  void test_Init()
  {
    DownloadFile alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_Bad_Address()
  {
    MockedInternetHelper internetHelper;
    std::string url = "www.google.com";
    Poco::TemporaryFile tmpFile;
    exec_alg(url,tmpFile.path(), "http://" + url);
  }

  void exec_alg(std::string address, std::string filename, std::string newAddress = "")
  {
    MockedDownloadFile alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Address", address) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", filename) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    if (newAddress.size() > 0)
    {
      TS_ASSERT_EQUALS(newAddress, alg.getPropertyValue("Address") );
    }
  }
  
  void test_DownloadFile_HTTP()
  {
    MockedInternetHelper internetHelper;
    std::string url = "http://www.google.com";
    Poco::TemporaryFile tmpFile;
    exec_alg(url,tmpFile.path());
    TSM_ASSERT("File has not been created.",tmpFile.exists()); 
    TSM_ASSERT("File is not a file.",tmpFile.isFile()); 
    std::fstream fs;
    TS_ASSERT_THROWS_NOTHING (fs.open (tmpFile.path().c_str(), std::fstream::in ));
    
    TSM_ASSERT("Cannot open file.",fs.is_open()); 
    
    std::stringstream ss;
    ss << fs.rdbuf();//read the file
    fs.close();

    TS_ASSERT_EQUALS ("HTTP request succeeded", ss.str());
  }

  void test_DownloadFile_HTTPS()
  {
    MockedInternetHelper internetHelper;
    std::string httpsUrl = "https://api.github.com/repos/mantidproject/mantid/contents";
    Poco::TemporaryFile tmpFile;
    exec_alg(httpsUrl,tmpFile.path());
    TSM_ASSERT("File has not been created.",tmpFile.exists()); 
    TSM_ASSERT("File is not a file.",tmpFile.isFile()); 
    std::fstream fs;
    TS_ASSERT_THROWS_NOTHING (fs.open (tmpFile.path().c_str(), std::fstream::in ));
    
    TSM_ASSERT("Cannot open file.",fs.is_open()); 
    
    std::stringstream ss;
    ss << fs.rdbuf();//read the file
    fs.close();

    TS_ASSERT_EQUALS ("HTTPS request succeeded", ss.str());
  }

};


#endif /* MANTID_DATAHANDLING_DOWNLOADFILETEST_H_ */