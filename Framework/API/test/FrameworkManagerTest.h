#ifndef FRAMEWORKMANAGERTEST_H_
#define FRAMEWORKMANAGERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include <stdexcept>

using namespace Mantid::Kernel;
using namespace Mantid::API;

class ToyAlgorithm2 : public Algorithm {
public:
  ToyAlgorithm2() {}
  ~ToyAlgorithm2() override {}
  const std::string name() const override {
    return "ToyAlgorithm2";
  }; ///< Algorithm's name for identification
  int version() const override {
    return 1;
  }; ///< Algorithm's version for identification
  const std::string summary() const override { return "Test summary"; }
  void init() override {
    declareProperty("Prop", "");
    declareProperty("P2", "");
    declareProperty("Filename", "");
  }
  void exec() override {}
  void final() {}
};

DECLARE_ALGORITHM(ToyAlgorithm2)

using namespace Mantid;

class FrameworkManagerTest : public CxxTest::TestSuite {
public:
#ifdef MPI_EXPERIMENTAL
  // Make sure FrameworkManager is always instantiated. This is needed to
  // initialize the MPI environment.
  FrameworkManagerTest() { FrameworkManager::Instance(); }
#endif

  void testConstructor() {
    // Not really much to test
    TS_ASSERT_THROWS_NOTHING(FrameworkManager::Instance());

#ifdef MPI_BUILD
    // If this is 'MPI Mantid' then test that the mpi environment has been
    // initialized
    TS_ASSERT(boost::mpi::environment::initialized());
#endif
  }

  void testcreateAlgorithm() {
    TS_ASSERT_THROWS_NOTHING(
        FrameworkManager::Instance().createAlgorithm("ToyAlgorithm2"))
    TS_ASSERT_THROWS(
        FrameworkManager::Instance().createAlgorithm("ToyAlgorithm2", "", 3),
        std::runtime_error)
    TS_ASSERT_THROWS(FrameworkManager::Instance().createAlgorithm("aaaaaa"),
                     std::runtime_error)
  }

  void testcreateAlgorithmWithProps() {
    IAlgorithm *alg = FrameworkManager::Instance().createAlgorithm(
        "ToyAlgorithm2", "Prop=Val;P2=V2");
    std::string prop;
    TS_ASSERT_THROWS_NOTHING(prop = alg->getPropertyValue("Prop"))
    TS_ASSERT(!prop.compare("Val"))
    TS_ASSERT_THROWS_NOTHING(prop = alg->getPropertyValue("P2"))
    TS_ASSERT(!prop.compare("V2"))

    TS_ASSERT_THROWS_NOTHING(
        FrameworkManager::Instance().createAlgorithm("ToyAlgorithm2", ""))
  }

  void testExec() {
    IAlgorithm *alg =
        FrameworkManager::Instance().exec("ToyAlgorithm2", "Prop=Val;P2=V2");
    TS_ASSERT(alg->isExecuted())
  }

  void testGetWorkspace() {
    TS_ASSERT_THROWS(FrameworkManager::Instance().getWorkspace("wrongname"),
                     std::runtime_error)
  }
};

#endif /*FRAMEWORKMANAGERTEST_H_*/
