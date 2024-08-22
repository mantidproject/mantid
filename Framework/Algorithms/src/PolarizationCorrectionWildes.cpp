// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/PolarizationCorrectionWildes.h"

#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include "MantidAlgorithms/PolarizationCorrections/SpinStateValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StringTokenizer.h"

#include <Eigen/Dense>
#include <boost/math/special_functions/pow.hpp>

namespace {
/// Property names.
namespace Prop {
static const std::string FLIPPERS{"Flippers"};
static const std::string SPIN_STATES{"SpinStates"};
static const std::string EFFICIENCIES{"Efficiencies"};
static const std::string INPUT_WS{"InputWorkspaces"};
static const std::string OUTPUT_WS{"OutputWorkspace"};
} // namespace Prop

/**
 * Throw if given ws is nullptr.
 * @param ws a workspace to check
 * @param tag a flipper configuration for the error message
 */
void checkInputExists(const Mantid::API::MatrixWorkspace_sptr &ws, const std::string &tag) {
  if (!ws) {
    throw std::runtime_error("A workspace designated as " + tag + " is missing in inputs.");
  }
}

/**
 * Calculate the corrected intensities and error estimates.
 * @param corrected an output vector for R00, R01, R10 and R11
 * @param errors an output vector for the error estimates
 * @param ppy intensity I00
 * @param ppyE error of ppy
 * @param pmy intensity I01
 * @param pmyE error of pmy
 * @param mpy intensity I10
 * @param mpyE error of mpy
 * @param mmy intensity I11
 * @param mmyE error of mmy
 * @param f1 polarizer efficiency
 * @param f1E error of f1
 * @param f2 analyzer efficiency
 * @param f2E error of f2
 * @param p1 polarizer flipper efficiency
 * @param p1E error of p1
 * @param p2 analyzer flipper efficiency
 * @param p2E error of p2
 */
void fourInputsCorrectedAndErrors(Eigen::Vector4d &corrected, Eigen::Vector4d &errors, const double ppy,
                                  const double ppyE, const double pmy, const double pmyE, const double mpy,
                                  const double mpyE, const double mmy, const double mmyE, const double f1,
                                  const double f1E, const double f2, const double f2E, const double p1,
                                  const double p1E, const double p2, const double p2E) {
  using namespace boost::math;
  // Note that f1 and f2 correspond to 1-F1 and 1-F2 in [Wildes, 1999].
  // These are inverted forms of the efficiency matrices.
  const auto diag1 = 1. / f1;
  const auto off1 = (f1 - 1.) / f1;
  Eigen::Matrix4d F1m;
  F1m << 1., 0., 0., 0., 0., 1., 0., 0., off1, 0., diag1, 0., 0., off1, 0., diag1;

  const auto diag2 = 1. / f2;
  const auto off2 = (f2 - 1.) / f2;
  Eigen::Matrix4d F2m;
  F2m << 1., 0., 0., 0., off2, diag2, 0., 0., 0., 0., 1., 0., 0., 0., off2, diag2;
  const auto diag3 = p1 / (2. * p1 - 1);
  const auto off3 = (p1 - 1.) / (2. * p1 - 1.);
  Eigen::Matrix4d P1m;
  P1m << diag3, 0, off3, 0, 0, diag3, 0, off3, off3, 0, diag3, 0, 0, off3, 0, diag3;
  const auto diag4 = p2 / (2. * p2 - 1.);
  const auto off4 = (p2 - 1.) / (2. * p2 - 1.);
  Eigen::Matrix4d P2m;
  P2m << diag4, off4, 0., 0., off4, diag4, 0., 0., 0., 0., diag4, off4, 0., 0., off4, diag4;
  const Eigen::Vector4d intensities(ppy, pmy, mpy, mmy);
  const auto FProduct = F2m * F1m;
  const auto PProduct = P2m * P1m;
  const auto PFProduct = PProduct * FProduct;
  corrected = PFProduct * intensities;
  // The error matrices here are element-wise algebraic derivatives of
  // the matrices above, multiplied by the error.
  const auto elemE1 = -1. / pow<2>(f1) * f1E;
  Eigen::Matrix4d F1Em;
  F1Em << 0., 0., 0., 0., 0., 0., 0., 0., -elemE1, 0., elemE1, 0., 0., -elemE1, 0., elemE1;
  const auto elemE2 = -1. / pow<2>(f2) * f2E;
  Eigen::Matrix4d F2Em;
  F2Em << 0., 0., 0., 0., -elemE2, elemE2, 0., 0., 0., 0., 0., 0., 0., 0., -elemE2, elemE2;
  const auto elemE3 = 1. / pow<2>(2. * p1 - 1.) * p1E;
  Eigen::Matrix4d P1Em;
  P1Em << elemE3, 0., -elemE3, 0., 0., elemE3, 0., -elemE3, -elemE3, 0., elemE3, 0., 0., -elemE3, 0., elemE3;
  const auto elemE4 = 1. / pow<2>(2. * p2 - 1.) * p2E;
  Eigen::Matrix4d P2Em;
  P2Em << elemE4, -elemE4, 0., 0., -elemE4, elemE4, 0., 0., 0., 0., elemE4, -elemE4, 0., 0., -elemE4, elemE4;
  const Eigen::Vector4d yErrors(ppyE, pmyE, mpyE, mmyE);
  const auto e1 = (P2Em * P1m * FProduct * intensities).array();
  const auto e2 = (P2m * P1Em * FProduct * intensities).array();
  const auto e3 = (PProduct * F2Em * F1m * intensities).array();
  const auto e4 = (PProduct * F2m * F1Em * intensities).array();
  const auto sqPFProduct = (PFProduct.array() * PFProduct.array()).matrix();
  const auto sqErrors = (yErrors.array() * yErrors.array()).matrix();
  const auto e5 = (sqPFProduct * sqErrors).array();
  errors = (e1 * e1 + e2 * e2 + e3 * e3 + e4 * e4 + e5).sqrt();
}

/**
 * Estimate errors for I01 in the two inputs case.
 * @param i00 intensity of 00 flipper configuration
 * @param e00 error of i00
 * @param i11 intensity of 11 flipper configuration
 * @param e11 error of i11
 * @param p1 polarizer efficiency
 * @param p1E error of p1
 * @param p2 analyzer efficiency
 * @param p2E error of p2
 * @param f1 polarizer flipper efficiency
 * @param f1E error of f1
 * @param f2 analyzer flipper efficiency
 * @param f2E error of f2
 * @return the error estimate
 */
double twoInputsErrorEstimate01(const double i00, const double e00, const double i11, const double e11, const double p1,
                                const double p1E, const double p2, const double p2E, const double f1, const double f1E,
                                const double f2, const double f2E) {
  using namespace boost::math;
  // Derivatives of the equation which solves the I01 intensities
  // with respect to i00, i11, f1, etc.
  const auto pmdi00 =
      -((f1 * (-1. + 2. * p1) * (-f2 * pow<2>(1. - 2. * p2) + pow<2>(f2) * pow<2>(1. - 2. * p2) + (-1. + p2) * p2)) /
        (f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) +
         f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2))));
  const auto pmdi11 = (f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2)) /
                      (f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) +
                       f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)));
  const auto pmdf1 =
      -(((-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)) *
         (f2 * i11 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) -
          f1 * i00 * (-1. + 2. * p1) *
              (-f2 * pow<2>(1. - 2. * p2) + pow<2>(f2) * pow<2>(1. - 2. * p2) + (-1. + p2) * p2))) /
        pow<2>(f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) +
               f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)))) -
      (i00 * (-1. + 2. * p1) * (-f2 * pow<2>(1. - 2. * p2) + pow<2>(f2) * pow<2>(1. - 2. * p2) + (-1. + p2) * p2)) /
          (f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) +
           f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)));
  const auto pmdf2 =
      -(((f1 * (-1. + 2. * p1) * (-1. + p1 + p2) * (-1 + 2 * p2) + p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2)) *
         (f2 * i11 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) -
          f1 * i00 * (-1. + 2. * p1) *
              (-f2 * pow<2>(1. - 2. * p2) + pow<2>(f2) * pow<2>(1. - 2. * p2) + (-1. + p2) * p2))) /
        pow<2>(f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) +
               f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)))) +
      (-f1 * i00 * (-1. + 2. * p1) * (-pow<2>(1. - 2. * p2) + 2 * f2 * pow<2>(1. - 2. * p2)) +
       i11 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2)) /
          (f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) +
           f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)));
  const auto pmdp1 =
      -(((f2 * i11 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) -
          f1 * i00 * (-1. + 2. * p1) *
              (-f2 * pow<2>(1. - 2. * p2) + pow<2>(f2) * pow<2>(1. - 2. * p2) + (-1. + p2) * p2)) *
         (f2 * p1 * (1. - 2. * p2) + f1 * f2 * (-1. + 2. * p1) * (-1. + 2. * p2) +
          f2 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) +
          2. * f1 * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)))) /
        pow<2>(f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) +
               f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)))) +
      (f2 * i11 * p1 * (1. - 2. * p2) + f2 * i11 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) -
       2. * f1 * i00 * (-f2 * pow<2>(1. - 2. * p2) + pow<2>(f2) * pow<2>(1. - 2. * p2) + (-1. + p2) * p2)) /
          (f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) +
           f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)));
  const auto pmdp2 =
      -(((f2 * i11 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) -
          f1 * i00 * (-1. + 2. * p1) *
              (-f2 * pow<2>(1. - 2. * p2) + pow<2>(f2) * pow<2>(1. - 2. * p2) + (-1. + p2) * p2)) *
         (f2 * (2. - 2. * p1) * p1 +
          f1 * (-1. + 2. * p1) * (1. - 2. * p2 + 2. * f2 * (-1. + p1 + p2) + f2 * (-1. + 2. * p2)))) /
        pow<2>(f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) +
               f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)))) +
      (f2 * i11 * (2. - 2. * p1) * p1 -
       f1 * i00 * (-1. + 2. * p1) * (-1. + 4. * f2 * (1. - 2. * p2) - 4. * pow<2>(f2) * (1. - 2. * p2) + 2. * p2)) /
          (f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) +
           f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)));
  // Estimate the error components using linearized extrapolation,
  // sum in squares.
  const auto e01_I00 = pow<2>(pmdi00 * e00);
  const auto e01_I11 = pow<2>(pmdi11 * e11);
  const auto e01_F1 = pow<2>(pmdf1 * f1E);
  const auto e01_F2 = pow<2>(pmdf2 * f2E);
  const auto e01_P1 = pow<2>(pmdp1 * p1E);
  const auto e01_P2 = pow<2>(pmdp2 * p2E);
  return std::sqrt(e01_I00 + e01_I11 + e01_F1 + e01_F2 + e01_P1 + e01_P2);
}

