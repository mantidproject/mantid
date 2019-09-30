// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALGORITHMMPITEST_H_
#define ALGORITHMMPITEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidKernel/CompositeValidator.h"

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/Property.h"
#include "MantidParallel/Communicator.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidTestHelpers/ParallelAlgorithmCreation.h"
#include "MantidTestHelpers/ParallelRunner.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace ParallelTestHelpers;

namespace {
class FakeWorkspaceA : public WorkspaceTester {
public:
  using WorkspaceTester::WorkspaceTester;
  const std::string id() const override { return "FakeWorkspaceA"; }

private:
  FakeWorkspaceA *doClone() const override { return new FakeWorkspaceA(*this); }
  FakeWorkspaceA *doCloneEmpty() const override {
    return new FakeWorkspaceA(storageMode());
  }
};

class FakeWorkspaceB : public WorkspaceTester {
public:
  using WorkspaceTester::WorkspaceTester;
  const std::string id() const override { return "FakeWorkspaceB"; }

private:
  FakeWorkspaceB *doClone() const override { return new FakeWorkspaceB(*this); }
  FakeWorkspaceB *doCloneEmpty() const override {
    return new FakeWorkspaceB(storageMode());
  }
};

class FakeAlgNoParallelism : public Algorithm {
public:
  const std::string name() const override { return "FakeAlgNoParallelism"; }
  int version() const override { return 1; }
  const std::string category() const override { return ""; }
  const std::string summary() const override { return ""; }

  void init() override {
    declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                          Direction::Input));
  }

  void exec() override {}
};

class FakeAlgTestGetInputWorkspaceStorageModes : public Algorithm {
public:
  const std::string name() const override {
    return "FakeAlgTestGetInputWorkspaceStorageModes";
  }
  int version() const override { return 1; }
  const std::string category() const override { return ""; }
  const std::string summary() const override { return ""; }
  void init() override {
    declareProperty(std::make_unique<WorkspaceProperty<>>(
        "Input1", "", Kernel::Direction::Input));
    declareProperty(std::make_unique<WorkspaceProperty<>>(
        "Input2", "", Kernel::Direction::Input, PropertyMode::Optional));
    declareProperty(std::make_unique<WorkspaceProperty<>>(
        "Input3", "", Kernel::Direction::Input, PropertyMode::Optional));
    declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
        "InOut1", "", Direction::InOut));
    declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
        "InOut2", "", Direction::InOut, PropertyMode::Optional));
    declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
        "InOut3", "", Direction::InOut, PropertyMode::Optional));
  }
  void exec() override {}

protected:
  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override {
    // The result of getInputWorkspaceStorageModes is passed to this virtual
    // method, so we can test it here. Only initialized workspaces are part of
    // the map.
    TS_ASSERT_EQUALS(storageModes.size(), 4);
    TS_ASSERT_EQUALS(storageModes.count("Input1"), 1);
    TS_ASSERT_EQUALS(storageModes.count("Input2"), 1);
    TS_ASSERT_EQUALS(storageModes.count("InOut1"), 1);
    TS_ASSERT_EQUALS(storageModes.count("InOut2"), 1);
    return Parallel::ExecutionMode::Identical;
  }
};

class FakeAlgBadGetParallelExecutionMode : public Algorithm {
public:
  const std::string name() const override {
    return "FakeAlgBadGetParallelExecutionMode ";
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

class FakeAlg1To1 : public Algorithm {
public:
  const std::string name() const override { return "FakeAlg1To1"; }
  int version() const override { return 1; }
  const std::string category() const override { return ""; }
  const std::string summary() const override { return ""; }
  void init() override {
    declareProperty(std::make_unique<WorkspaceProperty<>>(
        "InputWorkspace", "", Kernel::Direction::Input,
        std::make_unique<HistogramValidator>()));
    declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
        "OutputWorkspace", "", Direction::Output));
  }
  void exec() override {
    boost::shared_ptr<MatrixWorkspace> ws = getProperty("InputWorkspace");
    setProperty("OutputWorkspace", ws->clone());
  }

protected:
  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override {
    return getCorrespondingExecutionMode(storageModes.at("InputWorkspace"));
  }
};

