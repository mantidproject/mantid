
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
};

} // namespace CustomInterfaces
} // namespace MantidQt