/**
 * Estimate errors for I10 in the two inputs case.
 * @param i00 intensity of 00 flipper configuration
 * @param e00 error of i00
 * @param i11 intensity of 11 flipper configuration
 * @param e11 error of i11
 * @param p1 polarizer efficiency
 * @param p1E error of p1
 * @param p2 analyzer efficiency
 * @param p2E error of p2
 * @param f1 polarizer flipper efficiency
 * @param f1E error of f1
 * @param f2 analyzer flipper efficiency
 * @param f2E error of f2
 * @return the error estimate
 */
double twoInputsErrorEstimate10(const double i00, const double e00, const double i11, const double e11, const double p1,
                                const double p1E, const double p2, const double p2E, const double f1, const double f1E,
                                const double f2, const double f2E) {
  using namespace boost::math;
  // Derivatives of the equation which solves the I10 intensities
  // with respect to i00, i11, f1, etc.
  const auto a = -1. + p1 + 2. * p2 - 2. * p1 * p2;
  const auto b = -1. + 2. * p1;
  const auto c = -1. + 2. * p2;
  const auto d = -1. + p2;
  const auto mpdi00 = (-pow<2>(f1) * f2 * pow<2>(b) * c + f1 * f2 * pow<2>(b) * c + f2 * p1 * a) /
                      (f2 * p1 * a + f1 * b * (-d * p2 + f2 * (p1 + d) * c));
  const auto mpdi11 = -((f1 * b * d * p2) / (f2 * p1 * a + f1 * b * (-d * p2 + f2 * (p1 + d) * c)));
  const auto mpdf1 =
      -(((-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)) *
         (-pow<2>(f1) * f2 * i00 * pow<2>(1. - 2. * p1) * (-1. + 2. * p2) +
          f2 * i00 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) +
          f1 * (-1. + 2. * p1) * (-i11 * (-1. + p2) * p2 + f2 * i00 * (-1. + 2. * p1) * (-1. + 2. * p2)))) /
        pow<2>(f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) +
               f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)))) +
      (-2. * f1 * f2 * i00 * pow<2>(1. - 2. * p1) * (-1. + 2. * p2) +
       (-1. + 2. * p1) * (-i11 * (-1. + p2) * p2 + f2 * i00 * (-1. + 2. * p1) * (-1. + 2. * p2))) /
          (f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) +
           f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)));
  const auto mpdf2 = -(((f1 * b * (p1 + d) * c + p1 * a) * (-pow<2>(f1) * f2 * i00 * pow<2>(b) * c + f2 * i00 * p1 * a +
                                                            f1 * b * (-i11 * d * p2 + f2 * i00 * b * c))) /
                       pow<2>(f2 * p1 * a + f1 * b * (-d * p2 + f2 * (p1 + d) * c))) +
                     (-pow<2>(f1) * i00 * pow<2>(b) * c + f1 * i00 * pow<2>(b) * c + i00 * p1 * a) /
                         (f2 * p1 * a + f1 * b * (-d * p2 + f2 * (p1 + d) * c));
  const auto mpdp1 =
      -(((-pow<2>(f1) * f2 * i00 * pow<2>(b) * c + f2 * i00 * p1 * a + f1 * b * (-i11 * d * p2 + f2 * i00 * b * c)) *
         (f2 * p1 * -c + f1 * f2 * b * c + f2 * a + 2. * f1 * (-d * p2 + f2 * (p1 + d) * c))) /
        pow<2>(f2 * p1 * a + f1 * b * (-d * p2 + f2 * (p1 + d) * c))) +
      (f2 * i00 * p1 * -c + 4. * pow<2>(f1) * f2 * i00 * -b * c + 2. * f1 * f2 * i00 * b * c + f2 * i00 * a +
       2. * f1 * (-i11 * d * p2 + f2 * i00 * b * c)) /
          (f2 * p1 * a + f1 * b * (-d * p2 + f2 * (p1 + d) * c));
  const auto mpdp2 =
      -(((f2 * (2. - 2. * p1) * p1 + f1 * b * (1. - 2. * p2 + 2. * f2 * (p1 + d) + f2 * c)) *
         (-pow<2>(f1) * f2 * i00 * pow<2>(b) * c + f2 * i00 * p1 * a + f1 * b * (-i11 * d * p2 + f2 * i00 * b * c))) /
        pow<2>(f2 * p1 * a + f1 * b * (-d * p2 + f2 * (p1 + d) * c))) +
      (-2. * pow<2>(f1) * f2 * i00 * pow<2>(b) + f2 * i00 * (2. - 2. * p1) * p1 +
       f1 * b * (2. * f2 * i00 * b - i11 * d - i11 * p2)) /
          (f2 * p1 * a + f1 * b * (-d * p2 + f2 * (p1 + d) * c));
  // Estimate the error components using linearized extrapolation,
  // sum in squares.
  const auto e10_I00 = pow<2>(mpdi00 * e00);
  const auto e10_I11 = pow<2>(mpdi11 * e11);
  const auto e10_F1 = pow<2>(mpdf1 * f1E);
  const auto e10_F2 = pow<2>(mpdf2 * f2E);
  const auto e10_P1 = pow<2>(mpdp1 * p1E);
  const auto e10_P2 = pow<2>(mpdp2 * p2E);
  return std::sqrt(e10_I00 + e10_I11 + e10_F1 + e10_F2 + e10_P1 + e10_P2);
}

