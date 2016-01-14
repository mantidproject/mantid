#ifndef SPLITTING_FUNCTIONS_TEST_H_
#define SPLITTING_FUNCTIONS_TEST_H_

#include <cxxtest/TestSuite.h>
#include <boost/mpi.hpp>

#include "MantidMPI/SplittingFunctions.h"
#include "MantidAPI/FrameworkManager.h"

using namespace Mantid;

class SplittingFunctionsTest : public CxxTest::TestSuite {
public:
  static SplittingFunctionsTest *createSuite() {
    return new SplittingFunctionsTest();
  }
  static void destroySuite(SplittingFunctionsTest *suite) { delete suite; }

  SplittingFunctionsTest() {
    // Create the Framework manager so that MPI gets initialized
    API::FrameworkManager::Instance();
  }

  void testIndexIsOnThisRank() {
    boost::mpi::communicator world;
    TS_ASSERT_EQUALS(Mantid::MPI::indexIsOnThisRank(world.rank()), true);
    TS_ASSERT_EQUALS(
        Mantid::MPI::indexIsOnThisRank(world.size() + world.rank()), true);
  }
};

#endif /* SPLITTING_FUNCTIONS_TEST_H_*/
