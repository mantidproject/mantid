// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/AverageSpectrumBackground.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/Algorithm.tcc"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AverageSpectrumBackground)

/// Algorithm's name for identification. @see Algorithm::name
const std::string AverageSpectrumBackground::name() const {
  return "AverageSpectrumBackground";
}

/// Algorithm's version for identification. @see Algorithm::version
int AverageSpectrumBackground::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string AverageSpectrumBackground::category() const {
  return "Reflectometry;Reflectometry\\ISIS";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string AverageSpectrumBackground::summary() const { return ""; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void AverageSpectrumBackground::init() {

  // Input workspace
  declareWorkspaceInputProperties<API::MatrixWorkspace,
                                  API::IndexType::SpectrumNum |
                                      API::IndexType::WorkspaceIndex>(
      "InputWorkspace", "An input workspace",
      boost::make_shared<API::CommonBinsValidator>());

  // Output workspace
  declareProperty(Kernel::make_unique<API::WorkspaceProperty<>>(
                      "OutputWorkspace", "", Kernel::Direction::Output),
                  "A Workspace with the background removed.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void AverageSpectrumBackground::exec() {

  API::MatrixWorkspace_sptr inputWS;
  Indexing::SpectrumIndexSet indexSet;
  std::tie(inputWS, indexSet) =
      getWorkspaceAndIndices<API::MatrixWorkspace>("InputWorkspace");

  std::vector<specnum_t> spectraList;
  for (auto index : indexSet) {
    auto &spec = inputWS->getSpectrum(index);
    spectraList.push_back(spec.getSpectrumNo());
  }

  auto alg = this->createChildAlgorithm("GroupDetectors");
  alg->setProperty("InputWorkspace", inputWS);
  alg->setProperty("SpectraList", spectraList);
  alg->setProperty("Behaviour", "Average");
  alg->execute();
  API::MatrixWorkspace_sptr averageBgd = alg->getProperty("OutputWorkspace");

  auto subtract = createChildAlgorithm("Minus");
  subtract->setProperty("LHSWorkspace", inputWS);
  subtract->setProperty("RHSWorkspace", averageBgd);
  subtract->setProperty("AllowDifferentNumberSpectra", true);
  subtract->execute();
  API::MatrixWorkspace_sptr outputWS = subtract->getProperty("OutputWorkspace");

  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid
