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

void Meier::init() {
  declareParameter("A0", 0.5, "Amplitude");
  declareParameter("FreqD", 0.01, "Angular Frequency due to dipolar coupling (MHz)");
  declareParameter("FreqQ", 0.05,
                   "Angular Frequency due to quadrupole interaction of the nuclear spin (MHz) due to a field gradient "
                   "exerted by the presence of the muon");
  declareParameter("Sigma", 0.2, "Gaussian decay rate");
  declareParameter("Lambda", 0.1, "Exponential decay rate");
  // J, Total angular momentum quanutm number
  declareAttribute("Spin", API::IFunction::Attribute(3.5));
}

void Meier::function1D(double *out, const double *xValues, const size_t nData) const {
  const double A0 = getParameter("A0");
  const double J2 = round(getAttribute("Spin").asDouble() * 2);
  const double Lambda = getParameter("Lambda");
  const double Sigma = getParameter("Sigma");

  std::valarray<double> xValArray(xValues, nData);

  const std::valarray<double> gau = std::exp(-0.5 * pow((Sigma * xValArray), 2));
  const std::valarray<double> Lor = std::exp(-Lambda * xValArray);

  std::valarray<double> positiveLambda;
  std::valarray<double> negativeLambda;
  std::valarray<double> cos2AlphaSquared;
  std::valarray<double> sin2AlphaSquared;
  std::valarray<double> cosAlphaSquared;
  std::valarray<double> sinAlphaSquared;

  precomputeIntermediateSteps(sinAlphaSquared, cosAlphaSquared, sin2AlphaSquared, cos2AlphaSquared, negativeLambda,
                              positiveLambda, J2);

  std::valarray<double> Px;
  calculatePx(Px, xValArray, sinAlphaSquared, cosAlphaSquared, negativeLambda, positiveLambda, J2);

  std::valarray<double> Pz;
  calculatePz(Pz, xValArray, sin2AlphaSquared, cos2AlphaSquared, negativeLambda, positiveLambda, J2);

  const std::valarray<double> outValArray = A0 * gau * Lor * (1. / 3.) * (2 * Px + Pz);
  std::copy(begin(outValArray), end(outValArray), out);
}

/**
 * Precomputes intermediate terms used to calculate the polrization in the x and z directions. All value arrays will be
 * resized to J2 + 2 and set to their respective quantities
 * @param sinAlphaSquared :: sin of alpha squared
 * @param cosAlphaSquared :: cos of alpha squared
 * @param sin2AlphaSquared :: sin of 2*alpha squared
 * @param cos2AlphaSquared :: cos of 2*alpha squared
 * @param positiveLambda :: negative lambda
 * @param negativeLambda :: positive lambda
 * @param J2 :: 2 * total angular momentum quantum number
 */
void Meier::precomputeIntermediateSteps(std::valarray<double> &sinAlphaSquared, std::valarray<double> &cosAlphaSquared,
                                        std::valarray<double> &sin2AlphaSquared,
                                        std::valarray<double> &cos2AlphaSquared, std::valarray<double> &negativeLambda,
                                        std::valarray<double> &positiveLambda, const double &J2) const {
  const double FreqD = getParameter("FreqD");
  const double FreqQ = getParameter("FreqQ");
  const double J = J2 / 2;

  const double OmegaD = 2 * M_PI * FreqD;
  const double OmegaQ = 2 * M_PI * FreqQ;

  const size_t size = int(J2 + 2);
  sinAlphaSquared.resize(size);
  cosAlphaSquared.resize(size);
  sin2AlphaSquared.resize(size);
  cos2AlphaSquared.resize(size);
  negativeLambda.resize(size);
  positiveLambda.resize(size);
  std::valarray<double> Wm(size);

  for (size_t i = 0; i < size; i++) {
    const double m = static_cast<double>(i) - J;
    const double q1 = (OmegaQ + OmegaD) * (2 * m - 1);
    const double q2 = OmegaD * std::sqrt(J * (J + 1) - m * (m - 1));
    const double qq = pow(q1, 2) + pow(q2, 2);
    const double q3 = OmegaQ * (2 * pow(m, 2) - 2 * m + 1) + OmegaD;

    Wm[i] = std::sqrt(qq);

    if (static_cast<double>(i) < (J2 + 1)) {
      positiveLambda[i] = 0.5 * (q3 + Wm[i]);
    } else {
      positiveLambda[i] = OmegaQ * pow(J, 2) - OmegaD * J;
    }

    if (i > 0) {
      negativeLambda[i] = 0.5 * (q3 - Wm[i]);
    } else {
      negativeLambda[i] = OmegaQ * pow(J, 2) - OmegaD * J;
    }

    if (qq > 0) {
      cos2AlphaSquared[i] = pow(q1, 2) / qq;
    } else {
      cos2AlphaSquared[i] = 0;
    }
    sin2AlphaSquared[i] = 1 - cos2AlphaSquared[i];

    cosAlphaSquared[i] = 0.5 * (1 + std::sqrt(cos2AlphaSquared[i]));
    sinAlphaSquared[i] = 1 - cosAlphaSquared[i];
  }
}

