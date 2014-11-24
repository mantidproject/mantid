#ifndef MANTID_KERNEL_INTERNETSERVICETEST_H_
#define MANTID_KERNEL_INTERNETSERVICETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/NetworkProxy.h"
#include "MantidKernel/ProxyInfo.h"

#include <Poco/TemporaryFile.h>


#include <fstream>
#include <sstream>

using Mantid::Kernel::InternetHelper;
using namespace Mantid::Kernel;



namespace
{
  /**
   * Mock out the internet calls of this algorithm
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
}


class InternetHelperTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InternetHelperTest *createSuite() { return new InternetHelperTest(); }
  static void destroySuite( InternetHelperTest *suite ) { delete suite; }

  
  void test_sendRequest_HTTP()
  {
    MockedInternetHelper internetHelper;
    std::string url = "http://www.google.com";

    std::stringstream ss;
    int response = 0;
    TS_ASSERT_THROWS_NOTHING (response = internetHelper.sendRequest(url, ss);)
    TS_ASSERT_EQUALS (200, response);
    TS_ASSERT_EQUALS ("HTTP request succeeded", ss.str());
  }

  void test_sendRequest_HTTPS()
  {
    MockedInternetHelper internetHelper;
    std::string httpsUrl = "https://api.github.com/repos/mantidproject/mantid/contents";
    
    std::stringstream ss;
    int response = 0;
    TS_ASSERT_THROWS_NOTHING (response = internetHelper.sendRequest(httpsUrl, ss);)
    TS_ASSERT_EQUALS (200, response);
    TS_ASSERT_EQUALS ("HTTPS request succeeded", ss.str());
  }

  void test_DownloadFile_HTTP()
  {
    MockedInternetHelper internetHelper;
    std::string url = "http://www.google.com";
    Poco::TemporaryFile tmpFile;
    int response = internetHelper.downloadFile(url,tmpFile.path());
    TS_ASSERT_EQUALS (200, response);
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
    int response = internetHelper.downloadFile(httpsUrl,tmpFile.path());
    TS_ASSERT_EQUALS (200, response);
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


#endif /* MANTID_KERNEL_INTERNETSERVICETEST_H_ */
