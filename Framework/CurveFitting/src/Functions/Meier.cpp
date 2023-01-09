#include "MantidCurveFitting/Functions/Meier.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>
#include <valarray>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(MeierV2)

void MeierV2::init() {
  declareParameter("A0", 0.5, "Amplitude");
  declareParameter("FreqD", 0.01, "Frequency due to dipolar coupling (MHz)");
  declareParameter("FreqQ", 0.05, "Frequency due to quadrupole interaction of the nuclear spin (MHz)");
  declareParameter("Sigma", 0.2, "Gaussian decay rate");
  declareParameter("Lambda", 0.1, "Exponential decay rate");
  declareAttribute("Spin", API::IFunction::Attribute(3.5));
}

void MeierV2::function1D(double *out, const double *xValues, const size_t nData) const {
  std::valarray<double> xValArray(xValues, nData);

  const double A0 = getParameter("A0");
  double J = getAttribute("Spin").asDouble();
  const double Lambda = getParameter("Lambda");
  const double sigma = getParameter("Sigma");

  const std::valarray<double> gau = std::exp(-0.5 * pow((sigma * xValArray), 2));
  const std::valarray<double> Lor = std::exp(-Lambda * xValArray);

  const double J2 = round(2 * J);
  J = J2 / 2;

  std::valarray<double> lamp;
  std::valarray<double> lamm;
  std::valarray<double> cosSQ2alpha;
  std::valarray<double> sinSQ2alpha;
  std::valarray<double> cosSQalpha;
  std::valarray<double> sinSQalpha;

  calculateAlphaArrays(sinSQalpha, cosSQalpha, sinSQ2alpha, cosSQ2alpha, lamm, lamp, J, J2);

  std::valarray<double> Px;
  calculatePx(Px, xValArray, sinSQalpha, cosSQalpha, lamm, lamp, J, J2);

  std::valarray<double> Pz;
  calculatePz(Pz, xValArray, sinSQ2alpha, cosSQ2alpha, lamm, lamp, J, J2);

  const std::valarray<double> outValArray = A0 * gau * Lor * (1. / 3.) * (2 * Px + Pz);
  std::copy(begin(outValArray), end(outValArray), out);
}

void MeierV2::calculateAlphaArrays(std::valarray<double> &sinSQalpha, std::valarray<double> &cosSQalpha,
                                   std::valarray<double> &sinSQ2alpha, std::valarray<double> &cosSQ2alpha,
                                   std::valarray<double> &lamm, std::valarray<double> &lamp, const double &J,
                                   const double &J2) const {
  const double FreqD = getParameter("FreqD");
  const double FreqQ = getParameter("FreqQ");

  const double OmegaD = 2 * M_PI * FreqD;
  const double OmegaQ = 2 * M_PI * FreqQ;

  const size_t size = int(J2 + 2);
  sinSQalpha.resize(size);
  cosSQalpha.resize(size);
  sinSQ2alpha.resize(size);
  cosSQ2alpha.resize(size);
  lamm.resize(size);
  lamp.resize(size);
  std::valarray<double> Wm(size);

  for (size_t i = 0; i < size; i++) {
    const double m = static_cast<double>(i) - J;
    const double q1 = (OmegaQ + OmegaD) * (2 * m - 1);
    const double q2 = OmegaD * std::sqrt(J * (J + 1) - m * (m - 1));
    const double qq = pow(q1, 2) + pow(q2, 2);
    const double q3 = OmegaQ * (2 * pow(m, 2) - 2 * m + 1) + OmegaD;

    Wm[i] = std::sqrt(qq);

    if (static_cast<double>(i) < (J2 + 1)) {
      lamp[i] = 0.5 * (q3 + Wm[i]);
    } else {
      lamp[i] = OmegaQ * pow(J, 2) - OmegaD * J;
    }

    if (i > 0) {
      lamm[i] = 0.5 * (q3 - Wm[i]);
    } else {
      lamm[i] = OmegaQ * pow(J, 2) - OmegaD * J;
    }

    if (qq > 0) {
      cosSQ2alpha[i] = pow(q1, 2) / qq;
    } else {
      cosSQ2alpha[i] = 0;
    }
    sinSQ2alpha[i] = 1 - cosSQ2alpha[i];

    cosSQalpha[i] = 0.5 * (1 + std::sqrt(cosSQ2alpha[i]));
    sinSQalpha[i] = 1 - cosSQalpha[i];
  }
}

void MeierV2::calculatePx(std::valarray<double> &Px, const std::valarray<double> &xValArray,
                          const std::valarray<double> &sinSQalpha, const std::valarray<double> &cosSQalpha,
                          const std::valarray<double> &lamm, const std::valarray<double> &lamp, const double &J,
                          const double &J2) const {
  std::valarray<double> tx(xValArray.size());
  for (int i = 0; i < int(J2) + 1; i++) {
    const std::valarray<double> a = cosSQalpha[i + 1] * sinSQalpha[i] * std::cos((lamp[i + 1] - lamp[i]) * xValArray);
    const std::valarray<double> b = cosSQalpha[i + 1] * cosSQalpha[i] * std::cos((lamp[i + 1] - lamm[i]) * xValArray);
    const std::valarray<double> c = sinSQalpha[i + 1] * sinSQalpha[i] * std::cos((lamm[i + 1] - lamp[i]) * xValArray);
    const std::valarray<double> d = sinSQalpha[i + 1] * cosSQalpha[i] * std::cos((lamm[i + 1] - lamm[i]) * xValArray);
    tx = tx + a + b + c + d;
  }
  Px = tx / (2 * J + 1);
}

void MeierV2::calculatePz(std::valarray<double> &Pz, const std::valarray<double> &xValArray,
                          const std::valarray<double> &sinSQ2alpha, const std::valarray<double> &cosSQ2alpha,
                          const std::valarray<double> &lamm, const std::valarray<double> &lamp, const double &J,
                          const double &J2) const {
  std::valarray<double> tz(xValArray.size());
  for (size_t i = 1; i < (size_t)J2 + 1; i++) {
    tz = tz + cosSQ2alpha[i] + sinSQ2alpha[i] * std::cos((lamp[i] - lamm[i]) * xValArray);
  }
  Pz = (1 + tz) / (2 * J + 1);
}
} // namespace Mantid::CurveFitting::Functions
