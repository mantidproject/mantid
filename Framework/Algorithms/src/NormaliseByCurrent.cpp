//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/LogFilter.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAlgorithms/NormaliseByCurrent.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {

// Register with the algorithm factory
DECLARE_ALGORITHM(NormaliseByCurrent)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

/// Default constructor
NormaliseByCurrent::NormaliseByCurrent() : Algorithm() {}

// Destructor
NormaliseByCurrent::~NormaliseByCurrent() = default;

void NormaliseByCurrent::init() {
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Name of the input workspace");
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace");
}

/**
 * Extract a value for the charge from the input workspace. Handles either
 * single period or multi-period data.
 *
 * @param inputWS :: The input workspace to extract the log details from.
 *
 * @throws Exception::NotFoundError, std::domain_error or
 * std::runtime_error if the charge value(s) are not set in the
 * workspace logs or if the values are invalid (0)
 */
double NormaliseByCurrent::extractCharge(
    boost::shared_ptr<Mantid::API::MatrixWorkspace> inputWS) const {
  // Get the good proton charge and check it's valid
  double charge(-1.0);
  const Run &run = inputWS->run();

  int nPeriods = 0;
  try {
    Property *nPeriodsProperty = run.getLogData("nperiods");
    Kernel::toValue<int>(nPeriodsProperty->value(), nPeriods);
  } catch (Exception::NotFoundError &) {
    g_log.information() << "No nperiods property. If this is multi-period "
                           "data, then you will be normalising against the "
                           "wrong current.\n";
  }
  // Handle multiperiod data.
  // The number of periods is set above by reference
  // cppcheck-suppress knownConditionTrueFalse
  if (nPeriods > 1) {
    // Fetch the period property
    Property *currentPeriodNumberProperty = run.getLogData("current_period");
    int periodNumber = std::atoi(currentPeriodNumberProperty->value().c_str());

    // Fetch the charge property
    Property *chargeProperty = run.getLogData("proton_charge_by_period");
    ArrayProperty<double> *chargePropertyArray =
        dynamic_cast<ArrayProperty<double> *>(chargeProperty);
    if (chargePropertyArray) {
      charge = chargePropertyArray->operator()()[periodNumber - 1];
    } else {
      throw Exception::NotFoundError(
          "Proton charge log (proton_charge_by_period) not found for this "
          "multiperiod data workspace",
          "proton_charge_by_period");
    }

    if (charge == 0) {
      throw std::domain_error("The proton charge found for period number " +
                              std::to_string(periodNumber) +
                              " in the input workspace "
                              "run information is zero. When applying "
                              "NormaliseByCurrent on multiperiod data, a "
                              "non-zero value is required for every period in "
                              "the proton_charge_by_period log.");
    }

  } else {
    try {
      charge = inputWS->run().getProtonCharge();
    } catch (Exception::NotFoundError &) {
      g_log.error() << "The proton charge is not set for the run attached to "
                       "this workspace\n";
      throw;
    }

    if (charge == 0) {
      throw std::domain_error("The proton charge found in the input workspace "
                              "run information is zero");
    }
  }
  return charge;
}

void NormaliseByCurrent::exec() {
  // Get the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  // Get the good proton charge and check it's valid
  double charge = extractCharge(inputWS);

  g_log.information() << "Normalisation current: " << charge << " uamps"
                      << std::endl;

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

} // namespace Algorithm
} // namespace Mantid
