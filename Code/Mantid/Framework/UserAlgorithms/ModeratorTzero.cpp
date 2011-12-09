/*WIKI*
 This algorithm Corrects the time of flight (TOF) by a time offset that is dependent on the velocity of the neutron after passing through the moderator.
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

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ModeratorTzero)

/// Sets documentation strings for this algorithm
void SofQW::initDocs()
{
  this->setWikiSummary(" Corrects the time of flight by a time offset that is dependent on the velocity of the neutron after passing through the moderator. ");
  this->setOptionalMessage(" Corrects the time of flight by a time offset that is dependent on the velocity of the neutron after passing through the moderator.");
}

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

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
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  //Get the parameter map
  const ParameterMap& pmap = inputWS->constInstrumentParameters();

  //deltaE-mode ("direct" or "indirect")



  //gradient and intercept constants retrieved from the instrument parameters file

  double gradient = 0.0; //[gradient]=microsecond/Angstrom
  gradient *= 3.956E-06; //[gradient]=meter

  double intercept = 0.0; //[intercept]=microsecond

  //Get a pointer to the instrument contained in the workspace
  Instrument_const_sptr instrument = inputWS->getInstrument();

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


  // Loop over the spectra
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS);
  const int64_t numHists = static_cast<int64_t>(inputWS->getNumberHistograms());
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
