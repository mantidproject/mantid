#ifndef MANTID_API_NONMASTERDUMMYWORKSPACETEST_H_
#define MANTID_API_NONMASTERDUMMYWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/NonMasterDummyWorkspace.h"
#include "MantidKernel/make_unique.h"

#ifdef MPI_EXPERIMENTAL
#include "MantidParallel/ParallelRunner.h"
#endif

using namespace Mantid;
using API::NonMasterDummyWorkspace;

namespace {
void run_construct(const Parallel::Communicator &comm) {
  std::unique_ptr<NonMasterDummyWorkspace> ws;
  if (comm.rank() == 0) {
    TS_ASSERT_THROWS_EQUALS(
        ws = Kernel::make_unique<NonMasterDummyWorkspace>(comm),
        const std::runtime_error &e, std::string(e.what()),
        "NonMasterDummyWorkspace cannot be created on the master rank.");
  } else {
    TS_ASSERT_THROWS_NOTHING(
        ws = Kernel::make_unique<NonMasterDummyWorkspace>(comm));
    TS_ASSERT_EQUALS(ws->storageMode(), Parallel::StorageMode::MasterOnly);
  }
}
}

class NonMasterDummyWorkspaceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NonMasterDummyWorkspaceTest *createSuite() {
    return new NonMasterDummyWorkspaceTest();
  }
  static void destroySuite(NonMasterDummyWorkspaceTest *suite) { delete suite; }

  void test_construct() {
#ifndef MPI_EXPERIMENTAL
    Communicator comm;
    run_construct(comm);
#else
    Parallel::runParallel(run_construct);
#endif
  }
};

#endif /* MANTID_API_NONMASTERDUMMYWORKSPACETEST_H_ */
