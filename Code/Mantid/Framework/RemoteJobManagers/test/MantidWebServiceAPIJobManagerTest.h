#ifndef MANTID_REMOTEJOBMANAGERS_MANTIDWEBSERVICEAPIJOBMANAGERTEST_H_
#define MANTID_REMOTEJOBMANAGERS_MANTIDWEBSERVICEAPIJOBMANAGERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidRemoteJobManagers/MantidWebServiceAPIJobManager.h"
#include "MantidRemoteJobManagers/SimpleJSON.h"

#include <boost/make_shared.hpp>

using namespace Mantid::API;
using namespace Mantid::RemoteJobManagers;

// TODO: use gmock

// This very simple mock returns an error status code and does not
// return any error message, which causes bad exception in the job
// manager (in most methods, while in for example queryAllRemoteJobs
// the JSON parsing exception is caught and turned into an
// std::runtime_error
class MockMantidAPIStatusNotFound : public MantidWebServiceAPIJobManager {
protected:
  std::istream &httpGet(const std::string & /*path*/,
                        const std::string & /*query_str*/ = "",
                        const std::string & /*username*/ = "",
                        const std::string & /*password*/ = "") const {
    return is;
  }

  std::istream &
  httpPost(const std::string & /*path*/,
           const MantidWebServiceAPIHelper::PostDataMap & /*postData*/,
           const MantidWebServiceAPIHelper::PostDataMap & /*fileData*/ =
               MantidWebServiceAPIHelper::PostDataMap(),
           const std::string & /*username*/ = "",
           const std::string & /*password*/ = "") const {
    return is;
  }

  Poco::Net::HTTPResponse::HTTPStatus lastStatus() const {
    return Poco::Net::HTTPResponse::HTTP_NOT_FOUND;
  }

private:
  mutable std::istringstream is;
};

// This one returns an error status code with an error message as
// expected from a Mantid WS API, including the parameter 'Err_Msg'.
class MockMantidAPIStatusNotFoundWithErrMsg
    : public MantidWebServiceAPIJobManager {
public:
  MockMantidAPIStatusNotFoundWithErrMsg() : MantidWebServiceAPIJobManager() {
    is.str("{\"foo\": \"err_msg\", \"Err_Msg\"=\"fake error\", \"param\": "
           "\"1\", }");
  }

protected:
  std::istream &httpGet(const std::string & /*path*/,
                        const std::string & /*query_str*/ = "",
                        const std::string & /*username*/ = "",
                        const std::string & /*password*/ = "") const {
    return is;
  }

  std::istream &
  httpPost(const std::string & /*path*/,
           const MantidWebServiceAPIHelper::PostDataMap & /*postData*/,
           const MantidWebServiceAPIHelper::PostDataMap & /*fileData*/ =
               MantidWebServiceAPIHelper::PostDataMap(),
           const std::string & /*username*/ = "",
           const std::string & /*password*/ = "") const {
    return is;
  }

  Poco::Net::HTTPResponse::HTTPStatus lastStatus() const {
    return Poco::Net::HTTPResponse::HTTP_NOT_FOUND;
  }

private:
  mutable std::istringstream is;
};

// Very simple mock that always returns an HTTP_OK=200 status code,
// but empty response body. There is no generic response body that
// would work for many or all of the methods of the
// MantidWebServiceAPIJobManager. More sophisticated "OK" mocks would
// need to be able to provide different response bodies (JSON output
// parameters).
class MockMantidAPIStatusOK : public MantidWebServiceAPIJobManager {
protected:
  std::istream &httpGet(const std::string & /*path*/,
                        const std::string & /*query_str*/ = "",
                        const std::string & /*username*/ = "",
                        const std::string & /*password*/ = "") const {
    return is;
  }

  std::istream &
  httpPost(const std::string & /*path*/,
           const MantidWebServiceAPIHelper::PostDataMap & /*postData*/,
           const MantidWebServiceAPIHelper::PostDataMap & /*fileData*/ =
               MantidWebServiceAPIHelper::PostDataMap(),
           const std::string & /*username*/ = "",
           const std::string & /*password*/ = "") const {
    return is;
  }

  Poco::Net::HTTPResponse::HTTPStatus lastStatus() const {
    return Poco::Net::HTTPResponse::HTTP_OK;
  }

private:
  mutable std::istringstream is;
};

class MantidWebServiceAPIJobManagerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MantidWebServiceAPIJobManagerTest *createSuite() {
    return new MantidWebServiceAPIJobManagerTest();
  }
  static void destroySuite(MantidWebServiceAPIJobManagerTest *suite) {
    delete suite;
  }

  void test_constructCasts() {
    // can create
    boost::shared_ptr<MockMantidAPIStatusOK> djm;
    TS_ASSERT(djm = boost::make_shared<MockMantidAPIStatusOK>());
    // can cast to inherited interfaces and base classes

    MockMantidAPIStatusOK wsapi;
    TSM_ASSERT(
        "Job manager constructed dynamically should cast to IRemoteJobManager",
        dynamic_cast<Mantid::API::IRemoteJobManager *>(djm.get()));
    TSM_ASSERT(
        "Job manager constructed statically should cast to IRemoteJobManger",
        dynamic_cast<Mantid::API::IRemoteJobManager *>(&wsapi));

    TSM_ASSERT("Job manager constructed dynamically should cast to "
               "MantidWebServiceAPIJobManager",
               dynamic_cast<
                   Mantid::RemoteJobManagers::MantidWebServiceAPIJobManager *>(
                   djm.get()));
    TSM_ASSERT("Job manager constructed statically should cast to "
               "MantidWebServiceAPIJobManager",
               dynamic_cast<
                   Mantid::RemoteJobManagers::MantidWebServiceAPIJobManager *>(
                   &wsapi));
  }

  void test_createWithFactory() {
    // The factory is tested in its own unit test, but here we can specifically
    // test the creation of Mantid WS API objects.

    // save facility before test
    const Mantid::Kernel::FacilityInfo &prevFac =
        Mantid::Kernel::ConfigService::Instance().getFacility();

    Mantid::Kernel::ConfigService::Instance().setFacility(SNSFac);
    TSM_ASSERT_THROWS_NOTHING(
        "create() with " + FermiName + " in the facility " + SNSFac +
            " should not throw",
        IRemoteJobManager_sptr jobManager =
            RemoteJobManagerFactory::Instance().create(FermiName));
    // Important: don't feel tempted to use this job manager, it will
    // interact/send jobs to the actual compute resource (and will only work
    // within its facility).

    // it should not be available here...
    Mantid::Kernel::ConfigService::Instance().setFacility(ISISFac);
    TSM_ASSERT_THROWS("create() with " + FermiName +
                          " in a facility other than " + SNSFac +
                          " should fail",
                      IRemoteJobManager_sptr jobManager =
                          RemoteJobManagerFactory::Instance().create(FermiName),
                      std::runtime_error);

    // restore facility to what it was before test
    Mantid::Kernel::ConfigService::Instance().setFacility(prevFac.name());
  }

  // if the response code is HTTP_OK, it ignores the response content
  void test_OKResponse() {

    MockMantidAPIStatusOK jm;
    checkJMOKResponseNoMsg(jm);
  }

  // If the response code is not ok, a JSON string is expected in the
  // response, with the parameter 'Err_Msg'
  void test_errorResponseWithErrMsg() {

    MockMantidAPIStatusNotFoundWithErrMsg jmErrMsg;
    checkJMWithErrResponse(jmErrMsg);
  }

  // what if 'Err_Msg' is not included in the response
  void test_errorResponseNoErrMsg() {

    MockMantidAPIStatusNotFound jmErr;
    checkJMErrWithoutErrMessage(jmErr);
  }

  void test_missingOrWrongParamsWithoutLogin() {

    // Note well: here and below these tests throw JSONParseException
    // because the current behavior of these methods is that the
    // request is sent (httpGet() or httpPost()). So an exception is thrown when
    // trying to parse the (wrong) response from the server. This test cases
    // should not be interpreted as "this should be the behavior" but rather
    // as "this is the present behavior".

    // Note also that many of these checks will not throw if using
    // MockMantidAPIStatusOK (HTTP_OK status from server, even if the
    // response is empty or inconsistent).
    MockMantidAPIStatusNotFound jm;
    TSM_ASSERT_THROWS("abort job without job ID should throw",
                      jm.abortRemoteJob(""), JSONParseException);
    TSM_ASSERT_THROWS("abort job with wrong job ID should throw",
                      jm.abortRemoteJob("anything_wrong"), JSONParseException);

    TSM_ASSERT_THROWS("download with wrong transaction ID should throw",
                      jm.downloadRemoteFile("any_wrong_transID", "remote_fname",
                                            "local_fname"),
                      JSONParseException);

    // Note that as an exception queryAllRemoteJobs does a bit more of
    // checking and throws std::runtime_error when something is wrong
    // in the server response.
    std::vector<IRemoteJobManager::RemoteJobInfo> infos;
    TSM_ASSERT_THROWS("query all jobs without logging in should throw",
                      infos = jm.queryAllRemoteJobs(), std::runtime_error);
    TSM_ASSERT_EQUALS(
        "there should not be any job information returned from the remote",
        infos.size(), 0);

    std::vector<std::string> files;
    TSM_ASSERT_THROWS(
        "query remote files with wrong transaction ID should throw",
        files = jm.queryRemoteFile("any_wrong_transID"), JSONParseException);
    TSM_ASSERT_EQUALS("The file list for a wrong transaction should be empty",
                      files.size(), 0);

    IRemoteJobManager::RemoteJobInfo info;
    TSM_ASSERT_THROWS("query job info should throw for wrong job ID",
                      info = jm.queryRemoteJob("any_wrong_jobID"),
                      JSONParseException);

    std::string id;
    TSM_ASSERT_THROWS("start transaction without logging in should throw",
                      id = jm.startRemoteTransaction(), JSONParseException);
    TSM_ASSERT_EQUALS("failed start transaction should not return any ID", id,
                      "");

    TSM_ASSERT_THROWS("stop transaction without logging in should throw",
                      jm.stopRemoteTransaction("a_wrong_transID"),
                      JSONParseException);

    std::string jobID;
    TSM_ASSERT_THROWS("submit job without logging in should throw",
                      id = jm.submitRemoteJob("a_wrong_transID", "executable",
                                              "--params 0", "name_for_job"),
                      JSONParseException);
    TSM_ASSERT_EQUALS("failed submit job should not return any ID", jobID, "");

    TSM_ASSERT_THROWS(
        "upload file without logging in should throw",
        jm.uploadRemoteFile("wrong_transID", "remote_fname", "local_fname"),
        JSONParseException);

    // and failed login at the end
    TSM_ASSERT_THROWS("authenticate with empty credentials should throw",
                      jm.authenticate("", ""), JSONParseException);
    TSM_ASSERT_THROWS("mocked authenticate should throw",
                      jm.authenticate("wrong_user", "no_pass"),
                      JSONParseException);
  }