class FakeAlgNTo0 : public Algorithm {
public:
  const std::string name() const override { return "FakeAlgNTo0"; }
  int version() const override { return 1; }
  const std::string category() const override { return ""; }
  const std::string summary() const override { return ""; }
  void init() override {
    declareProperty(std::make_unique<WorkspaceProperty<>>(
        "InputWorkspace1", "", Kernel::Direction::Input,
        std::make_unique<HistogramValidator>()));
    declareProperty(std::make_unique<WorkspaceProperty<>>(
        "InputWorkspace2", "", Kernel::Direction::Input,
        std::make_unique<HistogramValidator>()));
  }
  void exec() override {
    boost::shared_ptr<MatrixWorkspace> ws1 = getProperty("InputWorkspace1");
    boost::shared_ptr<MatrixWorkspace> ws2 = getProperty("InputWorkspace2");
  }

protected:
  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override {
    return getCorrespondingExecutionMode(storageModes.at("InputWorkspace1"));
  }
};

class FakeAlgNTo1 : public Algorithm {
public:
  const std::string name() const override { return "FakeAlgNTo1"; }
  int version() const override { return 1; }
  const std::string category() const override { return ""; }
  const std::string summary() const override { return ""; }
  void init() override {
    declareProperty(std::make_unique<WorkspaceProperty<>>(
        "InputWorkspace1", "", Kernel::Direction::Input,
        std::make_unique<HistogramValidator>()));
    declareProperty(std::make_unique<WorkspaceProperty<>>(
        "InputWorkspace2", "", Kernel::Direction::Input,
        std::make_unique<HistogramValidator>()));
    declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
        "OutputWorkspace", "", Direction::Output));
  }
  void exec() override {
    boost::shared_ptr<MatrixWorkspace> ws1 = getProperty("InputWorkspace1");
    boost::shared_ptr<MatrixWorkspace> ws2 = getProperty("InputWorkspace2");
    setProperty("OutputWorkspace", ws1->clone());
  }

protected:
  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override {
    return getCorrespondingExecutionMode(storageModes.at("InputWorkspace1"));
  }
};

template <Parallel::StorageMode storageMode>
class FakeAlg0To1 : public Algorithm {
public:
  const std::string name() const override {
    return "FakeAlg0To1" + Parallel::toString(storageMode);
  }
  int version() const override { return 1; }
  const std::string category() const override { return ""; }
  const std::string summary() const override { return ""; }
  void init() override {
    declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
        "OutputWorkspace", "", Direction::Output));
  }
  void exec() override {
    auto ws = std::make_unique<FakeWorkspaceA>(storageMode);
    ws->initialize(1, 2, 1);
    setProperty("OutputWorkspace", std::move(ws));
  }

protected:
  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override {
    static_cast<void>(storageModes);
    return getCorrespondingExecutionMode(storageMode);
  }
};

template <Parallel::StorageMode storageModeOut>
class FakeAlg1To1StorageModeTransition : public Algorithm {
public:
  const std::string name() const override {
    return "FakeAlgAnyModeTo" + Parallel::toString(storageModeOut);
  }
  int version() const override { return 1; }
  const std::string category() const override { return ""; }
  const std::string summary() const override { return ""; }
  void init() override {
    declareProperty(std::make_unique<WorkspaceProperty<>>(
        "InputWorkspace", "", Kernel::Direction::Input,
        std::make_unique<HistogramValidator>()));
    declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
        "OutputWorkspace", "", Direction::Output));
  }
  void exec() override {
    boost::shared_ptr<MatrixWorkspace> ws = getProperty("InputWorkspace");
    setProperty("OutputWorkspace",
                std::make_unique<FakeWorkspaceA>(storageModeOut));
  }

protected:
  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override {
    static_cast<void>(storageModes);
    // ExecutionMode depends on *output* StorageMode.
    return getCorrespondingExecutionMode(storageModeOut);
  }
};

void runNoParallelism(const Parallel::Communicator &comm) {
  for (auto storageMode :
       {Parallel::StorageMode::Cloned, Parallel::StorageMode::Distributed,
        Parallel::StorageMode::MasterOnly}) {
    auto in = boost::make_shared<WorkspaceTester>(storageMode);
    auto alg = create<FakeAlgNoParallelism>(comm);
    alg->setProperty("InputWorkspace", in);
    if (comm.size() == 1) {
      TS_ASSERT_THROWS_NOTHING(alg->execute());
      TS_ASSERT(alg->isExecuted());
    } else {
      TS_ASSERT_THROWS_EQUALS(
          alg->execute(), const std::runtime_error &e, std::string(e.what()),
          "Algorithm does not support execution with input workspaces of the "
          "following storage types: \nInputWorkspace " +
              Parallel::toString(storageMode) + "\n.");
    }
  }
}

