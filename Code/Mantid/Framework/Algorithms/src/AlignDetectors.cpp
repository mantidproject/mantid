//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AlignDetectors.h"
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
namespace Algorithms
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(AlignDetectors)

using namespace Kernel;
using namespace API;
using Geometry::IInstrument_const_sptr;
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_sptr;
using DataObjects::EventWorkspace_const_sptr;

const double CONSTANT = (PhysicalConstants::h * 1e10) / (2.0 * PhysicalConstants::NeutronMass * 1e6);

//-----------------------------------------------------------------------
/** Calculate the conversion factor (tof -> d-spacing) for a single pixel.
 *
 * @param l1 :: Primary flight path.
 * @param beamline: vector = samplePos-sourcePos = a vector pointing from the source to the sample,
 *        the length of the distance between the two.
 * @param beamline_norm: (source to sample distance) * 2.0 (apparently)
 * @param samplePos: position of the sample
 * @param det: Geometry object representing the detector (position of the pixel)
 * @param offset: value (close to zero) that changes the factor := factor * (1+offset).
 */
double AlignDetectors::calcConversion(const double l1,
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
  detPos -= samplePos;
  // 0.5*cos(2theta)
  double l2=detPos.norm();
  double halfcosTwoTheta=detPos.scalar_prod(beamline)/(l2*beamline_norm);
  // This is sin(theta)
  double sinTheta=sqrt(0.5-halfcosTwoTheta);
  const double numerator = (1.0+offset);
  sinTheta *= (l1+l2);
  return (numerator * CONSTANT) / sinTheta;
}


//-----------------------------------------------------------------------
/** Calculate the conversion factor (tof -> d-spacing)
 * for a LIST of detectors assigned to a single spectrum.
 */
double AlignDetectors::calcConversion(const double l1,
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
  return factor / static_cast<double>(detectors.size());
}


/** Get several instrument parameters used in tof to D-space conversion
 *
 * @param instrument
 * @param l1
 * @param beamline
 * @param beamline_norm
 * @param samplePos
 */
void AlignDetectors::getInstrumentParameters(IInstrument_const_sptr instrument,
    double & l1, Geometry::V3D & beamline,
    double & beamline_norm, Geometry::V3D & samplePos)
{
  // Get some positions
  const Geometry::IObjComponent_sptr sourceObj = instrument->getSource();
  if (sourceObj == NULL)
  {
    throw Exception::InstrumentDefinitionError("Failed to get source component from instrument");
  }
  const Geometry::V3D sourcePos = sourceObj->getPos();
  samplePos = instrument->getSample()->getPos();
  beamline = samplePos-sourcePos;
  beamline_norm=2.0*beamline.norm();

  // Get the distance between the source and the sample (assume in metres)
  Geometry::IObjComponent_const_sptr sample = instrument->getSample();
  try
  {
    l1 = instrument->getSource()->getDistance(*sample);
  }
  catch (Exception::NotFoundError &e)
  {
    throw Exception::InstrumentDefinitionError("Unable to calculate source-sample distance ", instrument->getName());
  }
}

//-----------------------------------------------------------------------
/**
 * Make a map of the conversion factors between tof and D-spacing
 * for all pixel IDs in a workspace.
 * @params: inputWS: the workspace containing the instrument geometry
 *    of interest.
 * @params: offsets: map between pixelID and offset (from the calibration file)
 */
std::map<int, double> * AlignDetectors::calcTofToD_ConversionMap(Mantid::API::MatrixWorkspace_const_sptr inputWS,
                                  const std::map<int,double> &offsets)
{
  // Get a pointer to the instrument contained in the workspace
  IInstrument_const_sptr instrument = inputWS->getInstrument();

  double l1;
  Geometry::V3D beamline,samplePos;
  double beamline_norm;

  getInstrumentParameters(instrument,l1,beamline,beamline_norm, samplePos);

  std::map<int, double> * myMap = new std::map<int, double>();

  //To get all the detector ID's
  std::map<int, Geometry::IDetector_sptr> allDetectors = instrument->getDetectors();

  //Now go through all
  std::map<int, Geometry::IDetector_sptr>::iterator it;
  for (it = allDetectors.begin(); it != allDetectors.end(); it++)
  {
    int detectorID = it->first;
    Geometry::IDetector_sptr det = it->second;

    //Find the offset, if any
    double offset;
    std::map<int,double>::const_iterator off_iter = offsets.find(detectorID);
    if( off_iter != offsets.end() )
      offset = off_iter->second;
    else
      offset = 0.;

    //Compute the factor
    double factor = calcConversion(l1, beamline, beamline_norm, samplePos, det, offset);

    //Save in map
    (*myMap)[detectorID] = factor;
  }

  //Give back the map.
  return myMap;
}


//-----------------------------------------------------------------------
/** Compute a conversion factor for a LIST of detectors.
 * Averages out the conversion factors if there are several.
 */
