#ifndef MD_ALGORITHMS_MD_PARAMETER_PARSER_DECLARATIONS_H
#define MD_ALGORITHMS_MD_PARAMETER_PARSER_DECLARATIONS_H
#include "MantidAPI/SingleValueParameterParser.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    /// Typedef for Width Parameter Parser.
    typedef SingleValueParameterParser<WidthParameter> WidthParameterParser;
    
    /// Typedef for Height Parameter Parser.
    typedef SingleValueParameterParser<HeightParameter> HeightParameterParser;
    
    /// Typedef for Depth Parameter Parser.
    typedef SingleValueParameterParser<DepthParameter> DepthParameterParser;
  }
}

#endif