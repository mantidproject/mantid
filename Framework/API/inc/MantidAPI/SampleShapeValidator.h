// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ExperimentInfo.h"

#include "MantidKernel/TypedValidator.h"

namespace Mantid {
namespace API {

/**
  Verify that a workspace has valid sample shape.
*/
class MANTID_API_DLL SampleShapeValidator : public Kernel::TypedValidator<std::shared_ptr<ExperimentInfo>> {
public:
  std::string getType() const;
  Kernel::IValidator_sptr clone() const override;

private:
  std::string checkValidity(const std::shared_ptr<ExperimentInfo> &value) const override;
};

} // namespace API
} // namespace Mantid
