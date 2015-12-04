#ifndef MANTID_MDALGORITHMS_LOADSQW2TEST_H_
#define MANTID_MDALGORITHMS_LOADSQW2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidMDAlgorithms/LoadSQW2.h"
#include "MantidKernel/make_unique.h"

using Mantid::API::IAlgorithm;
using Mantid::API::IAlgorithm_uptr;
using Mantid::API::IMDEventWorkspace_sptr;
using Mantid::MDAlgorithms::LoadSQW2;
using Mantid::Kernel::make_unique;

class LoadSQW2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadSQW2Test *createSuite() { return new LoadSQW2Test(); }
  static void destroySuite(LoadSQW2Test *suite) { delete suite; }

  LoadSQW2Test() : CxxTest::TestSuite(), m_filename("test_horace_reader.sqw") {}

  //----------------------------------------------------------------------------
  // Success tests
  //----------------------------------------------------------------------------
  void test_Algorithm_Initializes_Correctly() {
    IAlgorithm_uptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = createAlgorithm());
    TS_ASSERT(alg->isInitialized());
  }

  void test_Algorithm_Is_Version_2_LoadSQW() {
    IAlgorithm_uptr alg = createAlgorithm();
    TS_ASSERT_EQUALS("LoadSQW", alg->name());
    TS_ASSERT_EQUALS(2, alg->version());
  }

  void test_SQW_Is_Accepted_Filename() {
    IAlgorithm_uptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = createAlgorithm());
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("Filename", m_filename));
  }

  void test_OutputWorkspace_Has_Correct_Data() {
    IMDEventWorkspace_sptr outputWS = runAlgorithm();
    TS_ASSERT_EQUALS(4, outputWS->getNumDims());
    TS_ASSERT_EQUALS(2, outputWS->getNumExperimentInfo());
  }
  
  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Filename_Property_Throws_If_Not_Found() {
    auto alg = createAlgorithm();
    TS_ASSERT_THROWS(alg->setPropertyValue("Filename", "x.sqw"),
                     std::invalid_argument);
  }

private:
  IMDEventWorkspace_sptr runAlgorithm() {
    auto algm = createAlgorithm();
    algm->setProperty("Filename", m_filename);
    algm->setProperty("OutputWorkspace", "__unused_value_for_child_algorithm");
    algm->execute();
    return algm->getProperty("OutputWorkspace");
  }

  IAlgorithm_uptr createAlgorithm() {
    IAlgorithm_uptr alg(make_unique<LoadSQW2>());
    alg->initialize();
    alg->setChild(true);
    return alg;
  }

  
  std::string m_filename;
};

#endif /* MANTID_MDALGORITHMS_LOADSQW2TEST_H_ */
