#ifndef MANTID_DATAOBJECTS_AFFINE_MATRIX_PARAMETER_PARSER
#define MANTID_DATAOBJECTS_AFFINE_MATRIX_PARAMETER_PARSER

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
class DLLExport AffineMatrixParameterParser
    : public Mantid::API::ImplicitFunctionParameterParser {
public:
  /// Constructor
  AffineMatrixParameterParser();
  /** Creates the parameter by reading the xml given.
   * @param parameterElement : xml element to parser from.
   * @return Fully constructed AffineMatrixParameter.
   */
  AffineMatrixParameter *
  createParameter(Poco::XML::Element *parameterElement) override;
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

#endif
