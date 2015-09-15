#ifndef MANTID_KERNEL_INTERNETSERVICETEST_H_
#define MANTID_KERNEL_INTERNETSERVICETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/NetworkProxy.h"
#include "MantidKernel/ProxyInfo.h"

#include <Poco/TemporaryFile.h>
#include <Poco/Net/HTMLForm.h>
#include "Poco/Net/PartSource.h"
#include "Poco/Net/StringPartSource.h"

#include <fstream>
#include <sstream>

using Mantid::Kernel::InternetHelper;
using namespace Mantid::Kernel;

namespace {
/**
 * Mock out the internet calls of this algorithm
 */
class MockedInternetHelper : public InternetHelper {
protected:
  virtual int
  sendHTTPSRequest(const std::string &url, std::ostream &responseStream) {
    UNUSED_ARG(url);

    responseStream << "HTTPS request succeeded";
    return 200;
  }
  virtual int
  sendHTTPRequest(const std::string &url, std::ostream &responseStream) {
    UNUSED_ARG(url);
    responseStream << "HTTP request succeeded";
    return 200;
  }
};
}

class InternetHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InternetHelperTest *createSuite() { return new InternetHelperTest(); }
  static void destroySuite(InternetHelperTest *suite) { delete suite; }

  void test_sendRequest_HTTP() {
    MockedInternetHelper internetHelper;
    std::string url = "http://www.google.com";

    std::stringstream ss;
    int response = 0;
    TS_ASSERT_THROWS_NOTHING(response = internetHelper.sendRequest(url, ss);)
    TS_ASSERT_EQUALS(200, response);
    TS_ASSERT_EQUALS("HTTP request succeeded", ss.str());
  }

  void test_sendRequest_HTTPS() {
    MockedInternetHelper internetHelper;
    std::string httpsUrl =
        "https://api.github.com/repos/mantidproject/mantid/contents";

    std::stringstream ss;
    int response = 0;
    TS_ASSERT_THROWS_NOTHING(response =
                                 internetHelper.sendRequest(httpsUrl, ss);)
    TS_ASSERT_EQUALS(200, response);
    TS_ASSERT_EQUALS("HTTPS request succeeded", ss.str());
  }

  void test_DownloadFile_HTTP() {
    MockedInternetHelper internetHelper;
    std::string url = "http://www.google.com";
    Poco::TemporaryFile tmpFile;
    int response = internetHelper.downloadFile(url, tmpFile.path());
    TS_ASSERT_EQUALS(200, response);
    TSM_ASSERT("File has not been created.", tmpFile.exists());
    TSM_ASSERT("File is not a file.", tmpFile.isFile());
    std::fstream fs;
    TS_ASSERT_THROWS_NOTHING(fs.open(tmpFile.path().c_str(), std::fstream::in));

    TSM_ASSERT("Cannot open file.", fs.is_open());

    std::stringstream ss;
    ss << fs.rdbuf(); // read the file
    fs.close();

    TS_ASSERT_EQUALS("HTTP request succeeded", ss.str());
  }

  void test_DownloadFile_HTTPS() {
    MockedInternetHelper internetHelper;
    std::string httpsUrl =
        "https://api.github.com/repos/mantidproject/mantid/contents";
    Poco::TemporaryFile tmpFile;
    int response = internetHelper.downloadFile(httpsUrl, tmpFile.path());
    TS_ASSERT_EQUALS(200, response);
    TSM_ASSERT("File has not been created.", tmpFile.exists());
    TSM_ASSERT("File is not a file.", tmpFile.isFile());
    std::fstream fs;
    TS_ASSERT_THROWS_NOTHING(fs.open(tmpFile.path().c_str(), std::fstream::in));

    TSM_ASSERT("Cannot open file.", fs.is_open());

    std::stringstream ss;
    ss << fs.rdbuf(); // read the file
    fs.close();

    TS_ASSERT_EQUALS("HTTPS request succeeded", ss.str());
  }

  void test_ContentType_GetSet()
  {
    MockedInternetHelper internetHelper;
    TSM_ASSERT_EQUALS("Default content type is not application/json",internetHelper.getContentType(),"application/json");
    internetHelper.setContentType("test value");
    TSM_ASSERT_EQUALS("setContentType failed",internetHelper.getContentType(),"test value");
  }
  
  void test_Method_GetSet()
  {
    MockedInternetHelper internetHelper;
    TSM_ASSERT_EQUALS("Default method is not GET",internetHelper.getMethod(),"GET");
    internetHelper.setMethod("POST");
    TSM_ASSERT_EQUALS("setMethod failed",internetHelper.getMethod(),"POST");
  }  

  void test_Timeout_GetSet()
  {
    MockedInternetHelper internetHelper;
    TSM_ASSERT_EQUALS("Default timeout is not 30",internetHelper.getTimeout(),30);
    internetHelper.setTimeout(1);
    TSM_ASSERT_EQUALS("setTimeout failed",internetHelper.getTimeout(),1);
  }
  
  void test_Body_GetSet()
  {
    MockedInternetHelper internetHelper;
    TSM_ASSERT_EQUALS("Default body is not empty",internetHelper.getBody(),"");
    internetHelper.setBody("Test string");
    TSM_ASSERT_EQUALS("setBody failed",internetHelper.getBody(),"Test string");
    TSM_ASSERT_EQUALS("method is not POST",internetHelper.getMethod(),"POST");
    TSM_ASSERT_EQUALS("Contentlength is wrong",internetHelper.getContentLength(),11);
    internetHelper.setBody("");
    TSM_ASSERT_EQUALS("setBody failed",internetHelper.getBody(),"");
    TSM_ASSERT_EQUALS("method is not GET",internetHelper.getMethod(),"GET");
    TSM_ASSERT_EQUALS("Contentlength is wrong",internetHelper.getContentLength(),0);
  }
  
  void test_BodyStream_GetSet()
  {
    MockedInternetHelper internetHelper;
    std::ostringstream ss;
    ss << "Test string";
    TSM_ASSERT_EQUALS("Default body is not empty",internetHelper.getBody(),"");
    internetHelper.setBody(ss);
    TSM_ASSERT_EQUALS("setBody failed",internetHelper.getBody(),ss.str());
    TSM_ASSERT_EQUALS("method is not POST",internetHelper.getMethod(),"POST");
    TSM_ASSERT_EQUALS("Contentlength is wrong",internetHelper.getContentLength(),11);
    ss.str("");
    internetHelper.setBody(ss);
    TSM_ASSERT_EQUALS("setBody failed",internetHelper.getBody(),"");
    TSM_ASSERT_EQUALS("method is not GET",internetHelper.getMethod(),"GET");
    TSM_ASSERT_EQUALS("Contentlength is wrong",internetHelper.getContentLength(),0);
  }

  void test_BodyForm_GetSet()
  {
    MockedInternetHelper internetHelper;
	  Poco::Net::HTMLForm form(Poco::Net::HTMLForm::ENCODING_MULTIPART);
	  form.set("field1", "value1");
	  form.set("field2", "value 2");
	  form.set("field3", "value=3");
	  form.set("field4", "value&4");
	
	  form.addPart("attachment1", new Poco::Net::StringPartSource("This is an attachment"));
	  Poco::Net::StringPartSource* pSPS = new Poco::Net::StringPartSource("This is another attachment", "text/plain", "att2.txt");
	  pSPS->headers().set("Content-ID", "1234abcd");
	  form.addPart("attachment2", pSPS);
    TSM_ASSERT_EQUALS("Default body is not empty",internetHelper.getBody(),"");
    TSM_ASSERT_EQUALS("method is not GET",internetHelper.getMethod(),"GET");
    internetHelper.setBody(form);
    std::string body = internetHelper.getBody();
    TSM_ASSERT_DIFFERS("setBody failed \"--MIME_boundary\"",body.find("--MIME_boundary"),std::string::npos);
    TSM_ASSERT_DIFFERS("setBody failed \"This is an attachment\"",body.find("This is an attachment"),std::string::npos);
    TSM_ASSERT_DIFFERS("setBody failed \"This is another attachment\"",body.find("This is another attachment"),std::string::npos);
    TSM_ASSERT_DIFFERS("setBody failed \"field1\"",body.find("field1"),std::string::npos);
    TSM_ASSERT_DIFFERS("setBody failed \"field2\"",body.find("field2"),std::string::npos);
    TSM_ASSERT_DIFFERS("setBody failed \"field3\"",body.find("field3"),std::string::npos);
    TSM_ASSERT_DIFFERS("setBody failed \"field4\"",body.find("field4"),std::string::npos);
    TSM_ASSERT_DIFFERS("setBody failed \"value1\"",body.find("value1"),std::string::npos);
    TSM_ASSERT_DIFFERS("setBody failed \"value 2\"",body.find("value 2"),std::string::npos);
    TSM_ASSERT_DIFFERS("setBody failed \"value=3\"",body.find("value=3"),std::string::npos);
    TSM_ASSERT_DIFFERS("setBody failed \"value&4\"",body.find("value&4"),std::string::npos);
    TSM_ASSERT_EQUALS("method is not POST",internetHelper.getMethod(),"POST");
    TSM_ASSERT_LESS_THAN("Contentlength is wrong",700,internetHelper.getContentLength());
  }

  void test_Headers_GetSet()
  {
    MockedInternetHelper internetHelper;
    TSM_ASSERT_EQUALS("Default headers are not empty",internetHelper.headers().size(),0);
    internetHelper.addHeader("Test","value");
    internetHelper.addHeader("Test2","value2");
    TSM_ASSERT_EQUALS("addHeader failed",internetHelper.getHeader("Test"),"value");
    TSM_ASSERT_EQUALS("addHeader failed",internetHelper.getHeader("Test2"),"value2");
    internetHelper.removeHeader("Test");
    TSM_ASSERT_EQUALS("Remove failed",internetHelper.headers().size(),1);
    internetHelper.clearHeaders();
    TSM_ASSERT_EQUALS("Clear failed",internetHelper.headers().size(),0);
  }
};

#endif /* MANTID_KERNEL_INTERNETSERVICETEST_H_ */
