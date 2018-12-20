#ifndef MANTID_PARALLEL_ENVIRONMENTSETUP_H_
#define MANTID_PARALLEL_ENVIRONMENTSETUP_H_

#ifdef MPI_EXPERIMENTAL
#include <boost/mpi/environment.hpp>
boost::mpi::environment env;
#endif

#endif /* MANTID_PARALLEL_ENVIRONMENTSETUP_H_ */
