//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/AlignDetectors.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidAPI/FileProperty.h"
#include "MantidGeometry/V3D.h"

#include <fstream>

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(AlignDetectors)

using namespace Kernel;
using namespace API;
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_sptr;
using DataObjects::EventWorkspace_const_sptr;

const double CONSTANT = (PhysicalConstants::h * 1e10) / (2.0 * PhysicalConstants::NeutronMass * 1e6);

/** Calculate the conversion factor for a single pixel. The result still needs
 * to be multiplied by CONSTANT.
 *
 * @param l1 Primary flight path.
 */
double calcConversion(const double l1,
                      const Geometry::V3D &beamline,
                      const double beamline_norm,
                      const Geometry::V3D &samplePos,
                      const Geometry::IDetector_const_sptr &det,
                      const double offset)
{
  // Get the sample-detector distance for this detector (in metres)

  // The scattering angle for this detector (in radians).
  Geometry::V3D detPos = det->getPos();
  // Now detPos will be set with respect to samplePos
  detPos-=samplePos;
  // 0.5*cos(2theta)
  double l2=detPos.norm();
  double halfcosTheta=detPos.scalar_prod(beamline)/(l2*beamline_norm);
  // This is sin(theta)
  double sinTheta=sqrt(0.5-halfcosTheta);
  const double numerator = (1.0+offset);
  sinTheta*= (l1+l2);
  return numerator / sinTheta;
}

/**
 * Calculate the conversion factor (tof -> d-spacing)
 * for a LIST of detectors assigned to a single spectrum.
 */
double calcConversion(const double l1,
                      const Geometry::V3D &beamline,
                      const double beamline_norm,
                      const Geometry::V3D &samplePos,
                      const IInstrument_const_sptr &instrument,
                      const std::vector<int> &detectors,
                      const std::map<int,double> &offsets)
{
  double factor = 0.;
  double offset;
  for (std::vector<int>::const_iterator iter = detectors.begin(); iter != detectors.end(); ++iter)
  {
    std::map<int,double>::const_iterator off_iter = offsets.find(*iter);
    if( off_iter != offsets.end() )
    {
      offset = offsets.find(*iter)->second;
    }
    else
    {
      offset = 0.;
    }
    factor += calcConversion(l1, beamline, beamline_norm, samplePos,
                             instrument->getDetector(*iter), offset);
  }
  return factor * CONSTANT / static_cast<double>(detectors.size());
}

/// (Empty) Constructor
AlignDetectors::AlignDetectors()
{}

/// Destructor
AlignDetectors::~AlignDetectors()
{}

//-----------------------------------------------------------------------
void AlignDetectors::init()
{
  this->g_log.setName("DataHandling::AlignDetectors");

  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  //Workspace unit must be TOF.
  wsValidator->add(new WorkspaceUnitValidator<>("TOF"));
  wsValidator->add(new RawCountValidator<>);
  declareProperty(
    new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input,wsValidator),
    "A workspace with units of TOF" );
  declareProperty(
    new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "The name to use for the output workspace" );
  declareProperty(new FileProperty("CalibrationFile", "", FileProperty::Load, ".cal"),
     "The CalFile containing the position correction factors");
}


//-----------------------------------------------------------------------
/** Executes the algorithm
 *  @throw Exception::FileError If the calibration file cannot be opened and read successfully
 *  @throw Exception::InstrumentDefinitionError If unable to obtain the source-sample distance
 */
