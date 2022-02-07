// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/ImplicitFunctionParameterParser.h"
#include "MantidDataObjects/AffineMatrixParameter.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataObjects {
/** Parser for a parameter of type affinematrixparameter
 *
 * @author Owen Arnold
 * @date 20/07/2011
 */
class DLLExport AffineMatrixParameterParser : public Mantid::API::ImplicitFunctionParameterParser {
public:
  /// Constructor
  AffineMatrixParameterParser();
  /** Creates the parameter by reading the xml given.
   * @param parameterElement : xml element to parser from.
   * @return Fully constructed AffineMatrixParameter.
   */
  AffineMatrixParameter *createParameter(Poco::XML::Element *parameterElement) override;
  /// Set a successor parser for chain-of-responsibility type reading.
  void setSuccessorParser(ImplicitFunctionParameterParser *) override;

private:
  /// Assignment operator
  AffineMatrixParameterParser &operator=(const AffineMatrixParameterParser &);
  /// Copy constructor
  AffineMatrixParameterParser(const AffineMatrixParameterParser &);
};
} // namespace DataObjects
} // namespace Mantid
