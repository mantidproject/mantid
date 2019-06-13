// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidRemoteAlgorithms/StopRemoteTransaction2.h"
#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/SimpleJSON.h"

namespace Mantid {
namespace RemoteAlgorithms {

// Register the algorithm into the Algorithm Factory
DECLARE_ALGORITHM(StopRemoteTransaction2)

using namespace Mantid::Kernel;

// A reference to the logger is provided by the base class, it is called g_log.

void StopRemoteTransaction2::init() {
  auto requireValue =
      boost::make_shared<Mantid::Kernel::MandatoryValidator<std::string>>();

  // Compute Resources
  std::vector<std::string> computes = Mantid::Kernel::ConfigService::Instance()
                                          .getFacility()
                                          .computeResources();
  declareProperty(
      "ComputeResource", "", boost::make_shared<StringListValidator>(computes),
      "The name of the remote computer where the transaction was created",
      Direction::Input);

  // The transaction ID comes from the StartRemoteTransaction algortithm
  declareProperty("TransactionID", "", requireValue,
                  "The ID string returned when the transaction was created",
                  Mantid::Kernel::Direction::Input);
}

void StopRemoteTransaction2::exec() {

  const std::string comp = getPropertyValue("ComputeResource");
  Mantid::API::IRemoteJobManager_sptr jm =
      Mantid::API::RemoteJobManagerFactory::Instance().create(comp);

  std::string tid = getPropertyValue("TransactionID");
  jm->stopRemoteTransaction(tid);

  g_log.information() << "Transaction with ID " << tid
                      << " stopped on the compute resource " << comp << '\n';
}

} // end namespace RemoteAlgorithms
} // end namespace Mantid
