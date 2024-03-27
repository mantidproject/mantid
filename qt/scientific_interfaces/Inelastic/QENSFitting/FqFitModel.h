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
namespace IDA {

class MANTIDQT_INELASTIC_DLL FqFitModel : public FittingModel {
public:
  FqFitModel();

private:
  std::string getResultXAxisUnit() const override;
  std::string getResultLogName() const override;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
