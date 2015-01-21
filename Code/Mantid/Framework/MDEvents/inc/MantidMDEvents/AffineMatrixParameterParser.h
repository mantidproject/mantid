#ifndef MANTID_MDEVENTS_AFFINE_MATRIX_PARAMETER_PARSER
#define MANTID_MDEVENTS_AFFINE_MATRIX_PARAMETER_PARSER

#include "MantidKernel/System.h"
#include "MantidAPI/ImplicitFunctionParameterParser.h"
#include "MantidMDEvents/AffineMatrixParameter.h"

namespace Mantid {
namespace MDEvents {
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
  virtual AffineMatrixParameter *
  createParameter(Poco::XML::Element *parameterElement);
  /// Set a successor parser for chain-of-responsibility type reading.
  virtual void setSuccessorParser(ImplicitFunctionParameterParser *);
  /// Destructor
  virtual ~AffineMatrixParameterParser();

private:
  /// Assignment operator
  AffineMatrixParameterParser &operator=(const AffineMatrixParameterParser &);
  /// Copy constructor
  AffineMatrixParameterParser(const AffineMatrixParameterParser &);
};
}
}

#endif
