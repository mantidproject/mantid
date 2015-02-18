#ifndef MANTID_REMOTEALGORITHMS_SCARFTOMORECONSTRUCTION_H_
#define MANTID_REMOTEALGORITHMS_SCARFTOMORECONSTRUCTION_H_

#include <cxxtest/TestSuite.h>

#include "MantidRemoteAlgorithms/SCARFTomoReconstruction.h"

using namespace Mantid::RemoteAlgorithms;

/**
 * Very crude mock for the interaction with the remote compute
 * resource (in real life, through the PAC web service of the LSF job
 * scheduler on SCARF).
 */
class MockedSCARFTomo: public SCARFTomoReconstruction
{
protected:
  virtual void doLogin(const std::string &username, const std::string &password) {
    UNUSED_ARG(username);
    UNUSED_ARG(password);
  }

  virtual void doLogout(const std::string &username) {
    UNUSED_ARG(username);
  }

  virtual bool doPing() {
    return true;
  }

  virtual void doSubmit(const std::string &username) {
    UNUSED_ARG(username);
  }

  virtual void doQueryStatus(const std::string &username,
                             const std::string &wsName) {
    UNUSED_ARG(username);
    // TODO create ws
    UNUSED_ARG(wsName);
  }

  virtual void doQueryStatusById(const std::string &username,
                                 const std::string &jobId,
                                 const std::string &wsName) {
    UNUSED_ARG(username);
    UNUSED_ARG(jobId);
    // TODO create ws
    UNUSED_ARG(wsName);
  }

  virtual void doCancel(const std::string &username,
                        const std::string& jobId) {
    UNUSED_ARG(username);
    UNUSED_ARG(jobId);
  }

  virtual void doUploadFile(const std::string &username,
                            const std::string &destDir,
                            const std::string &filename) {
    UNUSED_ARG(username);
    UNUSED_ARG(destDir);
    UNUSED_ARG(filename);
  }

  virtual void doDownload(const std::string &username,
                          const std::string &jobId,
                          const std::string &fname,
                          const std::string &localDir) {
    UNUSED_ARG(username);
    UNUSED_ARG(jobId);
    UNUSED_ARG(fname);
    UNUSED_ARG(localDir);
  }

};