Mantid::API::MatrixWorkspace_sptr createWorkspaceWithHistory(const Mantid::API::MatrixWorkspace_const_sptr &inputWS) {
  Mantid::API::MatrixWorkspace_sptr outputWS = Mantid::DataObjects::create<Mantid::DataObjects::Workspace2D>(*inputWS);
  outputWS->history().addHistory(inputWS->getHistory());
  return outputWS;
}
} // namespace

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PolarizationCorrectionWildes)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string PolarizationCorrectionWildes::name() const { return "PolarizationCorrectionWildes"; }

/// Algorithm's version for identification. @see Algorithm::version
int PolarizationCorrectionWildes::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PolarizationCorrectionWildes::category() const { return "Reflectometry"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string PolarizationCorrectionWildes::summary() const {
  return "Corrects a group of polarization analysis workspaces for polarizer "
         "and analyzer efficiencies.";
}

/// Algorithm's related algorithms. @see Algorithm::seeAlso
const std::vector<std::string> PolarizationCorrectionWildes::seeAlso() const { return {"PolarizationEfficiencyCor", "PolarizationEfficienciesWildes"}; }

/**
 * Count the non-nullptr workspaces
 * @return the count on non-nullptr workspaces.
 */
size_t PolarizationCorrectionWildes::WorkspaceMap::size() const noexcept {
  return (mmWS ? 1 : 0) + (mpWS ? 1 : 0) + (pmWS ? 1 : 0) + (ppWS ? 1 : 0);
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void PolarizationCorrectionWildes::init() {
  declareProperty(std::make_unique<Kernel::ArrayProperty<std::string>>(
                      Prop::INPUT_WS, "", std::make_shared<API::ADSValidator>(), Kernel::Direction::Input),
                  "A list of workspaces to be corrected corresponding to the "
                  "flipper configurations.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::WorkspaceGroup>>(Prop::OUTPUT_WS, "", Kernel::Direction::Output),
      "A group of polarization efficiency corrected workspaces.");

  const auto flipperConfigValidator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{1, 2, 3, 4}, true);
  declareProperty(Prop::FLIPPERS,
                  std::string(FlipperConfigurations::OFF_OFF) + ", " + FlipperConfigurations::OFF_ON + ", " +
                      FlipperConfigurations::ON_OFF + ", " + FlipperConfigurations::ON_ON,
                  flipperConfigValidator, "Flipper configurations of the input workspaces.");
  const auto spinStateValidator =
      std::make_shared<SpinStateValidator>(std::unordered_set<int>{0, 2, 4}, false, '+', '-', true);
  declareProperty(Prop::SPIN_STATES, "", spinStateValidator, "The order of the spin states in the output workspace.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(Prop::EFFICIENCIES, "", Kernel::Direction::Input),
      "A workspace containing the efficiency factors P1, P2, F1 and F2 as "
      "histograms");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PolarizationCorrectionWildes::exec() {
  const std::string flipperProperty = getProperty(Prop::FLIPPERS);
  const auto flippers = PolarizationCorrectionsHelpers::splitSpinStateString(flipperProperty);
  const auto inputs = mapInputsToDirections(flippers);
  checkConsistentNumberHistograms(inputs);
  const EfficiencyMap efficiencies = efficiencyFactors();
  checkConsistentX(inputs, efficiencies);
  WorkspaceMap outputs;
  switch (inputs.size()) {
  case 1:
    outputs = directBeamCorrections(inputs, efficiencies);
    break;
  case 2:
    // Check if the input flipper configuration includes an analyser
    if (flippers.front().size() > 1) {
      outputs = twoInputCorrections(inputs, efficiencies);
    } else {
      outputs = analyzerlessCorrections(inputs, efficiencies);
    }
    break;
  case 3:
    outputs = threeInputCorrections(inputs, efficiencies);
    break;
  case 4:
    outputs = fullCorrections(inputs, efficiencies);
  }
  setProperty(Prop::OUTPUT_WS, groupOutput(outputs));
}

/**
 * Validate the algorithm's input properties.
 * @return a map from property names to discovered issues
 */
std::map<std::string, std::string> PolarizationCorrectionWildes::validateInputs() {
  std::map<std::string, std::string> issues;
  API::MatrixWorkspace_const_sptr factorWS = getProperty(Prop::EFFICIENCIES);
  if (factorWS) {
    const auto &factorAxis = factorWS->getAxis(1);
    if (!factorAxis) {
      issues[Prop::EFFICIENCIES] = "The workspace is missing a vertical axis.";
    } else if (!factorAxis->isText()) {
      issues[Prop::EFFICIENCIES] = "The vertical axis in the workspace is not text axis.";
    } else if (factorWS->getNumberHistograms() < 4) {
      issues[Prop::EFFICIENCIES] = "The workspace should contain at least 4 histograms.";
    } else {
      std::vector<std::string> tags{{"P1", "P2", "F1", "F2"}};
      for (size_t i = 0; i != factorAxis->length(); ++i) {
        const auto label = factorAxis->label(i);
        auto found = std::find(tags.begin(), tags.end(), label);
        if (found != tags.cend()) {
          std::swap(tags.back(), *found);
          tags.pop_back();
        }
      }
      if (!tags.empty()) {
        issues[Prop::EFFICIENCIES] = "A histogram labeled " + tags.front() + " is missing from the workspace.";
      }
    }
  }
  const std::vector<std::string> inputs = getProperty(Prop::INPUT_WS);
  const auto flipperConfig = PolarizationCorrectionsHelpers::splitSpinStateString(getPropertyValue(Prop::FLIPPERS));
  const auto flipperCount = flipperConfig.size();
  if (inputs.size() != flipperCount) {
    issues[Prop::FLIPPERS] = "The number of flipper configurations (" + std::to_string(flipperCount) +
                             ") does not match the number of input workspaces (" + std::to_string(inputs.size()) + ")";
  }
  // SpinStates checks.
  const auto spinStates = PolarizationCorrectionsHelpers::splitSpinStateString(getPropertyValue(Prop::SPIN_STATES));
  if (inputs.size() == 1 && !spinStates.empty()) {
    issues[Prop::SPIN_STATES] = "Output workspace order cannot be set for direct beam calculations.";
  } else if (!spinStates.empty()) {
    if (flipperConfig.front().size() == 1 && spinStates.size() != 2) {
      issues[Prop::SPIN_STATES] =
          "Incorrect number of workspaces in output configuration: " + std::to_string(spinStates.size()) +
          ". Only two output workspaces are produced when an analyzer is not used.";
    }
    if (flipperConfig.front().size() == 2 && spinStates.size() != 4) {
      issues[Prop::SPIN_STATES] =
          "Incorrect number of workspaces in output configuration: " + std::to_string(spinStates.size()) +
          ". Four output workspaces are produced by the corrections.";
    }
  }
  return issues;
}

/**
 * Check that all workspaces in inputs have the same number of histograms.
 * @param inputs a set of workspaces to check
 */
void PolarizationCorrectionWildes::checkConsistentNumberHistograms(const WorkspaceMap &inputs) {
  size_t nHist{0};
  bool nHistValid{false};
  // A local helper function to check the number of histograms.
  auto checkNHist = [&nHist, &nHistValid](const API::MatrixWorkspace_sptr &ws, const std::string &tag) {
    if (nHistValid) {
      if (nHist != ws->getNumberHistograms()) {
        throw std::runtime_error("Number of histograms mismatch in " + tag);
      }
    } else {
      nHist = ws->getNumberHistograms();
      nHistValid = true;
    }
  };
  if (inputs.mmWS) {
    checkNHist(inputs.mmWS, FlipperConfigurations::ON_ON);
  }
  if (inputs.mpWS) {
    checkNHist(inputs.mpWS, FlipperConfigurations::ON_OFF);
  }
  if (inputs.pmWS) {
    checkNHist(inputs.pmWS, FlipperConfigurations::OFF_ON);
  }
  if (inputs.ppWS) {
    checkNHist(inputs.ppWS, FlipperConfigurations::OFF_OFF);
  }
}

/**
 * Check that all workspaces and efficicencies have the same X data.
 * @param inputs a set of workspaces to check
 * @param efficiencies efficiencies to check
 */
void PolarizationCorrectionWildes::checkConsistentX(const WorkspaceMap &inputs, const EfficiencyMap &efficiencies) {
  // Compare everything to F1 efficiency.
  const auto &F1x = efficiencies.F1->x();
  // A local helper function to check a HistogramX against F1.
  auto checkX = [&F1x](const HistogramData::HistogramX &x, const std::string &tag) {
    if (x.size() != F1x.size()) {
      throw std::runtime_error("Mismatch of histogram lengths between F1 and " + tag + '.');
    }
    for (size_t i = 0; i != x.size(); ++i) {
      if (x[i] != F1x[i]) {
        throw std::runtime_error("Mismatch of X data between F1 and " + tag + '.');
      }
    }
  };
  const auto &F2x = efficiencies.F2->x();
  checkX(F2x, "F2");
  const auto &P1x = efficiencies.P1->x();
  checkX(P1x, "P1");
  const auto &P2x = efficiencies.P2->x();
  checkX(P2x, "P2");
  // A local helper function to check an input workspace against F1.
  auto checkWS = [&checkX](const API::MatrixWorkspace_sptr &ws, const std::string &tag) {
    const auto nHist = ws->getNumberHistograms();
    for (size_t i = 0; i != nHist; ++i) {
      checkX(ws->x(i), tag);
    }
  };
  if (inputs.mmWS) {
    checkWS(inputs.mmWS, FlipperConfigurations::ON_ON);
  }
  if (inputs.mpWS) {
    checkWS(inputs.mpWS, FlipperConfigurations::ON_OFF);
  }
  if (inputs.pmWS) {
    checkWS(inputs.pmWS, FlipperConfigurations::OFF_ON);
  }
  if (inputs.ppWS) {
    checkWS(inputs.ppWS, FlipperConfigurations::OFF_OFF);
  }
}

/**
 * Make a workspace group out of the given set of workspaces.
 * The workspaces will be published in the ADS, their names appended by
 * appropriate suffices.
 * @param outputs a set of workspaces to group
 * @return a group workspace
 */
API::WorkspaceGroup_sptr PolarizationCorrectionWildes::groupOutput(const WorkspaceMap &outputs) {
  const auto &outWSName = getPropertyValue(Prop::OUTPUT_WS);
  auto spinStateOrder = getPropertyValue(Prop::SPIN_STATES);
  std::vector<std::string> names;
  if (!spinStateOrder.empty()) {
    names.resize(PolarizationCorrectionsHelpers::splitSpinStateString(spinStateOrder).size());
  }

  if (outputs.ppWS) {
    addSpinStateOutput(names, spinStateOrder, outWSName, outputs.ppWS, SpinStateConfigurationsWildes::PLUS_PLUS);
  }
  if (outputs.pmWS) {
    addSpinStateOutput(names, spinStateOrder, outWSName, outputs.pmWS, SpinStateConfigurationsWildes::PLUS_MINUS);
  }
  if (outputs.mpWS) {
    addSpinStateOutput(names, spinStateOrder, outWSName, outputs.mpWS, SpinStateConfigurationsWildes::MINUS_PLUS);
  }
  if (outputs.mmWS) {
    addSpinStateOutput(names, spinStateOrder, outWSName, outputs.mmWS, SpinStateConfigurationsWildes::MINUS_MINUS);
  }

  auto group = createChildAlgorithm("GroupWorkspaces");
  group->initialize();
  group->setProperty("InputWorkspaces", names);
  group->setProperty("OutputWorkspace", outWSName);
  group->execute();
  API::WorkspaceGroup_sptr outWS = group->getProperty("OutputWorkspace");
  return outWS;
}

/**
 * Add an output name in the correct position in the vector and to the ADS.
 * @param names A list of the names of the workspaces the algorithm has generated.
 * @param spinStateOrder The order the output should be in.
 * @param baseName The base name for the output workspaces ("BASENAME_SPINSTATE" e.g "OUTNAME_+-")
 * @param ws The workspace to add to the vector and ADS.
 * @param spinState The spin state the workspace represents.
 */
void PolarizationCorrectionWildes::addSpinStateOutput(std::vector<std::string> &names,
                                                      const std::string &spinStateOrder, const std::string &baseName,
                                                      const API::MatrixWorkspace_sptr &ws,
                                                      const std::string &spinState) {
  if (spinStateOrder.empty()) {
    names.emplace_back(baseName + "_" + spinState);
    API::AnalysisDataService::Instance().addOrReplace(names.back(), ws);
  } else {
    const auto &maybeIndex = PolarizationCorrectionsHelpers::indexOfWorkspaceForSpinState(
        PolarizationCorrectionsHelpers::splitSpinStateString(spinStateOrder), spinState);
    if (!maybeIndex.has_value()) {
      throw std::invalid_argument("Required spin state (" + spinState + ") not found in spin state order (" +
                                  spinStateOrder + ").");
    }
    const auto index = maybeIndex.value();
    names[index] = baseName + "_" + spinState;
    API::AnalysisDataService::Instance().addOrReplace(names[index], ws);
  }
}

/**
 * Make a convenience access object to the efficiency factors.
 * @return an EfficiencyMap object
 */
PolarizationCorrectionWildes::EfficiencyMap PolarizationCorrectionWildes::efficiencyFactors() {
  EfficiencyMap e;
  API::MatrixWorkspace_const_sptr factorWS = getProperty(Prop::EFFICIENCIES);
  const auto &vertAxis = factorWS->getAxis(1);
  for (size_t i = 0; i != vertAxis->length(); ++i) {
    const auto label = vertAxis->label(i);
    if (label == "P1") {
      e.P1 = &factorWS->getSpectrum(i);
    } else if (label == "P2") {
      e.P2 = &factorWS->getSpectrum(i);
    } else if (label == "F1") {
      e.F1 = &factorWS->getSpectrum(i);
    } else if (label == "F2") {
      e.F2 = &factorWS->getSpectrum(i);
    }
    // Ignore other histograms such as 'Phi' in ILL's efficiency ws.
  }
  return e;
}

/**
 * Correct a direct beam measurement for non-ideal instrument effects.
 * Only the non-analyzer, polarizer flipper off case is considered here.
 * @param inputs a set of workspaces to correct
 * @param efficiencies a set of efficiency factors
 * @return set of corrected workspaces
 */
PolarizationCorrectionWildes::WorkspaceMap
PolarizationCorrectionWildes::directBeamCorrections(const WorkspaceMap &inputs, const EfficiencyMap &efficiencies) {
  using namespace boost::math;
  checkInputExists(inputs.ppWS, FlipperConfigurations::OFF);
  WorkspaceMap outputs;
  outputs.ppWS = createWorkspaceWithHistory(inputs.ppWS);
  const size_t nHisto = inputs.ppWS->getNumberHistograms();
  for (size_t wsIndex = 0; wsIndex != nHisto; ++wsIndex) {
    const auto &ppY = inputs.ppWS->y(wsIndex);
    const auto &ppE = inputs.ppWS->e(wsIndex);
    auto &ppYOut = outputs.ppWS->mutableY(wsIndex);
    auto &ppEOut = outputs.ppWS->mutableE(wsIndex);
    for (size_t binIndex = 0; binIndex < ppY.size(); ++binIndex) {
      const auto P1 = efficiencies.P1->y()[binIndex];
      const auto P2 = efficiencies.P2->y()[binIndex];
      const double f = 1. - P1 - P2 + 2. * P1 * P2;
      ppYOut[binIndex] = ppY[binIndex] / f;
      const auto P1E = efficiencies.P1->e()[binIndex];
      const auto P2E = efficiencies.P2->e()[binIndex];
      const auto e1 = pow<2>(P1E * (2. * P1 - 1.) / pow<2>(f) * ppY[binIndex]);
      const auto e2 = pow<2>(P2E * (2. * P2 - 1.) / pow<2>(f) * ppY[binIndex]);
      const auto e3 = pow<2>(ppE[binIndex] / f);
      const auto errorSum = std::sqrt(e1 + e2 + e3);
      ppEOut[binIndex] = errorSum;
    }
  }
  return outputs;
}

/**
 * Correct for non-ideal instrument effects.
 * Deals with the case when the data was taken without the analyzer:
 * only the polarizer flipper is used.
 * @param inputs a set of workspaces to correct
 * @param efficiencies a set of efficiency factors
 * @return a set of corrected workspaces
 */
PolarizationCorrectionWildes::WorkspaceMap
PolarizationCorrectionWildes::analyzerlessCorrections(const WorkspaceMap &inputs, const EfficiencyMap &efficiencies) {
  using namespace boost::math;
  checkInputExists(inputs.mmWS, FlipperConfigurations::ON);
  checkInputExists(inputs.ppWS, FlipperConfigurations::OFF);
  WorkspaceMap outputs;
  outputs.mmWS = createWorkspaceWithHistory(inputs.mmWS);
  outputs.ppWS = createWorkspaceWithHistory(inputs.ppWS);
  const size_t nHisto = inputs.mmWS->getNumberHistograms();
  for (size_t wsIndex = 0; wsIndex != nHisto; ++wsIndex) {
    const auto &mmY = inputs.mmWS->y(wsIndex);
    const auto &mmE = inputs.mmWS->e(wsIndex);
    const auto &ppY = inputs.ppWS->y(wsIndex);
    const auto &ppE = inputs.ppWS->e(wsIndex);
    auto &mmYOut = outputs.mmWS->mutableY(wsIndex);
    auto &mmEOut = outputs.mmWS->mutableE(wsIndex);
    auto &ppYOut = outputs.ppWS->mutableY(wsIndex);
    auto &ppEOut = outputs.ppWS->mutableE(wsIndex);
    for (size_t binIndex = 0; binIndex < mmY.size(); ++binIndex) {
      const auto F1 = efficiencies.F1->y()[binIndex];
      const auto P1 = efficiencies.P1->y()[binIndex];
      Eigen::Matrix2d F1m;
      F1m << 1., 0., (F1 - 1.) / F1, 1. / F1;
      const double divisor = (2. * P1 - 1.);
      const double off = (P1 - 1.) / divisor;
      const double diag = P1 / divisor;
      Eigen::Matrix2d P1m;
      P1m << diag, off, off, diag;
      const Eigen::Vector2d intensities(ppY[binIndex], mmY[binIndex]);
      const auto PFProduct = P1m * F1m;
      const auto corrected = PFProduct * intensities;
      ppYOut[binIndex] = corrected[0];
      mmYOut[binIndex] = corrected[1];
      const auto F1E = efficiencies.F1->e()[binIndex];
      const auto P1E = efficiencies.P1->e()[binIndex];
      const auto elemE1 = -1. / pow<2>(F1) * F1E;
      Eigen::Matrix2d F1Em;
      F1Em << 0., 0., -elemE1, elemE1;
      const auto elemE2 = 1. / pow<2>(divisor) * P1E;
      Eigen::Matrix2d P1Em;
      P1Em << elemE2, -elemE2, -elemE2, elemE2;
      const Eigen::Vector2d errors(ppE[binIndex], mmE[binIndex]);
      const auto e1 = (P1Em * F1m * intensities).array();
      const auto e2 = (P1m * F1Em * intensities).array();
      const auto sqPFProduct = (PFProduct.array() * PFProduct.array()).matrix();
      const auto sqErrors = (errors.array() * errors.array()).matrix();
      const auto e3 = (sqPFProduct * sqErrors).array();
      const auto errorSum = (e1 * e1 + e2 * e2 + e3).sqrt();
      ppEOut[binIndex] = errorSum[0];
      mmEOut[binIndex] = errorSum[1];
    }
  }
  return outputs;
}

/**
 * Correct for non-ideal instrument effects.
 * Only 00 and 11 flipper configurations need to be provided;
 * the missing 01 and 10 data is solved from the assumption that
 * in the corrected data, R01 = R10 = 0.
 * @param inputs a set of workspaces to correct
 * @param efficiencies a set of efficiency factors
 * @return a set of corrected workspaces
 */
PolarizationCorrectionWildes::WorkspaceMap
PolarizationCorrectionWildes::twoInputCorrections(const WorkspaceMap &inputs, const EfficiencyMap &efficiencies) {
  using namespace boost::math;
  checkInputExists(inputs.mmWS, FlipperConfigurations::ON_ON);
  checkInputExists(inputs.ppWS, FlipperConfigurations::OFF_OFF);
  WorkspaceMap fullInputs = inputs;
  fullInputs.mpWS = createWorkspaceWithHistory(inputs.mmWS);
  fullInputs.pmWS = createWorkspaceWithHistory(inputs.ppWS);
  twoInputsSolve01And10(fullInputs, inputs, efficiencies);
  return fullCorrections(fullInputs, efficiencies);
}

/**
 * Correct for non-ideal instrument effects.
 * Needs the 00 and 11 flipper configurations as well as either 01 or 10.
 * The missing intensity (01 or 10) is solved from the assumption
 * that the corrected R01 = R10.
 * @param inputs a set of workspaces to correct
 * @param efficiencies a set of efficiency factors
 * @return a set of corrected workspaces
 */
PolarizationCorrectionWildes::WorkspaceMap
PolarizationCorrectionWildes::threeInputCorrections(const WorkspaceMap &inputs, const EfficiencyMap &efficiencies) {
  WorkspaceMap fullInputs = inputs;
  checkInputExists(inputs.mmWS, FlipperConfigurations::ON_ON);
  checkInputExists(inputs.ppWS, FlipperConfigurations::OFF_OFF);
  if (!inputs.mpWS) {
    checkInputExists(inputs.pmWS, FlipperConfigurations::OFF_ON);
    threeInputsSolve10(fullInputs, efficiencies);
  } else {
    checkInputExists(inputs.mpWS, FlipperConfigurations::ON_OFF);
    threeInputsSolve01(fullInputs, efficiencies);
  }
  return fullCorrections(fullInputs, efficiencies);
}

/**
 * Correct for non-ideal instrument effects.
 * Perform full polarization corrections. All flipper configurations
 * (00, 01, 10 and 11) are needed for this.
 * @param inputs a set of workspaces to correct
 * @param efficiencies a set of efficiency factors
 * @return a set of corrected workspaces
 */
PolarizationCorrectionWildes::WorkspaceMap
PolarizationCorrectionWildes::fullCorrections(const WorkspaceMap &inputs, const EfficiencyMap &efficiencies) {
  using namespace boost::math;
  checkInputExists(inputs.mmWS, FlipperConfigurations::ON_ON);
  checkInputExists(inputs.mpWS, FlipperConfigurations::ON_OFF);
  checkInputExists(inputs.pmWS, FlipperConfigurations::OFF_ON);
  checkInputExists(inputs.ppWS, FlipperConfigurations::OFF_OFF);
  WorkspaceMap outputs;
  outputs.mmWS = createWorkspaceWithHistory(inputs.mmWS);
  outputs.mpWS = createWorkspaceWithHistory(inputs.mpWS);
  outputs.pmWS = createWorkspaceWithHistory(inputs.pmWS);
  outputs.ppWS = createWorkspaceWithHistory(inputs.ppWS);
  const auto F1 = efficiencies.F1->y();
  const auto F1E = efficiencies.F1->e();
  const auto F2 = efficiencies.F2->y();
  const auto F2E = efficiencies.F2->e();
  const auto P1 = efficiencies.P1->y();
  const auto P1E = efficiencies.P1->e();
  const auto P2 = efficiencies.P2->y();
  const auto P2E = efficiencies.P2->e();
  const size_t nHisto = inputs.mmWS->getNumberHistograms();
  for (size_t wsIndex = 0; wsIndex != nHisto; ++wsIndex) {
    const auto &mmY = inputs.mmWS->y(wsIndex);
    const auto &mmE = inputs.mmWS->e(wsIndex);
    const auto &mpY = inputs.mpWS->y(wsIndex);
    const auto &mpE = inputs.mpWS->e(wsIndex);
    const auto &pmY = inputs.pmWS->y(wsIndex);
    const auto &pmE = inputs.pmWS->e(wsIndex);
    const auto &ppY = inputs.ppWS->y(wsIndex);
    const auto &ppE = inputs.ppWS->e(wsIndex);
    auto &mmYOut = outputs.mmWS->mutableY(wsIndex);
    auto &mmEOut = outputs.mmWS->mutableE(wsIndex);
    auto &mpYOut = outputs.mpWS->mutableY(wsIndex);
    auto &mpEOut = outputs.mpWS->mutableE(wsIndex);
    auto &pmYOut = outputs.pmWS->mutableY(wsIndex);
    auto &pmEOut = outputs.pmWS->mutableE(wsIndex);
    auto &ppYOut = outputs.ppWS->mutableY(wsIndex);
    auto &ppEOut = outputs.ppWS->mutableE(wsIndex);
    for (size_t binIndex = 0; binIndex < mmY.size(); ++binIndex) {
      Eigen::Vector4d corrected;
      Eigen::Vector4d errors;
      fourInputsCorrectedAndErrors(corrected, errors, ppY[binIndex], ppE[binIndex], pmY[binIndex], pmE[binIndex],
                                   mpY[binIndex], mpE[binIndex], mmY[binIndex], mmE[binIndex], F1[binIndex],
                                   F1E[binIndex], F2[binIndex], F2E[binIndex], P1[binIndex], P1E[binIndex],
                                   P2[binIndex], P2E[binIndex]);
      ppYOut[binIndex] = corrected[0];
      pmYOut[binIndex] = corrected[1];
      mpYOut[binIndex] = corrected[2];
      mmYOut[binIndex] = corrected[3];
      ppEOut[binIndex] = errors[0];
      pmEOut[binIndex] = errors[1];
      mpEOut[binIndex] = errors[2];
      mmEOut[binIndex] = errors[3];
    }
  }
  return outputs;
}

/**
 * Make a set of workspaces to correct from input properties.
 * @param flippers a vector of flipper configurations
 * @return a set of workspaces to correct
 */
PolarizationCorrectionWildes::WorkspaceMap
PolarizationCorrectionWildes::mapInputsToDirections(const std::vector<std::string> &flippers) {
  const std::vector<std::string> inputNames = getProperty(Prop::INPUT_WS);
  WorkspaceMap inputs;
  for (size_t i = 0; i < flippers.size(); ++i) {
    auto ws = (API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(inputNames[i]));
    if (!ws) {
      throw std::runtime_error("One of the input workspaces doesn't seem to be a MatrixWorkspace.");
    }
    const auto &f = flippers[i];
    if (f == FlipperConfigurations::ON_ON || f == FlipperConfigurations::ON) {
      inputs.mmWS = ws;
    } else if (f == FlipperConfigurations::ON_OFF) {
      inputs.mpWS = ws;
    } else if (f == FlipperConfigurations::OFF_ON) {
      inputs.pmWS = ws;
    } else if (f == FlipperConfigurations::OFF_OFF || f == FlipperConfigurations::OFF) {
      inputs.ppWS = ws;
    } else {
      throw std::runtime_error(std::string{"Unknown entry in "} + Prop::FLIPPERS);
    }
  }
  return inputs;
}

/**
 * Solve in-place the 01 flipper configuration from the assumption that
 * for the corrected intensities, R01 = R10.
 * @param inputs a set of input workspaces
 * @param efficiencies a set of efficiency factors
 */
void PolarizationCorrectionWildes::threeInputsSolve01(WorkspaceMap &inputs, const EfficiencyMap &efficiencies) {
  inputs.pmWS = createWorkspaceWithHistory(inputs.mpWS);
  const auto &F1 = efficiencies.F1->y();
  const auto &F2 = efficiencies.F2->y();
  const auto &P1 = efficiencies.P1->y();
  const auto &P2 = efficiencies.P2->y();
  const auto nHisto = inputs.pmWS->getNumberHistograms();
  for (size_t wsIndex = 0; wsIndex != nHisto; ++wsIndex) {
    const auto &I00 = inputs.ppWS->y(wsIndex);
    auto &I01 = inputs.pmWS->mutableY(wsIndex);
    const auto &I10 = inputs.mpWS->y(wsIndex);
    const auto &I11 = inputs.mmWS->y(wsIndex);
    for (size_t binIndex = 0; binIndex != I00.size(); ++binIndex) {
      const auto f1 = F1[binIndex];
      const auto f2 = F2[binIndex];
      const auto p1 = P1[binIndex];
      const auto p2 = P2[binIndex];
      const auto i00 = I00[binIndex];
      const auto i10 = I10[binIndex];
      const auto i11 = I11[binIndex];
      I01[binIndex] =
          (f1 * i00 * (-1. + 2. * p1) - (i00 - i10 + i11) * (p1 - p2) - f2 * (i00 - i10) * (-1. + 2. * p2)) /
          (-p1 + f1 * (-1. + 2. * p1) + p2);
      // The errors are left to zero.
    }
  }
}

/**
 * Solve in-place the 10 flipper configuration from the assumption that
 * for the corrected intensities R01 = R10.
 * @param inputs a set of input workspaces
 * @param efficiencies a set of efficiency factors
 */
void PolarizationCorrectionWildes::threeInputsSolve10(WorkspaceMap &inputs, const EfficiencyMap &efficiencies) {
  inputs.mpWS = createWorkspaceWithHistory(inputs.pmWS);
  const auto &F1 = efficiencies.F1->y();
  const auto &F2 = efficiencies.F2->y();
  const auto &P1 = efficiencies.P1->y();
  const auto &P2 = efficiencies.P2->y();
  const auto nHisto = inputs.mpWS->getNumberHistograms();
  for (size_t wsIndex = 0; wsIndex != nHisto; ++wsIndex) {
    const auto &I00 = inputs.ppWS->y(wsIndex);
    const auto &I01 = inputs.pmWS->y(wsIndex);
    auto &I10 = inputs.mpWS->mutableY(wsIndex);
    const auto &I11 = inputs.mmWS->y(wsIndex);
    for (size_t binIndex = 0; binIndex != I00.size(); ++binIndex) {
      const auto f1 = F1[binIndex];
      const auto f2 = F2[binIndex];
      const auto p1 = P1[binIndex];
      const auto p2 = P2[binIndex];
      const auto i00 = I00[binIndex];
      const auto i01 = I01[binIndex];
      const auto i11 = I11[binIndex];
      I10[binIndex] =
          (-f1 * (i00 - i01) * (-1. + 2. * p1) + (i00 - i01 + i11) * (p1 - p2) + f2 * i00 * (-1. + 2. * p2)) /
          (p1 - p2 + f2 * (-1. + 2. * p2));
      // The errors are left to zero.
    }
  }
}

/**
 * Solve in-place the 01 and 10 flipper configurations from the assumption that
 * for the corrected intensities R01 = R10 = 0.
 * @param fullInputs a set of output workspaces
 * @param inputs a set of input workspaces
 * @param efficiencies a set of efficiency factors
 */
void PolarizationCorrectionWildes::twoInputsSolve01And10(WorkspaceMap &fullInputs, const WorkspaceMap &inputs,
                                                         const EfficiencyMap &efficiencies) {
  using namespace boost::math;
  const auto &F1 = efficiencies.F1->y();
  const auto &F1E = efficiencies.F1->e();
  const auto &F2 = efficiencies.F2->y();
  const auto &F2E = efficiencies.F2->e();
  const auto &P1 = efficiencies.P1->y();
  const auto &P1E = efficiencies.P1->e();
  const auto &P2 = efficiencies.P2->y();
  const auto &P2E = efficiencies.P2->e();
  const auto nHisto = inputs.mmWS->getNumberHistograms();
  for (size_t wsIndex = 0; wsIndex != nHisto; ++wsIndex) {
    const auto &I00 = inputs.ppWS->y(wsIndex);
    const auto &E00 = inputs.ppWS->e(wsIndex);
    const auto &I11 = inputs.mmWS->y(wsIndex);
    const auto &E11 = inputs.mmWS->e(wsIndex);
    auto &I01 = fullInputs.pmWS->mutableY(wsIndex);
    auto &E01 = fullInputs.pmWS->mutableE(wsIndex);
    auto &I10 = fullInputs.mpWS->mutableY(wsIndex);
    auto &E10 = fullInputs.mpWS->mutableE(wsIndex);
    for (size_t binIndex = 0; binIndex != I00.size(); ++binIndex) {
      const auto i00 = I00[binIndex];
      const auto i11 = I11[binIndex];
      const auto f1 = F1[binIndex];
      const auto f2 = F2[binIndex];
      const auto p1 = P1[binIndex];
      const auto p2 = P2[binIndex];
      const auto a = -1. + p1 + 2. * p2 - 2. * p1 * p2;
      const auto b = -1. + 2. * p1;
      const auto c = -1. + 2. * p2;
      const auto d = -1. + p2;
      // Case: 01
      const auto divisor = f2 * p1 * a + f1 * b * (-d * p2 + f2 * (p1 + d) * c);
      I01[binIndex] = (f2 * i11 * p1 * a - f1 * i00 * b * (-f2 * pow<2>(c) + pow<2>(f2 * c) + d * p2)) / divisor;
      E01[binIndex] = twoInputsErrorEstimate01(i00, E00[binIndex], i11, E11[binIndex], p1, P1E[binIndex], p2,
                                               P2E[binIndex], f1, F1E[binIndex], f2, F2E[binIndex]);
      // Case: 10
      I10[binIndex] =
          (-pow<2>(f1) * f2 * i00 * pow<2>(b) * c + f2 * i00 * p1 * a + f1 * b * (-i11 * d * p2 + f2 * i00 * b * c)) /
          divisor;
      E10[binIndex] = twoInputsErrorEstimate10(i00, E00[binIndex], i11, E11[binIndex], p1, P1E[binIndex], p2,
                                               P2E[binIndex], f1, F1E[binIndex], f2, F2E[binIndex]);
    }
  }
}
} // namespace Mantid::Algorithms
