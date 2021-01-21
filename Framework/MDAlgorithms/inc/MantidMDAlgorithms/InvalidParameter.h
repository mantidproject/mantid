// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ImplicitFunctionParameter.h"
#include "MantidKernel/System.h"
#include <vector>

namespace Mantid {
namespace MDAlgorithms {
/**  Invalid parameter type. Modelled from Null object pattern.

@author Owen Arnold, Tessella plc
@date 01/10/2010
*/

class DLLExport InvalidParameter : public Mantid::API::ImplicitFunctionParameter {
private:
  std::string m_value;

public:
  InvalidParameter();

  InvalidParameter(std::string value);

  std::string getName() const override;

  std::string getValue() const;

  bool isValid() const override;

  Mantid::MDAlgorithms::InvalidParameter *clone() const override;

  std::string toXMLString() const override;

  static std::string parameterName() { return "InvalidParameter"; }
};
} // namespace MDAlgorithms
} // namespace Mantid
