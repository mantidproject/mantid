#ifndef ALGORITHMMPITEST_H_
#define ALGORITHMMPITEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Strings.h"


#include "MantidKernel/Property.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/FrameworkManager.h"
#ifdef MPI_EXPERIMENTAL
#include "MantidParallel/ParallelRunner.h"
#endif

using namespace Mantid::Kernel;
using namespace Mantid::API;

#ifdef MPI_EXPERIMENTAL
class NoParallelismAlgorithm : public Algorithm {
public:
  const std::string name() const override { return "NoParallelismAlgorithm"; }
  int version() const override { return 1; }
  const std::string category() const override { return ""; }
  const std::string summary() const override { return ""; }

  void init() override {
    declareProperty(make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                     Direction::Input));
  }

  void exec() override {}
};
DECLARE_ALGORITHM(NoParallelismAlgorithm)

class AlgorithmWithBad_getParallelExecutionMode : public Algorithm {
public:
  const std::string name() const override {
    return "AlgorithmWithBad_getParallelExecutionMode ";
  }
  int version() const override { return 1; }
  const std::string category() const override { return ""; }
  const std::string summary() const override { return ""; }
  void init() override {}
  void exec() override {}

protected:
  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override {
        static_cast<void>(storageModes);
        return Parallel::ExecutionMode::Serial;
      }
};
DECLARE_ALGORITHM(AlgorithmWithBad_getParallelExecutionMode)

class ParallelAlgorithm : public Algorithm {
public:
  const std::string name() const override {
    return "ParallelAlgorithm";
  }
  int version() const override { return 1; }
  const std::string category() const override { return ""; }
  const std::string summary() const override { return ""; }
  void init() override {
    auto wsValidator = boost::make_shared<Kernel::CompositeValidator>();
    wsValidator->add<HistogramValidator>();
    //wsValidator->add<RawCountValidator>();
    declareProperty(Kernel::make_unique<WorkspaceProperty<>>(
        "InputWorkspace", "", Kernel::Direction::Input, wsValidator));
    declareProperty(make_unique<WorkspaceProperty<Workspace>>(
        "OutputWorkspace", "", Direction::Output));
  }
  void exec() override {
    boost::shared_ptr<MatrixWorkspace> ws = getProperty("InputWorkspace");
    fprintf(stderr, "exec %d %s\n", communicator().rank(), Parallel::toString(ws->storageMode()).c_str());
    setProperty("OutputWorkspace", ws->clone());
  }

protected:
  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override {
    return getCorrespondingExecutionMode(storageModes.at("InputWorkspace"));
  }
};
DECLARE_ALGORITHM(ParallelAlgorithm)

void run_NoParallelismAlgorithm(const Parallel::Communicator &comm) {
  for (auto storageMode :
       {Parallel::StorageMode::Cloned, Parallel::StorageMode::Distributed,
        Parallel::StorageMode::MasterOnly}) {
    NoParallelismAlgorithm alg;
    alg.setCommunicator(comm);
    auto in = boost::make_shared<WorkspaceTester>(storageMode);
    alg.initialize();
    alg.setProperty("InputWorkspace", in);
    if (comm.size() == 1) {
      TS_ASSERT_THROWS_NOTHING(alg.execute());
    } else {
      TS_ASSERT_THROWS_EQUALS(
          alg.execute(), const std::runtime_error &e, std::string(e.what()),
          "Algorithm does not support execution with input workspaces of the "
          "following storage types: \nInputWorkspace " +
              Parallel::toString(storageMode) + "\n.");
    }
  }
}

void run_AlgorithmWithBad_getParallelExecutionMode(const Parallel::Communicator &comm) {
  AlgorithmWithBad_getParallelExecutionMode alg;
  alg.setCommunicator(comm);
  alg.initialize();
  if(comm.size() == 1) {
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  } else {
    TS_ASSERT_THROWS_EQUALS(alg.execute(), const std::runtime_error &e,
                            std::string(e.what()),
                            "Parallel::ExecutionMode::Serial is not a valid "
                            "*parallel* execution mode.");
  }
}

void run_ParallelAlgorithm(const Parallel::Communicator &comm) {
  for (auto storageMode :
       {//Parallel::StorageMode::Cloned, Parallel::StorageMode::Distributed,
        Parallel::StorageMode::MasterOnly}) {
    ParallelAlgorithm alg;
    alg.setCommunicator(comm);
    alg.initialize();
    auto in = boost::make_shared<WorkspaceTester>(storageMode);
    in->initialize(1, 2, 1);
    if (storageMode != Parallel::StorageMode::MasterOnly || comm.rank() == 0) {
      alg.setProperty("InputWorkspace", in);
    } else {
      alg.setProperty("InputWorkspace", in->cloneEmpty());
    }
    std::string outName("out" + Strings::toString(comm.rank()));
    alg.setProperty("OutputWorkspace", outName);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    auto out = AnalysisDataService::Instance().retrieve(outName);
    TS_ASSERT_EQUALS(out->storageMode(), storageMode);
  }
}
#endif

class AlgorithmMPITest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AlgorithmMPITest *createSuite() { return new AlgorithmMPITest(); }
  static void destroySuite(AlgorithmMPITest *suite) { delete suite; }

  AlgorithmMPITest() {
    Mantid::API::FrameworkManager::Instance();
    AnalysisDataService::Instance();
  }

  void testNoParallelismAlgorithm() {
#ifdef MPI_EXPERIMENTAL
    Parallel::ParallelRunner serial(1);
    serial.run(run_NoParallelismAlgorithm);
    runParallel(run_NoParallelismAlgorithm);
    runParallel(run_NoParallelismAlgorithm);
    runParallel(run_NoParallelismAlgorithm);
    Mantid::API::AnalysisDataService::Instance().clear();
#endif
  }

  void testAlgorithmWithBad_getParallelExecutionMode() {
#ifdef MPI_EXPERIMENTAL
    Parallel::ParallelRunner serial(1);
    serial.run(run_AlgorithmWithBad_getParallelExecutionMode);
    runParallel(run_AlgorithmWithBad_getParallelExecutionMode);
    Mantid::API::AnalysisDataService::Instance().clear();
#endif
  }

  void testParallelAlgorithm() {
#ifdef MPI_EXPERIMENTAL
    Parallel::ParallelRunner serial(1);
    serial.run(run_ParallelAlgorithm);
    runParallel(run_ParallelAlgorithm);
    Mantid::API::AnalysisDataService::Instance().clear();
#endif
  }
};

#endif /*ALGORITHMMPITEST_H_*/
