#ifndef MPI_HELPERS_TEST_H_
#define MPI_HELPERS_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMPI/Helpers.h"

using namespace Mantid::MPI;

class MPIHelpersTest : public CxxTest::TestSuite {
public:
#ifdef MPI_BUILD
  void testCommunicator() { TS_ASSERT_THROWS_NOTHING(communicator()); }
#endif
  void testRank() { TS_ASSERT_EQUALS(rank(), 0); }
  void testRootRank() { TS_ASSERT_EQUALS(rootRank(), 0); }
  void testIsRoot() { TS_ASSERT(isRoot()); }
  void testNumberOfRanks() { TS_ASSERT_EQUALS(numberOfRanks(), 1); }
};

#endif /* MPI_HELPERS_TEST_H_ */
