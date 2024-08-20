// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ISISEnergyTransferValidator.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"

#include <filesystem>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets::WorkspaceUtils;

namespace {
MatrixWorkspace_sptr load(std::string const &filename, int const specMin, int const specMax) {
  auto loader = AlgorithmManager::Instance().create("Load");
  loader->initialize();
  loader->setAlwaysStoreInADS(false);
  loader->setProperty("Filename", filename);
  if (loader->existsProperty("LoadLogFiles")) {
    loader->setProperty("LoadLogFiles", false);
  }
  loader->setPropertyValue("SpectrumMin", std::to_string(specMin));
  loader->setPropertyValue("SpectrumMax", std::to_string(specMax));
  loader->execute();
  Mantid::API::Workspace_sptr ws = loader->getProperty("OutputWorkspace");
  return std::dynamic_pointer_cast<MatrixWorkspace>(ws);
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {

std::string IETDataValidator::validateConversionData(IETConversionData conversionData) {
  const int specMin = conversionData.getSpectraMin();
  const int specMax = conversionData.getSpectraMax();

  if (specMin > specMax) {
    return "Minimum spectra must be less than maximum spectra.";
  }

  return "";
}

std::vector<std::string> IETDataValidator::validateBackgroundData(IETBackgroundData const &backgroundData,
                                                                  IETConversionData const &conversionData,
                                                                  std::string const &firstFileName,
                                                                  bool const isRunFileValid) {
  std::vector<std::string> errors;

  if (!isRunFileValid || !backgroundData.getRemoveBackground()) {
    return errors;
  }

  const int backgroundStart = backgroundData.getBackgroundStart();
  const int backgroundEnd = backgroundData.getBackgroundEnd();

  if (backgroundStart > backgroundEnd) {
    errors.push_back("Background Start must be less than Background End");
  }

  int specMin = conversionData.getSpectraMin();
  int specMax = conversionData.getSpectraMax();
  const auto workspace = load(firstFileName, specMin, specMax);

  const double minBack = workspace->x(0).front();
  const double maxBack = workspace->x(0).back();

  if (backgroundStart < minBack) {
    errors.push_back("The Start of Background Removal is less than the minimum of the data range");
  }

  if (backgroundEnd > maxBack) {
    errors.push_back("The End of Background Removal is more than the maximum of the data range");
  }

  return errors;
}

std::string IETDataValidator::validateAnalysisData(IETAnalysisData analysisData) {
  if (analysisData.getUseDetailedBalance()) {
    if (analysisData.getDetailedBalance() == 0.0) {
      return "Detailed Balance must be more than 0 K";
    }
  }
  return "";
}

} // namespace CustomInterfaces
} // namespace MantidQt