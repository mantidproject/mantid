// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_INSTRUMENTVALIDATOR_H_
#define MANTID_API_INSTRUMENTVALIDATOR_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidKernel/TypedValidator.h"

namespace Mantid {
namespace API {

class ExperimentInfo;

/**
  A validator which checks that a workspace has a valid instrument.
*/
class MANTID_API_DLL InstrumentValidator
    : public Kernel::TypedValidator<boost::shared_ptr<ExperimentInfo>> {
public:
  /// Enumeration describing requirements
  enum Requirements { SourcePosition = 0x1, SamplePosition = 0x2 };

  // The default is historical so I don't break a lot of user code
  InstrumentValidator(const unsigned int flags = SamplePosition);
  std::string getType() const;
  Kernel::IValidator_sptr clone() const override;
  std::string
  checkValidity(const boost::shared_ptr<ExperimentInfo> &value) const override;

private:
  unsigned int m_requires;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_INSTRUMENTVALIDATOR_H_ */
