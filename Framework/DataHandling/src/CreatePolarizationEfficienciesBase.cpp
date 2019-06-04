// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/CreatePolarizationEfficienciesBase.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Unit.h"

#include <boost/shared_ptr.hpp>

#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace Mantid {
namespace DataHandling {

std::string const CreatePolarizationEfficienciesBase::Pp("Pp");
std::string const CreatePolarizationEfficienciesBase::Ap("Ap");
std::string const CreatePolarizationEfficienciesBase::Rho("Rho");
std::string const CreatePolarizationEfficienciesBase::Alpha("Alpha");
std::string const CreatePolarizationEfficienciesBase::P1("P1");
std::string const CreatePolarizationEfficienciesBase::P2("P2");
std::string const CreatePolarizationEfficienciesBase::F1("F1");
std::string const CreatePolarizationEfficienciesBase::F2("F2");

const std::string CreatePolarizationEfficienciesBase::category() const {
  return "Reflectometry";
}

void CreatePolarizationEfficienciesBase::initOutputWorkspace() {
  declareProperty(std::make_unique<WorkspaceProperty<Mantid::API::MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

void CreatePolarizationEfficienciesBase::exec() {
  auto const labelsFredrikze = getNonDefaultProperties({Pp, Ap, Rho, Alpha});
  auto const labelsWildes = getNonDefaultProperties({P1, P2, F1, F2});

  if (labelsFredrikze.empty() && labelsWildes.empty()) {
    throw std::invalid_argument(
        "At least one of the efficiencies must be set.");
  }

  if (!labelsFredrikze.empty() && !labelsWildes.empty()) {
    throw std::invalid_argument(
        "Efficiencies belonging to different methods cannot mix.");
  }

  MatrixWorkspace_sptr efficiencies;
  if (!labelsFredrikze.empty()) {
    efficiencies = createEfficiencies(labelsFredrikze);
  } else {
    efficiencies = createEfficiencies(labelsWildes);
  }

  setProperty("OutputWorkspace", efficiencies);
}

/// Get names of non-default properties out of a list of names
/// @param labels :: Names of properties to check.
std::vector<std::string>
CreatePolarizationEfficienciesBase::getNonDefaultProperties(
    std::vector<std::string> const &labels) const {
  std::vector<std::string> outputLabels;
  for (auto const &label : labels) {
    if (!isDefault(label)) {
      outputLabels.emplace_back(label);
    }
  }
  return outputLabels;
}

} // namespace DataHandling
} // namespace Mantid
