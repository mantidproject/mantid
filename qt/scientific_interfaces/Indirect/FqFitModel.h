// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectFittingModel.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL FqFitModel : public IndirectFittingModel {
public:
  FqFitModel();

private:
  std::string getResultXAxisUnit() const override;
  std::string getResultLogName() const override;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
