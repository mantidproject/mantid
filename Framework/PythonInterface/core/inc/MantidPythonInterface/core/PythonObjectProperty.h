// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidPythonInterface/core/DllConfig.h"

#include "MantidKernel/IValidator.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/PropertyWithValue.h"

#include <boost/python/object.hpp>

// forward declare
namespace Json {
class Value;
}

namespace Mantid::Kernel {
using PythonObject = boost::python::object;

#if !defined(__APPLE__) && !(defined(__linux__) && defined(__clang__))
/** NOTE:
 *  For Linux builds (and maybe Windows), it is necessary that the below DLL export occur here.
 *  This declaration normally lives in Framework/Kernel/PropertyWithValue.cpp.  However, because the boost library is
 *  not linked in Kernel, this declaration can only occur inside the PythonInterface layer
 *  For Linux builds, this MUST be instantiated BEFORE the declaration of PythonObjectProperty
 *  Exception: Linux with clang needs this in the source file instead
 */
// Instantiate a copy of the class with our template type so we generate the symbols for the methods in the hxx header.
template class MANTID_PYTHONINTERFACE_CORE_DLL PropertyWithValue<PythonObject>;
#endif
} // namespace Mantid::Kernel

namespace Mantid::PythonInterface {

using Mantid::Kernel::Direction;
using Mantid::Kernel::IValidator_sptr;
using Mantid::Kernel::NullValidator;
using Mantid::Kernel::PythonObject;

class MANTID_PYTHONINTERFACE_CORE_DLL PythonObjectProperty : public Mantid::Kernel::PropertyWithValue<PythonObject> {
public:
  // Convenience typedefs
  using ValueType = PythonObject;
  using BaseClass = Mantid::Kernel::PropertyWithValue<ValueType>;

  /** No default constructor */
  PythonObjectProperty() = delete;

  /** Constructor
   *  @param name ::         The name to assign to the property
   *  @param defaultValue :: The default python object to assign to the property
   *  @param validator ::    The validator to use for this property, if required.
   *  @param direction ::    The direction (Input/Output/InOut) of this property
   */
  PythonObjectProperty(std::string const &name, PythonObject const &defaultValue,
                       IValidator_sptr const &validator = std::make_shared<NullValidator>(),
                       unsigned int const direction = Direction::Input)
      : BaseClass(name, defaultValue, validator, direction) {}

  /** Constructor that's useful for output properties or inputs with non-None default and no validator.
   *  @param name ::         The name to assign to the property
   *  @param defaultValue :: The default python object to assign to the property
   *  @param direction ::    The direction (Input/Output/InOut) of this property
   */
  PythonObjectProperty(std::string const &name, PythonObject const &defaultValue,
                       unsigned int const direction = Direction::Input)
      : BaseClass(name, defaultValue, std::make_shared<NullValidator>(), direction) {}

  /** Constructor
   *  Will lead to the property having default value of None
   *  @param name ::      The name to assign to the property
   *  @param validator :: The validator to use for this property, if required
   *  @param direction :: The direction (Input/Output/InOut) of this property
   */
  PythonObjectProperty(std::string const &name, IValidator_sptr const &validator,
                       unsigned int const direction = Direction::Input)
      : BaseClass(name, PythonObject(), validator, direction) {}

  /** Constructor that's useful for output properties or inputs with default value None and no validator.
   *  Will lead to the property having a default initial value of None and no validator
   *  @param name ::      The name to assign to the property
   *  @param direction :: The direction (Input/Output/InOut) of this property
   */
  PythonObjectProperty(std::string const &name, unsigned int const direction = Direction::Input)
      : BaseClass(name, PythonObject(), std::make_shared<NullValidator>(), direction) {}

  /** Constructor from which you can set the property's values through a string:
   *  @param name ::      The name to assign to the property
   *  @param strvalue ::     A string which will set the property being stored
   *  @param validator :: The validator to use for this property, if required
   *  @param direction :: The direction (Input/Output/InOut) of this property
   *  @throw std::invalid_argument if the string passed is not compatible with a python object
   */
  PythonObjectProperty(std::string const &name, std::string const &strvalue,
                       IValidator_sptr const &validator = std::make_shared<NullValidator>(),
                       unsigned int const direction = Direction::Input)
      : BaseClass(name, PythonObject(), strvalue, validator, direction) {}

  /** Copy constructor */
  PythonObjectProperty(PythonObjectProperty const &other) : BaseClass(other) {}

  /** This is required by Property interface */
  PythonObjectProperty *clone() const override { return new PythonObjectProperty(*this); }

  // Unhide the base class assignment operator
  using BaseClass::operator=;

  std::string getDefault() const override;
  std::string setValue(PythonObject const &obj);
  std::string setValue(std::string const &value) override;
  std::string setValueFromJson(Json::Value const &value) override;
  std::string setDataItem(std::shared_ptr<Kernel::DataItem> const &value) override;
  bool isDefault() const override;
};

} // namespace Mantid::PythonInterface
