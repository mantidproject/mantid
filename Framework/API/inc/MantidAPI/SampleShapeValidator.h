// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_SAMPLESHAPEVALIDATOR_H_
#define MANTID_API_SAMPLESHAPEVALIDATOR_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ExperimentInfo.h"

#include "MantidKernel/TypedValidator.h"

namespace Mantid {
namespace API {

/**
  Verify that a workspace has valid sample shape.
*/
class MANTID_API_DLL SampleShapeValidator
    : public Kernel::TypedValidator<boost::shared_ptr<ExperimentInfo>> {
public:
  std::string getType() const;
  Kernel::IValidator_sptr clone() const override;

private:
  std::string
  checkValidity(const boost::shared_ptr<ExperimentInfo> &value) const override;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_SAMPLESHAPEVALIDATOR_H_ */
