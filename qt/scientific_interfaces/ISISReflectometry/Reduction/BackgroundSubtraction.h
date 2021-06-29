// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_BACKGROUNDSUBTRACTION_H_
#define MANTID_CUSTOMINTERFACES_BACKGROUNDSUBTRACTION_H_
#include "Common/DllConfig.h"
#include <boost/optional.hpp>
#include <stdexcept>
#include <string>
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
enum class BackgroundSubtractionType { PerDetectorAverage, Polynomial, AveragePixelFit };
enum class CostFunctionType { LeastSquares, UnweightedLeastSquares };

inline BackgroundSubtractionType backgroundSubtractionTypeFromString(std::string const &subtractionType) {
  if (subtractionType.empty() || subtractionType == "PerDetectorAverage")
    return BackgroundSubtractionType::PerDetectorAverage;
  else if (subtractionType == "Polynomial")
    return BackgroundSubtractionType::Polynomial;
  else if (subtractionType == "AveragePixelFit")
    return BackgroundSubtractionType::AveragePixelFit;
  else
    throw std::invalid_argument("Unexpected background subtraction type.");
}

inline std::string backgroundSubtractionTypeToString(BackgroundSubtractionType subtractionType) {
  switch (subtractionType) {
  case BackgroundSubtractionType::PerDetectorAverage:
    return "PerDetectorAverage";
  case BackgroundSubtractionType::Polynomial:
    return "Polynomial";
  case BackgroundSubtractionType::AveragePixelFit:
    return "AveragePixelFit";
  }
  throw std::invalid_argument("Unexpected background subtraction type.");
}

inline CostFunctionType costFunctionTypeFromString(std::string const &costFunction) {
  if (costFunction.empty() || costFunction == "Least squares")
    return CostFunctionType::LeastSquares;
  else if (costFunction == "Unweighted least squares")
    return CostFunctionType::UnweightedLeastSquares;
  else
    throw std::invalid_argument("Unexpected cost function type");
}

inline std::string costFunctionTypeToString(CostFunctionType costFunctionType) {
  switch (costFunctionType) {
  case CostFunctionType::LeastSquares:
    return "Least squares";
  case CostFunctionType::UnweightedLeastSquares:
    return "Unweighted least squares";
  };
  throw std::invalid_argument("Unexpected cost function type.");
}

/** @class BackgroundSubtraction

    The BackgroundSubtraction model holds information about what
    background subtraction, if any, should be done prior to reduction.
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL BackgroundSubtraction {
public:
  BackgroundSubtraction();
  BackgroundSubtraction(bool subtractBackground, BackgroundSubtractionType subtractionType, int degreeOfPolynomial,
                        CostFunctionType costFunction);

  bool subtractBackground() const;
  BackgroundSubtractionType subtractionType() const;
  int degreeOfPolynomial() const;
  CostFunctionType costFunction() const;

private:
  bool m_subtractBackground;
  BackgroundSubtractionType m_subtractionType;
  int m_degreeOfPolynomial;
  CostFunctionType m_costFunction;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(BackgroundSubtraction const &lhs, BackgroundSubtraction const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(BackgroundSubtraction const &lhs, BackgroundSubtraction const &rhs);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_BACKGROUNDSUBTRACTION_H_
