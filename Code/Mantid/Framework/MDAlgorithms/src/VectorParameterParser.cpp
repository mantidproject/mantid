#include "MantidMDAlgorithms/VectorParameterParser.h"
#include "MantidAPI/ImplicitFunctionParameterParserFactory.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    DECLARE_IMPLICIT_FUNCTION_PARAMETER_PARSER(NormalParameterParser)
    DECLARE_IMPLICIT_FUNCTION_PARAMETER_PARSER(OriginParameterParser)
    DECLARE_IMPLICIT_FUNCTION_PARAMETER_PARSER(UpParameterParser)
    DECLARE_IMPLICIT_FUNCTION_PARAMETER_PARSER(PerpendicularParameterParser)
  }
}
