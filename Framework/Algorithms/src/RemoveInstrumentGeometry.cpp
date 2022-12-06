// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/RemoveInstrumentGeometry.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>

using namespace Mantid::API;
using namespace Mantid::Geometry;
namespace Mantid {
namespace Algorithms {
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RemoveInstrumentGeometry)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string RemoveInstrumentGeometry::name() const { return "RemoveInstrumentGeometry"; }

/// Algorithm's version for identification. @see Algorithm::version
int RemoveInstrumentGeometry::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string RemoveInstrumentGeometry::category() const { return "Utility\\Workspaces"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string RemoveInstrumentGeometry::summary() const {
  return "Removes instrument geometry records from a given workspace.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void RemoveInstrumentGeometry::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("InputWorkspace", "", Direction::Input),
                  "An input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
  declareProperty(std::make_unique<Kernel::ArrayProperty<int>>("MDExperimentInfoNumbers"),
                  "For MD workspaces, the ExperimentInfo indices to have the instrument removed."
                  "If empty, the instrument will be removed from all ExperimentInfo objects."
                  "The parameter is ignored for any other workspace type.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void RemoveInstrumentGeometry::exec() {
  Workspace_const_sptr inputWS = this->getProperty("InputWorkspace");

  Workspace_sptr outputWS = this->getProperty("OutputWorkspace");
  if (outputWS != inputWS) {
    outputWS = inputWS->clone();
  }

  // create an empty instrument
  Instrument_sptr emptyInstrument(new Geometry::Instrument());

  MatrixWorkspace_sptr outputMtrxWS = std::dynamic_pointer_cast<API::MatrixWorkspace>(outputWS);
  if (outputMtrxWS != nullptr) { // it is a matrix workspace
    outputMtrxWS->setInstrument(emptyInstrument);
  } else {
    MultipleExperimentInfos_sptr outputMDWS = std::dynamic_pointer_cast<Mantid::API::MultipleExperimentInfos>(outputWS);
    if (outputMDWS != nullptr) { // it is an MD workspace
                                 // which experiments do we remove instrument geometry from?
      std::vector<int> indicesToRemoveInstrument = this->getProperty("MDExperimentInfoNumbers");

      if (indicesToRemoveInstrument.empty()) { // remove instrument geometry from all experiments
        indicesToRemoveInstrument.resize(outputMDWS->getNumExperimentInfo());
        std::iota(indicesToRemoveInstrument.begin(), indicesToRemoveInstrument.end(), 0);
      }

      for (uint16_t i = 0; i < indicesToRemoveInstrument.size(); i++) {
        auto ei = outputMDWS->getExperimentInfo(static_cast<uint16_t>(indicesToRemoveInstrument[i]));
        ei->setInstrument(emptyInstrument);
      }
    } else {
      throw std::invalid_argument("Wrong type of input workspace");
    }
  }

  this->setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid
