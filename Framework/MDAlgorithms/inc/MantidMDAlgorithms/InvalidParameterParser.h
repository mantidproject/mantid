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
#include <vector>

#include "MantidAPI/ImplicitFunctionParameterParser.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/InvalidParameter.h"

namespace Mantid {
namespace MDAlgorithms {
/** XML Parser for invalid parameter types

@author Owen Arnold, Tessella plc
@date 01/10/2010
*/

class DLLExport InvalidParameterParser : public Mantid::API::ImplicitFunctionParameterParser {
public:
  InvalidParameterParser();
  Mantid::API::ImplicitFunctionParameter *createParameter(Poco::XML::Element *parameterElement) override;
  void setSuccessorParser(Mantid::API::ImplicitFunctionParameterParser *parser) override;

protected:
  ImplicitFunctionParameterParser::SuccessorType m_successor;
  InvalidParameter *parseInvalidParameter(std::string value);
};
} // namespace MDAlgorithms
} // namespace Mantid
