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
#include "MantidKernel/PropertyWithValue.h"

#include <boost/python/object.hpp>

namespace Mantid::PythonInterface {

class MANTID_PYTHONINTERFACE_CORE_DLL PythonObjectProperty
    : public Mantid::Kernel::PropertyWithValue<boost::python::object> {
public:
  // Convenience typedefs
  using ValueType = boost::python::object;
  using BaseClass = Mantid::Kernel::PropertyWithValue<ValueType>;

  PythonObjectProperty(const std::string &name, const unsigned int direction);
  PythonObjectProperty(const std::string &name, const boost::python::object &defaultValue,
                       const unsigned int direction);
  PythonObjectProperty(const std::string &name, const boost::python::object &defaultValue,
                       Mantid::Kernel::IValidator_sptr validator, const unsigned int direction);
  PythonObjectProperty(const PythonObjectProperty &other) = default;
  PythonObjectProperty &operator=(const PythonObjectProperty &right) = default;
  PythonObjectProperty *clone() const override { return new PythonObjectProperty(*this); }

  std::string getDefault() const override;
  std::string setValue(const std::string &value) override;
  std::string setValueFromJson(const Json::Value &value) override;
  bool isDefault() const override;
};

} // namespace Mantid::PythonInterface