void runTestGetInputWorkspaceStorageModes(const Parallel::Communicator &comm) {
  auto alg = create<FakeAlgTestGetInputWorkspaceStorageModes>(comm);
  alg->setProperty("Input1", boost::make_shared<WorkspaceTester>());
  alg->setProperty("Input2", boost::make_shared<WorkspaceTester>());
  alg->setProperty("InOut1", boost::make_shared<WorkspaceTester>());
  alg->setProperty("InOut2", boost::make_shared<WorkspaceTester>());
  TS_ASSERT_THROWS_NOTHING(alg->execute());
  TS_ASSERT(alg->isExecuted());
}

void runBadGetParallelExecutionMode(const Parallel::Communicator &comm) {
  auto alg = create<FakeAlgBadGetParallelExecutionMode>(comm);
  if (comm.size() == 1) {
    TS_ASSERT_THROWS_NOTHING(alg->execute());
  } else {
    TS_ASSERT_THROWS_EQUALS(alg->execute(), const std::runtime_error &e,
                            std::string(e.what()),
                            "Parallel::ExecutionMode::Serial is not a valid "
                            "*parallel* execution mode.");
  }
}

void run1To1(const Parallel::Communicator &comm) {
  for (auto storageMode :
       {Parallel::StorageMode::Cloned, Parallel::StorageMode::Distributed,
        Parallel::StorageMode::MasterOnly}) {
    auto in = boost::make_shared<FakeWorkspaceA>(storageMode);
    in->initialize(1, 2, 1);
    auto alg = create<FakeAlg1To1>(comm);
    if (storageMode != Parallel::StorageMode::MasterOnly || comm.rank() == 0) {
      alg->setProperty("InputWorkspace", in);
    }
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    Workspace_const_sptr out = alg->getProperty("OutputWorkspace");
    if (storageMode != Parallel::StorageMode::MasterOnly || comm.rank() == 0) {
      TS_ASSERT_EQUALS(out->storageMode(), storageMode);
      TS_ASSERT_EQUALS(out->id(), "FakeWorkspaceA");
    } else {
      TS_ASSERT_EQUALS(out, nullptr);
    }
  }
}

void runNTo0(const Parallel::Communicator &comm) {
  for (auto storageMode :
       {Parallel::StorageMode::Cloned, Parallel::StorageMode::Distributed,
        Parallel::StorageMode::MasterOnly}) {
    auto in1 = boost::make_shared<FakeWorkspaceA>(storageMode);
    auto in2 = boost::make_shared<FakeWorkspaceB>(storageMode);
    in1->initialize(1, 2, 1);
    in2->initialize(1, 2, 1);
    auto alg = create<FakeAlgNTo0>(comm);
    if (storageMode != Parallel::StorageMode::MasterOnly || comm.rank() == 0) {
      alg->setProperty("InputWorkspace1", in1);
      alg->setProperty("InputWorkspace2", in2);
    }
    TS_ASSERT_THROWS_NOTHING(alg->execute());
  }
}

void runNTo1(const Parallel::Communicator &comm) {
  for (auto storageMode :
       {Parallel::StorageMode::Cloned, Parallel::StorageMode::Distributed,
        Parallel::StorageMode::MasterOnly}) {
    auto in1 = boost::make_shared<FakeWorkspaceA>(storageMode);
    auto in2 = boost::make_shared<FakeWorkspaceB>(storageMode);
    in1->initialize(1, 2, 1);
    in2->initialize(1, 2, 1);
    auto alg = create<FakeAlgNTo1>(comm);
    if (storageMode != Parallel::StorageMode::MasterOnly || comm.rank() == 0) {
      alg->setProperty("InputWorkspace1", in1);
      alg->setProperty("InputWorkspace2", in2);
    }
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    Workspace_const_sptr out = alg->getProperty("OutputWorkspace");
    if (storageMode != Parallel::StorageMode::MasterOnly || comm.rank() == 0) {
      // Preserving storage mode is actually not guaranteed, but this
      // implementation does of FakeAlgNTo1 does.
      TS_ASSERT_EQUALS(out->storageMode(), storageMode);
      TS_ASSERT_EQUALS(out->id(), "FakeWorkspaceA");
    } else {
      TS_ASSERT_EQUALS(out, nullptr);
    }
  }
}

template <Parallel::StorageMode storageMode>
void run0To1(const Parallel::Communicator &comm) {
  auto alg = create<FakeAlg0To1<storageMode>>(comm);
  TS_ASSERT_THROWS_NOTHING(alg->execute());
  TS_ASSERT(alg->isExecuted());
  Workspace_const_sptr out = alg->getProperty("OutputWorkspace");
  if (storageMode != Parallel::StorageMode::MasterOnly || comm.rank() == 0) {
    TS_ASSERT_EQUALS(out->storageMode(), storageMode);
    TS_ASSERT_EQUALS(out->id(), "FakeWorkspaceA");
  } else {
    TS_ASSERT_EQUALS(out, nullptr);
  }
}

