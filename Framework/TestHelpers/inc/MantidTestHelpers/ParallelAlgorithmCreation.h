// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PARALLELALGORITHMCREATION_H_
#define PARALLELALGORITHMCREATION_H_

#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/Property.h"


namespace Mantid {
namespace Parallel {
class Communicator;
}
} // namespace Mantid

namespace ParallelTestHelpers {

/** Create an initialized algorithm that does not store workspaces in the ADS.
 *
 * This function is the recommended way to create algorithm for use with
 * ParallelRunner since it avoids name clashes in the ADS from different ranks.
 * This factory function is intended for unit tests that need to call algorithms
 * in MPI tests. The communicator argument is obtained from
 * ParallelTestHelpers::runParallel(). The algorithm is setup such that
 * workspaces will not be stored in the ADS, i.e., it is not necessary to set
 * OutputWorkspace properties, just like for child algorithms. */
template <class T>
std::unique_ptr<T> create(const Mantid::Parallel::Communicator &comm) {
  using namespace Mantid;
  auto alg = std::make_unique<T>();
  alg->setChild(true);
  alg->setCommunicator(comm);
  alg->initialize();
  const auto &props = alg->getProperties();
  for (auto prop : props) {
    auto wsProp = dynamic_cast<API::IWorkspaceProperty *>(prop);
    if (prop->direction() == Kernel::Direction::Output && wsProp) {
      if (prop->value().empty()) {
        prop->createTemporaryValue();
      }
    }
  }
  return alg;
}

} // namespace ParallelTestHelpers

#endif /*PARALLELALGORITHMCREATION_H_*/
