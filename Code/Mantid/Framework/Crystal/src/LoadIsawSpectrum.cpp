/*WIKI* 
Read ISAW Spectra file and put in workspace.

*WIKI*/
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidCrystal/LoadIsawSpectrum.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ListValidator.h"
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
  DECLARE_ALGORITHM(LoadIsawSpectrum)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadIsawSpectrum::LoadIsawSpectrum()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadIsawSpectrum::~LoadIsawSpectrum()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void LoadIsawSpectrum::initDocs()
  {
    this->setWikiSummary("Save a PeaksWorkspace to a ASCII .hkl file.");
    this->setOptionalMessage("Save a PeaksWorkspace to a ASCII .hkl file.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadIsawSpectrum::init()
  {
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
        "An output Workspace.");
    declareProperty(new FileProperty("InstrumentFilename", "", FileProperty::Load, ".xml"),
            "Path to the instrument definition file on which to base the Workspace.");
    declareProperty(new FileProperty("SpectraFile", "", API::FileProperty::Load, ".dat"),
        " Spectrum data read from a spectrum file.");

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadIsawSpectrum::exec()
  {
    std::string InstrumentFilename = getPropertyValue("InstrumentFilename");
    Algorithm_sptr childAlg = createChildAlgorithm("LoadInstrument",0.0,0.2);
    MatrixWorkspace_sptr tempWS(new Workspace2D());
    childAlg->setProperty<MatrixWorkspace_sptr>("Workspace", tempWS);
    childAlg->setPropertyValue("Filename", InstrumentFilename);
    childAlg->setProperty("RewriteSpectraMap", false);
    childAlg->executeAsChildAlg();
	Instrument_const_sptr inst = tempWS->getInstrument();

    // If sample not at origin, shift cached positions.
    const V3D samplePos = inst->getSample()->getPos();
    const V3D pos = inst->getSource()->getPos()-samplePos;
    double l1 = pos.norm();

	std::vector<double> spec(11);
	std::string STRING;
	std::ifstream infile;
			std::string spectraFile = getPropertyValue("SpectraFile");
	infile.open (spectraFile.c_str());

	size_t a = -1;
    std::vector<std::vector<double> > spectra;
    std::vector<std::vector<double> > time;
    int iSpec = 0;
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
			time.resize(a+1);
			spectra.resize(a+1);
			getline(infile,STRING); // Saves the line in STRING.
			std::stringstream ss(STRING);
			if(STRING.find("Bank") == std::string::npos)
			{
	  double time0, spectra0;
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
	//Build a list of Rectangular Detectors
	std::vector<boost::shared_ptr<RectangularDetector> > detList;
	for (int i=0; i < inst->nelements(); i++)
	{
	boost::shared_ptr<RectangularDetector> det;
	boost::shared_ptr<ICompAssembly> assem;
	boost::shared_ptr<ICompAssembly> assem2;

	det = boost::dynamic_pointer_cast<RectangularDetector>( (*inst)[i] );
	if (det)
	{
	  detList.push_back(det);
	}
	else
	{
	  //Also, look in the first sub-level for RectangularDetectors (e.g. PG3).
	  // We are not doing a full recursive search since that will be very long for lots of pixels.
	  assem = boost::dynamic_pointer_cast<ICompAssembly>( (*inst)[i] );
	  if (assem)
	  {
		for (int j=0; j < assem->nelements(); j++)
		{
		  det = boost::dynamic_pointer_cast<RectangularDetector>( (*assem)[j] );
		  if (det)
		  {
			detList.push_back(det);
		  }
		  else
		  {
			//Also, look in the second sub-level for RectangularDetectors (e.g. PG3).
			// We are not doing a full recursive search since that will be very long for lots of pixels.
			assem2 = boost::dynamic_pointer_cast<ICompAssembly>( (*assem)[j] );
			if (assem2)
			{
			  for (int k=0; k < assem2->nelements(); k++)
			  {
				det = boost::dynamic_pointer_cast<RectangularDetector>( (*assem2)[k] );
				if (det)
				{
				  detList.push_back(det);
				}
			  }
			}
		  }
		}
	  }
	}
	}

    MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>
        ( API::WorkspaceFactory::Instance().create("Workspace2D", spectra.size(), spectra[0].size(), spectra[0].size()));
    outWS->setInstrument(inst);
    outWS->getAxis(0)->setUnit("TOF");
    outWS->setYUnit("Counts");
    outWS->isDistribution(true);
    outWS->rebuildSpectraMapping(false);

    // Go through each point at this run / bank
    for (size_t i=0; i < spectra.size(); i++)
    {
        ISpectrum * outSpec = outWS->getSpectrum(i);
        outSpec->clearDetectorIDs();
        for (int j=0; j < detList[i]->xpixels(); j++)
        	for (int k=0; k < detList[i]->ypixels(); k++)
        		outSpec->addDetectorID( static_cast<detid_t>(detList[i]->getDetectorIDAtXY(j,k)));
        MantidVec & outY = outSpec->dataY();
        MantidVec & outE = outSpec->dataE();
        MantidVec & outX = outSpec->dataX();

    	for (size_t j=0; j < spectra[i].size(); j++)
		{
			double spect = spectra[i][j];
			// Find spectra at wavelength of 1 for normalization
			std::vector<double> xdata(1,1.0);  // wl = 1
			std::vector<double> ydata;

			// This is the scattered beam direction
			V3D dir = detList[i]->getPos() - samplePos;
			double l2 = dir.norm();
			// Two-theta = polar angle = scattering angle = between +Z vector and the scattered beam
			double theta2 = dir.angle( V3D(0.0, 0.0, 1.0) );

			Mantid::Kernel::Unit_sptr unit = UnitFactory::Instance().create("Wavelength");
			unit->toTOF(xdata, ydata, l1, l2, theta2, 0, 0.0, 0.0);
			double one = xdata[0];
			double spect1 = spectrumCalc(one, iSpec, time, spectra, i);
			double relSigSpect = std::sqrt((1.0/spect) + (1.0/spect1));
			if(spect1 != 0.0)
			{
				spect /= spect1;
				outX[j] = time[i][j];
	            outY[j] = spect;
	            outE[j] = relSigSpect;
			}
			else
			{
				throw std::runtime_error("Wavelength for normalizing to spectrum is out of range.");
			}
		}
    }

    Algorithm_sptr convertAlg = createChildAlgorithm("ConvertToHistogram",0.0,0.2);
    convertAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outWS);
    // Now execute the convert Algorithm but allow any exception to bubble up
    convertAlg->execute();
    outWS = convertAlg->getProperty("OutputWorkspace");

    setProperty("OutputWorkspace", outWS);
  }

  double LoadIsawSpectrum::spectrumCalc(double TOF, int iSpec, std::vector<std::vector<double> > time, std::vector<std::vector<double> > spectra, size_t id)
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

