// Mantid Repository : https: // github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
// NScD Oak Ridge National Laboratory, European Spallation Source,
// Institut Laue - Langevin &CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier : GPL - 3.0 +
#include "MantidCurveFitting/Functions/Meier.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>
#include <valarray>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(Meier)

namespace {
bool isMultipleOf05(double value) {
  double temp = value * 2;
  return floor(temp) == ceil(temp);
}

double getSinSquared(const double &cosSquared) { return 1 - cosSquared; }

double getCosSquared(const double &cos2squared) { return 0.5 * (1 + std::sqrt(cos2squared)); }
} // namespace

void Meier::init() {
  declareParameter("A0", 0.5, "Amplitude");
  declareParameter("FreqD", 0.01, "Angular Frequency due to dipolar coupling (MHz)");
  declareParameter("FreqQ", 0.05,
                   "Angular Frequency due to quadrupole interaction of the nuclear spin (MHz) due to a field gradient"
                   "exerted by the presence of the muon");
  declareParameter("Sigma", 0.2, "Gaussian decay rate");
  declareParameter("Lambda", 0.1, "Exponential decay rate");
  // J, Total angular momentum quanutm number
  declareAttribute("Spin", API::IFunction::Attribute(3.5));
}

void Meier::function1D(double *out, const double *xValues, const size_t nData) const {
  const double J = getAttribute("Spin").asDouble();
  if (!isMultipleOf05(J)) {
    throw std::invalid_argument("Spin value is not a multiple of 0.5");
  }

  const double A0 = getParameter("A0");
  const double J2 = round(2 * J);
  const double Lambda = getParameter("Lambda");
  const double Sigma = getParameter("Sigma");

  std::valarray<double> xValArray(xValues, nData);

  const std::valarray<double> gau = std::exp(-0.5 * pow((Sigma * xValArray), 2));
  const std::valarray<double> Lor = std::exp(-Lambda * xValArray);

  std::valarray<double> positiveLambda;
  std::valarray<double> negativeLambda;
  std::valarray<double> cos2AlphaSquared;

  precomputeIntermediateSteps(cos2AlphaSquared, negativeLambda, positiveLambda, J2);

  std::valarray<double> Px;
  calculatePx(Px, xValArray, cos2AlphaSquared, negativeLambda, positiveLambda, J2);

  std::valarray<double> Pz;
  calculatePz(Pz, xValArray, cos2AlphaSquared, negativeLambda, positiveLambda, J2);

  const std::valarray<double> outValArray = A0 * gau * Lor * (1. / 3.) * (2 * Px + Pz);
  std::copy(begin(outValArray), end(outValArray), out);
}

/**
 * Precomputes intermediate terms used to calculate the polrization in the x and z directions. All value arrays will be
 * resized to J2 + 2 and set to their respective quantities
 * @param cos2AlphaSquared :: cos of 2*alpha squared (output parameter)
 * @param positiveLambda :: negative lambda (output parameter)
 * @param negativeLambda :: positive lambda (output parameter)
 * @param J2 :: 2 * total angular momentum quantum number (input parameter)
 */
