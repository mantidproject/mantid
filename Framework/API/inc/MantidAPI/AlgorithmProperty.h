// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_ALGORITHMPROPERTY_H_
#define MANTID_API_ALGORITHMPROPERTY_H_

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/PropertyWithValue.h"

namespace Mantid {
namespace API {
//-------------------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------------------
class IAlgorithm;

#ifdef _WIN32
#pragma warning(push)
// Disable 'multiple assignment operators specified' warning for this class
//  - it's not an accident that we have more than one
#pragma warning(disable : 4522)
#endif

/**
Define an algorithm property that can be used to supply an algorithm object
to a subsequent algorithm. It is a specialized version of PropertyWithValue
where the type a pointer to an object implementing the
API::IAlgorithm interface.

@author Martyn Gigg, Tessella Plc
@date 24/03/2011
*/
class DLLExport AlgorithmProperty
    : public Kernel::PropertyWithValue<boost::shared_ptr<IAlgorithm>> {
public:
  /// Typedef the held type
  using HeldType = boost::shared_ptr<IAlgorithm>;

  /// Constructor
  AlgorithmProperty(const std::string &propName,
                    Kernel::IValidator_sptr validator =
                        Kernel::IValidator_sptr(new Kernel::NullValidator),
                    unsigned int direction = Kernel::Direction::Input);
  // Unhide base class member that would be hidden by implicitly declared
  // assignment operator
  using Kernel::PropertyWithValue<HeldType>::operator=;

  /// 'Virtual copy constructor'
  inline AlgorithmProperty *clone() const override {
    return new AlgorithmProperty(*this);
  }

  /// Add the value of another property. Doesn't make sense here.
  AlgorithmProperty &operator+=(Kernel::Property const *) override {
    throw Kernel::Exception::NotImplementedError(
        "+= operator is not implemented for AlgorithmProperty.");
    return *this;
  }
  /// Return the algorithm as string
  std::string value() const override;
  /// Create a Json::Value from the algorithm value
  Json::Value valueAsJson() const override;
  /// Get the default
  std::string getDefault() const override;
  /// Sets the value of the algorithm from a string representation
  std::string setValue(const std::string &value) override;
  /// Sets the value of the algorithm from a Json representation
  std::string setValueFromJson(const Json::Value &value) override;

private:
  std::string setBaseValue(const HeldType &algm);

  // Cached string as value() can be called frequently
  std::string m_algmStr;
};

#ifdef _WIN32
#pragma warning(                                                               \
    pop) // Re-enable the warning about multiple assignment operators
#endif

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_ALGORITHMPROPERTY_H_ */
