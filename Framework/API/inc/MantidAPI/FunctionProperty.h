// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_FUNCTIONPROPERTY_H_
#define MANTID_API_FUNCTIONPROPERTY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/PropertyWithValue.h"

#include <string>

namespace Mantid {
namespace API {
/** A property class for functions. Holds a shared pointer to a function. The
string representation
is the creation string accepted by the FunctionFactory.

@author Roman Tolchenov, Tessella plc
@date 08/02/2012
*/
class MANTID_API_DLL FunctionProperty
    : public Kernel::PropertyWithValue<boost::shared_ptr<IFunction>> {
public:
  /// Constructor.
  FunctionProperty(const std::string &name,
                   const unsigned int direction = Kernel::Direction::Input);

  /// Copy constructor
  FunctionProperty(const FunctionProperty &right);

  /// Copy assignment operator. Only copies the value (i.e. the pointer to the
  /// function)
  FunctionProperty &operator=(const FunctionProperty &right);

  /// Bring in the PropertyWithValue assignment operator explicitly (avoids
  /// VSC++ warning)
  FunctionProperty &
  operator=(const boost::shared_ptr<IFunction> &value) override;

  /// Add the value of another property
  FunctionProperty &operator+=(Kernel::Property const *) override;

  /// 'Virtual copy constructor'
  FunctionProperty *clone() const override {
    return new FunctionProperty(*this);
  }

  /// Get the function definition string
  std::string value() const override;

  /// Return a Json::Value object encoding the function string
  Json::Value valueAsJson() const override;

  /// Get the value the property was initialised with -its default value
  std::string getDefault() const override;

  /// Set the value of the property.
  std::string setValue(const std::string &value) override;

  /// Set the value of the property as a Json representation
  std::string setValueFromJson(const Json::Value &value) override;

  /// Checks whether the entered function is valid.
  std::string isValid() const override;

  /// Is default
  bool isDefault() const override;

  /// Create a history record
  const Kernel::PropertyHistory createHistory() const override;

private:
  /// The function definition string (as used by the FunctionFactory)
  std::string m_definition;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_FUNCTIONPROPERTY_H_*/