double calcConversionFromMap(std::map<int, double> * tofToDmap, const std::vector<int> &detectors)
{
  double factor = 0.;
  int numDetectors = 0;
  for (std::vector<int>::const_iterator iter = detectors.begin(); iter != detectors.end(); ++iter)
  {
    int detectorID = *iter;
    std::map<int, double>::iterator it;
    it = tofToDmap->find(detectorID);
    if (it != tofToDmap->end())
    {
      factor += it->second; //The factor for that ID
      numDetectors++;
    }
  }
  if (numDetectors > 0)
    return factor / static_cast<double>(numDetectors);
  else
    return 0;
}



//========================================================================
//========================================================================
/// (Empty) Constructor
AlignDetectors::AlignDetectors()
{
  this->tofToDmap = NULL;
}

/// Destructor
AlignDetectors::~AlignDetectors()
{
  delete this->tofToDmap;
}

//-----------------------------------------------------------------------
void AlignDetectors::init()
{
  this->setOptionalMessage(
      "Converts time-of-flight to d-Spacing, using a calibration file to correct by an offset.\n"
      );

  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  //Workspace unit must be TOF.
  wsValidator->add(new WorkspaceUnitValidator<>("TOF"));
  wsValidator->add(new RawCountValidator<>);
  wsValidator->add(new InstrumentValidator<>);
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

  // Read in the calibration data
  const std::string calFileName = getProperty("CalibrationFile");
  std::map<int,double> offsets;
  std::map<int,int> groups; // will be ignored
  progress(0.0,"Reading calibration file");
  if ( ! this->readCalFile(calFileName, offsets, groups) )
    throw Exception::FileError("Problem reading calibration file", calFileName);

  // Ref. to the SpectraDetectorMap
  const SpectraDetectorMap& specMap = inputWS->spectraMap();
  const int numberOfSpectra = inputWS->getNumberHistograms();

  // generate map of the tof->d conversion factors
  this->tofToDmap = calcTofToD_ConversionMap(inputWS, offsets);

  //Check if its an event workspace
  EventWorkspace_const_sptr eventW = boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (eventW != NULL)
  {
    this->execEvent();
    return;
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

  // Initialise the progress reporting object
  Progress progress(this,0.0,1.0,numberOfSpectra);

  // Loop over the histograms (detector spectra)
  PARALLEL_FOR2(inputWS,outputWS)
  for (int i = 0; i < numberOfSpectra; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    try {
      // Get the spectrum number for this histogram
      const int spec = inputWS->getAxis(1)->spectraNo(i);
      double factor = calcConversionFromMap(this->tofToDmap, specMap.getDetectors(spec));

      // Get references to the x data
      MantidVec& xOut = outputWS->dataX(i);
      // Make sure reference to input X vector is obtained after output one because in the case
      // where the input & output workspaces are the same, it might move if the vectors were shared.
      const MantidVec& xIn = inputWS->readX(i);
      std::transform( xIn.begin(), xIn.end(), xOut.begin(), std::bind2nd(std::multiplies<double>(), factor) );
      // Copy the Y&E data
      outputWS->dataY(i) = inputWS->dataY(i);
      outputWS->dataE(i) = inputWS->dataE(i);

    } catch (Exception::NotFoundError &e) {
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
  //g_log.information("Processing event workspace");

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
    //Make a brand new EventWorkspace
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
    //Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, false);
    //outputWS->mutableSpectraMap().clear();
    //You need to copy over the data as well.
    outputWS->copyDataFrom( (*inputWS) );

    //Cast to the matrixOutputWS and save it
    matrixOutputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
    this->setProperty("OutputWorkspace", matrixOutputWS);
  }

  // Set the final unit that our output workspace will have
  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("dSpacing");

  // Ref. to the SpectraDetectorMap
  const SpectraDetectorMap& specMap = inputWS->spectraMap();
  const int numberOfSpectra = inputWS->getNumberHistograms();

  // Initialise the progress reporting object
  Progress progress(this,0.0,1.0,numberOfSpectra);

  PARALLEL_FOR2(inputWS,outputWS)
  for (int i = 0; i < numberOfSpectra; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    // Get the spectrum number for this histogram
    int spec = inputWS->getAxis(1)->spectraNo(i);
    double factor = calcConversionFromMap(this->tofToDmap, specMap.getDetectors(spec));

    //Perform the multiplication on all events
    outputWS->getEventList(i).convertTof(factor);

    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

}

//-----------------------------------------------------------------------
/** Reads the calibration file. Returns true for success, false otherwise.
 *
 * @param calFileName :: .cal file path
 * @param offsets :: map of udet::offset value
 * @param groups :: map of udet::group number
 * @return true if successful.
 * @throw std::runtime_error if cannot open file
 */
bool AlignDetectors::readCalFile(const std::string& calFileName, std::map<int,double>& offsets, std::map<int,int>& groups)
{
    std::ifstream grFile(calFileName.c_str());
    if (!grFile)
    {
        throw std::runtime_error("Unable to open calibration file " + calFileName);
        return false;
    }

    offsets.clear();
    groups.clear();
    std::string str;
    while(getline(grFile,str))
    {
      if (str.empty() || str[0] == '#') continue;
      std::istringstream istr(str);
      int n,udet,select,group;
      double offset;
      istr >> n >> udet >> offset >> select >> group;
      offsets.insert(std::make_pair(udet,offset));
      groups.insert(std::make_pair(udet,group));
    }
    return true;
}


} // namespace Algorithms
} // namespace Mantid
