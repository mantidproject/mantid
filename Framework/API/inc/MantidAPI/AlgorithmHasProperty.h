// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_ALGORITHMHASPROPERTY_H_
#define MANTID_API_ALGORITHMHASPROPERTY_H_

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/TypedValidator.h"

namespace Mantid {
namespace API {

class IAlgorithm;

/**
  A validator to check whether a given algorithm has a named property.
  The algorithm's property must be valid for the validator to pass.

  @author Martyn Gigg, Tessella plc
  @date 30/03/2011
*/
class MANTID_API_DLL AlgorithmHasProperty
    : public Kernel::TypedValidator<boost::shared_ptr<IAlgorithm>> {
public:
  AlgorithmHasProperty(const std::string &propName);
  std::string getType() const;
  Kernel::IValidator_sptr clone() const override;

protected:
  std::string
  checkValidity(const boost::shared_ptr<IAlgorithm> &value) const override;

private:
  /// Store the property name
  std::string m_propName;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_ALGORITHMHASPROPERTY_H_ */
