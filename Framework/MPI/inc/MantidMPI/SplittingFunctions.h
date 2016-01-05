#ifndef MANTID_MPI_SPLITTING_FUNCTIONS_H_
#define MANTID_MPI_SPLITTING_FUNCTIONS_H_

#include <cstdlib>

namespace Mantid {
namespace MPI {

///
bool indexIsOnThisRank(int64_t index);
int rankOfIndex(int64_t index);

} // namespace MPI
} // namespace Mantid

#endif /* MANTID_MPI_SPLITTING_FUNCTIONS_H_*/