void Meier::precomputeIntermediateSteps(std::valarray<double> &cos2AlphaSquared, std::valarray<double> &negativeLambda,
                                        std::valarray<double> &positiveLambda, const double &J2) const {
  const double FreqD = getParameter("FreqD");
  const double FreqQ = getParameter("FreqQ");
  const double J = J2 / 2;

  const double OmegaD = 2 * M_PI * FreqD;
  const double OmegaQ = 2 * M_PI * FreqQ;

  const size_t size = int(J2 + 2);
  cos2AlphaSquared.resize(size);
  negativeLambda.resize(size);
  positiveLambda.resize(size);

  double m = -J;
  double q1 = getQ1(m, OmegaQ, OmegaD);
  double q2 = getQ2(m, J, OmegaD);
  double q3 = getQ3(m, OmegaQ, OmegaD);
  double qq = getQQ(q1, q2);
  double Wm = std::sqrt(qq);

  positiveLambda[0] = getPositiveLambda(q3, Wm);
  negativeLambda[0] = getBaseLambda(OmegaQ, OmegaD, J);
  cos2AlphaSquared[0] = getCos2AlphaSquared(q1, qq);

  for (size_t i = 1; i < size - 1; i++) {
    m = static_cast<double>(i) - J;
    q1 = getQ1(m, OmegaQ, OmegaD);
    q2 = getQ2(m, J, OmegaD);
    q3 = getQ3(m, OmegaQ, OmegaD);
    qq = getQQ(q1, q2);
    Wm = std::sqrt(qq);

    positiveLambda[i] = getPositiveLambda(q3, Wm);
    negativeLambda[i] = getNegativeLambda(q3, Wm);
    cos2AlphaSquared[i] = getCos2AlphaSquared(q1, qq);
  }

  m = size - 1 - J;
  q1 = getQ1(m, OmegaQ, OmegaD);
  q2 = getQ2(m, J, OmegaD);
  q3 = getQ3(m, OmegaQ, OmegaD);
  qq = getQQ(q1, q2);
  Wm = std::sqrt(qq);

  positiveLambda[size - 1] = getBaseLambda(OmegaQ, OmegaD, J);
  negativeLambda[size - 1] = getNegativeLambda(q3, Wm);
  cos2AlphaSquared[size - 1] = getCos2AlphaSquared(q1, qq);
}

/**
 * Calculates and returns the value of q1
 * This function is used by **precomputeIntermediateSteps** to calculate intermediate steps
 * @param m :: Current Index
 * @param OmegaQ :: Angular Frequency due to dipolar coupling (MHz)
 * @param OmegaD :: Angular Frequency due to quadrupole interaction of the nuclear spin (MHz) due to a field gradient
 * @return :: the value of q1
 */
double Meier::getQ1(const double &m, const double &OmegaQ, const double &OmegaD) const {
  return (OmegaQ + OmegaD) * (2 * m - 1);
}

/**
 * Calculates and returns the value of q2
 * This function is used by **precomputeIntermediateSteps** to calculate intermediate steps
 * @param m :: Current Index
 * @param J :: Total angular momentum quanutm number
 * @param OmegaD :: Angular Frequency due to quadrupole interaction of the nuclear spin (MHz) due to a field gradient
 * @return :: the value of q2
 */
double Meier::getQ2(const double &m, const double &J, const double &OmegaD) const {
  return OmegaD * std::sqrt(J * (J + 1) - m * (m - 1));
}

/**
 * Calculates and returns the value of q2
 * This function is used by **precomputeIntermediateSteps** to calculate intermediate steps
 * @param m :: Current Index
 * @param OmegaQ :: Angular Frequency due to dipolar coupling (MHz)
 * @param OmegaD :: Angular Frequency due to quadrupole interaction of the nuclear spin (MHz) due to a field gradient
 * @return :: the value of q3
 */
double Meier::getQ3(const double &m, const double &OmegaQ, const double &OmegaD) const {
  return OmegaQ * (2 * pow(m, 2) - 2 * m + 1) + OmegaD;
}

/**
 * Calculates and returns the value of qq
 * This function is used by **precomputeIntermediateSteps** to calculate intermediate steps
 * @param q1 :: the value of q1
 * @param q2 :: the value of q2
 * @return :: the value of qq
 */
double Meier::getQQ(const double &q1, const double &q2) const { return pow(q1, 2) + pow(q2, 2); }

/**
 * Calculates and returns the value of positive lambda
 * This function is used by **precomputeIntermediateSteps** to calculate intermediate steps
 * @param q3 :: The value of q3
 * @param Wm :: The value of Wm
 * @return :: the value of positive lambda
 */
double Meier::getPositiveLambda(const double &q3, const double &Wm) const { return 0.5 * (q3 + Wm); }

/**
 * Calculates and returns the value of negative lambda
 * This function is used by **precomputeIntermediateSteps** to calculate intermediate steps
 * @param q3 :: The value of q3
 * @param Wm :: The value of Wm
 * @return :: the value of negative lambda
 */
double Meier::getNegativeLambda(const double &q3, const double &Wm) const { return 0.5 * (q3 - Wm); }

