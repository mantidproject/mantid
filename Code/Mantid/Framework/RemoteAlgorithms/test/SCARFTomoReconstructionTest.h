#ifndef MANTID_REMOTEALGORITHMS_SCARFTOMORECONSTRUCTION_H_
#define MANTID_REMOTEALGORITHMS_SCARFTOMORECONSTRUCTION_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ITableWorkspace.h"
#include "MantidRemoteAlgorithms/SCARFTomoReconstruction.h"

using namespace Mantid::RemoteAlgorithms;

/**
 * Very crude mock up for the interaction with the remote compute
 * resource (in real life, through the PAC web service of the LSF job
 * scheduler on SCARF). This one returns 200 OK and a simple response
 * string.
 */
class MockedSCARFTomo: public SCARFTomoReconstruction
{
protected:
  virtual int doSendRequestGetResponse(const std::string &url,
                                       std::ostream &response,
                                       const StringToStringMap &headers =
                                       StringToStringMap(),
                                       const std::string &method = std::string(),
                                       const std::string &body = "") {
    UNUSED_ARG(url);
    UNUSED_ARG(headers);
    UNUSED_ARG(method);
    UNUSED_ARG(body);

    response << "response OK - mocked up";
    return 200;
  }
};

/**
 * One more crude mock up for the interaction with the remote compute
 * resource. This one returns an error (the connection is fine, but
 * the response from the server is an error; example: wrong path,
 * server bug, etc.).
 */
class MockedErrorResponse_SCARFTomo: public SCARFTomoReconstruction
{
protected:
  virtual int doSendRequestGetResponse(const std::string &url,
                                       std::ostream &response,
                                       const StringToStringMap &headers =
                                       StringToStringMap(),
                                       const std::string &method = std::string(),
                                       const std::string &body = "") {
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
 * One more crude mock up for the interaction with the remote compute
 * resource. This one raises an exception as if the (underlying)
 * InternetHelper had found a connection issue.
 */
class MockedConnectionError_SCARFTomo: public SCARFTomoReconstruction
{
protected:
  virtual int doSendRequestGetResponse(const std::string &url,
                                       std::ostream &response,
                                       const StringToStringMap &headers =
                                       StringToStringMap(),
                                       const std::string &method = std::string(),
                                       const std::string &body = "") {
    UNUSED_ARG(url);
    UNUSED_ARG(response);
    UNUSED_ARG(headers);
    UNUSED_ARG(method);
    UNUSED_ARG(body);

    // throw as if there was a connection error
    throw Mantid::Kernel::Exception::InternetError("Mocked up exception - connection error");
  }
};

/**
 * One more crude mock up for the interaction with the remote compute
 * resource. This one returns an OK code and a string that reads like
 * what we expect when doing a successful login request.
 */
class MockedGoodLoginResponse_SCARFTomo: public SCARFTomoReconstruction
{
protected:
  virtual int doSendRequestGetResponse(const std::string &url,
                                       std::ostream &response,
                                       const StringToStringMap &headers =
                                       StringToStringMap(),
                                       const std::string &method = std::string(),
                                       const std::string &body = "") {
    UNUSED_ARG(url);
    UNUSED_ARG(response);
    UNUSED_ARG(headers);
    UNUSED_ARG(method);
    UNUSED_ARG(body);

    response << "https://portal.scarf.rl.ac.uk - response OK and login successful - mocked up";
    return 200;
  }
};

/**
 * One more crude mock up for the interaction with the remote compute
 * resource. This one returns an OK code and a string that reads like
 * a response with basic job status information.
 */
class MockedGoodJobStatus_SCARFTomo: public SCARFTomoReconstruction
{
public:
 MockedGoodJobStatus_SCARFTomo(const std::string &id): SCARFTomoReconstruction(),
  jobID(id)
  {};

protected:
  virtual int doSendRequestGetResponse(const std::string &url,
                                       std::ostream &response,
                                       const StringToStringMap &headers =
                                       StringToStringMap(),
                                       const std::string &method = std::string(),
                                       const std::string &body = "") {
    UNUSED_ARG(url);
    UNUSED_ARG(response);
    UNUSED_ARG(headers);
    UNUSED_ARG(method);
    UNUSED_ARG(body);

    response << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
      "<Jobs><Job><cmd>python /work/imat/webservice_test/test.py.py "
      "/work/imat/webservice_test/test_out/</cmd><extStatus>-</extStatus>"
      "<id>" << jobID << "</id><name>Mantid_tomography_1</name><status>Running</status>"
      "</Job></Jobs>";
    return 200;
  }
private:
  std::string jobID;
};

class SCARFTomoReconstructionTest: public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SCARFTomoReconstructionTest *createSuite() { return new SCARFTomoReconstructionTest(); }
  static void destroySuite(SCARFTomoReconstructionTest *suite) { delete suite; }

  void test_castAlgorithm()
  {
    // can create
    boost::shared_ptr<MockedSCARFTomo> a;
    TS_ASSERT(a = boost::make_shared<MockedSCARFTomo>());
    // can cast to inherited interfaces and base classes

    MockedSCARFTomo alg;
    TS_ASSERT( dynamic_cast<Mantid::RemoteAlgorithms::SCARFTomoReconstruction*>(&alg) );
    TS_ASSERT( dynamic_cast<Mantid::API::Algorithm*>(&alg) );
    TS_ASSERT( dynamic_cast<Mantid::Kernel::PropertyManagerOwner*>(&alg) );
    TS_ASSERT( dynamic_cast<Mantid::API::IAlgorithm*>(&alg) );
    TS_ASSERT( dynamic_cast<Mantid::Kernel::IPropertyManager*>(&alg) );
  }

  void test_initAlgorithm()
  {
    MockedSCARFTomo tomo;
    TS_ASSERT_THROWS_NOTHING( tomo.initialize() );
  }

  void test_propertiesMissing()
  {
    MockedSCARFTomo alg1;
    TS_ASSERT_THROWS_NOTHING( alg1.initialize() );
    // password missing
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("UserName", "anything") );
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("Action","LogIn") );

