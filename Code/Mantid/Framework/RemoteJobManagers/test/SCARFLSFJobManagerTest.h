#ifndef MANTID_REMOTEJOBMANAGERS_SCARFLSFJOBMANAGERTEST_H_
#define MANTID_REMOTEJOBMANAGERS_SCARFLSFJOBMANAGERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidRemoteJobManagers/SCARFLSFJobManager.h"

#include <boost/make_shared.hpp>
#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::RemoteJobManagers;

/**
 * Too simple mock up for the SCARF job manager. It will run any
 * method without any real communication with the remote compute
 * resource (in real life, the PAC web service of the LSF job
 * scheduler on SCARF). It simply returns 200 OK and a response
 * string.
 */
class MockedSCARFLSFJM : public SCARFLSFJobManager {
protected:
  virtual int doSendRequestGetResponse(
      const Poco::URI & /*url*/, std::ostream &response,
      const StringToStringMap & /*headers*/ = StringToStringMap(),
      const std::string & /*method*/ = std::string(),
      const std::string & /*body*/ = "") const {

    response << "response OK - mocked up";
    return 200;
  }
};

/**
 * One more crude mock up for the interaction with SCARF. This one
 * returns an error (the connection is fine, but the response from the
 * server is an error; example: wrong path, server bug, etc.).
 */
class MockedErrorResponse_SCARFLSFJM : public SCARFLSFJobManager {
protected:
  virtual int doSendRequestGetResponse(
      const Poco::URI & /*url*/, std::ostream &response,
      const StringToStringMap & /*headers*/ = StringToStringMap(),
      const std::string & /*method*/ = std::string(),
      const std::string & /*body*/ = "") const {

    response << "Error response - mocked up";
    return 404;
  }
};

/**
 * One more crude mock up for the interaction with SCARF. This one
 * raises an exception as if the (underlying) InternetHelper had found
 * a connection issue.
 */
class MockedConnectionError_SCARFLSFJM : public SCARFLSFJobManager {
protected:
  virtual int doSendRequestGetResponse(
      const Poco::URI & /*url*/, std::ostream & /*response*/,
      const StringToStringMap & /*headers*/ = StringToStringMap(),
      const std::string & /*method*/ = std::string(),
      const std::string & /*body*/ = "") const {

    // throw as if there was a connection error
    throw Mantid::Kernel::Exception::InternetError(
        "Mocked up exception - connection error");

    return 400;
  }
};

/**
 * One more crude mock up for the interaction with SCARF. This one
 * returns an OK code and a string that reads like what we expect when
 * doing a successful login request. That response only makes sense
 * for login calls.
 */
class MockedGoodLoginResponse_SCARFLSFJM : public SCARFLSFJobManager {
protected:
  virtual int doSendRequestGetResponse(
      const Poco::URI & /*url*/, std::ostream &response,
      const StringToStringMap & /*headers*/ = StringToStringMap(),
      const std::string & /*method*/ = std::string(),
      const std::string & /*body*/ = "") const {

    response << makeGoodLoginResponse();
    return 200;
  }

  std::string makeGoodLoginResponse() const {
    return "https://portal.scarf.rl.ac.uk/pltf/\n"
           "scarf9999\"2011-02-10T18:50:"
           "00Z\"cT6jHNOxZ0TpH0lZHxMyXNVCMv2ncX8b7u\n"
           "- response OK and login successful - mocked up";
    // this last line is not very orthodox, watch out if it
    // creates issues in the future
  }
};

/**
 * One more crude mock up for the interaction with SCARF. This one
 * derives from the "Login OK" mockup. It returns an OK code and
 * produces a response that: 1) looks like a succesful login when
 * authenticating, or 2) reads like a response with basic job status
 * information.
 */
class MockedGoodJobStatus_SCARFLSFJM
    : public MockedGoodLoginResponse_SCARFLSFJM {
public:
  MockedGoodJobStatus_SCARFLSFJM(const std::string &id, const std::string &name)
      : MockedGoodLoginResponse_SCARFLSFJM(), jobID(id), jobName(name){};

protected:
  virtual int doSendRequestGetResponse(
      const Poco::URI &url, std::ostream &response,
      const StringToStringMap & /*headers*/ = StringToStringMap(),
      const std::string & /*method*/ = std::string(),
      const std::string & /*body*/ = "") const {

    if (url.toString().find("cgi-bin/token.py")) {
      response << makeGoodLoginResponse();
    } else {
      response
          << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
             "<Jobs><Job><cmd>python /work/imat/webservice_test/test.py.py "
             "/work/imat/webservice_test/test_out/</cmd><extStatus>-</"
             "extStatus>"
             "<id>" << jobID << "</id><name>" << jobName
          << "</name><status>Running</status>"
             "</Job></Jobs>";
    }
    return 200;
  }

private:
  std::string jobID;
  std::string jobName;
};

