// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "BackgroundSubtraction.h"
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

BackgroundSubtraction::BackgroundSubtraction()
    : m_subtractBackground(false), m_subtractionType(BackgroundSubtractionType::PerDetectorAverage),
      m_degreeOfPolynomial(0), m_costFunction(CostFunctionType::LeastSquares) {}

BackgroundSubtraction::BackgroundSubtraction(bool subtractBackground, BackgroundSubtractionType subtractionType,
                                             int degreeOfPolynomial, CostFunctionType costFunction)
    : m_subtractBackground(subtractBackground), m_subtractionType(subtractionType),
      m_degreeOfPolynomial(degreeOfPolynomial), m_costFunction(costFunction) {}

bool BackgroundSubtraction::subtractBackground() const { return m_subtractBackground; }

BackgroundSubtractionType BackgroundSubtraction::subtractionType() const { return m_subtractionType; }

int BackgroundSubtraction::degreeOfPolynomial() const { return m_degreeOfPolynomial; }

CostFunctionType BackgroundSubtraction::costFunction() const { return m_costFunction; }

bool operator!=(BackgroundSubtraction const &lhs, BackgroundSubtraction const &rhs) { return !(lhs == rhs); }

bool operator==(BackgroundSubtraction const &lhs, BackgroundSubtraction const &rhs) {
  return lhs.subtractBackground() == rhs.subtractBackground() && lhs.subtractionType() == rhs.subtractionType() &&
         lhs.degreeOfPolynomial() == rhs.degreeOfPolynomial() && lhs.costFunction() == rhs.costFunction();
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