class SCARFTomoReconstructionTest: public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SCARFTomoReconstructionTest *createSuite() { return new SCARFTomoReconstructionTest(); }
  static void destroySuite(SCARFTomoReconstructionTest *suite) { delete suite; }

  void test_initAlgorithm() {
    MockedSCARFTomo tomo;
    TS_ASSERT_THROWS_NOTHING( tomo.initialize() );
  }

  void test_propertiesMissing() {
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Username", "anything") );
    // missing password
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Action","Login") );

    TS_ASSERT_THROWS( alg.execute(), std::runtime_error );
    TS_ASSERT( !alg.isExecuted() );


    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Password", "whatever") );
    // missing username
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Action","Login") );

    TS_ASSERT_THROWS( alg.execute(), std::runtime_error );
    TS_ASSERT( !alg.isExecuted() );
  }

  void test_castAlgorithm() {
    // TODO
  }

  void test_actionWithoutUsernameBeforeLogin() {
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Action", "JobStatus") );

    MockedSCARFTomo tomo;
    TS_ASSERT_THROWS_NOTHING( tomo.initialize() );
    // Forget this and you should get an exception
    // tomo.setProperty("UserName", 3));

    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("Action", "SubmitJob") );

    TS_ASSERT_THROWS( tomo.execute(), std::runtime_error );
    TS_ASSERT( !tomo.isExecuted() );
  }

  void test_actionWithoutLogin() {
    // Even if you provide all required params, you should get an exception
    // if not logged in
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Action", "JobStatus") );

    TS_ASSERT_THROWS(alg.execute(), std::runtime_error );
    TS_ASSERT( !alg.isExecuted() );

    MockedSCARFTomo tomo;
    TS_ASSERT_THROWS_NOTHING( tomo.initialize() );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("Username", goodUsername) );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("Action", "SubmitJob") );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("RunnablePath", "/foo/bar.sh") );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("JobOptions", "--test --baz") );

    TS_ASSERT_THROWS( tomo.execute(), std::runtime_error );
    TS_ASSERT( !tomo.isExecuted() );
  }

  /// Login is required before running the other actions (except ping)
  // The good username is: foo_user
  void test_login() {
    goodUsername = "foo_user";
    goodPassword = "foo_password";

    MockedSCARFTomo tomo;
    TS_ASSERT_THROWS_NOTHING( tomo.initialize() );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("UserName", goodUsername) );
    TS_ASSERT_THROWS_NOTHING(tomo.setProperty("Password", goodPassword));

    TS_ASSERT_THROWS_NOTHING(tomo.execute());
    TS_ASSERT( tomo.isExecuted() );
  }

  void test_actionWithoutUsernameAfterLogin() {
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

  void test_actionWrongUsername() {
    // Once you log out all actions should produce an exception
    MockedSCARFTomo tomo;
    TS_ASSERT_THROWS_NOTHING( tomo.initialize() );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("UserName", "fail_" + goodUsername) );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("Action", "Jobstatus") );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("RunnablePath", "/foo/bar.sh") );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("JobOptions", "--test --baz") );

    TS_ASSERT_THROWS( tomo.execute(), std::runtime_error );
    TS_ASSERT( !tomo.isExecuted() );
  }

  void test_ping() {
    MockedSCARFTomo tomo;
    TS_ASSERT_THROWS_NOTHING( tomo.initialize() );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("RandomName", 3) );

    TS_ASSERT_THROWS_NOTHING( tomo.execute() );
    TS_ASSERT( tomo.isExecuted() );
  }

  void test_submit() {
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

  void test_queryStatus() {
    MockedSCARFTomo tomo;
    TS_ASSERT_THROWS_NOTHING( tomo.initialize() );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("RandomName", 3) );

    TS_ASSERT_THROWS_NOTHING( tomo.execute() );
    TS_ASSERT( tomo.isExecuted()) ;
  }

  void test_queryStatusByID() {
    MockedSCARFTomo tomo;
    TS_ASSERT_THROWS_NOTHING( tomo.initialize() );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("RandomName", 3) );

    TS_ASSERT_THROWS_NOTHING( tomo.execute() );
    TS_ASSERT( tomo.isExecuted() );
  }

  void test_cancel() {
    MockedSCARFTomo tomo;
    TS_ASSERT_THROWS_NOTHING( tomo.initialize() );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("RandomName", 3) );

    TS_ASSERT_THROWS_NOTHING( tomo.execute() );
    TS_ASSERT( tomo.isExecuted() );
  }

  void test_upload() {
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Action", "UploadFile") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("FileToUpload", "random_file") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("DestinationDirectory", "random_path/") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Action", "Upload") );

    /*
    MockedSCARFTomo tomo;
    tomo.TS_ASSERT_THROWS_NOTHING( initialize() );
    TS_ASSERT_THROWS_NOTHING(tomo.setProperty("RandomName", 3));

    TS_ASSERT_THROWS_NOTHING(tomo.execute());
    TS_ASSERT(tomo.isExecuted());
    */
  }

  void test_download() {
    MockedSCARFTomo tomo;
    TS_ASSERT_THROWS_NOTHING( tomo.initialize() );

    // Download with empty filename (get all files)
    TS_ASSERT_THROWS_NOTHING(tomo.setProperty("Filename", ""));
    TS_ASSERT_THROWS_NOTHING(tomo.execute());
    TS_ASSERT(tomo.isExecuted());

    tomo.initialize();
    // Download a single file (giving its name)
    TS_ASSERT_THROWS_NOTHING(tomo.setProperty("Filename", "test_name.nxs"));
    TS_ASSERT_THROWS_NOTHING(tomo.execute());
    TS_ASSERT(tomo.isExecuted());
  }

  // logout must run after all the (positive) tests
  void test_logout() {
    MockedSCARFTomo tomo;
    tomo.initialize();
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("RandomName", 3) );

    TS_ASSERT_THROWS_NOTHING( tomo.execute() );
    TS_ASSERT( tomo.isExecuted() );
  }

  void test_actionAfterLogout() {
    // Once you log out all actions should produce an exception, regardless of the username given
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("UserName", "fail_" + goodUsername) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Action", "Jobstatus") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("RunnablePath", "/foo/bar.sh") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("JobOptions", "--test --baz") );

    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
    TS_ASSERT( !alg.isExecuted() );

    MockedSCARFTomo tomo;
    TS_ASSERT_THROWS_NOTHING( tomo.initialize() );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("UserName", goodUsername) );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("Action", "Jobstatus") );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("RunnablePath", "/foo/bar.sh") );
    TS_ASSERT_THROWS_NOTHING( tomo.setProperty("JobOptions", "--test --baz") );

    TS_ASSERT_THROWS(tomo.execute(), std::runtime_error);
    TS_ASSERT( !tomo.isExecuted() );
  }

private:
  MockedSCARFTomo alg;
  std::string goodUsername;
  std::string goodPassword;
};

#endif // MANTID_REMOTEALGORITHMS_SCARFTOMORECONSTRUCTION_H_
