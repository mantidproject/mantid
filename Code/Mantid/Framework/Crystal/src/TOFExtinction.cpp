/*WIKI* 


*WIKI*/
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidCrystal/TOFExtinction.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Statistics.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include <fstream>

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(TOFExtinction)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  TOFExtinction::TOFExtinction()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  TOFExtinction::~TOFExtinction()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void TOFExtinction::initDocs()
  {
    this->setWikiSummary("Sorts a PeaksWorkspace by HKL.");
    this->setOptionalMessage("Sorts a PeaksWorkspace by HKL.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void TOFExtinction::init()
  {
    declareProperty(new WorkspaceProperty<PeaksWorkspace>("InputWorkspace","",Direction::InOut),
        "An input PeaksWorkspace with an instrument.");
    declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace","",Direction::Output));
    BoundedValidator<double> *mustBePositive = new BoundedValidator<double> ();
    mustBePositive->setLower(0.0);
    declareProperty("LinearScatteringCoef", -1.0, mustBePositive,
      "Linear scattering coefficient in 1/cm");
    declareProperty("LinearAbsorptionCoef", -1.0, mustBePositive->clone(),
      "Linear absorption coefficient at 1.8 Angstroms in 1/cm");
    declareProperty("Radius", 0.10, "Radius of spherical crystal in cm");
    declareProperty("Mosaic", 0.262, "Mosaic Spread (FWHM) (Degrees)");
    declareProperty("Cell", 255.0, "Unit Cell Volume (Angstroms^3)");
    declareProperty("RCrystallite", 6.0, "Becker-Coppens Crystallite Radius (micron)");
    declareProperty("MinD", 0.40, "Minimum d-spacing (Angstroms)");
    declareProperty("ScaleFactor", 1.2, "Multiply FSQ and sig(FSQ) by scaleFactor");
    declareProperty("MinWL", 0.45, "Minimum wavelength (Angstroms)");
    declareProperty("MaxWL", 3.6, "Maximum wavelength (Angstroms)");

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void TOFExtinction::exec()
  {

    PeaksWorkspace_sptr peaksW = getProperty("InputWorkspace");

    int NumberPeaks = peaksW->getNumberPeaks();
    double mosaic = getProperty("Mosaic");
    double cell = getProperty("Cell");
    double r_crystallite = getProperty("RCrystallite");
    double dMin = getProperty("MinD");
    double scaleFactor = getProperty("ScaleFactor");
    double wlMin = getProperty("MinWL");
    double wlMax = getProperty("MaxWL");
    double Eg = getEg(mosaic); // defined by Zachariasen, W. H. (1967). Acta Cryst. A23, 558
    for (int i = 0; i < NumberPeaks; i++)
    {
      Peak & peak1 = peaksW->getPeaks()[i];
      double h = peak1.getH();
      double k = peak1.getK();
      double l = peak1.getL();
      double fsq = peak1.getIntensity()*scaleFactor;
      double sigfsq = peak1.getSigmaIntensity()*scaleFactor;
      double wl = peak1.getWavelength();
      double dsp = peak1.getDSpacing();
      double twoth = peak1.getScattering();
      double tbar = 0.0;
      double transmission = absor_sphere(twoth, wl, tbar);
      std::cout << twoth<<"  "<<wl <<"  "<<tbar<<"\n";
      //if(dsp < dMin || wl < wlMin || wl > wlMax) continue;
      // Extinction Correction
      // Use measured Fo_squared in first iteration
      // y_corr = extinctionCorrection(mosaic, wl, fsq, tbar, twoth, cell, r_crystallite)

      double Xqt = getXqt(Eg, cell, wl, twoth, tbar, fsq);

      double y_corr = getTypeIZachariasen(Xqt);
      //double y_corrI_G =getTypeIGaussian(Xqt, twoth);
      //double y_corrI_L = getTypeILorentzian(Xqt, twoth);

      // Test for Type-II
      //double Rg = getRg(Eg, r_crystallite, wl, twoth); // defined by Zachariasen, W. H. (1967). Acta Cryst. A23, 558
      //double XqtII = getXqtII(Rg, cell, wl, twoth, tbar, fsq);
      //double y_corrII_Z = getTypeIIZachariasen(XqtII);
      //double y_corrII_G = getTypeIIGaussian(XqtII, twoth);
      //double y_corrII_L = getTypeIILorentzian(XqtII, twoth);
      // Apply correction to fsq with Type-I Z for testing
      //double y_corr = y_corrI_Z;
      double fsq_ys = fsq * y_corr;
      double sigfsq_ys = sigfsq * y_corr;
      sigfsq_ys = std::sqrt(1+sigfsq_ys*sigfsq_ys+std::pow(0.005*sigfsq_ys,2));   // Ad a constant 1 for background and 0.5% of Fsq for instrument error

      peak1.setIntensity(fsq_ys);
      peak1.setSigmaIntensity(sigfsq_ys);

      // output reflection to log file and to hkl file
      /*printf('%4d%4d%4d%8.2f%8.2f%4d%8.4f%7.4f%7d%7d%7.4f%4d%9.5f%9.5f%9.5f%9.5f%9.5f%9.5f%9.5f%9.5f%9.5f\n' 
                , (h, k, l, fsq_ys, sigfsq_ys, hstnum, wl, tbar, curhst, seqnum, transmission, dn, twoth, dsp, 
                y_corr, y_corrI_Z, y_corrI_G, y_corrI_L, y_corrII_Z, y_corrII_G, y_corrII_L));

        // last record all zeros for shelx
        std::string lastLine = '   0   0   0    0.00    0.00   0  0.0000 0.0000      0      0 0.0000   0  0.0000   0.00000  0.00000';

        printf(lastLine);*/

    }
    setProperty("OutputWorkspace",peaksW);

  }
  double TOFExtinction::getEg(double mosaic)
  {
        double Eg = 2.0*std::sqrt(std::log(2)/(2*M_PI))/(mosaic*M_PI/180.0);
        return Eg;
  }
;
  double TOFExtinction::getXqt(double Eg, double cellV, double wl, double twoth, double tbar, double fsq)
  {
        // Xqt calculated from measured Fsqr;
        double beta = Eg / std::pow(cellV,2) * std::pow(wl,4) / std::pow((std::sin(twoth/2)),2) * tbar * fsq/10;
        double Xqt = std::pow(beta,2) + beta * std::sqrt(std::pow(beta,2) + 1);
        return Xqt;
  }
  double TOFExtinction::getTypeIZachariasen(double Xqt)
  {
        // TYPE-I, Zachariasen, W. H. (1967). Acta Cryst. A23, 558 ;
        double y_ext = std::sqrt(1 + 2 * Xqt);
        return y_ext;
  }
;
  double TOFExtinction::getTypeIGaussian(double Xqt, double twoth)
  {
        // Type-I, Gaussian, Becker, P. J. & Coppens, P. (1974). Acta Cryst. A30, 129;
        double y_ext =std::sqrt(1 + 2*Xqt + (0.58 + 0.48*std::cos(twoth) + 0.24*std::pow((std::cos(twoth)),2))*std::pow(Xqt,2)/(1 + (0.02 - 0.025*std::cos(twoth))*Xqt));
        return y_ext;
  }
;
  double TOFExtinction::getTypeILorentzian(double Xqt, double twoth)
  {
        //TYPE-I Lorentzian, Becker, P. J. & Coppens, P. (1974). Acta Cryst. A30, 129;
        double y_ext;
        if (twoth < M_PI)
             y_ext =std::sqrt(1+2*Xqt+(0.025+0.285*std::cos(twoth))*std::pow(Xqt,2)/(1 + 0.15*Xqt-0.2*std::pow((0.75-std::cos(twoth)),2)*Xqt));
        else
             y_ext =std::sqrt(1+2*Xqt+(0.025+0.285*std::cos(twoth))*std::pow(Xqt,2)/(1-0.45*Xqt*std::cos(twoth)));
        return y_ext;
  }
;
        // Type-II extinction correction;
  double TOFExtinction::getRg(double Eg, double r_crystallite, double wl, double twoth)
  {
        double r = r_crystallite; // micron
        // Two-theta dependence by Becker & Coppens, Acta Cryst A 30, 129 (1974)
        // The factor is std::pow((std::sin(twoth/2)/wl),2) for tof neutron 
        double Es = r*std::pow((1000.0*std::sin(twoth/2)/wl),2);
        double Rg = Es/std::sqrt(1+std::pow((Es/Eg),2));
        return Rg;
  }
;
  double TOFExtinction::getXqtII(double Rg, double cellV, double wl, double twoth, double tbar, double fsq)
  {
        double betaII = Rg / std::pow(cellV,2) * std::pow(wl,4) / std::pow((std::sin(twoth/2)),2) * tbar * fsq/10;
        double XqtII = std::pow(betaII,2) + betaII * std::sqrt(std::pow(betaII,2) + 1);
        return XqtII;
  }
;
  double TOFExtinction::getTypeIIZachariasen(double XqtII)
  {
        // TYPE-II, Zachariasen, W. H. (1967). Acta Cryst. A23, 558 ;
        double y_ext_II = std::sqrt(1 + 2 * XqtII);
        return y_ext_II;
  }
;
  double TOFExtinction::getTypeIIGaussian(double XqtII, double twoth)
  {
        //Becker, P. J. & Coppens, P. (1974). Acta Cryst. A30, 129;
        double y_ext_II =std::sqrt(1 + 2*XqtII + (0.58 + 0.48*std::cos(twoth) + 0.24*std::pow((std::cos(twoth)),2))*std::pow(XqtII,2)/(1 + (0.02 - 0.025*std::cos(twoth))*XqtII));
        return y_ext_II;
  }
;
  double TOFExtinction::getTypeIILorentzian(double XqtII, double twoth)
  {
        //TYPE-II Lorentzian, Becker, P. J. & Coppens, P. (1974). Acta Cryst. A30, 129;
        double y_ext_II;
        if (twoth < M_PI)
             y_ext_II =std::sqrt(1+2*XqtII+(0.025+0.285*std::cos(twoth))*std::pow(XqtII,2)/(1 + 0.15*XqtII-0.2*std::pow((0.75-std::cos(twoth)),2)*XqtII));
        else
             y_ext_II =std::sqrt(1+2*XqtII+(0.025+0.285*std::cos(twoth))*std::pow(XqtII,2)/(1-0.45*XqtII*std::cos(twoth)));
        return y_ext_II;
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
  double TOFExtinction::absor_sphere(double& twoth, double& wl, double& tbar)
  {
    int i;
    double mu, mur;         //mu is the linear absorption coefficient,
                            //r is the radius of the spherical sample.
    double theta,astar1,astar2,frac,astar;
    double trans;

//  For each of the 19 theta values in dwiggins (theta = 0.0 to 90.0
//  in steps of 5.0 deg.), the astar values vs.mur were fit to a third
//  order polynomial in excel. these values are given in the static array
//  pc[][]

    double smu = getProperty("LinearScatteringCoef");
    double amu = getProperty("LinearAbsorptionCoef");
    double radius = getProperty("Radius");
    mu = smu + (amu/1.8f)*wl;

    mur = mu*radius;
    if (mur < 0. || mur > 2.5)
    {
      std::ostringstream s;
      s << mur;
      throw std::runtime_error("muR is not in range of Dwiggins' table :" + s.str());
    }

    theta = twoth*radtodeg_half;
    if (theta < 0. || theta > 90.)
    {
      std::ostringstream s;
      s << theta;
      throw std::runtime_error("theta is not in range of Dwiggins' table :" + s.str());
    }

//  using the polymial coefficients, calulate astar (= 1/transmission) at
//  theta values below and above the actual theta value.

    i = (int)(theta/5.);
    astar1 = pc[0][i] + mur * (pc[1][i] + mur * (pc[2][i] + pc[3][i] * mur));

    i = i+1;
    astar2 = pc[0][i] + mur * (pc[1][i] + mur * (pc[2][i] + pc[3][i] * mur));

//  do a linear interpolation between theta values.

    frac = theta - static_cast<double>(static_cast<int>( theta / 5. )) * 5.;//theta%5.
    frac = frac/5.;

    astar = astar1*(1-frac) + astar2*frac;       // astar is the correction
    trans = 1.f/astar;                           // trans is the transmission
                                                 // trans = exp(-mu*tbar)

//  calculate tbar as defined by coppens.
    tbar = -(double)std::log(trans)/mu;

    return trans;
  }



} // namespace Mantid
} // namespace Crystal