template <Parallel::StorageMode modeIn, Parallel::StorageMode modeOut>
void run1To1StorageModeTransition(const Parallel::Communicator &comm) {
  auto in = boost::make_shared<FakeWorkspaceA>(modeIn);
  in->initialize(1, 2, 1);
  auto alg = create<FakeAlg1To1StorageModeTransition<modeOut>>(comm);
  if (modeIn != Parallel::StorageMode::MasterOnly || comm.rank() == 0) {
    alg->setProperty("InputWorkspace", in);
  }
  TS_ASSERT_THROWS_NOTHING(alg->execute());
  TS_ASSERT(alg->isExecuted());
  Workspace_const_sptr out = alg->getProperty("OutputWorkspace");
  if (modeOut != Parallel::StorageMode::MasterOnly || comm.rank() == 0) {
    TS_ASSERT_EQUALS(out->storageMode(), modeOut);
    TS_ASSERT_EQUALS(out->id(), "FakeWorkspaceA");
  } else {
    TS_ASSERT_EQUALS(out, nullptr);
  }
}

void runChained(const Parallel::Communicator &comm) {
  using Parallel::StorageMode;
  auto alg1 = create<FakeAlg0To1<StorageMode::MasterOnly>>(comm);
  TS_ASSERT_THROWS_NOTHING(alg1->execute());
  TS_ASSERT(alg1->isExecuted());
  Workspace_sptr ws1 = alg1->getProperty("OutputWorkspace");
  if (comm.rank() == 0) {
    TS_ASSERT_EQUALS(ws1->storageMode(), StorageMode::MasterOnly);
  } else {
    TS_ASSERT_EQUALS(ws1, nullptr);
  }

  auto alg2 =
      create<FakeAlg1To1StorageModeTransition<StorageMode::Distributed>>(comm);
  alg2->setProperty("InputWorkspace", ws1);
  TS_ASSERT_THROWS_NOTHING(alg2->execute());
  TS_ASSERT(alg2->isExecuted());
  Workspace_const_sptr ws2 = alg2->getProperty("OutputWorkspace");
  TS_ASSERT_EQUALS(ws2->storageMode(), StorageMode::Distributed);
}
} // namespace

class AlgorithmMPITest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AlgorithmMPITest *createSuite() { return new AlgorithmMPITest(); }
  static void destroySuite(AlgorithmMPITest *suite) { delete suite; }

  void testNoParallelism() { runParallel(runNoParallelism); }

  void testGetInputWorkspaceStorageModes() {
    runParallel(runTestGetInputWorkspaceStorageModes);
  }

  void testBadGetParallelExecutionMode() {
    runParallel(runBadGetParallelExecutionMode);
  }

  void test1To1() { runParallel(run1To1); }

  void testNTo0() { runParallel(runNTo0); }

  void testNTo1() { runParallel(runNTo1); }

  void test0To1() {
    runParallel(run0To1<Parallel::StorageMode::Cloned>);
    runParallel(run0To1<Parallel::StorageMode::Distributed>);
    runParallel(run0To1<Parallel::StorageMode::MasterOnly>);
  }

  void test1To1StorageModeTransition() {
    using Parallel::StorageMode;
    runParallel(
        run1To1StorageModeTransition<StorageMode::Cloned, StorageMode::Cloned>);
    runParallel(run1To1StorageModeTransition<StorageMode::Cloned,
                                             StorageMode::Distributed>);
    runParallel(run1To1StorageModeTransition<StorageMode::Cloned,
                                             StorageMode::MasterOnly>);
    runParallel(run1To1StorageModeTransition<StorageMode::Distributed,
                                             StorageMode::Cloned>);
    runParallel(run1To1StorageModeTransition<StorageMode::Distributed,
                                             StorageMode::Distributed>);
    runParallel(run1To1StorageModeTransition<StorageMode::Distributed,
                                             StorageMode::MasterOnly>);
    runParallel(run1To1StorageModeTransition<StorageMode::MasterOnly,
                                             StorageMode::Cloned>);
    runParallel(run1To1StorageModeTransition<StorageMode::MasterOnly,
                                             StorageMode::Distributed>);
    runParallel(run1To1StorageModeTransition<StorageMode::MasterOnly,
                                             StorageMode::MasterOnly>);
  }

  void testChained() {
    // Test that output from one algorithm can be fed into another (in
    // combination with non-trivial storage modes).
    runParallel(runChained);
  }
};

#endif /*ALGORITHMMPITEST_H_*/