/**
 * Calculates the polarization in the x direction
 * @param Px :: the polarization in the x direction
 * @param xValues :: input x values
 * @param sinAlphaSquared :: sin of alpha squared
 * @param cosAlphaSquared :: cos of alpha squared
 * @param sin2AlphaSquared :: sin of 2*alpha squared
 * @param cos2AlphaSquared :: cos of 2*alpha squared
 * @param positiveLambda :: negative lambda
 * @param negativeLambda :: positive lambda
 * @param J2 :: 2 * total angular momentum quantum number multiplied by 2
 */
void Meier::calculatePx(std::valarray<double> &Px, const std::valarray<double> &xValues,
                        const std::valarray<double> &sinAlphaSquared, const std::valarray<double> &cosAlphaSquared,
                        const std::valarray<double> &negativeLambda, const std::valarray<double> &positiveLambda,
                        const double &J2) const {
  std::valarray<double> tx(xValues.size());
  for (int i = 0; i < int(J2) + 1; i++) {
    const std::valarray<double> a =
        cosAlphaSquared[i + 1] * sinAlphaSquared[i] * std::cos((positiveLambda[i + 1] - positiveLambda[i]) * xValues);
    const std::valarray<double> b =
        cosAlphaSquared[i + 1] * cosAlphaSquared[i] * std::cos((positiveLambda[i + 1] - negativeLambda[i]) * xValues);
    const std::valarray<double> c =
        sinAlphaSquared[i + 1] * sinAlphaSquared[i] * std::cos((negativeLambda[i + 1] - positiveLambda[i]) * xValues);
    const std::valarray<double> d =
        sinAlphaSquared[i + 1] * cosAlphaSquared[i] * std::cos((negativeLambda[i + 1] - negativeLambda[i]) * xValues);
    tx = tx + a + b + c + d;
  }
  const double J = J2 / 2;
  Px = tx / (2 * J + 1);
}

/**
 * Calculates the polarization in the z direction
 * @param Pz :: the polarization in the x direction
 * @param xValues :: input x values
 * @param sinAlphaSquared :: sin of alpha squared
 * @param cosAlphaSquared :: cos of alpha squared
 * @param sin2AlphaSquared :: sin of 2*alpha squared
 * @param cos2AlphaSquared :: cos of 2*alpha squared
 * @param positiveLambda :: negative lambda
 * @param negativeLambda :: positive lambda
 * @param J2 :: 2 * total angular momentum quantum number multiplied by 2
 */
void Meier::calculatePz(std::valarray<double> &Pz, const std::valarray<double> &xValues,
                        const std::valarray<double> &sin2AlphaSquared, const std::valarray<double> &cos2AlphaSquared,
                        const std::valarray<double> &negativeLambda, const std::valarray<double> &positiveLambda,
                        const double &J2) const {
  std::valarray<double> tz(xValues.size());
  for (size_t i = 1; i < (size_t)J2 + 1; i++) {
    tz = tz + cos2AlphaSquared[i] + sin2AlphaSquared[i] * std::cos((positiveLambda[i] - negativeLambda[i]) * xValues);
  }
  const double J = J2 / 2;
  Pz = (1 + tz) / (2 * J + 1);
}
} // namespace Mantid::CurveFitting::Functions
