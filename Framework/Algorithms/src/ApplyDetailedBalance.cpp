// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ApplyDetailedBalance.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "boost/lexical_cast.hpp"
#include <cmath>

using std::string;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ApplyDetailedBalance)

/** Initialize the algorithm's properties.
 */
void ApplyDetailedBalance::init() {
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("DeltaE");
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, wsValidator),
                  "An input workspace.");
  declareProperty(std::make_unique<PropertyWithValue<string>>("Temperature", "", Direction::Input),
                  "SampleLog variable name that contains the temperature, or a number");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
  std::vector<std::string> unitOptions{"Energy", "Frequency"};
  declareProperty("OutputUnits", "Energy", std::make_shared<StringListValidator>(unitOptions),
                  "Susceptibility as a function of energy (meV) or frequency (GHz)");
}

/** Execute the algorithm.
 */
void ApplyDetailedBalance::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  // If input and output workspaces are not the same, create a new workspace for
  // the output
  if (outputWS != inputWS) {
    outputWS = create<MatrixWorkspace>(*inputWS);
  }

  std::string Tstring = getProperty("Temperature");
  double Temp;
  try {
    if (inputWS->run().hasProperty(Tstring)) {
      if (const auto log =
              dynamic_cast<const Kernel::TimeSeriesProperty<double> *>(inputWS->run().getProperty(Tstring))) {
        Temp = log->getStatistics().mean;
      } else {
        throw std::invalid_argument(Tstring + " is not a double-valued log.");
      }
    } else {
      Temp = boost::lexical_cast<double>(Tstring);
    }
  } catch (...) {
    Tstring += " is not a valid log, nor is it a number";
    throw std::invalid_argument(Tstring);
  }

  double oneOverT = PhysicalConstants::meVtoKelvin / Temp;
  // Run the exponential correction algorithm explicitly to enable progress
  // reporting
  auto expcor = createChildAlgorithm("OneMinusExponentialCor", 0.0, 1.0);
  expcor->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS);
  expcor->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", outputWS);
  expcor->setProperty<double>("C1", M_PI);
  expcor->setProperty<double>("C", oneOverT);
  expcor->setPropertyValue("Operation", "Multiply");
  expcor->executeAsChildAlg();
  // Get back the result
  outputWS = expcor->getProperty("OutputWorkspace");

  // Select the unit, transform if different than energy
  std::string unit = getProperty("OutputUnits");
  if (unit == "Frequency") {
    auto convert = createChildAlgorithm("ConvertUnits");
    convert->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outputWS);
    convert->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", outputWS);
    convert->setProperty<std::string>("Target", "DeltaE_inFrequency");
    convert->executeAsChildAlg();
  }
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid::Algorithms
