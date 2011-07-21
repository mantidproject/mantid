#ifndef MANTID_MDEVENTS_AFFINE_MATRIX_PARAMETER_PARSER
#define MANTID_MDEVENTS_AFFINE_MATRIX_PARAMETER_PARSER

#include "MantidKernel/System.h"
#include "MantidAPI/ImplicitFunctionParameterParser.h"
#include "MantidMDEvents/AffineMatrixParameter.h"

namespace Mantid
{
  namespace MDEvents
  {
    /** Parser for a parameter of type affinematrixparameter
    *
    * @author Owen Arnold
    * @date 20/07/2011
    */
    class DLLExport AffineMatrixParameterParser : public Mantid::API::ImplicitFunctionParameterParser
    {
    public:

      AffineMatrixParameterParser();
      virtual AffineMatrixParameter* createParameter(Poco::XML::Element* parameterElement);
      virtual void setSuccessorParser(ImplicitFunctionParameterParser*);
      virtual ~AffineMatrixParameterParser();

    private:

      AffineMatrixParameterParser& operator=(const AffineMatrixParameterParser&);
      AffineMatrixParameterParser(const AffineMatrixParameterParser&);
    };
  }
}

#endif