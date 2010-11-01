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

  const unsigned int size = inputWS->blocksize();
  // Calculate the number of spectra in this workspace
  const int numberOfSpectra = inputWS->size() / size;
  Progress prog(this,0.0,0.5,numberOfSpectra);
  const bool histogram = inputWS->isHistogramData();
  bool negativeEnergyWarning = false;
    
  const std::string emodeStr = getProperty("EMode");
  const double efixedProp = getProperty("Efixed");
  

  // There are 4 cases: (direct, indirect) * (histogram,no_histogram)

  PARALLEL_FOR2(inputWS,outputWS)
  for (int i = 0; i < numberOfSpectra; ++i) 
  {
    PARALLEL_START_INTERUPT_REGION 
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
      double kioverkf = 0.;
      if (emodeStr == "Direct")  //Ei=Efixed
      {
        Ei=efixedProp;
        Ef=efixedProp - deltaE;
      } else                     //Ef=Efixed
      {
        Ef=efixedProp;
        Ei=efixedProp + deltaE;
      }
      // if Ei or Ef is negative, it should be a warning
      // however, if the intensity is 0 (histogram goes to energy transfer higher than Ei) it is still ok, so no warning.
      if ((Ei < 0)||(Ef < 0))
      {
        kioverkf=0.;
        if (yIn[j]!=0) negativeEnergyWarning=true;//g_log.information() <<"Ef < 0!!!! Spectrum:"<<i<<std::endl;
      } 
      else kioverkf = std::sqrt( Ei / Ef );
      yOut[j] = yIn[j]*kioverkf;
      eOut[j] = eIn[j]*kioverkf;
    }
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }//end for i 
  PARALLEL_CHECK_INTERUPT_REGION

  if (negativeEnergyWarning) g_log.information() <<"Ef < 0 or Ei <0 in at least one spectrum!!!!"<<std::endl;
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
