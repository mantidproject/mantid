/*WIKI* 
Used same format that works successfully in GSAS and SHELX from ISAW:
	
hklFile.write('%4d%4d%4d%8.2f%8.2f%4d%8.4f%7.4f%7d%7d%7.4f%4d%9.5f%9.4f\n'% (H, K, L, FSQ, SIGFSQ, hstnum, WL, TBAR, CURHST, SEQNUM, TRANSMISSION, DN, TWOTH, DSP))
	
HKL is flipped by -1 due to different q convention in ISAW vs Mantid.
	
FSQ = integrated intensity of peak (scaled)
	
SIGFSQ = sigma from integrating peak
	
hstnum = number of sample orientation (starting at 1)
	
WL = wavelength of peak
	
TBAR = output of absorption correction (-log(transmission)/mu)
	
CURHST = run number of sample
	
SEQNUM = peak number (unique number for each peak in file)
	
TRANSMISSION = output of absorption correction (exp(-mu*tbar))
	
DN = detector bank number
	
TWOTH = two-theta scattering angle
	
DSP = d-Spacing of peak (Angstroms)/TR

Last line must have all 0's



*WIKI*/
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidCrystal/SaveHKL.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/BoundedValidator.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::PhysicalConstants;

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SaveHKL)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveHKL::SaveHKL()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveHKL::~SaveHKL()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SaveHKL::initDocs()
  {
    this->setWikiSummary("Save a PeaksWorkspace to a ASCII .hkl file.");
    this->setOptionalMessage("Save a PeaksWorkspace to a ASCII .hkl file.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SaveHKL::init()
  {
    declareProperty(new WorkspaceProperty<PeaksWorkspace>("InputWorkspace","",Direction::Input),
        "An input PeaksWorkspace.");

    auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
    mustBePositive->setLower(0.0);
    declareProperty("ScalePeaks", 1.0, mustBePositive,
      "Multiply FSQ and sig(FSQ) by scaleFactor");
    declareProperty("MinDSpacing", 0.0, "Minimum d-spacing (Angstroms)");
    declareProperty("MinWavelength", 0.0, "Minimum wavelength (Angstroms)");
    declareProperty("MaxWavelength", 100.0, "Maximum wavelength (Angstroms)");

    declareProperty("AppendFile", false, "Append to file if true.\n"
      "If false, new file (default).");
    declareProperty("ApplyAnvredCorrections", false, "Apply anvred corrections to peaks if true.\n"
      "If false, no corrections during save (default).");

    std::vector<std::string> exts;
    exts.push_back(".hkl");

    declareProperty(new FileProperty("Filename", "", FileProperty::Save, exts),
        "Path to an hkl file to save.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SaveHKL::exec()
  {

    std::string filename = getPropertyValue("Filename");
    PeaksWorkspace_sptr ws = getProperty("InputWorkspace");

    const Kernel::Material *m_sampleMaterial = &(ws->sample().getMaterial());
    if( m_sampleMaterial->totalScatterXSection(NeutronAtom::ReferenceLambda) != 0.0)
    {
  	  double rho =  m_sampleMaterial->numberDensity();
  	  smu =  m_sampleMaterial->totalScatterXSection(NeutronAtom::ReferenceLambda) * rho;
  	  amu = m_sampleMaterial->absorbXSection(NeutronAtom::ReferenceLambda) * rho;
    }
    else
    {
    	smu = 0.;
    	amu = 0.;
    }
    const API::Run & run = ws->run();
    if ( run.hasProperty("Radius") )
    {
      Kernel::Property* prop = run.getProperty("Radius");
      radius = boost::lexical_cast<double,std::string>(prop->value());
    }
    else
    {
      radius = 0.0;
    }

    double scaleFactor = getProperty("ScalePeaks"); 
    double dMin = getProperty("MinDSpacing");
    double wlMin = getProperty("MinWavelength");
    double wlMax = getProperty("MaxWavelength");

    // Sequence and run number
    int seqNum = 1;
    int firstrun = 0;
    int bankold = -1;

    std::fstream out;
    bool append = getProperty("AppendFile");
    if (append)
    {
      out.open( filename.c_str(), std::ios::in|std::ios::out|std::ios::ate);
      std::streamoff pos = out.tellp();
      out.seekp (28);
      out >> firstrun;
      out.seekp (pos - 110);
      out >> seqNum;
      out.seekp (pos - 73);
      seqNum ++;
    }
    else
    {
      out.open( filename.c_str(), std::ios::out);
    }

    // We must sort the peaks by bank #
    std::vector< std::pair<std::string, bool> > criteria;
    criteria.push_back( std::pair<std::string, bool>("BankName", true) );
    ws->sort(criteria);

    std::vector<Peak> peaks = ws->getPeaks();

    // ============================== Save all Peaks =========================================

    // Go through each peak at this run / bank
    for (int wi=0; wi < ws->getNumberPeaks(); wi++)
    {

      Peak & p = peaks[wi];
      if (p.getIntensity() == 0.0 || boost::math::isnan(p.getIntensity()) || 
        boost::math::isnan(p.getSigmaIntensity())) continue;
      int run = p.getRunNumber();
      int bank = 0;
      std::string bankName = p.getBankName();
      // Take out the "bank" part of the bank name and convert to an int
      bankName.erase(remove_if(bankName.begin(), bankName.end(), not1(std::ptr_fun (::isdigit))), bankName.end());
      Strings::convert(bankName, bank);

      double tbar = 0;

      // Two-theta = polar angle = scattering angle = between +Z vector and the scattered beam
      double scattering = p.getScattering();
      double lambda =  p.getWavelength();
      double dsp = p.getDSpacing();
      double transmission = absor_sphere(scattering, lambda, tbar);
      if(dsp < dMin || lambda < wlMin || lambda > wlMax) continue;

      // Anvred write from Art Schultz/
      //hklFile.write('%4d%4d%4d%8.2f%8.2f%4d%8.4f%7.4f%7d%7d%7.4f%4d%9.5f%9.4f\n' 
      //    % (H, K, L, FSQ, SIGFSQ, hstnum, WL, TBAR, CURHST, SEQNUM, TRANSMISSION, DN, TWOTH, DSP))
      // HKL is flipped by -1 due to different q convention in ISAW vs mantid.
      if (p.getH() == 0 && p.getK() == 0 && p.getL() == 0) continue;
      out <<  std::setw( 4 ) << Utils::round(-p.getH())
          <<  std::setw( 4 ) << Utils::round(-p.getK())
          <<  std::setw( 4 ) << Utils::round(-p.getL());
      double correc = scaleFactor;
      bool correctPeaks = getProperty("ApplyAnvredCorrections");
      if(correctPeaks)
      {
		  double sinsqt = std::pow(lambda/(2.0*dsp),2);
		  double eff = 1.0;
		  double wl4 = std::pow(lambda,4);
		  double cmonx = 1.0;
		  if(p.getMonitorCount() > 0) cmonx = 1e6 / p.getMonitorCount();
		  double spect = 1; // need to read in spectra
		  correc = scaleFactor*( sinsqt * cmonx ) / ( wl4*spect*eff )/transmission;
      }

      // SHELX can read data without the space between the l and intensity
      out << std::setw( 8 ) << std::fixed << std::setprecision( 2 ) << correc*p.getIntensity();

      out << std::setw( 8 ) << std::fixed << std::setprecision( 2 ) << correc*p.getSigmaIntensity();

      if (bank != bankold) firstrun++;
      out << std::setw( 4 ) << firstrun;

      out << std::setw( 8 ) << std::fixed << std::setprecision( 4 ) << lambda;

      out << std::setw( 7 ) <<  std::fixed << std::setprecision( 4 ) << tbar;

      out <<  std::setw( 7 ) <<  run;

      out <<  std::setw( 7 ) <<  seqNum;

      out << std::setw( 7 ) <<  std::fixed << std::setprecision( 4 ) << transmission;

      out <<  std::setw( 4 ) << std::right <<  bank;
      bankold = bank;

      out << std::setw( 9 ) << std::fixed << std::setprecision( 5 )
        << scattering; //two-theta scattering

      out << std::setw( 9 ) << std::fixed << std::setprecision( 4 )
        << dsp;

      out << std::endl;

      // Count the sequence
      seqNum++;
    }
    out << "   0   0   0    0.00    0.00   0  0.0000 0.0000      0      0 0.0000   0";
    out << std::endl;

    out.flush();
    out.close();


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
  double SaveHKL::absor_sphere(double& twoth, double& wl, double& tbar)
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
    if(std::fabs(mu) < 1e-300)
      tbar=0.0;
    else
      tbar = -(double)std::log(trans)/mu;

    return trans;
  }





} // namespace Mantid
} // namespace Crystal