/**
 * One more crude mock up for the interaction with SCARF. This one
 * returns an OK code and a string that reads like what we expect when
 * doing ping. This response only makes sense for ping calls.
 */
class MockedGoodPingResponse_SCARFLSFJM : public SCARFLSFJobManager {
protected:
  virtual int doSendRequestGetResponse(
      const Poco::URI & /*url*/, std::ostream &response,
      const StringToStringMap & /*headers*/ = StringToStringMap(),
      const std::string & /*method*/ = std::string(),
      const std::string & /*body*/ = "") const {

    response << "Web Services are ready:  mocked up";
    return 200;
  }
};

class SCARFLSFJobManagerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SCARFLSFJobManagerTest *createSuite() {
    return new SCARFLSFJobManagerTest();
  }
  static void destroySuite(SCARFLSFJobManagerTest *suite) { delete suite; }

  void test_createWithFactory() {
    // The factory is tested in its own unit test, but here we can specifically
    // test the creation of SCARFLSFJobManager objects.

    // save facility before test
    const Mantid::Kernel::FacilityInfo &prevFac =
        Mantid::Kernel::ConfigService::Instance().getFacility();

    Mantid::Kernel::ConfigService::Instance().setFacility("ISIS");
    TSM_ASSERT_THROWS_NOTHING(
        "The factory should create " + SCARFName + " without throwing",
        IRemoteJobManager_sptr jobManager =
            RemoteJobManagerFactory::Instance().create(SCARFName));
    // Important: don't feel tempted to use this job manager, it will
    // interact/send jobs to the actual cluster and will only work
    // within ISIS.

    // it should not be available here...
    Mantid::Kernel::ConfigService::Instance().setFacility("SNS");
    TSM_ASSERT_THROWS("The factory should throw when creating " + SCARFName +
                          " in a wrong facility",
                      IRemoteJobManager_sptr jobManager =
                          RemoteJobManagerFactory::Instance().create(SCARFName),
                      std::runtime_error);

    // restore facility to what it was before test
    Mantid::Kernel::ConfigService::Instance().setFacility(prevFac.name());
  }

  void test_construct() {
    // can create
    boost::shared_ptr<SCARFLSFJobManager> jm;
    TSM_ASSERT("Dynamic allocation of SCARFLSF job managers",
               jm = boost::make_shared<SCARFLSFJobManager>());
    SCARFLSFJobManager jm2;
    // do not use / call methods on these two

    TSM_ASSERT("SCARFLSF job managers allocated statically should cast to LSF "
               "job managers",
               dynamic_cast<Mantid::RemoteJobManagers::LSFJobManager *>(&jm2));
    TSM_ASSERT("SCARFLSF job managers allocated statically should cast to "
               "IRemoteJobManager",
               dynamic_cast<Mantid::API::IRemoteJobManager *>(&jm2));
    TSM_ASSERT(
        "SCARFLSF job managers allocated dynamically should cast to LSF job "
        "managers",
        dynamic_cast<Mantid::RemoteJobManagers::LSFJobManager *>(jm.get()));
    TSM_ASSERT("SCARFLSF job managers allocated dynamically should cast to "
               "IRemoteJobManager",
               dynamic_cast<Mantid::API::IRemoteJobManager *>(jm.get()));
  }

  void test_missingOrWrongParamsWithoutLogin() {
    MockedSCARFLSFJM jm;

    TSM_ASSERT_THROWS("abort job without job ID should throw",
                      jm.abortRemoteJob(""), std::runtime_error);
    TSM_ASSERT_THROWS("abort job with wrong job ID should throw",
                      jm.abortRemoteJob("anything_wrong"), std::runtime_error);

    TSM_ASSERT_THROWS("download with wrong transaction ID should throw",
                      jm.downloadRemoteFile("any_wrong_transID", "remote_fname",
                                            "local_fname"),
                      std::invalid_argument);

    std::vector<IRemoteJobManager::RemoteJobInfo> infos;
    TSM_ASSERT_THROWS("query all jobs without logging in should throw",
                      infos = jm.queryAllRemoteJobs(), std::runtime_error);
    TSM_ASSERT_EQUALS(
        "there should not be any job information returned from the remote",
        infos.size(), 0);

    std::vector<std::string> files;
    TSM_ASSERT_THROWS(
        "query remote files with wrong transaction ID should throw",
        files = jm.queryRemoteFile("any_wrong_transID"), std::invalid_argument);
    TSM_ASSERT_EQUALS("The file list for a wrong transaction should be empty",
                      files.size(), 0);

    IRemoteJobManager::RemoteJobInfo info;
    TSM_ASSERT_THROWS("query job info should throw for wrong job ID",
                      info = jm.queryRemoteJob("any_wrong_jobID"),
                      std::runtime_error);

    std::string id;
    TSM_ASSERT_THROWS("start transaction without logging in should throw",
                      id = jm.startRemoteTransaction(), std::runtime_error);
    TSM_ASSERT_EQUALS("failed start transaction should not return any ID", id,
                      "");

    TSM_ASSERT_THROWS("stop transaction without logging in should throw",
                      jm.stopRemoteTransaction("a_wrong_transID"),
                      std::runtime_error);

    std::string jobID;
    TSM_ASSERT_THROWS("submit job without logging in should throw",
                      id = jm.submitRemoteJob("a_wrong_transID", "executable",
                                              "--params 0", "name_for_job"),
                      std::runtime_error);
    TSM_ASSERT_EQUALS("failed submit job should not return any ID", jobID, "");

    TSM_ASSERT_THROWS(
        "upload file without logging in should throw",
        jm.uploadRemoteFile("wrong_transID", "remote_fname", "local_fname"),
        std::runtime_error);

    // and failed login at the end
    TSM_ASSERT_THROWS("authenticate with empty credentials should throw",
                      jm.authenticate("", ""), std::runtime_error);
    TSM_ASSERT_THROWS("mocked authenticate should throw",
                      jm.authenticate("wrong_user", "no_pass"),
                      std::runtime_error);
  }

  void test_missingOrWrongParamsFakeLogin() {
    goodUsername = "foo_user";
    goodPassword = "foo_password";

    // with this mock login succeeds, and otherwise the response corresponds to
    // a job status query by id
    MockedGoodJobStatus_SCARFLSFJM jm("job_id001", "job_name");
    TSM_ASSERT_THROWS_NOTHING("successful authentication should not throw",
                              jm.authenticate(goodUsername, goodPassword));

    TSM_ASSERT_THROWS("abort job without job ID should throw",
                      jm.abortRemoteJob(""), std::runtime_error);
    TSM_ASSERT_THROWS_NOTHING(
        "abort job with wrong job ID should not throw but show a warning",
        jm.abortRemoteJob("anything_wrong"));

    TSM_ASSERT_THROWS("download with wrong transaction ID should throw",
                      jm.downloadRemoteFile("any_wrong_transID", "remote_fname",
                                            "local_fname"),
                      std::invalid_argument);

    std::vector<std::string> files;
    TSM_ASSERT_THROWS(
        "query remote files with wrong transaction ID should throw",
        files = jm.queryRemoteFile("any_wrong_transID"), std::invalid_argument);
    TSM_ASSERT_EQUALS("The file list for a wrong transaction should be empty",
                      files.size(), 0);

    IRemoteJobManager::RemoteJobInfo info;
    TSM_ASSERT_THROWS("query job info should throw for wrong job ID",
                      info = jm.queryRemoteJob("any_wrong_jobID"),
                      std::runtime_error);

    TSM_ASSERT_THROWS(
        "stop transaction when logged in, but with wrong transaction ID, "
        "should throw",
        jm.stopRemoteTransaction("a_wrong_transID"), std::invalid_argument);

    std::string id;
    std::string jobID;
    TSM_ASSERT_THROWS(
        "submit job when logged in, with a wrong transaction ID, should throw",
        id = jm.submitRemoteJob("a_wrong_transID", "executable", "--params 0",
                                "name_for_job"),
        std::invalid_argument);
    TSM_ASSERT_EQUALS("failed submit job should not return any ID", jobID, "");

    TSM_ASSERT_THROWS(
        "upload file when logged in, with a wrong transaction ID, should throw",
        jm.uploadRemoteFile("wrong_transID", "remote_fname", "local_fname"),
        std::invalid_argument);
  }

  /// Login is required before running any other command with SCARF (except
  /// ping)
  void test_auth() {
    goodUsername = "foo_user";
    goodPassword = "foo_password";

    // severe (connection) error
    MockedConnectionError_SCARFLSFJM err;
    TSM_ASSERT_THROWS(
        "authentication should throw if there is a connection error",
        err.authenticate(goodUsername, goodPassword), std::runtime_error);

    // standard mocked response: looks like an unsuccessful login attempt
    MockedSCARFLSFJM loginFails;
    TSM_ASSERT_THROWS(
        "authentication should throw if the server response looks wrong",
        loginFails.authenticate(goodUsername, goodPassword),
        std::runtime_error);

    // successful login attempt
    MockedGoodLoginResponse_SCARFLSFJM login;
    TSM_ASSERT_THROWS_NOTHING("successful authentication should not throw",
                              login.authenticate(goodUsername, goodPassword));
  }

  void test_startRemoteTransaction() {
    boost::shared_ptr<MockedGoodLoginResponse_SCARFLSFJM> jm;
    TSM_ASSERT("dynamical allocation of job manager should not fail",
               jm = boost::make_shared<MockedGoodLoginResponse_SCARFLSFJM>());

    std::string tid;
    TSM_ASSERT_THROWS("start transaction should throw when not logged in",
                      tid = jm->startRemoteTransaction(), std::runtime_error);

    TSM_ASSERT_THROWS_NOTHING("successful authentication should not throw",
                              jm->authenticate("user", "pass"));
    TSM_ASSERT_THROWS_NOTHING(
        "start transaction should not throw when logged in",
        tid = jm->startRemoteTransaction());
    TSM_ASSERT("a successful start transaction should not return an empty ID",
               tid != "");
  }

  void test_stopRemoteTransaction() {
    boost::shared_ptr<MockedGoodLoginResponse_SCARFLSFJM> jm;
    TSM_ASSERT("dynamical allocation of job manager should not fail",
               jm = boost::make_shared<MockedGoodLoginResponse_SCARFLSFJM>());

    TSM_ASSERT_THROWS_NOTHING("successful authentication should not throw",
                              jm->authenticate("user", "pass"));
    std::string tid;
    TSM_ASSERT_THROWS_NOTHING(
        "start remote transaction should not throw when logged in",
        tid = jm->startRemoteTransaction());
    TSM_ASSERT("a successful start transaction should return non-empty ID",
               tid != "");

    TSM_ASSERT_THROWS("stop transaction with wrong ID should throw",
                      jm->stopRemoteTransaction("wrong_stop_id"),
                      std::invalid_argument);

    // positive at last:
    TSM_ASSERT_THROWS_NOTHING(
        "stop transaction with correct ID should not throw",
        jm->stopRemoteTransaction(tid));
    TSM_ASSERT_THROWS("stop transaction with an ID of a transaction already "
                      "stopped should throw",
                      jm->stopRemoteTransaction(tid), std::invalid_argument);
  }

  void test_submit() {
    boost::shared_ptr<MockedSCARFLSFJM> jm;
    TSM_ASSERT("dynamical allocation of job manager should throw fail",
               jm = boost::make_shared<MockedSCARFLSFJM>());

    std::string id;
    TSM_ASSERT_THROWS("submit job wihtout logging in should throw",
                      id = jm->submitRemoteJob("a_wrong_transID", "executable",
                                               "--params 0", "name_for_job"),
                      std::runtime_error);
    TSM_ASSERT_EQUALS("faild submit job should not return non-empty ID", id,
                      "");

    MockedErrorResponse_SCARFLSFJM err;
    TSM_ASSERT_THROWS("submit job with error response from server should throw",
                      id = err.submitRemoteJob("a_wrong_transID", "executable",
                                               "--params 1", "name_for_job"),
                      std::runtime_error);
    TSM_ASSERT_EQUALS("faild submit job should not return non-empty ID", id,
                      "");
  }

  void test_download() {
    MockedGoodLoginResponse_SCARFLSFJM jm;
    TSM_ASSERT_THROWS_NOTHING("successful authentication should not throw",
                              jm.authenticate("user", "pass"));
    std::string tid;
    TSM_ASSERT_THROWS_NOTHING("successful start transaction should not throw",
                              tid = jm.startRemoteTransaction());
    TSM_ASSERT("successful start transaction should return non-empty ID",
               tid != "");
    std::string localName = "local_name";
    // no job submitted - cannot get files
    TSM_ASSERT_THROWS("download when no job has been submitted should throw",
                      jm.downloadRemoteFile(tid, "remote_name", localName),
                      std::runtime_error);
    // submit one job and it should be possible to download files
    TSM_ASSERT_THROWS_NOTHING(
        "successful submit job should not throw",
        jm.submitRemoteJob(tid, "executable", "--params 1", "name_for_job"));
    TSM_ASSERT_THROWS_NOTHING(
        "successful download should not throw",
        jm.downloadRemoteFile(tid, "remote_name", localName));
    // but being a fake, the file is not there:
    TSM_ASSERT(
        "this fake job manager for testing should not create downloaded files",
        !Poco::File(localName).exists());
  }

  void test_queryStatus() {
    // this one is the basic mock up which doesn't provide the response content
    // that we need
    MockedSCARFLSFJM jm0;

    std::vector<IRemoteJobManager::RemoteJobInfo> infos;
    TSM_ASSERT_THROWS("query all jobs should throw when not logged in",
                      infos = jm0.queryAllRemoteJobs(), std::runtime_error);
    TSM_ASSERT_EQUALS(
        "failed query all jobs should not return non-empty information",
        infos.size(), 0);

    MockedErrorResponse_SCARFLSFJM err;
    TSM_ASSERT_THROWS(
        "query all jobs should throw when the server returns an error message",
        infos = err.queryAllRemoteJobs(), std::runtime_error);
    TSM_ASSERT_EQUALS("failed query all jobs should not return, and even less "
                      "return non-empty list of job info objects",
                      infos.size(), 0);

    std::string id("id0001");
    std::string name("name1");
    MockedGoodJobStatus_SCARFLSFJM jm(id, name);
    TSM_ASSERT_THROWS_NOTHING("successful authentication should not throw",
                              jm.authenticate("user", "password"));
    TSM_ASSERT_THROWS_NOTHING("successful query all jobs should not throw",
                              infos = jm.queryAllRemoteJobs());
    std::string tid;
    TSM_ASSERT_THROWS_NOTHING("successful start transaction should not throw",
                              tid = jm.startRemoteTransaction());
    TSM_ASSERT_THROWS_NOTHING(
        "successful submit job should not throw",
        jm.submitRemoteJob(tid, "executable", "--params 1", "name_for_job"));
    TSM_ASSERT_EQUALS(
        "no job information should be returned from this fake job queries",
        infos.size(), 0);
    if (infos.size() > 0) {
      TSM_ASSERT_EQUALS("the job ID provided by query all jobs should match "
                        "the ID obtained when submitting the job",
                        infos[0].id, id);
      TSM_ASSERT_EQUALS("the job name provided by query all jobs should match "
                        "the name obtained when submitting the job",
                        infos[0].name, name);
    }
  }

  void test_queryRemoteFile() {
    MockedGoodLoginResponse_SCARFLSFJM jm;
    TSM_ASSERT_THROWS_NOTHING("successful authentication should not throw",
                              jm.authenticate("user", "pass"));
    std::string tid;
    TSM_ASSERT_THROWS_NOTHING("successful start transaction should not throw",
                              tid = jm.startRemoteTransaction());
    TSM_ASSERT("successful start transaction should return non-empty ID",
               tid != "");
    // should get a bad/unrecognized response
    TSM_ASSERT_THROWS(
        "query remote file with wrong response from server should throw",
        jm.queryRemoteFile(tid), std::runtime_error);

    TSM_ASSERT_THROWS_NOTHING(
        "successful submit job should not throw",
        jm.submitRemoteJob(tid, "executable", "--params 1", "name_for_job"));
    TSM_ASSERT_THROWS_NOTHING("successful query remote file with correct "
                              "transaction ID should not throw",
                              jm.queryRemoteFile(tid));
  }

  void test_queryStatusByID() {
    MockedSCARFLSFJM jmFail;

    std::string id("id001");
    IRemoteJobManager::RemoteJobInfo info;
    TSM_ASSERT_THROWS("query job status without logging in should throw",
                      info = jmFail.queryRemoteJob(id), std::runtime_error);
    TSM_ASSERT_THROWS(
        "authentication with wrong response from server should throw",
        jmFail.authenticate("user", "pass"), std::runtime_error);
    TSM_ASSERT_THROWS(
        "query job status without successful authentication should throw",
        info = jmFail.queryRemoteJob(id), std::runtime_error);

    MockedErrorResponse_SCARFLSFJM err;
    TSM_ASSERT_THROWS("query job status with error response should throw",
                      info = err.queryRemoteJob(id), std::runtime_error);
    TSM_ASSERT_EQUALS("failed query status should not return non-empty job id",
                      info.id, "");
    TSM_ASSERT_EQUALS(
        "failed query status should not return non-empty job name", info.name,
        "");

    std::string name("name01");
    MockedGoodJobStatus_SCARFLSFJM jm(id, name);
    TSM_ASSERT_THROWS_NOTHING("successful authentication should not throw",
                              jm.authenticate("user", "password"));
    TSM_ASSERT_THROWS(
        "quer job status without having submitted the job should throw",
        info = jm.queryRemoteJob(id), std::runtime_error);

    std::string tid;
    TSM_ASSERT_THROWS_NOTHING("successful start transaction should not throw",
                              tid = jm.startRemoteTransaction());
    TSM_ASSERT_THROWS_NOTHING(
        "successful submit job with correct transaction ID should not throw",
        id = jm.submitRemoteJob(tid, "exec", "--p 1", "job_name"));
    TSM_ASSERT_THROWS("query job status with strange response from sever (not "
                      "containing expected status xml tree) should fail",
                      info = jm.queryRemoteJob(id), std::runtime_error);
  }

  void test_cancel() {
    MockedSCARFLSFJM jmFail;
    std::string tid("trans001");
    TSM_ASSERT_THROWS("stop transaction without logging in should throw",
                      jmFail.stopRemoteTransaction(tid), std::runtime_error);

    MockedErrorResponse_SCARFLSFJM err;
    TSM_ASSERT_THROWS(
        "stop transaction with error response from server should throw",
        err.stopRemoteTransaction(tid), std::runtime_error);
    TSM_ASSERT_THROWS(
        "authenticate with error response from server should throw",
        err.authenticate("user", "pass"), std::runtime_error);
    IRemoteJobManager::RemoteJobInfo info;
    TSM_ASSERT_THROWS("query job info with wrong job ID should throw",
                      info = err.queryRemoteJob("012"), std::runtime_error);

    std::string name("name01");
    MockedGoodLoginResponse_SCARFLSFJM jm;
    std::string newID;
    TSM_ASSERT_THROWS_NOTHING("successful authentication should not throw",
                              jm.authenticate("user", "pass"));
    TSM_ASSERT_THROWS_NOTHING("successful start transaction should not throw",
                              newID = jm.startRemoteTransaction());
    TSM_ASSERT_THROWS(
        "stop transaction with a wrong transaction ID should throw",
        jm.stopRemoteTransaction(tid), std::invalid_argument);
    TSM_ASSERT_THROWS_NOTHING(
        "stop transaction with correct ID should not throw",
        jm.stopRemoteTransaction(newID));
  }

  void test_upload() {
    MockedGoodLoginResponse_SCARFLSFJM jm;
    TSM_ASSERT_THROWS_NOTHING("successful authentication should not throw",
                              jm.authenticate("userid", "pass"));
    std::string tid;
    TSM_ASSERT_THROWS_NOTHING("successful start transaction should not throw",
                              tid = jm.startRemoteTransaction());
    TSM_ASSERT(
        "successful start transaction should return non-empty transaction ID",
        tid != "");
    /// fails - missing file
    TSM_ASSERT_THROWS_NOTHING(
        "successful upload should not throw",
        jm.uploadRemoteFile(tid, "remote_name", "local_name"));
  }

  void test_errorResponseFromServer() {
    MockedErrorResponse_SCARFLSFJM err;
    TSM_ASSERT_THROWS(
        "authentication with error response from server should throw",
        err.authenticate("whoami", "pass"), std::runtime_error);
    TSM_ASSERT_THROWS("ping with error response from server should throw",
                      err.ping(), std::runtime_error);
  }

  // logout must run after all the (positive) tests
  void test_logout() {
    MockedErrorResponse_SCARFLSFJM err;
    TSM_ASSERT_THROWS("authenticate with empty credentials should throw",
                      err.authenticate("", ""), std::runtime_error);
    TSM_ASSERT_THROWS("logout with error response from server should throw",
                      err.logout(), std::runtime_error);

    MockedGoodLoginResponse_SCARFLSFJM jm;
    TSM_ASSERT_THROWS_NOTHING("successful authentication should not throw",
                              jm.authenticate("user", "pass"));
    std::string tid;
    TSM_ASSERT_THROWS_NOTHING("successful start transaction should not throw",
                              tid = jm.startRemoteTransaction());
    TSM_ASSERT_THROWS_NOTHING("successful stop transaction should not throw",
                              jm.stopRemoteTransaction(tid));
    TSM_ASSERT_THROWS_NOTHING("logout should not throw when logged in",
                              jm.logout());
  }

  void test_ping() {
    MockedConnectionError_SCARFLSFJM err;
    bool res = false;
    TSM_ASSERT_THROWS("ping with connection error should throw",
                      res = err.ping(), std::runtime_error);
    TSM_ASSERT("failed ping should not return, and even less return true",
               !res);

    MockedErrorResponse_SCARFLSFJM errResp;
    TSM_ASSERT_THROWS("ping with error response from server should throw",
                      res = errResp.ping(), std::runtime_error);
    TSM_ASSERT("failed ping should not return, and even less return true",
               !res);

    /// ping is fine without logging in
    MockedGoodPingResponse_SCARFLSFJM good;
    TSM_ASSERT_THROWS_NOTHING("successful ping should not throw",
                              res = good.ping());
    TSM_ASSERT("successful ping should return true", res);
  }

  void test_failConnect() {
    MockedConnectionError_SCARFLSFJM fail;
    TSM_ASSERT_THROWS("authentication with connection error should throw",
                      fail.authenticate("userlogin", "pass"),
                      std::runtime_error);
    TSM_ASSERT_THROWS(
        "download with connection error, without logging in, should throw",
        fail.downloadRemoteFile("any_wrong_transID", "remote_fname",
                                "local_fname"),
        std::invalid_argument);
    TSM_ASSERT_THROWS(
        "ping with connection error, without logging in,  should throw",
        fail.ping(), std::runtime_error);
  }

  void test_commandAfterLogout() {
    MockedGoodLoginResponse_SCARFLSFJM jm;
    TSM_ASSERT_THROWS_NOTHING("successful authentication should not throw",
                              jm.authenticate("username", "pass"));
    TSM_ASSERT_THROWS_NOTHING("successful logout should not throw",
                              jm.logout());

    // Once you log out all actions should produce an exception
    std::string tid, jid;
    TSM_ASSERT_THROWS("start transaction after logging out should throw",
                      tid = jm.startRemoteTransaction(), std::runtime_error);

    TSM_ASSERT_THROWS("submit job after logging out should throw",
                      jid = jm.submitRemoteJob("a_wrong_transID", "executable",
                                               "--params 1", "name_for_job"),
                      std::runtime_error);

    // log in again, back to normal
    TSM_ASSERT_THROWS_NOTHING(
        "second successful authentication should not throw",
        jm.authenticate("user", "pass"));
    TSM_ASSERT_THROWS_NOTHING(
        "successful start transaction after second log in should not throw",
        tid = jm.startRemoteTransaction());
    TSM_ASSERT("successful start transaction should return non-empty ID",
               "" != tid);

    TSM_ASSERT_THROWS(
        "submit job with wrong transaction ID should throw",
        jid = jm.submitRemoteJob("no_no_wrong_ID", "executable", "--params 1"),
        std::invalid_argument);
    TSM_ASSERT_THROWS_NOTHING(
        "successful submit job (correct transaction ID, after logging in for a "
        "second time) should not throw",
        jid = jm.submitRemoteJob(tid, "executable", "--params 1"));
    TSM_ASSERT("successful submit job, after logging in for a second time, "
               "should return non-empty ID",
               "" != jid);
  }

private:
  std::string goodUsername;
  std::string goodPassword;
  static const std::string SCARFName;
};

const std::string SCARFLSFJobManagerTest::SCARFName = "SCARF@STFC";

#endif // MANTID_REMOTEJOBMANAGERS_SCARFLSFJOBMANAGERTEST_H_
