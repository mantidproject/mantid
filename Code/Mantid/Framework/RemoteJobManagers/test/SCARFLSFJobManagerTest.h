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
      const std::string &url, std::ostream &response,
      const StringToStringMap &headers = StringToStringMap(),
      const std::string &method = std::string(),
      const std::string &body = "") const {
    UNUSED_ARG(url);
    UNUSED_ARG(headers);
    UNUSED_ARG(method);
    UNUSED_ARG(body);

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
      const std::string &url, std::ostream &response,
      const StringToStringMap &headers = StringToStringMap(),
      const std::string &method = std::string(),
      const std::string &body = "") const {
    UNUSED_ARG(url);
    UNUSED_ARG(response);
    UNUSED_ARG(headers);
    UNUSED_ARG(method);
    UNUSED_ARG(body);

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
      const std::string &url, std::ostream &response,
      const StringToStringMap &headers = StringToStringMap(),
      const std::string &method = std::string(),
      const std::string &body = "") const {
    UNUSED_ARG(url);
    UNUSED_ARG(response);
    UNUSED_ARG(headers);
    UNUSED_ARG(method);
    UNUSED_ARG(body);

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
      const std::string &url, std::ostream &response,
      const StringToStringMap &headers = StringToStringMap(),
      const std::string &method = std::string(),
      const std::string &body = "") const {
    UNUSED_ARG(url);
    UNUSED_ARG(headers);
    UNUSED_ARG(method);
    UNUSED_ARG(body);

    response << makeGoodLoginResponse();
    return 200;
  }

  std::string makeGoodLoginResponse() const {
    return "https://portal.scarf.rl.ac.uk/platf\n"
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
      const std::string &url, std::ostream &response,
      const StringToStringMap &headers = StringToStringMap(),
      const std::string &method = std::string(),
      const std::string &body = "") const {
    UNUSED_ARG(url);
    UNUSED_ARG(headers);
    UNUSED_ARG(method);
    UNUSED_ARG(body);

    if (url.find("cgi-bin/token.py")) {
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
      const std::string &url, std::ostream &response,
      const StringToStringMap &headers = StringToStringMap(),
      const std::string &method = std::string(),
      const std::string &body = "") const {
    UNUSED_ARG(url);
    UNUSED_ARG(response);
    UNUSED_ARG(headers);
    UNUSED_ARG(method);
    UNUSED_ARG(body);

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
    TS_ASSERT_THROWS_NOTHING(
        IRemoteJobManager_sptr jobManager =
            RemoteJobManagerFactory::Instance().create(SCARFName));
    // Important: don't feel tempted to use this job manager, it will
    // interact/send jobs to the actual cluster and will only work
    // within ISIS.

    // it should not be available here...
    Mantid::Kernel::ConfigService::Instance().setFacility("SNS");
    TS_ASSERT_THROWS(IRemoteJobManager_sptr jobManager =
                         RemoteJobManagerFactory::Instance().create(SCARFName),
                     std::runtime_error);

    // restore facility to what it was before test
    Mantid::Kernel::ConfigService::Instance().setFacility(prevFac.name());
  }

  void test_construct() {
    // can create
    boost::shared_ptr<SCARFLSFJobManager> jm;
    TS_ASSERT(jm = boost::make_shared<SCARFLSFJobManager>());
    SCARFLSFJobManager jm2;
    // do not use / call methods on these two

    TS_ASSERT(dynamic_cast<Mantid::RemoteJobManagers::LSFJobManager *>(&jm2));
    TS_ASSERT(dynamic_cast<Mantid::API::IRemoteJobManager *>(&jm2));
    TS_ASSERT(
        dynamic_cast<Mantid::RemoteJobManagers::LSFJobManager *>(jm.get()));
    TS_ASSERT(dynamic_cast<Mantid::API::IRemoteJobManager *>(jm.get()));
  }

  void test_missingOrWrongParamsWithoutLogin() {
    MockedSCARFLSFJM jm;

    TS_ASSERT_THROWS(jm.abortRemoteJob(""), std::runtime_error);
    TS_ASSERT_THROWS(jm.abortRemoteJob("anything_wrong"), std::runtime_error);

    TS_ASSERT_THROWS(jm.downloadRemoteFile("any_wrong_transID", "remote_fname",
                                           "local_fname"),
                     std::invalid_argument);

    std::vector<IRemoteJobManager::RemoteJobInfo> infos;
    TS_ASSERT_THROWS(infos = jm.queryAllRemoteJobs(), std::runtime_error);
    TS_ASSERT_EQUALS(infos.size(), 0);

    std::vector<std::string> files;
    TS_ASSERT_THROWS(files = jm.queryRemoteFile("any_wrong_transID"),
                     std::invalid_argument);
    TS_ASSERT_EQUALS(files.size(), 0);

    IRemoteJobManager::RemoteJobInfo info;
    TS_ASSERT_THROWS(info = jm.queryRemoteJob("any_wrong_jobID"),
                     std::runtime_error);

    // not logged in so it should throw
    std::string id;
    TS_ASSERT_THROWS(id = jm.startRemoteTransaction(), std::runtime_error);
    TS_ASSERT_EQUALS(id, "");

    // should fail with runtime_error (not logged in)
    TS_ASSERT_THROWS(jm.stopRemoteTransaction("a_wrong_transID"),
                     std::runtime_error);

    std::string jobID;
    TS_ASSERT_THROWS(id = jm.submitRemoteJob("a_wrong_transID", "executable",
                                             "--params 0", "name_for_job"),
                     std::runtime_error);
    TS_ASSERT_EQUALS(jobID, "");

    TS_ASSERT_THROWS(
        jm.uploadRemoteFile("wrong_transID", "remote_fname", "local_fname"),
        std::runtime_error);

    // and failed login at the end
    TS_ASSERT_THROWS(jm.authenticate("", ""), std::runtime_error);
    TS_ASSERT_THROWS(jm.authenticate("wrong_user", "no_pass"),
                     std::runtime_error);
  }

  void test_missingOrWrongParamsFakeLogin() {}

  /// Login is required before running any other command with SCARF (except
  /// ping)
  void test_auth() {
    goodUsername = "foo_user";
    goodPassword = "foo_password";

    // severe (connection) error
    MockedConnectionError_SCARFLSFJM err;
    TS_ASSERT_THROWS(err.authenticate(goodUsername, goodPassword),
                     std::runtime_error);

    // standard mocked response: looks like an unsuccessful login attempt
    MockedSCARFLSFJM loginFails;
    TS_ASSERT_THROWS(loginFails.authenticate(goodUsername, goodPassword),
                     std::runtime_error);

    // successful login attempt
    MockedGoodLoginResponse_SCARFLSFJM login;
    TS_ASSERT_THROWS_NOTHING(login.authenticate(goodUsername, goodPassword));
  }

  void test_startRemoteTransaction() {
    boost::shared_ptr<MockedGoodLoginResponse_SCARFLSFJM> jm;
    TS_ASSERT(jm = boost::make_shared<MockedGoodLoginResponse_SCARFLSFJM>());

    TS_ASSERT_THROWS_NOTHING(jm->authenticate("user", "pass"));
    std::string tid;
    TS_ASSERT_THROWS_NOTHING(tid = jm->startRemoteTransaction());
    TS_ASSERT(tid != "");
  }

  void test_stopRemoteTransaction() {
    boost::shared_ptr<MockedGoodLoginResponse_SCARFLSFJM> jm;
    TS_ASSERT(jm = boost::make_shared<MockedGoodLoginResponse_SCARFLSFJM>());

    TS_ASSERT_THROWS_NOTHING(jm->authenticate("user", "pass"));
    std::string tid;
    TS_ASSERT_THROWS_NOTHING(tid = jm->startRemoteTransaction());
    TS_ASSERT(tid != "");

    TS_ASSERT_THROWS(jm->stopRemoteTransaction("wrong_stop_id"),
                     std::invalid_argument);

    // positive at last:
    TS_ASSERT_THROWS_NOTHING(jm->stopRemoteTransaction(tid));
    TS_ASSERT_THROWS(jm->stopRemoteTransaction(tid), std::invalid_argument);
  }

  void test_submit() {
    boost::shared_ptr<MockedSCARFLSFJM> jm;
    TS_ASSERT(jm = boost::make_shared<MockedSCARFLSFJM>());

    std::string id;
    TS_ASSERT_THROWS(id = jm->submitRemoteJob("a_wrong_transID", "executable",
                                              "--params 0", "name_for_job"),
                     std::runtime_error);
    TS_ASSERT_EQUALS(id, "");

    MockedErrorResponse_SCARFLSFJM err;
    TS_ASSERT_THROWS(id = err.submitRemoteJob("a_wrong_transID", "executable",
                                              "--params 1", "name_for_job"),
                     std::runtime_error);
    TS_ASSERT_EQUALS(id, "");
  }

  void test_download() {
    MockedGoodLoginResponse_SCARFLSFJM jm;
    TS_ASSERT_THROWS_NOTHING(jm.authenticate("user", "pass"));
    std::string tid;
    TS_ASSERT_THROWS_NOTHING(tid = jm.startRemoteTransaction());
    TS_ASSERT(tid != "");
    std::string localName = "local_name";
    // no job submitted - cannot get files
    TS_ASSERT_THROWS(jm.downloadRemoteFile(tid, "remote_name", localName),
                     std::runtime_error);
    // submit one job and it should be possible to download files
    TS_ASSERT_THROWS_NOTHING(
        jm.submitRemoteJob(tid, "executable", "--params 1", "name_for_job"));
    TS_ASSERT_THROWS_NOTHING(
        jm.downloadRemoteFile(tid, "remote_name", localName));
    // but being a fake, the file is not there:
    TS_ASSERT(!Poco::File(localName).exists());
  }

  void test_queryStatus() {
    // this one is the basic mock up which doesn't provide the response content
    // that we need
    MockedSCARFLSFJM jm0;

    std::vector<IRemoteJobManager::RemoteJobInfo> infos;
    TS_ASSERT_THROWS(infos = jm0.queryAllRemoteJobs(), std::runtime_error);
    TS_ASSERT_EQUALS(infos.size(), 0);

    MockedErrorResponse_SCARFLSFJM err;
    TS_ASSERT_THROWS(infos = err.queryAllRemoteJobs(), std::runtime_error);
    TS_ASSERT_EQUALS(infos.size(), 0);

    std::string id("id0001");
    std::string name("name1");
    MockedGoodJobStatus_SCARFLSFJM jm(id, name);
    TS_ASSERT_THROWS_NOTHING(jm.authenticate("user", "password"));
    TS_ASSERT_THROWS_NOTHING(infos = jm.queryAllRemoteJobs());
    std::string tid;
    TS_ASSERT_THROWS_NOTHING(tid = jm.startRemoteTransaction());
    TS_ASSERT_THROWS_NOTHING(
        jm.submitRemoteJob(tid, "executable", "--params 1", "name_for_job"));
    TS_ASSERT_EQUALS(infos.size(), 0);
    if (infos.size() > 0) {
      TS_ASSERT_EQUALS(infos[0].id, id);
      TS_ASSERT_EQUALS(infos[0].name, name);
    }
  }

  void test_queryRemoteFile() {
    MockedGoodLoginResponse_SCARFLSFJM jm;
    TS_ASSERT_THROWS_NOTHING(jm.authenticate("user", "pass"));
    std::string tid;
    TS_ASSERT_THROWS_NOTHING(tid = jm.startRemoteTransaction());
    TS_ASSERT(tid != "");
    // should get a bad/unrecognized response
    TS_ASSERT_THROWS(jm.queryRemoteFile(tid), std::runtime_error);

    TS_ASSERT_THROWS_NOTHING(
        jm.submitRemoteJob(tid, "executable", "--params 1", "name_for_job"));
    TS_ASSERT_THROWS_NOTHING(jm.queryRemoteFile(tid));
  }

  void test_queryStatusByID() {
    MockedSCARFLSFJM jmFail;

    std::string id("id001");
    IRemoteJobManager::RemoteJobInfo info;
    TS_ASSERT_THROWS(info = jmFail.queryRemoteJob(id), std::runtime_error);
    TS_ASSERT_THROWS(jmFail.authenticate("user", "pass"), std::runtime_error);
    TS_ASSERT_THROWS(info = jmFail.queryRemoteJob(id), std::runtime_error);

    MockedErrorResponse_SCARFLSFJM err;
    TS_ASSERT_THROWS(info = err.queryRemoteJob(id), std::runtime_error);
    TS_ASSERT_EQUALS(info.id, "");
    TS_ASSERT_EQUALS(info.name, "");

    std::string name("name01");
    MockedGoodJobStatus_SCARFLSFJM jm(id, name);
    TS_ASSERT_THROWS_NOTHING(jm.authenticate("user", "password"));
    TS_ASSERT_THROWS(info = jm.queryRemoteJob(id), std::runtime_error);

    std::string tid;
    TS_ASSERT_THROWS_NOTHING(tid = jm.startRemoteTransaction());
    TS_ASSERT_THROWS_NOTHING(
        id = jm.submitRemoteJob(tid, "exec", "--p 1", "job_name"));
    TS_ASSERT_THROWS(info = jm.queryRemoteJob(id), std::runtime_error);
  }

  void test_cancel() {
    MockedSCARFLSFJM jmFail;
    std::string tid("trans001");
    TS_ASSERT_THROWS(jmFail.stopRemoteTransaction(tid), std::runtime_error);

    MockedErrorResponse_SCARFLSFJM err;
    TS_ASSERT_THROWS(err.stopRemoteTransaction(tid), std::runtime_error);
    TS_ASSERT_THROWS(err.authenticate("user", "pass"), std::runtime_error);
    IRemoteJobManager::RemoteJobInfo info;
    TS_ASSERT_THROWS(info = err.queryRemoteJob("012"), std::runtime_error);

    std::string name("name01");
    MockedGoodLoginResponse_SCARFLSFJM jm;
    std::string newID;
    TS_ASSERT_THROWS_NOTHING(jm.authenticate("user", "pass"));
    TS_ASSERT_THROWS_NOTHING(newID = jm.startRemoteTransaction());
    TS_ASSERT_THROWS(jm.stopRemoteTransaction(tid), std::invalid_argument);
    TS_ASSERT_THROWS_NOTHING(jm.stopRemoteTransaction(newID));
  }

  void test_upload() {
    MockedGoodLoginResponse_SCARFLSFJM jm;
    TS_ASSERT_THROWS_NOTHING(jm.authenticate("userid", "pass"));
    std::string tid;
    TS_ASSERT_THROWS_NOTHING(tid = jm.startRemoteTransaction());
    TS_ASSERT(tid != "");
    /// fails - missing file
    TS_ASSERT_THROWS_NOTHING(
        jm.uploadRemoteFile(tid, "remote_name", "local_name"));
  }

  void test_errorResponseFromServer() {
    MockedErrorResponse_SCARFLSFJM err;
    TS_ASSERT_THROWS(err.authenticate("whoami", "pass"), std::runtime_error);
    TS_ASSERT_THROWS(err.ping(), std::runtime_error);
  }

  // logout must run after all the (positive) tests
  void test_logout() {
    MockedGoodLoginResponse_SCARFLSFJM jm;
    TS_ASSERT_THROWS_NOTHING(jm.authenticate("user", "pass"));
    std::string tid;
    TS_ASSERT_THROWS_NOTHING(tid = jm.startRemoteTransaction());
    TS_ASSERT_THROWS_NOTHING(jm.stopRemoteTransaction(tid));
    TS_ASSERT_THROWS_NOTHING(jm.logout());
  }

  void test_ping() {
    MockedConnectionError_SCARFLSFJM err;
    bool res = false;
    TS_ASSERT_THROWS(res = err.ping(), std::runtime_error);
    TS_ASSERT(!res);

    MockedErrorResponse_SCARFLSFJM errResp;
    TS_ASSERT_THROWS(res = errResp.ping(), std::runtime_error);
    TS_ASSERT(!res);

    /// ping is fine without logging in
    MockedGoodPingResponse_SCARFLSFJM good;
    TS_ASSERT_THROWS_NOTHING(res = good.ping());
    TS_ASSERT(res);
  }

  void test_failConnect() {
    MockedConnectionError_SCARFLSFJM fail;
    TS_ASSERT_THROWS(fail.authenticate("userlogin", "pass"),
                     std::runtime_error);
    TS_ASSERT_THROWS(fail.downloadRemoteFile("any_wrong_transID",
                                             "remote_fname", "local_fname"),
                     std::invalid_argument);
    TS_ASSERT_THROWS(fail.ping(), std::runtime_error);
  }

  void test_commandAfterLogout() {
    MockedGoodLoginResponse_SCARFLSFJM jm;
    TS_ASSERT_THROWS_NOTHING(jm.authenticate("username", "pass"));
    TS_ASSERT_THROWS_NOTHING(jm.logout());

    // Once you log out all actions should produce an exception
    std::string tid, jid;
    TS_ASSERT_THROWS(tid = jm.startRemoteTransaction(), std::runtime_error);

    TS_ASSERT_THROWS(jid = jm.submitRemoteJob("a_wrong_transID", "executable",
                                              "--params 1", "name_for_job"),
                     std::runtime_error);

    // log in again, back to normal
    TS_ASSERT_THROWS_NOTHING(jm.authenticate("user", "pass"));
    TS_ASSERT_THROWS_NOTHING(tid = jm.startRemoteTransaction());
    TS_ASSERT("" != tid);

    TS_ASSERT_THROWS(
        jid = jm.submitRemoteJob("no_no_wrong_ID", "executable", "--params 1"),
        std::invalid_argument);
    TS_ASSERT_THROWS_NOTHING(
        jid = jm.submitRemoteJob(tid, "executable", "--params 1"));
    TS_ASSERT("" != jid);
  }

private:
  std::string goodUsername;
  std::string goodPassword;
  static const std::string SCARFName;
};

const std::string SCARFLSFJobManagerTest::SCARFName = "SCARF@STFC";

#endif // MANTID_REMOTEJOBMANAGERS_SCARFLSFJOBMANAGERTEST_H_
