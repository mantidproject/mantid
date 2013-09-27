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
#include "MantidKernel/UnitFactory.h"
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
    declareProperty("LinearScatteringCoef", EMPTY_DBL(), mustBePositive,
      "Linear scattering coefficient in 1/cm if not set with SetSampleMaterial");
    declareProperty("LinearAbsorptionCoef", EMPTY_DBL(), mustBePositive,
      "Linear absorption coefficient at 1.8 Angstroms in 1/cm if not set with SetSampleMaterial");
    declareProperty("Radius", EMPTY_DBL(), mustBePositive, "Radius of the sample in centimeters");
    declareProperty("PowerLambda", 4.0, "Power of lamda ");
    declareProperty(new FileProperty("SpectraFile", "", API::FileProperty::OptionalLoad, ".dat"),
        " Spectrum data read from a spectrum file.");

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

    double scaleFactor = getProperty("ScalePeaks"); 
    double dMin = getProperty("MinDSpacing");
    double wlMin = getProperty("MinWavelength");
    double wlMax = getProperty("MaxWavelength");

    // Sequence and run number
    int seqNum = 1;
    int bankSequence = 0;
    int runSequence = 0;
    int bankold = -1;
    int runold = -1;
    std::map<int, double> detScale;

    if (ws->getInstrument()->getName() == "TOPAZ")
    {
      detScale[17] = 1.092114823;
      detScale[18] = 0.869105443;
      detScale[22] = 1.081377685;
      detScale[26] = 1.055199489;
      detScale[27] = 1.070308725;
      detScale[28] = 0.886157884;
      detScale[36] = 1.112773972;
      detScale[37] = 1.012894506;
      detScale[38] = 1.049384146;
      detScale[39] = 0.890313805;
      detScale[47] = 1.068553893;
      detScale[48] = 0.900566426;
      detScale[58] = 0.911249203;
    }

    std::fstream out;
    bool append = getProperty("AppendFile");
    if (append)
    {
      out.open( filename.c_str(), std::ios::in|std::ios::out|std::ios::ate);
      std::streamoff pos = out.tellp();
      out.seekp (28);
      out >> runSequence;
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

    bool correctPeaks = getProperty("ApplyAnvredCorrections");
    std::vector<Peak> peaks= ws->getPeaks();
    std::vector<std::vector<double> > spectra;
    std::vector<std::vector<double> > time;
    int iSpec = 0;
    smu = getProperty("LinearScatteringCoef"); // in 1/cm
    amu = getProperty("LinearAbsorptionCoef"); // in 1/cm
    radius = getProperty("Radius"); // in cm
    power_th = getProperty("PowerLambda"); // in cm
    const Material& sampleMaterial = ws->sample().getMaterial();
    if( sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda) != 0.0)
    {
      double rho =  sampleMaterial.numberDensity();
      if (smu == EMPTY_DBL()) smu =  sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda) * rho;
      if (amu == EMPTY_DBL()) amu = sampleMaterial.absorbXSection(NeutronAtom::ReferenceLambda) * rho;
    }
    else  //Save input in Sample with wrong atomic number and name
    {
      NeutronAtom neutron(static_cast<uint16_t>(EMPTY_DBL()), static_cast<uint16_t>(0),
    			0.0, 0.0, smu, 0.0, smu, amu);
      Material mat("SetInAnvredCorrection", neutron, 1.0);
      ws->mutableSample().setMaterial(mat);
    }
    API::Run & run = ws->mutableRun();
    if ( run.hasProperty("Radius") )
    {
      Kernel::Property* prop = run.getProperty("Radius");
      if (radius == EMPTY_DBL()) radius = boost::lexical_cast<double,std::string>(prop->value());
    }
    else
     {
      run.addProperty<double>("Radius", radius, true);
    }
    if(correctPeaks)
    {
		std::vector<double> spec(11);
		std::string STRING;
		std::ifstream infile;
                std::string spectraFile = getPropertyValue("SpectraFile");
		infile.open (spectraFile);
		size_t a = 0;
		if (iSpec == 1)
		{
			while(!infile.eof()) // To get you all the lines.
			{
				// Set up sizes. (HEIGHT x WIDTH)
				spectra.resize(a+1);
				getline(infile,STRING); // Saves the line in STRING.
				infile >> spec[0] >> spec[1] >> spec[2] >> spec[3] >> spec[4] >> spec[5] >> spec[6]
					   >> spec[7] >> spec[8]>> spec[9]>> spec[10];
				for (int i=0; i < 11; i++)spectra[a].push_back(spec[i]);
				a++;
			}
		}
		else
		{
			for (int wi=0; wi < 8; wi++)getline(infile,STRING); // Saves the line in STRING.
			while(!infile.eof()) // To get you all the lines.
			{
				double time0, spectra0;
				time.resize(a+1);
				spectra.resize(a+1);
				getline(infile,STRING); // Saves the line in STRING.
				std::stringstream ss(STRING);
				if(STRING.find("Bank") == std::string::npos)
				{
					ss >> time0 >> spectra0;
					time[a].push_back(time0);
					spectra[a].push_back(spectra0);

				}
				else
				{
					a++;
				}
			}
		}
		infile.close();
    }

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
      double relSigSpect = 0.0;
      if (bank != bankold) bankSequence++;
      if (run != runold) runSequence++;
      if(correctPeaks)
      {
			// correct for the slant path throught the scintillator glass
    	    double mu = (9.614 * lambda) + 0.266;    // mu for GS20 glass
			double depth = 0.2;
			double eff_center = 1.0 - std::exp(-mu * depth);  // efficiency at center of detector
			IObjComponent_const_sptr sample = ws->getInstrument()->getSample();
			double cosA = ws->getInstrument()->getComponentByName(p.getBankName())->getDistance(*sample) / p.getL2();
			double pathlength = depth / cosA;
			double eff_R = 1.0 - exp(-mu * pathlength);   // efficiency at point R
			double sp_ratio = eff_center / eff_R;  // slant path efficiency ratio

			double sinsqt = std::pow(lambda/(2.0*dsp),2);
			double wl4 = std::pow(lambda,4);
			double cmonx = 1.0;
			if(p.getMonitorCount() > 0) cmonx = 100e6 / p.getMonitorCount();
			double spect = spectrumCalc(p.getTOF(), iSpec, time, spectra, bankSequence);
			// Find spectra at wavelength of 1 for normalization
			std::vector<double> xdata(1,1.0);  // wl = 1
			std::vector<double> ydata;
			double theta2 = p.getScattering();
			double l1 = p.getL1();
			double l2 = p.getL2();
			Mantid::Kernel::Unit_sptr unit = UnitFactory::Instance().create("Wavelength");
			unit->toTOF(xdata, ydata, l1, l2, theta2, 0, 0.0, 0.0);
			double one = xdata[0];
			double spect1 = spectrumCalc(one, iSpec, time, spectra, bankSequence);
			relSigSpect = std::sqrt((1.0/spect) + (1.0/spect1));
			if(spect1 != 0.0)
			{
				spect /= spect1;
			}
			else
			{
				throw std::runtime_error("Wavelength for normalizing to spectrum is out of range.");
			}
			correc = scaleFactor * sinsqt * cmonx * sp_ratio / ( wl4 * spect * transmission);
			if (ws->getInstrument()->getName() == "TOPAZ" && detScale.find(bank) != detScale.end())
                          correc *= detScale[bank];
      }

      // SHELX can read data without the space between the l and intensity
      out << std::setw( 8 ) << std::fixed << std::setprecision( 2 ) << correc*p.getIntensity();

      out << std::setw( 8 ) << std::fixed << std::setprecision( 2 ) <<
    		  std::sqrt(std::pow(correc*p.getSigmaIntensity(),2)+std::pow(relSigSpect*correc*p.getIntensity(),2));

      out << std::setw( 4 ) << runSequence;

      out << std::setw( 8 ) << std::fixed << std::setprecision( 4 ) << lambda;

      out << std::setw( 7 ) <<  std::fixed << std::setprecision( 4 ) << tbar;

      out <<  std::setw( 7 ) <<  run;

      out <<  std::setw( 7 ) <<  wi + seqNum;

      out << std::setw( 7 ) <<  std::fixed << std::setprecision( 4 ) << transmission;

      out <<  std::setw( 4 ) << std::right <<  bank;
      bankold = bank;
      runold = run;

      out << std::setw( 9 ) << std::fixed << std::setprecision( 5 )
        << scattering; //two-theta scattering

      out << std::setw( 9 ) << std::fixed << std::setprecision( 4 )
        << dsp;

      out << std::endl;

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

  double SaveHKL::spectrumCalc(double TOF, int iSpec, std::vector<std::vector<double> > time, std::vector<std::vector<double> > spectra, size_t id)
  {
	  double spect = 0;
	  if (iSpec == 1)
	  {
		  //"Calculate the spectrum using spectral coefficients for the GSAS Type 2 incident spectrum."
		  double T = TOF/1000.;            // time-of-flight in milliseconds

		  double c1 = spectra[id][0];
		  double c2 = spectra[id][1];
		  double c3 = spectra[id][2];
		  double c4 = spectra[id][3];
		  double c5 = spectra[id][4];
		  double c6 = spectra[id][5];
		  double c7 = spectra[id][6];
		  double c8 = spectra[id][7];
		  double c9 = spectra[id][8];
		  double c10 = spectra[id][9];
		  double c11 = spectra[id][10];

		  spect = c1 + c2*exp(-c3/std::pow(T,2))/std::pow(T,5)
			  + c4*exp(-c5*std::pow(T,2))
			  + c6*exp(-c7*std::pow(T,3))
			  + c8*exp(-c9*std::pow(T,4))
			  + c10*exp(-c11*std::pow(T,5));
	  }
	  else
	  {
		  size_t i = 1;
		  for (i = 1; i < spectra[id].size(); ++i) if(TOF < time[id][i])break;
		  spect = spectra[id][i-1] + (TOF - time[id][i-1])/(time[id][i] - time[id][i-1])*(spectra[id][i]-spectra[id][i-1]);
	  }

      return spect;
  }


} // namespace Mantid
} // namespace Crystal