void AlignDetectors::exec()
{
  // Get the input workspace
  const MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  //Check if its an event workspace
  EventWorkspace_const_sptr eventW = boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (eventW != NULL)
  {
    this->execEvent();
    return;
  }

  // Read in the calibration data
  const std::string calFileName = getProperty("CalibrationFile");
  std::map<int,double> offsets;

  progress(0.0,"Reading calibration file");

  if ( ! this->readCalFile(calFileName, offsets) )
  {
    throw Exception::FileError("Problem reading calibration file", calFileName);
  }

  API::MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  // If input and output workspaces are not the same, create a new workspace for the output
  if (outputWS != inputWS )
  {
    outputWS = WorkspaceFactory::Instance().create(inputWS);
    setProperty("OutputWorkspace",outputWS);
  }
  // Set the final unit that our output workspace will have
  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("dSpacing");

  // Get a pointer to the instrument contained in the workspace
  IInstrument_const_sptr instrument = inputWS->getInstrument();
  // And one to the SpectraDetectorMap
  const SpectraDetectorMap& specMap = inputWS->spectraMap();

  // Get the distance between the source and the sample (assume in metres)
  Geometry::IObjComponent_const_sptr sample = instrument->getSample();
  double l1;
  try
  {
    l1 = instrument->getSource()->getDistance(*sample);
    g_log.debug() << "Source-sample distance: " << l1 << std::endl;
  }
  catch (Exception::NotFoundError e)
  {
    g_log.error("Unable to calculate source-sample distance");
    throw Exception::InstrumentDefinitionError("Unable to calculate source-sample distance", inputWS->getTitle());
  }

  // Calculate the number of spectra in this workspace
  const int numberOfSpectra = inputWS->getNumberHistograms();

  // Get some positions
  const Geometry::V3D sourcePos = inputWS->getInstrument()->getSource()->getPos();
  const Geometry::V3D samplePos = inputWS->getInstrument()->getSample()->getPos();
  const Geometry::V3D beamline = samplePos-sourcePos;
  const double beamline_norm=2.0*beamline.norm();

  // Initialise the progress reporting object
  Progress progress(this,0.0,1.0,numberOfSpectra);

  //std::cout << "About to start the looping\n";

  // Loop over the histograms (detector spectra)
  PARALLEL_FOR2(inputWS,outputWS)
  for (int i = 0; i < numberOfSpectra; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    try {
      // Get the spectrum number for this histogram
      const int spec = inputWS->getAxis(1)->spectraNo(i);
      double factor = calcConversion(l1, beamline, beamline_norm, samplePos, instrument,
                                     specMap.getDetectors(spec), offsets);

      // Get references to the x data
      MantidVec& xOut = outputWS->dataX(i);
      // Make sure reference to input X vector is obtained after output one because in the case
      // where the input & output workspaces are the same, it might move if the vectors were shared.
      const MantidVec& xIn = inputWS->readX(i);
      std::transform( xIn.begin(), xIn.end(), xOut.begin(), std::bind2nd(std::multiplies<double>(), factor) );
      // Copy the Y&E data
      outputWS->dataY(i) = inputWS->dataY(i);
      outputWS->dataE(i) = inputWS->dataE(i);

    } catch (Exception::NotFoundError e) {
      // Zero the data in this case
      outputWS->dataX(i).assign(outputWS->dataX(i).size(),0.0);
      outputWS->dataY(i).assign(outputWS->dataY(i).size(),0.0);
      outputWS->dataE(i).assign(outputWS->dataE(i).size(),0.0);
    }
    
    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

//-----------------------------------------------------------------------
/**
 * Execute the align detectors algorithm for an event workspace.
 */
void AlignDetectors::execEvent()
{
  g_log.information("Processing event workspace");

  // the calibration information is already read in at this point

  // convert the input workspace into the event workspace we already know it is
  const MatrixWorkspace_const_sptr matrixInputWS = this->getProperty("InputWorkspace");
  EventWorkspace_const_sptr inputWS
                 = boost::dynamic_pointer_cast<const EventWorkspace>(matrixInputWS);

  // generate the output workspace pointer
  API::MatrixWorkspace_sptr matrixOutputWS = this->getProperty("OutputWorkspace");
  EventWorkspace_sptr outputWS;
  if (matrixOutputWS == matrixInputWS)
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(matrixOutputWS);
  else
  {
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
    //API::WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, true);
    outputWS->mutableSpectraMap().clear();
    this->setProperty("OutputWorkspace", matrixOutputWS);
  }

  // Read in the calibration data
  const std::string calFileName = this->getProperty("CalibrationFile");
  std::map<int,double> offsets;
  progress(0.0,"Reading calibration file");

  if ( ! this->readCalFile(calFileName, offsets) )
  {
    throw Exception::FileError("Problem reading calibration file", calFileName);
  }

  // Set the final unit that our output workspace will have
  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("dSpacing");

  // Get a pointer to the instrument contained in the workspace
  IInstrument_const_sptr instrument = inputWS->getInstrument();
  // And one to the SpectraDetectorMap
  const SpectraDetectorMap& specMap = inputWS->spectraMap();

  // Get the distance between the source and the sample (assume in metres)
  Geometry::IObjComponent_const_sptr sample = instrument->getSample();
  double l1;
  try
  {
    l1 = instrument->getSource()->getDistance(*sample);
    g_log.debug() << "Source-sample distance: " << l1 << std::endl;
  }
  catch (Exception::NotFoundError e)
  {
    g_log.error("Unable to calculate source-sample distance");
    throw Exception::InstrumentDefinitionError("Unable to calculate source-sample distance", inputWS->getTitle());
  }

  // Get some positions
  const Geometry::V3D sourcePos = inputWS->getInstrument()->getSource()->getPos();
  const Geometry::V3D samplePos = inputWS->getInstrument()->getSample()->getPos();
  const Geometry::V3D beamline = samplePos-sourcePos;
  const double beamline_norm=2.0*beamline.norm();

//  for (std::map<int,double>::const_iterator iter = offsets.begin(); iter != offsets.end(); ++iter)
//  {
//    int pixel_id = iter->first;
//    double offset = iter->second;
//
//    // Get the spectrum number for this histogram
//    const int spec = inputWS->getAxis(1)->spectraNo(i);
//    double factor = calcConversion(l1, beamline, beamline_norm, samplePos, instrument,
//                                   specMap.getDetectors(spec), offsets);
//
//    // this should do the wonderful calculation of the geometric position as
//    // done in the histogram case
//    conversions[pixel_id] = factor;
//  }

  const int numberOfSpectra = inputWS->getNumberHistograms();
  // Conversion map: key = spectrum #; value = factor
  std::map<int,double> conversions;

  // generate map of the tof->d conversion factors
  int spec;
  double factor;
  for (int i = 0; i < numberOfSpectra; ++i)
  {
    // Get the spectrum number for this histogram
    spec = inputWS->getAxis(1)->spectraNo(i);
    factor = calcConversion(l1, beamline, beamline_norm, samplePos, instrument,
                                   specMap.getDetectors(spec), offsets);

    // this should do the wonderful calculation of the geometric position as
    // done in the histogram case
    conversions[spec] = factor;

    //Perform the multiplication on all events
    outputWS->getEventListAtWorkspaceIndex(i).convertTof(factor);
    //std::cout << "converting " << spec << " with factor " << factor << ".\n";
  }

}

//-----------------------------------------------------------------------
/// Reads the calibration file. Returns true for success, false otherwise.
bool AlignDetectors::readCalFile(const std::string& calFileName, std::map<int,double>& offsets)
{
    std::ifstream grFile(calFileName.c_str());
    if (!grFile)
    {
        g_log.error() << "Unable to open calibration file " << calFileName << std::endl;
        return false;
    }

    offsets.clear();
    std::string str;
    while(getline(grFile,str))
    {
      if (str.empty() || str[0] == '#') continue;
      std::istringstream istr(str);
      int n,udet;
      double offset;
      istr >> n >> udet >> offset;
      // Check the line wasn't badly formatted - return a failure if it is
      if ( ! istr.good() ) return false;
      offsets.insert(std::make_pair(udet,offset));
    }
    return true;
}


} // namespace DataHandling
} // namespace Mantid
