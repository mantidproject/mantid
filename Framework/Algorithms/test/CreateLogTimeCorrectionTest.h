#ifndef MANTID_ALGORITHMS_CREATELOGTIMECORRECTIONTEST_H_
#define MANTID_ALGORITHMS_CREATELOGTIMECORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CreateLogTimeCorrection.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/TableRow.h"

#include <fstream>
#include <sstream>
#include <Poco/File.h>

using Mantid::Algorithms::CreateLogTimeCorrection;

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

using namespace std;

class CreateLogTimeCorrectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateLogTimeCorrectionTest *createSuite() {
    return new CreateLogTimeCorrectionTest();
  }
  static void destroySuite(CreateLogTimeCorrectionTest *suite) { delete suite; }

  CreateLogTimeCorrectionTest() {
    m_inpws = createEmptyWorkspace("VULCAN");
    AnalysisDataService::Instance().add("Vulcan", m_inpws);
  }

  ~CreateLogTimeCorrectionTest() { AnalysisDataService::Instance().clear(); }

  /** Test against a Vulcan run
    */
  void test_VulcanNoFileOutput() {
    CreateLogTimeCorrection alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("InputWorkspace", m_inpws);
    alg.setProperty("OutputWorkspace", "CorrectionTable");
    alg.setProperty("OutputFilename", "dummpy.dat");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr outws = boost::dynamic_pointer_cast<TableWorkspace>(
        AnalysisDataService::Instance().retrieve("CorrectionTable"));
    TS_ASSERT(outws);

    int numrows = static_cast<int>(outws->rowCount());
    TS_ASSERT_EQUALS(numrows, 7392);

    // get some value to check
    double l1 = 43.754;

    vector<size_t> checkrows;
    checkrows.push_back(0);
    checkrows.push_back(100);
    checkrows.push_back(1000);
    checkrows.push_back(5000);

    for (size_t i = 0; i < checkrows.size(); ++i) {
      TableRow row = outws->getRow(i);
      int detid;
      double correction, l2;
      row >> detid >> correction >> l2;
      TS_ASSERT(detid > 0);

      TS_ASSERT_DELTA(correction * (l1 + l2) / l1, 1.0, 0.0001);
    }

    // clean workspaces and file written
    AnalysisDataService::Instance().remove("CorrectionTable");

    return;
  }

  /** Test against a Vulcan run
    */
  void WindowsFailed_test_VulcanFileOutput() {
    CreateLogTimeCorrection alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("InputWorkspace", m_inpws);
    alg.setProperty("OutputWorkspace", "CorrectionTable");
    alg.setProperty("OutputFilename", "VucanCorrection.dat");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr outws = boost::dynamic_pointer_cast<TableWorkspace>(
        AnalysisDataService::Instance().retrieve("CorrectionTable"));
    TS_ASSERT(outws);

    int numrows = static_cast<int>(outws->rowCount());
    TS_ASSERT_EQUALS(numrows, 7392);

    // get some value to check
    double l1 = 43.754;

    vector<size_t> checkrows;
    checkrows.push_back(0);
    checkrows.push_back(100);
    checkrows.push_back(1000);
    checkrows.push_back(5000);

    for (size_t i = 0; i < checkrows.size(); ++i) {
      TableRow row = outws->getRow(i);
      int detid;
      double correction, l2;
      row >> detid >> correction >> l2;
      TS_ASSERT(detid > 0);

      TS_ASSERT_DELTA(correction * (l1 + l2) / l1, 1.0, 0.0001);
    }

    // check output file
    ifstream ifile;
    ifile.open("VucanCorrection.dat");
    TS_ASSERT(ifile.is_open());
    size_t numrowsinfile = 0;
    char line[256];
    while (!ifile.eof()) {
      ifile.getline(line, 256);
      string templine(line);
      if (templine.size() > 0)
        ++numrowsinfile;
    }
    TS_ASSERT_EQUALS(numrowsinfile, 7392);

    // clean workspaces and file written
    AnalysisDataService::Instance().remove("CorrectionTable");

    Poco::File file("VucanCorrection.dat");
    file.remove(false);

    return;
  }

  /** Test if there is no instrument in given workspace
    */
  void test_NoInstrument() {
    MatrixWorkspace_sptr inpws = createEmptyWorkspace("");

    CreateLogTimeCorrection alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS(alg.setProperty("InputWorkspace", inpws),
                     std::invalid_argument);
  }

  void test_not_allowed_output_workspace_same_as_input() {
    CreateLogTimeCorrection alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", m_inpws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "Vulcan"))
    TS_ASSERT(!alg.execute())
  }

private:
  /** Generate an empty Vulcan workspace
    */
  API::MatrixWorkspace_sptr createEmptyWorkspace(const string &instrument) {
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1));

    if (!instrument.empty()) {
      DataHandling::LoadInstrument load;
      load.initialize();
      load.setProperty("Workspace", ws);
      load.setProperty("InstrumentName", instrument);
      load.execute();
    }

    return ws;
  }

  MatrixWorkspace_sptr m_inpws;
};

#endif /* MANTID_ALGORITHMS_CREATELOGTIMECORRECTIONTEST_H_ */