private:
  // for when the server returns status == HTTP_OK
  void checkJMOKResponseNoMsg(MantidWebServiceAPIJobManager &jm) {
    TSM_ASSERT_THROWS_NOTHING(
        "abort job with ok response code from server should not throw",
        jm.abortRemoteJob("anything"));

    TSM_ASSERT_THROWS_NOTHING(
        "authenticate with ok response code from server should not throw",
        jm.authenticate("any_user", "any_pass"));

    TSM_ASSERT_THROWS_NOTHING(
        "download with ok response code from server should not throw",
        jm.downloadRemoteFile("any_transID", "remote_fname", "local_fname"));

    std::vector<IRemoteJobManager::RemoteJobInfo> infos;
    TSM_ASSERT_THROWS("query all jobs with ok response code but no content "
                      "from server should throw",
                      infos = jm.queryAllRemoteJobs(), std::runtime_error);

    std::vector<std::string> files;
    TSM_ASSERT_THROWS("query remote files with ok response code but no content "
                      "from server should throw",
                      files = jm.queryRemoteFile("any"), JSONParseException);
    TSM_ASSERT_EQUALS("The file list for a transaction should be empty",
                      files.size(), 0);

    IRemoteJobManager::RemoteJobInfo info;
    TSM_ASSERT_THROWS("query job info with ok response code from but no "
                      "content from server should throw",
                      info = jm.queryRemoteJob("any"), JSONParseException);

    std::string id;
    TSM_ASSERT_THROWS("start transaction with ok response code but no content "
                      "from server should throw",
                      id = jm.startRemoteTransaction(), JSONParseException);
    TSM_ASSERT_EQUALS("failed start transaction should not return any ID", id,
                      "");

    TSM_ASSERT_THROWS_NOTHING(
        "stop transaction with ok response code from server should not throw",
        jm.stopRemoteTransaction("a_wrong_transID"));

    std::string jobID;
    TSM_ASSERT_THROWS("submit job with ok response code but no content from "
                      "server should throw",
                      id = jm.submitRemoteJob("a_wrong_transID", "executable",
                                              "--params 0", "name_for_job"),
                      JSONParseException);
    TSM_ASSERT_EQUALS("mock submit job should not return non-empty ID", jobID,
                      "");

    TSM_ASSERT_THROWS(
        "upload file with ok response code but no content from server should "
        "throw",
        jm.uploadRemoteFile("wrong_transID", "remote_fname", "local_fname"),
        JSONParseException);
  }

  // for when the server returns status != HTTP_OK and a correctly
  // formated error response body
  void checkJMWithErrResponse(MantidWebServiceAPIJobManager &jm) {
    TSM_ASSERT_THROWS(
        "abort job with error response code from server should throw",
        jm.abortRemoteJob("anything"), JSONParseException);

    TSM_ASSERT_THROWS(
        "authenticate with error response code from server should throw",
        jm.authenticate("any_user", "any_pass"), JSONParseException);

    TSM_ASSERT_THROWS(
        "download with error response code from server should throw",
        jm.downloadRemoteFile("any_transID", "remote_fname", "local_fname"),
        JSONParseException);

    std::vector<IRemoteJobManager::RemoteJobInfo> infos;
    TSM_ASSERT_THROWS(
        "query all jobs with error response from server should throw",
        infos = jm.queryAllRemoteJobs(), std::runtime_error);

    std::vector<std::string> files;
    TSM_ASSERT_THROWS(
        "query remote files with error response code from server should throw",
        files = jm.queryRemoteFile("any"), JSONParseException);
    TSM_ASSERT_EQUALS("The file list for a wrong transaction should be empty",
                      files.size(), 0);

    IRemoteJobManager::RemoteJobInfo info;
    TSM_ASSERT_THROWS(
        "query job info with error response from server should throw",
        info = jm.queryRemoteJob("any"), JSONParseException);

    std::string id;
    TSM_ASSERT_THROWS(
        "start transaction with error response from server should throw",
        id = jm.startRemoteTransaction(), JSONParseException);
    TSM_ASSERT_EQUALS("failed start transaction should not return any ID", id,
                      "");

    TSM_ASSERT_THROWS(
        "stop transaction with error response from server should throw",
        jm.stopRemoteTransaction("a_wrong_transID"), JSONParseException);

    std::string jobID;
    TSM_ASSERT_THROWS("submit job with error response from server should throw",
                      id = jm.submitRemoteJob("a_wrong_transID", "executable",
                                              "--params 0", "name_for_job"),
                      JSONParseException);
    TSM_ASSERT_EQUALS("failed submit job should not return any ID", jobID, "");

    TSM_ASSERT_THROWS(
        "upload file with error response from server should throw",
        jm.uploadRemoteFile("wrong_transID", "remote_fname", "local_fname"),
        JSONParseException);
  }

  // for when the server returns an status code != HTTP_OK but the
  // response body is empty or an unexpected/badly formated JSON
  // output
  void checkJMErrWithoutErrMessage(MantidWebServiceAPIJobManager &jm) {
    TSM_ASSERT_THROWS("abort job with error response code but no content from "
                      "server should throw",
                      jm.abortRemoteJob("anything"), JSONParseException);

    TSM_ASSERT_THROWS(
        "authenticate with error response code but no content from server but "
        "no content should throw",
        jm.authenticate("any_user", "any_pass"), JSONParseException);

    TSM_ASSERT_THROWS(
        "download with error response code but no content from server should "
        "throw",
        jm.downloadRemoteFile("any_transID", "remote_fname", "local_fname"),
        JSONParseException);

    std::vector<IRemoteJobManager::RemoteJobInfo> infos;
    TSM_ASSERT_THROWS("query all jobs with error response from but no content "
                      "server should throw",
                      infos = jm.queryAllRemoteJobs(), std::runtime_error);

    std::vector<std::string> files;
    TSM_ASSERT_THROWS("query remote files with error response code but no "
                      "content from server should throw",
                      files = jm.queryRemoteFile("any"), JSONParseException);
    TSM_ASSERT_EQUALS("The file list for a wrong transaction should be empty",
                      files.size(), 0);

    IRemoteJobManager::RemoteJobInfo info;
    TSM_ASSERT_THROWS("query job info with error response but no content from "
                      "server should throw",
                      info = jm.queryRemoteJob("any"), JSONParseException);

    std::string id;
    TSM_ASSERT_THROWS("start transaction with error response but no content "
                      "from server should throw",
                      id = jm.startRemoteTransaction(), JSONParseException);
    TSM_ASSERT_EQUALS("failed start transaction should not return any ID", id,
                      "");

    TSM_ASSERT_THROWS("stop transaction with error response but no content "
                      "from server should throw",
                      jm.stopRemoteTransaction("a_wrong_transID"),
                      JSONParseException);

    std::string jobID;
    TSM_ASSERT_THROWS("submit job with error response but no content from "
                      "server should throw",
                      id = jm.submitRemoteJob("a_wrong_transID", "executable",
                                              "--params 0", "name_for_job"),
                      JSONParseException);
    TSM_ASSERT_EQUALS("failed submit job should not return any ID", jobID, "");

    TSM_ASSERT_THROWS(
        "upload file with error response but no content from server should "
        "throw",
        jm.uploadRemoteFile("wrong_transID", "remote_fname", "local_fname"),
        JSONParseException);
  }

  static const std::string SNSFac;
  static const std::string ISISFac;
  static const std::string FermiName;
  static const std::string SCARFName;
};

const std::string MantidWebServiceAPIJobManagerTest::SNSFac = "SNS";
const std::string MantidWebServiceAPIJobManagerTest::ISISFac = "ISIS";
const std::string MantidWebServiceAPIJobManagerTest::FermiName = "Fermi";
const std::string MantidWebServiceAPIJobManagerTest::SCARFName = "SCARF@STFC";

#endif // MANTID_REMOTEJOGMANAGERS_MANTIDWEBSERVICEJOBMANAGERTEST_H_
