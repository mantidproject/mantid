// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PARALLEL_ENVIRONMENTSETUP_H_
#define MANTID_PARALLEL_ENVIRONMENTSETUP_H_

#ifdef MPI_EXPERIMENTAL
#include <boost/mpi/environment.hpp>
boost::mpi::environment env;
#endif

#endif /* MANTID_PARALLEL_ENVIRONMENTSETUP_H_ */
