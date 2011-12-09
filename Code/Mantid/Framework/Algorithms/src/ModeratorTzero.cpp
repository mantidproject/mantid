/*WIKI*
 This algorithm Corrects the time of flight (TOF) of an indirect geometry instrument by a time offset that is dependent on the velocity of the neutron after passing through the moderator.
 The TOF measured by the BASIS data acquisition system (DAS) should be reduced by this moderator emission time. The DAS "erroneously"
 thinks that it takes longer for neutrons to reach the sample and detectors, because it does not "know" that the neutrons
 spend some time in the moderator before being emitted and starting flying

 A heuristic formula for the correction, stored in the instrument definition file, is taken as linear on the initial neutron wavelength lambda_i:
     t_0 = gradient * lambda_i + intercept,  [gradient]=microsec/Angstrom and [intercept]=microsec

Required Properties:
  InputWorkspace  - EventWorkSpace in TOF units. </LI>
  OutputWorkspace - EventWorkSpace in TOF units. </LI>
  Instrument Geometry - Indirect (obtained from the instrument parameter file)
  Moderator.Tzero.gradient - Variation of the time offset with initial neutron wavelength (obtained from the instrument parameter file)
  Moderator.Tzero.intercept - time offset common to all neutrons (obtained from the instrument parameter file)

  The recorded TOF = t_0 + t_i + t_f with
 	 t_0: moderator emission time
  	 t_i: time from moderator to sample
  	 t_f: time from sample to detector

This algorithm will replace TOF with TOF' = TOF-t_0 = t_i+t_f

	 For a direct geometry instrument, lambda_i is (approximately) the same for all neutrons. Hence the moderator emission time is the same for all neutrons.
	     There is already an algorithm, getEi, that calculates t_0 for the direct geometry instrument. Thus we skip this step.
 	 For an indirect geometry instrument, lambda_i is not known but the final energy, E_f, selected by the analyzers is known. For this geometry:
 	 	 t_f = L_f/v_f   L_f: distance from sample to detector, v_f: final velocity derived from E_f
 	 	 t_i = L_i/v_i   L_i: distance from moderator to sample, v_i: initial velocity unknown
 	 	 t_0 = a/v_i+b   a and b are constants derived from the aforementioned heuristic formula.
 	 	                 a=gradient*3.956E-03, [a]=meter,    b=intercept, [b]=microsec
 	 	 Putting all together:  TOF' = (L_i/(L_i+a))*(TOF-t_f-b) + t_i,   [TOF']=microsec
*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "ModeratorTzero.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ModeratorTzero)

/// Sets documentation strings for this algorithm
void SofQW::initDocs()
{
  setWikiSummary(" Corrects the time of flight of an indirect geometry instrument by a time offset that is dependent on the velocity of the neutron after passing through the moderator. ");
  setOptionalMessage(" Corrects the time of flight of an indirect geometry instrument by a time offset that is dependent on the velocity of the neutron after passing through the moderator.");
}

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void ModeratorTzero::init()
{

  //Workspace should be of type EventWorkspace, and with Time-of-Flight units
  CompositeWorkspaceValidator<> *wsValidator = new CompositeWorkspaceValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("TOF"));  //or is it Time-of-Flight ???
  declareProperty(new WorkspaceProperty<IEventWorkspace>("InputWorkspace","",Direction::Input,wsValidator));

  declareProperty(new WorkspaceProperty<IEventWorkspace>("OutputWorkspace","",Direction::Output));

}

void ModeratorTzero::exec()
{
  //Efixed retrieved from the instrument definition file. There's a value of Efixed for each pixel, since value varies slightly.

  //retrieve the input workspace.
  EventWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  //Get a pointer to the instrument contained in the workspace
  Instrument_const_sptr instrument = inputWS->getInstrument();

  //Get the parameter map
  const ParameterMap& pmap = inputWS->constInstrumentParameters();

  //deltaE-mode (should be "indirect")
  std::string Emode;
  try
  {
	Emode = instrument->getStringParameter("deltaE-mode")[0];
    g_log.debug() << "Instrument Geometry: " << Emode << std::endl;
  }
  catch (Exception::NotFoundError &)
  {
    g_log.error("Unable to retrieve instrument geometry (direct or indirect) parameter");
    throw Exception::InstrumentDefinitionError("Unable to retrieve instrument geometry (direct or indirect) parameter", inputWS->getTitle());
  }

  //gradient and intercept constants
  double gradient, intercept;
  try
  {
	gradient = instrument->getNumberParameter("Moderator.TimeZero.gradient")[0]; //[gradient]=microsecond/Angstrom
	//conversion factor for gradient from microsecond/Angstrom to meters
	double convfactor = 1e+4*PhysicalConstants::h/PhysicalConstants::NeutronMass;
	gradient *= convfactor; //[gradient] = meter
	intercept = instrument->getNumberParameter("Moderator.TimeZero.intercept")[0]; //[intercept]=microsecond
    g_log.debug() << "Moderator Time Zero: gradient=" << gradient << "intercept=" << intercept << std::endl;
  }
  catch (Exception::NotFoundError &)
  {
    g_log.error("Unable to retrieve Moderator Time Zero parameters (gradient and intercept)");
    throw Exception::InstrumentDefinitionError("Unable to retrieve Moderator Time Zero parameters (gradient and intercept)", inputWS->getTitle());
  }

  //Get the distance L_i between the source and the sample ([Li]=meters)
  IObjComponent_const_sptr source = instrument->getSource();
  IObjComponent_const_sptr sample = instrument->getSample();
  double L_i;
  try
  {
    L_i = source->getDistance(*sample);
    g_log.debug() << "Source-sample distance: " << L_i << std::endl;
  }
  catch (Exception::NotFoundError &)
  {
    g_log.error("Unable to calculate source-sample distance");
    throw Exception::InstrumentDefinitionError("Unable to calculate source-sample distance", inputWS->getTitle());
  }
  double const factor = L_i/(L_i+gradient);

  // generate the output workspace pointer
  const int64_t numHists = static_cast<int64_t>(inputWS->getNumberHistograms());
  API::MatrixWorkspace_sptr matrixOutputWS = this->getProperty("OutputWorkspace");
  EventWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (inputWS != outputWS)
  {
    //Make a brand new EventWorkspace
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(WorkspaceFactory::Instance().create("EventWorkspace", numHists, 2, 1));
    //Copy geometry over.
    WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, false);
    //Copy over the data as well.
    outputWS->copyDataFrom( (*inputWS) );

    setProperty("OutputWorkspace", outputWS);
  }

  // Loop over the spectra
  Progress prog(this,0.0,1.0,numHists); //report progress of algorithm
  PARALLEL_FOR2(inputWS,outputWS)
  for (int64_t i = 0; i < int64_t(numHists); ++i)
  {
	PARALLEL_START_INTERUPT_REGION

	// Get detector position
    IDetector_const_sptr det;
    try
    {
      det = inputWS->getDetector(i);
    } catch (Exception::NotFoundError&)
    {
	  // Catch if no detector. Next line tests whether this happened - test placed
      // outside here because Mac Intel compiler doesn't like 'continue' in a catch
	  // in an openmp block.
	}
	// If no detector found, skip onto the next spectrum
	if ( !det ) continue;

	//Get Efixed value
    try {
      Parameter_sptr par = pmap.get(det.get(),"Efixed");
      if (par)
      {
        //calculate L_f and t_f

    	//iterate over events for each histogram

    	  //retrieve ToF for the event

    	  //new ToF = factor * (Tof - t_f - intercept), to be stored in the new workspace!

      }
    } catch (std::runtime_error&) { /* Throws if a DetectorGroup, use single provided value */ }


	prog.report();
	PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

}

} // namespace Algorithms
} // namespace Mantid
