//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CorrectKiKf.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid
{
namespace Algorithms
{

// Register with the algorithm factory
DECLARE_ALGORITHM(CorrectKiKf)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace Geometry;

/// Default constructor
CorrectKiKf::CorrectKiKf() : Algorithm()
{
}

/// Destructor
CorrectKiKf::~CorrectKiKf()
{
}

/// Initialisation method
void CorrectKiKf::init()
{
  this->setWikiSummary("Performs <math>k_i/k_f</math> multiplication, in order to transform differential scattering cross section into dynamic structure factor. Both <math>E_i</math> and <math>E_f</math> must be positive. However, if this requirement is not met, it will give an error only if the data is not 0. This allows applying the algorithms to rebinned data, where one can rebin in Direct EMode to energies higher than EFixed. If no value is defined for EFixed, the algorithm will try to find <math>E_i</math> in the workspace properties for direct geometry spectrometry, or in the instrument definition, for indirect geometry spectrometry");
  this->setOptionalMessage("Performs <math>k_i/k_f</math> multiplication, in order to transform differential scattering cross section into dynamic structure factor. Both <math>E_i</math> and <math>E_f</math> must be positive. However, if this requirement is not met, it will give an error only if the data is not 0. This allows applying the algorithms to rebinned data, where one can rebin in Direct EMode to energies higher than EFixed. If no value is defined for EFixed, the algorithm will try to find <math>E_i</math> in the workspace properties for direct geometry spectrometry, or in the instrument definition, for indirect geometry spectrometry");
  
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("DeltaE"));

  this->declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input,wsValidator),
    "Name of the input workspace");
  this->declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "Name of the output workspace, can be the same as the input" );

  std::vector<std::string> propOptions;
  propOptions.push_back("Direct");
  propOptions.push_back("Indirect");
  this->declareProperty("EMode","Direct",new ListValidator(propOptions),
    "The energy mode (default: Direct)");
  BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
  mustBePositive->setLower(0.0);
  this->declareProperty("EFixed",EMPTY_DBL(),mustBePositive,
    "Value of fixed energy in meV : EI (EMode=Direct) or EF (EMode=Indirect) .");
}


void CorrectKiKf::exec()
{
  // Get the workspaces
  this->inputWS = this->getProperty("InputWorkspace");
  this->outputWS = this->getProperty("OutputWorkspace");

  // If input and output workspaces are not the same, create a new workspace for the output
  if (this->outputWS != this->inputWS)
  {
    this->outputWS = API::WorkspaceFactory::Instance().create(this->inputWS);
  }

  //Check if it is an event workspace
  EventWorkspace_const_sptr eventW = boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (eventW != NULL)
  {
    g_log.information() << "Executing CorrectKiKf for event workspace" << std::endl;
    this->execEvent();
  }  
  

  const unsigned int size = this->inputWS->blocksize();
  // Calculate the number of spectra in this workspace
  const int numberOfSpectra = this->inputWS->size() / size;
  Progress prog(this,0.0,1.0,numberOfSpectra);
  const bool histogram = this->inputWS->isHistogramData();
  bool negativeEnergyWarning = false;
    
  const std::string emodeStr = getProperty("EMode");
  double efixedProp = getProperty("EFixed");

  if( efixedProp == EMPTY_DBL() )
  {
    if (emodeStr == "Direct")
    {
      // Check if it has been store on the run object for this workspace
      if( this->inputWS->run().hasProperty("Ei"))
      {
        Kernel::Property* eiprop = this->inputWS->run().getProperty("Ei");
        efixedProp = boost::lexical_cast<double>(eiprop->value());
        g_log.debug() << "Using stored Ei value " << efixedProp << "\n";
      }
      else
      {
        throw std::invalid_argument("No Ei value has been set or stored within the run information.");
      }
    }
    else
    {
      // If not specified, will try to get Ef from the parameter file for indirect geometry, 
      // but it will be done for each spectrum separately, in case of different analyzer crystals
    }
  }


  PARALLEL_FOR2(inputWS,outputWS)
  for (int i = 0; i < numberOfSpectra; ++i) 
  {
    PARALLEL_START_INTERUPT_REGION 
    double Efi = 0;
    // Now get the detector object for this histogram to check if monitor
    // or to get Ef for indirect geometry
    if (emodeStr == "Indirect") 
    {
      if ( efixedProp != EMPTY_DBL()) Efi = efixedProp;
      else try 
      {
        IDetector_sptr det = inputWS->getDetector(i);  
        if (!det->isMonitor())
        {
          std::vector< double >  wsProp=det->getNumberParameter("Efixed");
          if ( wsProp.size() > 0 )
          {
            Efi=wsProp.at(0);
            g_log.debug() << i << " Ef: "<< Efi<<" (from parameter file)\n";     
          }
          else
          { 
            g_log.information() <<"Ef not found for spectrum "<< i << std::endl;
            throw std::invalid_argument("No Ef value has been set or found.");
          }
        }

      }
      catch(std::runtime_error&) { g_log.information() << "Spectrum " << i << ": cannot find detector" << "\n"; }
    }

    MantidVec& yOut = outputWS->dataY(i);
    MantidVec& eOut = outputWS->dataE(i);
    const MantidVec& xIn = inputWS->readX(i);
    const MantidVec& yIn = inputWS->readY(i);
    const MantidVec& eIn = inputWS->readE(i);
    //Copy the energy transfer axis
    outputWS->setX( i, inputWS->refX(i) );
    for (unsigned int j = 0; j < size; ++j)
    {
      const double deltaE = histogram ? 0.5*(xIn[j]+xIn[j+1]) : xIn[j];
      double Ei=0.;
      double Ef=0.;
      double kioverkf = 1.;
      if (emodeStr == "Direct")  //Ei=Efixed
      {
        Ei = efixedProp;
        Ef = Ei - deltaE;
      } else                     //Ef=Efixed
      { 
        Ef = Efi;
        Ei = Efi + deltaE;
      }
      // if Ei or Ef is negative, it should be a warning
      // however, if the intensity is 0 (histogram goes to energy transfer higher than Ei) it is still ok, so no warning.
      if ((Ei <= 0)||(Ef <= 0))
      {
        kioverkf=0.;
        if (yIn[j]!=0) negativeEnergyWarning=true;
      } 
      else kioverkf = std::sqrt( Ei / Ef );

      yOut[j] = yIn[j]*kioverkf;
      eOut[j] = eIn[j]*kioverkf;
    }
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }//end for i 
  PARALLEL_CHECK_INTERUPT_REGION

  if (negativeEnergyWarning) g_log.information() <<"Ef <= 0 or Ei <= 0 in at least one spectrum!!!!"<<std::endl;
  if ((negativeEnergyWarning) && ( efixedProp == EMPTY_DBL())) g_log.information()<<"Try to set fixed energy"<<std::endl ;
  this->setProperty("OutputWorkspace",this->outputWS);
  return;
}

/**
 * Execute CorrectKiKf for event workspaces
 *
 */

void CorrectKiKf::execEvent()
{
  g_log.information("You should not apply this algorithm to an event workspace. I will exit now, with a not implemented error.");
  throw Kernel::Exception::NotImplementedError("EventWorkspaces are not supported!");
}


} // namespace Algorithm
} // namespace Mantid
