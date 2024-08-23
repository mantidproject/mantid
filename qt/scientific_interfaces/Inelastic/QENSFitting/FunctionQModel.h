// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FittingModel.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class MANTIDQT_INELASTIC_DLL FunctionQModel : public FittingModel {
public:
  FunctionQModel();

private:
  std::string getResultXAxisUnit() const override;
  std::string getResultLogName() const override;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