/**
 * Calculates and returns the value of lambda for the special cases
 * i.e it is the value of the first element of the negative lambda and the value of the last element of the negative
 * This function is used by **precomputeIntermediateSteps** to calculate intermediate steps
 * @param OmegaQ :: Angular Frequency due to dipolar coupling (MHz)
 * @param OmegaD :: Angular Frequency due to quadrupole interaction of the nuclear spin (MHz) due to a field gradient
 * @param J :: Total angular momentum quanutm number
 * @return :: the value of base lambda
 */
double Meier::getBaseLambda(const double &OmegaQ, const double &OmegaD, const double &J) const {
  return OmegaQ * pow(J, 2) - OmegaD * J;
}

/**
 * Calculates and returns the value of cos 2*alpha sequared at a given index i
 * This function is used by **precomputeIntermediateSteps** to calculate intermediate steps
 * @param q1 :: the value of q1
 * @param qq :: the value of qq
 * @return :: the value of cos 2 * alpha squared
 */
double Meier::getCos2AlphaSquared(const double &q1, const double &qq) const { return qq > 0 ? pow(q1, 2) / qq : 0; }

/**
 * Calculates the polarization in the x direction
 * @param Px :: the polarization in the x direction (output parameter)
 * @param xValues :: input x values (input parameter)
 * @param cos2AlphaSquared :: cos of 2*alpha squared (input parameter)
 * @param positiveLambda :: negative lambda (input parameter)
 * @param negativeLambda :: positive lambda (input parameter)
 * @param J2 :: 2 * total angular momentum quantum number multiplied by 2 (input parameter)
 */
void Meier::calculatePx(std::valarray<double> &Px, const std::valarray<double> &xValues,
                        const std::valarray<double> &cos2AlphaSquared, const std::valarray<double> &negativeLambda,
                        const std::valarray<double> &positiveLambda, const double &J2) const {
  std::valarray<double> tx(xValues.size());
  for (int i = 0; i < int(J2) + 1; i++) {
    const double cosAlphaSquared = getCosSquared(cos2AlphaSquared[i]);
    const double sinAlphaSquared = getSinSquared(cosAlphaSquared);

    const double cosAlphaSquared2 = getCosSquared(cos2AlphaSquared[i + 1]);
    const double sinAlphaSquared2 = getSinSquared(cosAlphaSquared2);

    const std::valarray<double> a =
        cosAlphaSquared2 * sinAlphaSquared * std::cos((positiveLambda[i + 1] - positiveLambda[i]) * xValues);
    const std::valarray<double> b =
        cosAlphaSquared2 * cosAlphaSquared * std::cos((positiveLambda[i + 1] - negativeLambda[i]) * xValues);
    const std::valarray<double> c =
        sinAlphaSquared2 * sinAlphaSquared * std::cos((negativeLambda[i + 1] - positiveLambda[i]) * xValues);
    const std::valarray<double> d =
        sinAlphaSquared2 * cosAlphaSquared * std::cos((negativeLambda[i + 1] - negativeLambda[i]) * xValues);
    tx += a + b + c + d;
  }
  const double J = J2 / 2;
  Px = tx / (2 * J + 1);
}

/**
 * Calculates the polarization in the z direction
 * @param Pz :: the polarization in the x direction (output parameter)
 * @param xValues :: input x values (input parameter)
 * @param cos2AlphaSquared :: cos of 2*alpha squared (input parameter)
 * @param positiveLambda :: negative lambda (input parameter)
 * @param negativeLambda :: positive lambda (input parameter)
 * @param J2 :: 2 * total angular momentum quantum number multiplied by 2 (input parameter)
 */
void Meier::calculatePz(std::valarray<double> &Pz, const std::valarray<double> &xValues,
                        const std::valarray<double> &cos2AlphaSquared, const std::valarray<double> &negativeLambda,
                        const std::valarray<double> &positiveLambda, const double &J2) const {
  std::valarray<double> tz(xValues.size());
  for (size_t i = 1; i < (size_t)J2 + 1; i++) {
    const double sin2AlphaSquared = getSinSquared(cos2AlphaSquared[i]);
    tz += cos2AlphaSquared[i] + sin2AlphaSquared * std::cos((positiveLambda[i] - negativeLambda[i]) * xValues);
  }
  const double J = J2 / 2;
  Pz = (1 + tz) / (2 * J + 1);
}
} // namespace Mantid::CurveFitting::Functions
