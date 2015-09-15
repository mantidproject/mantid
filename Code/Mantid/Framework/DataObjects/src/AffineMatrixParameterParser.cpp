#include "MantidDataObjects/AffineMatrixParameterParser.h"
#include <boost/algorithm/string.hpp>

namespace Mantid {
namespace DataObjects {

AffineMatrixParameterParser::AffineMatrixParameterParser() {}

//----------------------------------------------------------------------------------------------

AffineMatrixParameter *AffineMatrixParameterParser::createParameter(
    Poco::XML::Element *parameterElement) {
  std::string typeName = parameterElement->getChildElement("Type")->innerText();
  if (AffineMatrixParameter::parameterName() != typeName) {
    throw std::runtime_error(std::string(
        "AffineMatrixParameterParser cannot parse parameter of type: " +
        typeName));
  } else {
    // Convenience typedefs
    typedef std::vector<std::string> VecStrings;
    typedef std::vector<coord_t> VecDoubles;

    std::string sParameterValue =
        parameterElement->getChildElement("Value")->innerText();

    VecStrings vecStrRows;
    VecStrings vecStrCols;

    boost::split(vecStrRows, sParameterValue, boost::is_any_of(";"));
    size_t nRows = vecStrRows.size();

    VecStrings::iterator row_it = vecStrRows.begin();
    VecStrings::iterator col_it;

    size_t nCols = 0;
    VecDoubles elements;

    // Process each row and column and extract each element as a double.
    while (row_it != vecStrRows.end()) {
      boost::split(vecStrCols, *row_it, boost::is_any_of(","));
      nCols = vecStrCols.size();
      col_it = vecStrCols.begin();
      while (col_it != vecStrCols.end()) {
        coord_t val = coord_t(atof(col_it->c_str()));
        elements.push_back(val);
        ++col_it;
      }
      ++row_it;
    }

    // Create the Matrix.
    AffineMatrixType m(nRows, nCols);
    size_t count = 0;
    for (size_t i = 0; i < nRows; i++) {
      for (size_t j = 0; j < nCols; j++) {
        m[i][j] = elements[count];
        count++;
      }
    }

    // Create the parameter and set the matrix.
    AffineMatrixParameter *parameter =
        new AffineMatrixParameter(nRows - 1, nCols - 1);
    parameter->setMatrix(m);

    // Now return the fully constructed parameter.
    return parameter;
  }
}

//----------------------------------------------------------------------------------------------

void AffineMatrixParameterParser::setSuccessorParser(
    Mantid::API::ImplicitFunctionParameterParser *) {
  throw std::runtime_error(
      "Cannot set a successor parser on a AffineMatrixParameterParser");
}

AffineMatrixParameterParser::~AffineMatrixParameterParser() {}
}
}
