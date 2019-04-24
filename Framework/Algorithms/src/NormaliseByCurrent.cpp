// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/NormaliseByCurrent.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/LogFilter.h"

namespace Mantid {
namespace Algorithms {

// Register with the algorithm factory
DECLARE_ALGORITHM(NormaliseByCurrent)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

void NormaliseByCurrent::init() {
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Name of the input workspace");
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace");
  declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<bool>>(
                      "RecalculatePCharge", false, Kernel::Direction::Input),
                  "Re-integrates the proton charge. This will modify the "
                  "gd_prtn_chrg. Does nothing for multi-period data");
}

/**
 * Extract a value for the charge from the input workspace. Handles either
 * single period or multi-period data.
 *
 * @param inputWS :: The input workspace to extract the log details from.
 * @param integratePCharge :: recalculate the integrated proton charge if true

 * @throws Exception::NotFoundError, std::domain_error or
 * std::runtime_error if the charge value(s) are not set in the
 * workspace logs or if the values are invalid (0)
 */
double NormaliseByCurrent::extractCharge(
    boost::shared_ptr<Mantid::API::MatrixWorkspace> inputWS,
    const bool integratePCharge) const {
  // Get the good proton charge and check it's valid
  double charge(-1.0);
  const Run &run = inputWS->run();

  int nPeriods = 0;
  try {
    nPeriods = run.getPropertyValueAsType<int>("nperiods");
  } catch (Exception::NotFoundError &) {
    g_log.information() << "No nperiods property. If this is multi-period "
                           "data, then you will be normalising against the "
                           "wrong current.\n";
  }
  // Handle multiperiod data.
  // The number of periods is set above by reference
  if (nPeriods > 1) {
    // Fetch the period property
    Property *currentPeriodNumberProperty = run.getLogData("current_period");
    int periodNumber = std::stoi(currentPeriodNumberProperty->value());

    // Fetch the charge property
    Property *chargeProperty = run.getLogData("proton_charge_by_period");
    ArrayProperty<double> *chargePropertyArray =
        dynamic_cast<ArrayProperty<double> *>(chargeProperty);
    if (chargePropertyArray) {
      charge = chargePropertyArray->operator()()[periodNumber - 1];
    } else {
      throw Exception::NotFoundError(
          "Proton charge log (proton_charge_by_period) not found for this "
          "multiperiod data workspace (" +
              inputWS->getName() + ")",
          "proton_charge_by_period");
    }

    if (charge == 0) {
      throw std::domain_error("The proton charge found for period number " +
                              std::to_string(periodNumber) +
                              " in the input workspace (" + inputWS->getName() +
                              ") run information is zero. When applying "
                              "NormaliseByCurrent on multiperiod data, a "
                              "non-zero value is required for every period in "
                              "the proton_charge_by_period log.");
    }

  } else {
    try {
      if (integratePCharge) {
        inputWS->run().integrateProtonCharge();
      }
      charge = inputWS->run().getProtonCharge();
    } catch (Exception::NotFoundError &) {
      g_log.error() << "The proton charge is not set for the run attached to "
                       "the workspace(" +
                           inputWS->getName() + ")\n";
      throw;
    }

    if (charge == 0) {
      throw std::domain_error(
          "The proton charge found in the input workspace (" +
          inputWS->getName() + ") run information is zero");
    }
  }
  return charge;
}

void NormaliseByCurrent::exec() {
  // Get the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  const bool integratePCharge = getProperty("RecalculatePCharge");

  // Get the good proton charge and check it's valid
  double charge = extractCharge(inputWS, integratePCharge);

  g_log.information() << "Normalisation current: " << charge << " uamps\n";

  double invcharge = 1.0 / charge; // Inverse of the charge to be multiplied by

  // The operator overloads properly take into account of both EventWorkspaces
  // and doing it in place or not.
  if (inputWS != outputWS) {
    outputWS = inputWS * invcharge;
    setProperty("OutputWorkspace", outputWS);
  } else {
    inputWS *= invcharge;
  }
  outputWS->mutableRun().addLogData(
      new Kernel::PropertyWithValue<double>("NormalizationFactor", charge));
  outputWS->setYUnitLabel("Counts per microAmp.hour");
}

} // namespace Algorithms
} // namespace Mantid