    TS_ASSERT_THROWS_NOTHING( alg1.execute() );
    TS_ASSERT( !alg1.isExecuted() );

    MockedSCARFTomo alg2;
    TS_ASSERT_THROWS_NOTHING( alg2.initialize() );
    // username missing
    TS_ASSERT_THROWS_NOTHING( alg2.setPropertyValue("Password", "whatever") );
    TS_ASSERT_THROWS_NOTHING( alg2.setPropertyValue("Action","LogIn") );

    TS_ASSERT_THROWS( alg2.execute(), std::runtime_error );
    TS_ASSERT( !alg2.isExecuted() );

    MockedSCARFTomo alg3;
    TS_ASSERT_THROWS_NOTHING( alg3.initialize() );
    // mispellings...
    // these try to set inexistent propeties => runtime_error
    TS_ASSERT_THROWS( alg3.setPropertyValue("sername", "anything"), std::runtime_error );
    TS_ASSERT_THROWS( alg3.setPropertyValue("Passw", "anything"), std::runtime_error );
    // these try to set wrong values for valid properties => invalid_argument
    TS_ASSERT_THROWS( alg3.setPropertyValue("Action","Loggin"), std::invalid_argument );
    TS_ASSERT_THROWS( alg3.setProperty("Action", "unknown_opt"), std::invalid_argument );
    TS_ASSERT_THROWS( alg3.setPropertyValue("JobID","strings_not_allowed"), std::invalid_argument );
  }

  void test_actionWithoutUsernameBeforeLogin()
  {
    MockedSCARFTomo alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );

    // Forget the username and you should get an exception
    // alg.setProperty("UserName", "foo_user"));
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Action", "JobStatus") );

    TS_ASSERT_THROWS( alg.execute(), std::runtime_error );
    TS_ASSERT( !alg.isExecuted() );

    MockedSCARFTomo tomo;
    TS_ASSERT_THROWS_NOTHING( tomo.initialize() );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("Action", "SubmitJob") );

    TS_ASSERT_THROWS( tomo.execute(), std::runtime_error );
    TS_ASSERT( !tomo.isExecuted() );
  }

  void test_actionWithoutLogin()
  {
    MockedSCARFTomo alg;

    // Even if you provide all required params, you should get an exception
    // if not logged in
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Action", "JobStatus") );

    TS_ASSERT_THROWS(alg.execute(), std::runtime_error );
    TS_ASSERT( !alg.isExecuted() );

    MockedSCARFTomo tomo;
    TS_ASSERT_THROWS_NOTHING( tomo.initialize() );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("UserName", "anyone") );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("Action", "SubmitJob") );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("RunnablePath", "/foo/bar.sh") );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("JobOptions", "--test --baz") );

    TS_ASSERT_THROWS_NOTHING( tomo.execute() );
    TS_ASSERT( !tomo.isExecuted() );
  }

  /// Login is required before running the other actions (except ping)
  // The good username is: foo_user
  void test_login()
  {
    goodUsername = "foo_user";
    goodPassword = "foo_password";

    // severe (connection) error
    MockedConnectionError_SCARFTomo err;
    TS_ASSERT_THROWS_NOTHING( err.initialize() );
    TS_ASSERT_THROWS_NOTHING( err.setProperty("UserName", goodUsername) );
    TS_ASSERT_THROWS_NOTHING( err.setProperty("Password", goodPassword) );
    TS_ASSERT_THROWS_NOTHING( err.setProperty("Action", "LogIn") );

    TS_ASSERT_THROWS_NOTHING( err.execute() );
    TS_ASSERT( !err.isExecuted() );

    // standard mocked response: looks like an unsuccessful login attempt
    MockedSCARFTomo tomo;
    TS_ASSERT_THROWS_NOTHING( tomo.initialize() );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("UserName", goodUsername) );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("Password", goodPassword) );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("Action", "LogIn") );

    TS_ASSERT_THROWS_NOTHING( tomo.execute() );
    TS_ASSERT( !tomo.isExecuted() );

    // successful login attempt
    MockedGoodLoginResponse_SCARFTomo login;
    TS_ASSERT_THROWS_NOTHING( login.initialize() );
    TS_ASSERT_THROWS_NOTHING( login.setProperty("UserName", goodUsername) );
    TS_ASSERT_THROWS_NOTHING( login.setProperty("Password", goodPassword) );
    TS_ASSERT_THROWS_NOTHING( login.setProperty("Action", "LogIn") );

    TS_ASSERT_THROWS_NOTHING( login.execute() );
    TS_ASSERT( login.isExecuted() );
  }

  void test_actionWithoutUsernameAfterLogin()
  {
    MockedSCARFTomo alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );

    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Action", "JobStatus") );

    TS_ASSERT_THROWS( alg.execute(), std::runtime_error );
    TS_ASSERT( !alg.isExecuted() );

    MockedSCARFTomo tomo;
    TS_ASSERT_THROWS_NOTHING( tomo.initialize() );
    // Forget this and you should get an exception
    // tomo.setProperty("UserName", 3));
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("Action", "SubmitJob") );

    TS_ASSERT_THROWS( tomo.execute(), std::runtime_error );
    TS_ASSERT( !tomo.isExecuted() );
  }

  void test_actionWrongUsername()
  {
    // Once you log out all actions should produce an exception
    MockedSCARFTomo tomo;
    TS_ASSERT_THROWS_NOTHING( tomo.initialize() );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("UserName", "fail_" + goodUsername) );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("Action", "JobStatus") );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("RunnablePath", "/foo/bar.sh") );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("JobOptions", "--test --baz") );

    TS_ASSERT_THROWS_NOTHING( tomo.execute() );
    TS_ASSERT( !tomo.isExecuted() );
  }

  void test_wrongExec()
  {
    MockedSCARFTomo alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT_THROWS( alg.setProperty("RandomName", 32), std::runtime_error );

    TS_ASSERT_THROWS( alg.execute(), std::runtime_error );
    TS_ASSERT( !alg.isExecuted() );
  }

  void test_ping()
  {
    MockedSCARFTomo alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Action", "Ping") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Username", goodUsername) );

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );
  }

  void test_submit()
  {
    MockedSCARFTomo alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("UserName", goodUsername) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Action", "SubmitJob") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("RunnablePath", "/foo/bar.sh") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("JobOptions", "--test --baz") );

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    // second submit in a row
    MockedSCARFTomo tomo;
    TS_ASSERT_THROWS_NOTHING( tomo.initialize() );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("UserName", goodUsername) );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("Action", "SubmitJob") );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("RunnablePath", "/foo/bar.sh") );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("JobOptions", "--random --baz") );

    TS_ASSERT_THROWS_NOTHING( tomo.execute() );
    TS_ASSERT( tomo.isExecuted() );
  }

  void test_queryStatus()
  {
    // this one is the basic mock up which doesn't provide the response content that we need
    MockedSCARFTomo err;
    TS_ASSERT_THROWS_NOTHING( err.initialize() );
    TS_ASSERT_THROWS_NOTHING( err.setProperty("UserName", goodUsername) );
    TS_ASSERT_THROWS_NOTHING( err.setProperty("Action", "JobStatus") );

    TS_ASSERT_THROWS_NOTHING( err.execute() );
    TS_ASSERT( err.isExecuted()) ;

    std::vector<std::string> vec;
    TS_ASSERT_THROWS_NOTHING(vec = err.getProperty("RemoteJobsID"));
    TS_ASSERT_EQUALS( vec.size(), 0 );
    TS_ASSERT_THROWS_NOTHING(vec = err.getProperty("RemoteJobsNames"));
    TS_ASSERT_EQUALS( vec.size(), 0 );
    TS_ASSERT_THROWS_NOTHING(vec = err.getProperty("RemoteJobsStatus"));
    TS_ASSERT_EQUALS( vec.size(), 0 );
    TS_ASSERT_THROWS_NOTHING(vec = err.getProperty("RemoteJobsCommands"));
    TS_ASSERT_EQUALS( vec.size(), 0 );

    // this one gives a basic/sufficient response with job status information
    MockedGoodJobStatus_SCARFTomo alg("wrong id");
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("UserName", goodUsername) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Action", "JobStatus") );

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted()) ;

    // the mock produces info on one job
    TS_ASSERT_THROWS_NOTHING(vec = alg.getProperty("RemoteJobsID"));
    TS_ASSERT_EQUALS( vec.size(), 1 );
    TS_ASSERT( vec.size()>0 && !vec.front().empty() );
    TS_ASSERT_THROWS_NOTHING(vec = alg.getProperty("RemoteJobsNames"));
    TS_ASSERT_EQUALS( vec.size(), 1 );
    TS_ASSERT( vec.size()>0 && !vec.front().empty() );
    TS_ASSERT_THROWS_NOTHING(vec = alg.getProperty("RemoteJobsStatus"));
    TS_ASSERT_EQUALS( vec.size(), 1 );
    TS_ASSERT( vec.size()>0 && !vec.front().empty() );
    TS_ASSERT_THROWS_NOTHING(vec = alg.getProperty("RemoteJobsCommands"));
    TS_ASSERT_EQUALS( vec.size(), 1 );
    TS_ASSERT( vec.size()>0 && !vec.front().empty() );
  }

  void test_queryStatusByID()
  {
    // this one is the basic mockup: doesn't provide the response content that we need
    MockedSCARFTomo err;
    TS_ASSERT_THROWS_NOTHING( err.initialize() );
    TS_ASSERT_THROWS_NOTHING( err.setProperty("UserName", goodUsername) );
    TS_ASSERT_THROWS_NOTHING( err.setProperty("Action", "JobStatusByID") );
    TS_ASSERT_THROWS_NOTHING( err.setProperty("JobID", 123456789) );

    TS_ASSERT_THROWS_NOTHING( err.execute() );
    TS_ASSERT( err.isExecuted()) ;

    std::string tmp;
    TS_ASSERT_THROWS_NOTHING( tmp = err.getPropertyValue("RemoteJobName") );
    TS_ASSERT( tmp.empty() );
    TS_ASSERT_THROWS_NOTHING( tmp = err.getPropertyValue("RemoteJobStatus") );
    TS_ASSERT( tmp.empty() );
    TS_ASSERT_THROWS_NOTHING( tmp = err.getPropertyValue("RemoteJobsCommands") );
    TS_ASSERT( tmp.empty() );

    // this one gives a basic/sufficient response with job status information
    std::string jobID = "444449";
    MockedGoodJobStatus_SCARFTomo alg(jobID);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("UserName", goodUsername) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Action", "JobStatusByID") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("JobID", jobID) );

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted()) ;

    // It could also check that it gets the names, etc. that the mock-up produces
    TS_ASSERT_THROWS_NOTHING( tmp = alg.getPropertyValue("RemoteJobName") );
    TS_ASSERT( !tmp.empty() );
    TS_ASSERT_THROWS_NOTHING( tmp = alg.getPropertyValue("RemoteJobStatus") );
    TS_ASSERT( !tmp.empty() );
    TS_ASSERT_THROWS_NOTHING( tmp = alg.getPropertyValue("RemoteJobCommand") );
    TS_ASSERT( !tmp.empty() );
  }

  void test_cancel()
  {
    MockedSCARFTomo alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("UserName", goodUsername) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Action", "CancelJob") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("CancelJobID", 123456789) );

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted()) ;
  }

  void test_upload()
  {
    MockedSCARFTomo alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Username", goodUsername) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Action", "Upload") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("FileToUpload", "random_file") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("DestinationDirectory", "random_path/") );

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted()) ;
  }

  void test_download()
  {
    MockedSCARFTomo alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );

    // Download with empty filename (get all files)
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("UserName", goodUsername) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Action", "Download") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("DownloadJobID", 12345) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("RemoteJobFilename", "") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("LocalDirectory", "/tmp/foo") );
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    MockedSCARFTomo alg2;
    TS_ASSERT_THROWS_NOTHING( alg2.initialize() );

    // Download a single file (giving its name)
    TS_ASSERT_THROWS_NOTHING( alg2.setProperty("UserName", goodUsername) );
    TS_ASSERT_THROWS_NOTHING( alg2.setProperty("Action", "Download") );
    TS_ASSERT_THROWS_NOTHING( alg2.setProperty("DownloadJobID", 12345) );
    TS_ASSERT_THROWS_NOTHING( alg2.setProperty("RemoteJobFilename", "inexistent_test_name.nxs.foo") );
    TS_ASSERT_THROWS_NOTHING( alg2.setProperty("LocalDirectory", "/tmp/foo") );
    TS_ASSERT_THROWS_NOTHING( alg2.execute() );
    TS_ASSERT( !alg2.isExecuted() );
  }

  void test_errorResponseFromServer()
  {
    MockedErrorResponse_SCARFTomo err;
    TS_ASSERT_THROWS_NOTHING( err.initialize() );
    TS_ASSERT_THROWS_NOTHING( err.setPropertyValue("Username", goodUsername) );
    TS_ASSERT_THROWS_NOTHING( err.setPropertyValue("Action","JobStatus") );

    TS_ASSERT_THROWS_NOTHING( err.execute() );
    TS_ASSERT( !err.isExecuted() );
  }

  // logout must run after all the (positive) tests
  void test_logout()
  {
    MockedSCARFTomo alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("UserName", goodUsername));
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Action", "LogOut") );

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );
  }

  void test_actionAfterLogout()
  {
    MockedSCARFTomo alg;
    // Once you log out all actions should produce an exception, regardless of the username given
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("UserName", "fail_" + goodUsername) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Action", "JobStatus") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("RunnablePath", "/foo/bar.sh") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("JobOptions", "--test --baz") );

    TS_ASSERT_THROWS_NOTHING(alg.execute() );
    TS_ASSERT( !alg.isExecuted() );

    MockedSCARFTomo alg2;
    TS_ASSERT_THROWS_NOTHING( alg2.initialize() );
    TS_ASSERT_THROWS_NOTHING( alg2.setProperty("UserName", goodUsername) );
    TS_ASSERT_THROWS_NOTHING( alg2.setProperty("Action", "JobStatus") );
    TS_ASSERT_THROWS_NOTHING( alg2.setProperty("RunnablePath", "/foo/bar.sh") );
    TS_ASSERT_THROWS_NOTHING( alg2.setProperty("JobOptions", "--test --baz") );

    TS_ASSERT_THROWS_NOTHING(alg2.execute() );
    TS_ASSERT( !alg2.isExecuted() );
  }

  void test_failConnect()
  {
    MockedConnectionError_SCARFTomo fail;
    TS_ASSERT_THROWS_NOTHING( fail.initialize() );
    TS_ASSERT_THROWS_NOTHING( fail.setPropertyValue("Action", "Ping") );

    TS_ASSERT_THROWS( fail.execute(), std::runtime_error );
    TS_ASSERT( !fail.isExecuted() );

    MockedConnectionError_SCARFTomo fail2;
    TS_ASSERT_THROWS_NOTHING( fail2.initialize() );
    // username missing
    TS_ASSERT_THROWS_NOTHING( fail2.setPropertyValue("Username", "uname") );
    TS_ASSERT_THROWS_NOTHING( fail2.setPropertyValue("Password", "whatever") );
    TS_ASSERT_THROWS_NOTHING( fail2.setPropertyValue("Action","LogIn") );

    TS_ASSERT_THROWS_NOTHING( fail2.execute() );
    TS_ASSERT( !fail2.isExecuted() );
  }

  void test_errorResponseFromServerAfterLogout()
  {
    MockedErrorResponse_SCARFTomo err;
    TS_ASSERT_THROWS_NOTHING( err.initialize() );
    TS_ASSERT_THROWS_NOTHING( err.setPropertyValue("Username", "foo") );
    TS_ASSERT_THROWS_NOTHING( err.setPropertyValue("Action", "Ping") );

    TS_ASSERT_THROWS_NOTHING( err.execute() );
    TS_ASSERT( !err.isExecuted() );
  }

private:
  std::string goodUsername;
  std::string goodPassword;
  static const std::string SCARFName;
};

const std::string SCARFTomoReconstructionTest::SCARFName = "SCARF@STFC";

#endif // MANTID_REMOTEALGORITHMS_SCARFTOMORECONSTRUCTION_H_
