#ifndef MD_ALGORITHMS_MD_PARAMETER_PARSER_DECLARATIONS_H
#define MD_ALGORITHMS_MD_PARAMETER_PARSER_DECLARATIONS_H

#include "MantidAPI/SingleValueParameterParser.h"
#include "MantidMDAlgorithms/DepthParameter.h"
#include "MantidMDAlgorithms/HeightParameter.h"
#include "MantidMDAlgorithms/WidthParameter.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    /// Typedef for Width Parameter Parser.
    typedef API::SingleValueParameterParser<WidthParameter> WidthParameterParser;
    
    /// Typedef for Height Parameter Parser.
    typedef API::SingleValueParameterParser<HeightParameter> HeightParameterParser;
    
    /// Typedef for Depth Parameter Parser.
    typedef API::SingleValueParameterParser<DepthParameter> DepthParameterParser;
  }
}

#endif