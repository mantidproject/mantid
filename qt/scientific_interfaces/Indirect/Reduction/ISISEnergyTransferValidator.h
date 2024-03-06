
// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ISISEnergyTransferData.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

namespace MantidQt {
namespace CustomInterfaces {
class IETDataValidator {

public:
  IETDataValidator() = default;

  std::string validateConversionData(IETConversionData conversionData);
  std::vector<std::string> validateBackgroundData(IETBackgroundData backgroundData, IETConversionData conversionData,
                                                  std::string firstFileName, bool isRunFileValid);
  std::string validateAnalysisData(IETAnalysisData analysisData);

  std::string validateDetectorGrouping(Mantid::API::AlgorithmRuntimeProps *groupingProperties,
                                       std::size_t const &defaultSpectraMin, std::size_t const &defaultSpectraMax);

private:
  std::vector<std::size_t> getCustomGroupingNumbers(std::string const &customString);
  std::string checkCustomGroupingNumbersInRange(std::vector<std::size_t> const &customGroupingNumbers,
                                                std::size_t const &spectraMin, std::size_t const &spectraMax) const;
  bool numberInCorrectRange(std::size_t const &spectraNumber, std::size_t const &spectraMin,
                            std::size_t const &spectraMax) const;
};

} // namespace CustomInterfaces
} // namespace MantidQt