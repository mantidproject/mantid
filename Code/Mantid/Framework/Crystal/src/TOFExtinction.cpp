#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidCrystal/TOFExtinction.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/ListValidator.h"
#include <fstream>

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::PhysicalConstants;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
// DECLARE_ALGORITHM(TOFExtinction)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
TOFExtinction::TOFExtinction() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
TOFExtinction::~TOFExtinction() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void TOFExtinction::init() {
  declareProperty(new WorkspaceProperty<PeaksWorkspace>("InputWorkspace", "",
                                                        Direction::InOut),
                  "An input PeaksWorkspace with an instrument.");
  declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace", "",
                                                        Direction::Output));
  std::vector<std::string> corrOptions;
  corrOptions.push_back("Type I Zachariasen");
  corrOptions.push_back("Type I Gaussian");
  corrOptions.push_back("Type I Lorentzian");
  corrOptions.push_back("Type II Zachariasen");
  corrOptions.push_back("Type II Gaussian");
  corrOptions.push_back("Type II Lorentzian");
  corrOptions.push_back("Type I&II Zachariasen");
  corrOptions.push_back("Type I&II Gaussian");
  corrOptions.push_back("Type I&II Lorentzian");
  corrOptions.push_back("None, Scaling Only");
  declareProperty("ExtinctionCorrectionType", corrOptions[0],
                  boost::make_shared<StringListValidator>(corrOptions),
                  "Select the type of extinction correction.");

  declareProperty("Mosaic", 0.262, "Mosaic Spread (FWHM) (Degrees)");
  declareProperty("Cell", 255.0, "Unit Cell Volume (Angstroms^3)");
  declareProperty("RCrystallite", 6.0,
                  "Becker-Coppens Crystallite Radius (micron)");
  declareProperty("ScaleFactor", 1.0,
                  "Multiply FSQ and sig(FSQ) by scaleFactor");
  declareProperty("DivBeam", 0.005, "Minimum beam divergence in radian");
  declareProperty("BetaBeam", 0.002,
                  "Wavelength dependence of beam divergence");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void TOFExtinction::exec() {

  PeaksWorkspace_sptr inPeaksW = getProperty("InputWorkspace");
  /// Output peaks workspace, create if needed
  PeaksWorkspace_sptr peaksW = getProperty("OutputWorkspace");
  if (peaksW != inPeaksW)
    peaksW = inPeaksW->clone();

  const Kernel::Material *m_sampleMaterial =
      &(inPeaksW->sample().getMaterial());
  if (m_sampleMaterial->totalScatterXSection(NeutronAtom::ReferenceLambda) !=
      0.0) {
    double rho = m_sampleMaterial->numberDensity();
    smu = m_sampleMaterial->totalScatterXSection(NeutronAtom::ReferenceLambda) *
          rho;
    amu = m_sampleMaterial->absorbXSection(NeutronAtom::ReferenceLambda) * rho;
  } else {
    throw std::invalid_argument(
        "Could not retrieve LinearScatteringCoef from material");
  }
  const API::Run &run = inPeaksW->run();
  if (run.hasProperty("Radius")) {
    Kernel::Property *prop = run.getProperty("Radius");
    radius = boost::lexical_cast<double, std::string>(prop->value());
  } else {
    throw std::invalid_argument("Could not retrieve Radius from run object");
  }

  std::string cType = getProperty("ExtinctionCorrectionType");
  int NumberPeaks = peaksW->getNumberPeaks();
  double mosaic = getProperty("Mosaic");
  double cell = getProperty("Cell");
  double r_crystallite = getProperty("RCrystallite");
  double scaleFactor = getProperty("ScaleFactor");
  double divBeam = getProperty("DivBeam");
  double betaBeam = getProperty("BetaBeam");
  double Eg = getEg(
      mosaic); // defined by Zachariasen, W. H. (1967). Acta Cryst. A23, 558
  double y_corr = 1.0;
  double sigfsq_ys = 0.0;
  for (int i = 0; i < NumberPeaks; i++) {
    Peak &peak1 = peaksW->getPeaks()[i];
    double fsq = peak1.getIntensity() * scaleFactor;
    double sigfsq = peak1.getSigmaIntensity() * scaleFactor;
    double wl = peak1.getWavelength();
    double twoth = peak1.getScattering();
    double tbar = absor_sphere(twoth, wl);
    // Extinction Correction

    if (cType.compare("Type I Zachariasen") == 0) {
      // Apply correction to fsq with Type-I Z for testing
      double EgLaueI = getEgLaue(Eg, twoth, wl, divBeam, betaBeam);
      double Xqt = getXqt(EgLaueI, cell, wl, twoth, tbar, fsq);
      y_corr = getZachariasen(Xqt);
      sigfsq_ys = getSigFsqr(EgLaueI, cell, wl, twoth, tbar, fsq, sigfsq);
    } else if (cType.compare("Type I Gaussian") == 0) {
      // Apply correction to fsq with Type-I BCG for testing
      double EgLaueI = std::sqrt(2.0) *
                       getEgLaue(Eg, twoth, wl, divBeam, betaBeam) * 2.0 / 3.0;
      double Xqt = getXqt(EgLaueI, cell, wl, twoth, tbar, fsq);
      y_corr = getGaussian(Xqt, twoth);
      sigfsq_ys = getSigFsqr(EgLaueI, cell, wl, twoth, tbar, fsq, sigfsq);
    } else if (cType.compare("Type I Lorentzian") == 0) {
      // Apply correction to fsq with Type-I BCL for testing
      double EgLaueI = getEgLaue(Eg, twoth, wl, divBeam, betaBeam);
      double Xqt = getXqt(EgLaueI, cell, wl, twoth, tbar, fsq);
      y_corr = getLorentzian(Xqt, twoth);
      sigfsq_ys = getSigFsqr(EgLaueI, cell, wl, twoth, tbar, fsq, sigfsq);
    } else if (cType.compare("Type II Zachariasen") == 0) {
      // Apply correction to fsq with Type-II Z for testing
      double EsLaue = getEgLaue(r_crystallite, twoth, wl, divBeam, betaBeam);
      double Xqt = getXqt(EsLaue, cell, wl, twoth, tbar, fsq);
      y_corr = getZachariasen(Xqt);
      sigfsq_ys = getSigFsqr(EsLaue, cell, wl, twoth, tbar, fsq, sigfsq);
    } else if (cType.compare("Type II Gaussian") == 0) {
      // Apply correction to fsq with Type-II BCG for testing
      double EsLaue = getEgLaue(r_crystallite, twoth, wl, divBeam, betaBeam);
      double Xqt = getXqt(EsLaue, cell, wl, twoth, tbar, fsq);
      y_corr = getGaussian(Xqt, twoth);
      sigfsq_ys = getSigFsqr(EsLaue, cell, wl, twoth, tbar, fsq, sigfsq);
    } else if (cType.compare("Type II Lorentzian") == 0) {
      // Apply correction to fsq with Type-II BCL for testing
      double EsLaue = getEgLaue(r_crystallite, twoth, wl, divBeam, betaBeam);
      double Xqt = getXqt(EsLaue, cell, wl, twoth, tbar, fsq);
      y_corr = getLorentzian(Xqt, twoth);
      sigfsq_ys = getSigFsqr(EsLaue, cell, wl, twoth, tbar, fsq, sigfsq);
    } else if (cType.compare("Type I&II Zachariasen") == 0) {
      // Apply correction to fsq with Type-II Z for testing
      double EgLaueI = getEgLaue(Eg, twoth, wl, divBeam, betaBeam);
      double EsLaue = getEgLaue(r_crystallite, twoth, wl, divBeam, betaBeam);
      double Rg = getRg(EgLaueI, EsLaue, wl, twoth);
      double Xqt = getXqtII(Rg, cell, wl, twoth, tbar, fsq);
      y_corr = getTypeIIZachariasen(Xqt);
      sigfsq_ys = getSigFsqr(EsLaue, cell, wl, twoth, tbar, fsq, sigfsq);
    } else if (cType.compare("Type I&II Gaussian") == 0) {
      // Apply correction to fsq with Type-II BCG for testing
      double EgLaueI = getEgLaue(Eg, twoth, wl, divBeam, betaBeam);
      double Rg = getRgGaussian(EgLaueI, r_crystallite, wl, twoth);
      double Xqt = getXqtII(Rg, cell, wl, twoth, tbar, fsq);
      y_corr = getTypeIIGaussian(Xqt, twoth);
      sigfsq_ys = getSigFsqr(Rg, cell, wl, twoth, tbar, fsq, sigfsq);
    } else if (cType.compare("Type I&II Lorentzian") == 0) {
      // Apply correction to fsq with Type-II BCL for testing
      double EgLaueI = getEgLaue(Eg, twoth, wl, divBeam, betaBeam);
      double Rg = getRgLorentzian(EgLaueI, r_crystallite, wl, twoth);
      double Xqt = getXqtII(Rg, cell, wl, twoth, tbar, fsq);
      y_corr = getTypeIILorentzian(Xqt, twoth);
      sigfsq_ys = getSigFsqr(Rg, cell, wl, twoth, tbar, fsq, sigfsq);
    } else if (cType.compare("None, Scaling Only") == 0) {
      y_corr = 1.0; // No extinction correction
      sigfsq_ys = sigfsq;
    }

    double ys = fsq / y_corr;
    // std::cout << fsq << "  " << y_corr<<"  "<<wl<<"  "<<twoth<<"  "<<tbar<< "
    // " << ys <<"\n";
    if (!boost::math::isnan(ys))
      peak1.setIntensity(ys);
    else
      peak1.setIntensity(0.0);
    sigfsq_ys =
        std::sqrt(1.0 + sigfsq_ys * sigfsq_ys + std::pow(0.005 * sigfsq_ys, 2));
    peak1.setSigmaIntensity(sigfsq_ys);

    // output reflection to log file and to hkl file with SaveHKL
  }
  setProperty("OutputWorkspace", peaksW);
}
double TOFExtinction::getEg(double mosaic) {
  double Eg = 2.0 * std::sqrt(std::log(static_cast<double>(2.0)) / (2 * M_PI)) /
              (mosaic * M_PI / 180.0);
  return Eg;
}
double TOFExtinction::getEgLaue(double Eg, double twoth, double wl,
                                double divBeam, double betaBeam) {
  // divbeam is the default [minimum] beam divergence in radian.
  // Tomiyoshi, Yamada and Watanabe
  UNUSED_ARG(wl); // Keep arguments consistent with tofExtinction.py
  double EgLaue =
      1.0 / std::sqrt(std::pow(betaBeam * std::tan(twoth / 2.0), 2) +
                      std::pow(divBeam, 2) + 1.0 / std::pow(Eg, 2));
  return EgLaue;
}
double TOFExtinction::getXqt(double Eg, double cellV, double wl, double twoth,
                             double tbar, double fsq) {
  // Xqt calculated from measured Fsqr;
  // Maslen & Spadaccini
  double beta = Eg / std::pow(cellV, 2) * std::pow(wl, 4) / 2 /
                std::pow((std::sin(twoth / 2)), 2) * tbar * fsq / 10;
  double Xqt = std::pow(beta, 2) + beta * std::sqrt(std::pow(beta, 2) + 1);
  return Xqt;
}
double TOFExtinction::getZachariasen(double Xqt) {
  // TYPE-I, Zachariasen, W. H. (1967). Acta Cryst. A23, 558 ;
  double y_ext = std::sqrt(1 + 2 * Xqt);
  return y_ext;
}
double TOFExtinction::getGaussian(double Xqt, double twoth) {
  if (Xqt < 0.0 || Xqt > 30.0)
    return 1;
  // Type-I, Gaussian, Becker, P. J. & Coppens, P. (1974). Acta Cryst. A30, 129;
  double y_ext = std::sqrt(
      1 + 2 * Xqt +
      (0.58 + 0.48 * std::cos(twoth) + 0.24 * std::pow((std::cos(twoth)), 2)) *
          std::pow(Xqt, 2) / (1 + (0.02 - 0.025 * std::cos(twoth)) * Xqt));
  return y_ext;
}
double TOFExtinction::getLorentzian(double Xqt, double twoth) {
  // TYPE-I Lorentzian, Becker, P. J. & Coppens, P. (1974). Acta Cryst. A30,
  // 129;
  double y_ext;
  if (twoth < M_PI / 2.0)
    y_ext = std::sqrt(1 + 2 * Xqt +
                      (0.025 + 0.285 * std::cos(twoth)) * std::pow(Xqt, 2) /
                          (1 + 0.15 * Xqt -
                           0.2 * std::pow((0.75 - std::cos(twoth)), 2) * Xqt));
  else
    y_ext = std::sqrt(1 + 2 * Xqt + (0.025 + 0.285 * std::cos(twoth)) *
                                        std::pow(Xqt, 2) /
                                        (1 - 0.45 * Xqt * std::cos(twoth)));
  return y_ext;
}
double TOFExtinction::getEsLaue(double r, double twoth, double wl) {
  // Type II mosaic distribution radius in micron
  // Tomiyoshi, Yamada and Watanabe
  double EsLaue = r * 10000.0 * 2.0 * std::pow(std::sin(twoth / 2 / wl), 2);
  return EsLaue;
}
double TOFExtinction::getRg(double EgLaue, double EsLaue, double wl,
                            double twoth) {
  UNUSED_ARG(wl)
  UNUSED_ARG(twoth)
  // Two-theta dependence by Becker & Coppens, Acta Cryst A 30, 129 (1974)
  // The factor is std::pow((std::sin(twoth/2)/wl),2) for tof neutron
  // double Es = r*std::pow((1000.0*std::sin(twoth/2)/wl),2);
  double Rg = EsLaue / std::sqrt(1 + EsLaue * EsLaue / EgLaue / EgLaue);
  return Rg;
}
double TOFExtinction::getRgGaussian(double EgLaue, double r_crystallite,
                                    double wl, double twoth) {
  // Combined Type I and Type II correction by Becker & Coppens
  double r = r_crystallite; // micron
  double Es = 1.5 * r * 10000 * 2 * pow(std::sin(twoth / 2) / wl, 2);
  double RgGaussian = Es / std::sqrt(1.0 + Es * Es / EgLaue / EgLaue / 2.0);
  RgGaussian = 2.0 / 3.0 * RgGaussian;
  return RgGaussian;
}
double TOFExtinction::getRgLorentzian(double EgLaue, double r_crystallite,
                                      double wl, double twoth) {
  // Combined Type I and Type II correction by Becker & Coppens
  double r = r_crystallite; // micron
  double Es = 1.5 * r * 10000 * 2 * pow(std::sin(twoth / 2) / wl, 2);
  double RgLorentzian = Es / (1 + 2 * Es / EgLaue / 3.0);
  RgLorentzian = 2.0 / 3.0 * RgLorentzian;
  return RgLorentzian;
}
double TOFExtinction::getXqtII(double Rg, double cellV, double wl, double twoth,
                               double tbar, double fsq) {
  double betaII = Rg / std::pow(cellV, 2) * std::pow(wl, 4) / 2.0 /
                  std::pow((std::sin(twoth / 2)), 2) * tbar * fsq / 10;
  double XqtII =
      std::pow(betaII, 2) + betaII * std::sqrt(std::pow(betaII, 2) + 1);
  return XqtII;
}
double TOFExtinction::getTypeIIZachariasen(double XqtII) {
  // TYPE-II, Zachariasen, W. H. (1967). Acta Cryst. A23, 558 ;
  double y_ext_II = std::sqrt(1 + 2 * XqtII);
  return y_ext_II;
}
double TOFExtinction::getTypeIIGaussian(double XqtII, double twoth) {
  if (XqtII < 0.0 || XqtII > 30.0)
    return 1;
  // Becker, P. J. & Coppens, P. (1974). Acta Cryst. A30, 129;
  double y_ext_II = std::sqrt(
      1 + 2 * XqtII +
      (0.58 + 0.48 * std::cos(twoth) + 0.24 * std::pow((std::cos(twoth)), 2)) *
          std::pow(XqtII, 2) / (1 + (0.02 - 0.025 * std::cos(twoth)) * XqtII));
  return y_ext_II;
}
double TOFExtinction::getTypeIILorentzian(double XqtII, double twoth) {
  // TYPE-II Lorentzian, Becker, P. J. & Coppens, P. (1974). Acta Cryst. A30,
  // 129;
  double y_ext_II;
  if (twoth < M_PI)
    y_ext_II =
        std::sqrt(1 + 2 * XqtII +
                  (0.025 + 0.285 * std::cos(twoth)) * std::pow(XqtII, 2) /
                      (1 + 0.15 * XqtII -
                       0.2 * std::pow((0.75 - std::cos(twoth)), 2) * XqtII));
  else
    y_ext_II = std::sqrt(
        1 + 2 * XqtII + (0.025 + 0.285 * std::cos(twoth)) * std::pow(XqtII, 2) /
                            (1 - 0.45 * XqtII * std::cos(twoth)));
  return y_ext_II;
}
double TOFExtinction::getSigFsqr(double Rg, double cellV, double wl,
                                 double twoth, double tbar, double fsq,
                                 double sigfsq, double relSigRg) {
  double sig_Rg = relSigRg * Rg; // Estimated
  double beta = Rg / std::pow(cellV, 2) * std::pow(wl, 4) / 2 /
                std::pow(sin(twoth / 2), 2) * tbar * fsq / 10;
  double bb = beta * beta;
  double sigSqr =
      std::pow(2 * beta + bb / std::sqrt(bb + 1) + std::sqrt(bb + 1), 2) *
          sigfsq * sigfsq +
      fsq * fsq * std::pow(beta / Rg, 2) *
          std::pow(1 + beta / std::sqrt(bb + 1), 2) * sig_Rg * sig_Rg;
  double sig = std::sqrt(sigSqr);
  return sig;
}
/**
 *       function to calculate a spherical absorption correction
 *       and tbar. based on values in:
 *
 *       c. w. dwiggins, jr., acta cryst. a31, 395 (1975).
 *
 *       in this paper, a is the transmission and a* = 1/a is
 *       the absorption correction.
 *
 *       input are the smu (scattering) and amu (absorption at 1.8 ang.)
 *       linear absorption coefficients, the radius r of the sample
 *       the theta angle and wavelength.
 *       the absorption (absn) and tbar are returned.
 *
 *       a. j. schultz, june, 2008
 */
double TOFExtinction::absor_sphere(double &twoth, double &wl) {
  int i;
  double mu, mur; // mu is the linear absorption coefficient,
  // r is the radius of the spherical sample.
  double theta, astar1, astar2, frac, astar;
  double trans;

  //  For each of the 19 theta values in dwiggins (theta = 0.0 to 90.0
  //  in steps of 5.0 deg.), the astar values vs.mur were fit to a third
  //  order polynomial in excel. these values are given in the static array
  //  pc[][]

  mu = smu + (amu / 1.8f) * wl;

  mur = mu * radius;
  if (mur < 0. || mur > 2.5) {
    std::ostringstream s;
    s << mur;
    throw std::runtime_error("muR is not in range of Dwiggins' table :" +
                             s.str());
  }

  theta = twoth * radtodeg_half;
  if (theta < 0. || theta > 90.) {
    std::ostringstream s;
    s << theta;
    throw std::runtime_error("theta is not in range of Dwiggins' table :" +
                             s.str());
  }

  //  using the polymial coefficients, calulate astar (= 1/transmission) at
  //  theta values below and above the actual theta value.

  i = (int)(theta / 5.);
  astar1 = pc[0][i] + mur * (pc[1][i] + mur * (pc[2][i] + pc[3][i] * mur));

  i = i + 1;
  astar2 = pc[0][i] + mur * (pc[1][i] + mur * (pc[2][i] + pc[3][i] * mur));

  //  do a linear interpolation between theta values.

  frac = theta -
         static_cast<double>(static_cast<int>(theta / 5.)) * 5.; // theta%5.
  frac = frac / 5.;

  astar = astar1 * (1 - frac) + astar2 * frac; // astar is the correction
  trans = 1.f / astar;                         // trans is the transmission
                                               // trans = exp(-mu*tbar)

  //  calculate tbar as defined by coppens.
  double tbar;
  if (mu == 0.0)
    tbar = 0.0;
  else
    tbar = -(double)std::log(trans) / mu;

  return tbar;
}

} // namespace Mantid
} // namespace Crystal
