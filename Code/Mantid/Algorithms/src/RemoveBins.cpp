//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/RemoveBins.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid
{
namespace Algorithms
{

using namespace Kernel;
using namespace API;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(RemoveBins)

// Get a reference to the logger
Logger& RemoveBins::g_log = Logger::get("RemoveTimeBins");

RemoveBins::RemoveBins() : API::Algorithm(), m_rangeUnit()
{}

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void RemoveBins::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>);
  wsValidator->add(new HistogramValidator<>);
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator),
    "Name of the input workspace");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "Name of the output workspace");

  MandatoryValidator<double> *mustHaveValue = new MandatoryValidator<double>;
  declareProperty("XMin",Mantid::EMPTY_DBL(), mustHaveValue,
    "The lower bound of the region to be removed");
  declareProperty("XMax",Mantid::EMPTY_DBL(), mustHaveValue->clone(),
    "The upper bound of the region to be removed");

  std::vector<std::string> units = UnitFactory::Instance().getKeys();
  units.insert(units.begin(),"AsInput");
  declareProperty( "RangeUnit", "AsInput", new ListValidator(units),
    "The units of XMin and XMax" );

  std::vector<std::string> propOptions;
  propOptions.push_back("None");
  propOptions.push_back("Linear");
  declareProperty("Interpolation", "None", new ListValidator(propOptions),
    "Used when the region to be removed is within a bin. Linear scales the value in that bin by the proportion of it that is outside the region to be removed and none sets it to zero" );
}

/** Executes the algorithm
 *
 */
void RemoveBins::exec()
{
	this->checkProperties();

  // If the X range has been given in a different unit, or if the workspace isn't square, then we will need
  // to calculate the bin indices to cut out each time.
  const std::string rangeUnit = getProperty("RangeUnit");
  const std::string inputUnit = m_inputWorkspace->getAxis(0)->unit()->unitID();
  const bool unitChange = (rangeUnit != "AsInput" && rangeUnit != "inputUnit");
  if (unitChange) m_rangeUnit = UnitFactory::Instance().create(rangeUnit);
  const bool commonBins = WorkspaceHelpers::commonBoundaries(m_inputWorkspace);
  const bool recalcRange = ( unitChange || !commonBins);

  // If the above evaluates to false, and the range given is at the edge of the workspace, then we can just call
  // CropWorkspace as a subalgorithm and we're done.
  const std::vector<double>& X0 = m_inputWorkspace->readX(0);
  if ( !recalcRange && ( m_startX <= X0.front() || m_endX >= X0.back() ) )
  {
    double start,end;
    if (m_startX <= X0.front())
    {
      start = m_endX;
      end = X0.back();
    }
    else
    {
      start = X0.front();
      end = m_startX;
    }

    try {
      this->crop(start,end);
      return;
    } catch(...) {}   // If this fails for any reason, just carry on and do it the other way
  }


  MatrixWorkspace_sptr outputWS= getProperty("OutputWorkspace");

  if (m_inputWorkspace!=outputWS) // Create the output workspace onlt if not the same as input
	{
	  outputWS = WorkspaceFactory::Instance().create(m_inputWorkspace);
	}

  // Loop over the spectra
  int start=0,end=0;
  const int blockSize = m_inputWorkspace->readX(0).size();
  const int numHists = m_inputWorkspace->getNumberHistograms();
  for (int i=0; i < numHists; ++i)
  {
    double startX(m_startX),endX(m_endX);
    // Calculate the X limits for this spectrum, if necessary
    if (unitChange)
    {
      this->transformRangeUnit(i,startX,endX);
    }

    // Get references to the data and errors
    const std::vector<double>& X = m_inputWorkspace->readX(i);

    // Calculate the bin indices corresponding to the X range, if necessary
    if ( recalcRange || !i )
    {
      start = this->findIndex(startX,X);
      end = this->findIndex(endX,X);
    }

    // Copy over the data
    std::vector<double>& myX=outputWS->dataX(i);
    myX = X;
    const std::vector<double>& Y=m_inputWorkspace->readY(i);
    std::vector<double>& myY=outputWS->dataY(i);
    myY=Y;
    const std::vector<double>& E=m_inputWorkspace->readE(i);
    std::vector<double>& myE=outputWS->dataE(i);
    myE=E;

    if ( start == 0 || end == blockSize )
    {
      // Remove bins from either end
      this->RemoveFromEnds(start,end,myY,myE);
    }
    else
    {
      // Remove bins from middle
      const double startFrac = (X[start]-startX)/(X[start]-X[start-1]);
      const double endFrac = (endX-X[end-1])/(X[end]-X[end-1]);
      this->RemoveFromMiddle(start-1,end,startFrac,endFrac,myY,myE);
    }

  } // Loop over spectra

  // Assign to the output workspace property
  setProperty("OutputWorkspace",outputWS);
  m_inputWorkspace.reset();
}

/** Retrieve the input properties and check that they are valid
 *  @throw std::invalid_argument If XMin or XMax are not set, or XMax is less than XMin
 */
void RemoveBins::checkProperties()
{
  //Get input workspace
  m_inputWorkspace = getProperty("InputWorkspace");

  // If that was OK, then we can get their values
  m_startX = getProperty("XMin");
  m_endX = getProperty("XMax");

  if (m_startX > m_endX)
  {
    const std::string failure("XMax must be greater than XMin.");
    g_log.error(failure);
    throw std::invalid_argument(failure);
  }

  const std::string interpolation = getProperty("Interpolation");
  m_interpolate = ( interpolation == "Linear" ? true : false );

  return;
}

/// Calls CropWorkspace as a sub-algorithm to remove bins from the start or end of a square workspace
void RemoveBins::crop(const double& start, const double& end)
{
  IAlgorithm_sptr childAlg = createSubAlgorithm("CropWorkspace");
  DataObjects::Workspace2D_const_sptr input2D = boost::dynamic_pointer_cast<const DataObjects::Workspace2D>(m_inputWorkspace);
  childAlg->setProperty<DataObjects::Workspace2D_sptr>("InputWorkspace", boost::const_pointer_cast<DataObjects::Workspace2D>(input2D));
  childAlg->setProperty<double>("XMin", start);
  childAlg->setProperty<double>("XMax", end);

  // Now execute the sub-algorithm. Catch and log any error
  try
  {
    childAlg->execute();
  }
  catch (std::runtime_error&)
  {
    g_log.error("Unable to successfully run sub-algorithm");
    throw;
  }

  if ( ! childAlg->isExecuted() )
  {
    g_log.error("Unable to successfully run sub-algorithm");
    throw std::runtime_error("Unable to successfully run sub-algorithm");
  }

  // Only get to here if successful
  // Assign the result to the output workspace property
  MatrixWorkspace_sptr outputWS = childAlg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace",outputWS);
  return;
}

/** Convert the X range given into the unit of the input workspace
 *  @param index  The current spectrum index
 *  @param startX Returns the start of the range in the workspace's unit
 *  @param endX   Returns the end of the range in the workspace's unit
 */
void RemoveBins::transformRangeUnit(const int& index, double& startX, double& endX)
{
  const Kernel::Unit_sptr inputUnit = m_inputWorkspace->getAxis(0)->unit();
  // First check for a 'quick' conversion
  double factor,power;
  if ( inputUnit->quickConversion(*m_rangeUnit,factor,power) )
  {
    startX = factor * std::pow(m_startX,power);
    endX = factor * std::pow(m_endX,power);
  }
  else
  {
    double l1,l2,theta;
    this->calculateDetectorPosition(index,l1,l2,theta);
    std::vector<double> endPoints;
    endPoints.push_back(startX);
    endPoints.push_back(endX);
    std::vector<double> emptyVec;
    m_rangeUnit->toTOF(endPoints,emptyVec,l1,l2,theta,0,0.0,0.0);
    inputUnit->fromTOF(endPoints,emptyVec,l1,l2,theta,0,0.0,0.0);
    startX = endPoints.front();
    endX = endPoints.back();
  }

  if (startX > endX)
  {
    const double temp = startX;
    startX = endX;
    endX = temp;
  }

  g_log.debug() << "For index " << index << ", X range given corresponds to " << startX << "-" << endX << " in workspace's unit" << std::endl;
  return;
}

/** Retrieves the detector postion for a given spectrum
 *  @param index    The workspace index of the spectrum
 *  @param l1       Returns the source-sample distance
 *  @param l2       Returns the sample-detector distance
 *  @param twoTheta Returns the detector's scattering angle
 */
void RemoveBins::calculateDetectorPosition(const int& index, double& l1, double& l2, double& twoTheta)
{
  // Get a pointer to the instrument contained in the workspace
  IInstrument_const_sptr instrument = m_inputWorkspace->getInstrument();
  // Get the distance between the source and the sample (assume in metres)
  Geometry::IObjComponent_const_sptr sample = instrument->getSample();
  l1 = instrument->getSource()->getDistance(*sample);
  Geometry::IDetector_const_sptr det = m_inputWorkspace->getDetector(index);
  //Geometry::V3D detPos = det->getPos();
  // Get the sample-detector distance for this detector (in metres)
  if ( ! det->isMonitor() )
  {
    l2 = det->getDistance(*sample);
    // The scattering angle for this detector (in radians).
    twoTheta = m_inputWorkspace->detectorTwoTheta(det);
  }
  else  // If this is a monitor then make l1+l2 = source-detector distance and twoTheta=0
  {
    l2 = det->getDistance(*(instrument->getSource()));
    l2 = l2 - l1;
    twoTheta = 0.0;
  }
  g_log.debug() << "Detector for index " << index << " has L1+L2=" << l1+l2 << " & 2theta= " << twoTheta << std::endl;
  return;
}

/** Finds the index in an ordered vector which follows the given value
 *  @param value The value to search for
 *  @param vec   The vector to search
 *  @return The index (will give vec.size()+1 if the value is past the end of the vector)
 */
int RemoveBins::findIndex(const double& value, const std::vector<double>& vec)
{
  std::vector<double>::const_iterator pos = std::lower_bound(vec.begin(),vec.end(),value);
  return pos-vec.begin();
}

/** Zeroes data (Y/E) at the end of a spectrum
 *  @param start The index to start zeroing at
 *  @param end   The index to end zeroing at
 *  @param Y     The data vector
 *  @param E     The error vector
 */
void RemoveBins::RemoveFromEnds(int start, int end, std::vector<double>& Y, std::vector<double>& E)
{
  if ( start ) --start;
  if ( end > static_cast<int>(Y.size()) ) end = Y.size();
  for (int j = start; j < end; ++j)
  {
    Y[j] = 0.0;
    E[j] = 0.0;
  }

  return;
}

/** Removes bins in the middle of the data (Y/E).
 *  According to the value of the Interpolation property, they are either zeroed or the gap is interpolated linearly.
 *  If the former, the edge bins will be scaled according to how much of them falls within the range being removed.
 *  @param start     The first index to remove
 *  @param end       The last index to remove
 *  @param startFrac The fraction of the first bin that's outside the range being zeroed
 *  @param endFrac   The fraction of the last bin that's outside the range being zeroed
 *  @param Y         The data vector
 *  @param E         The error vector
 */
void RemoveBins::RemoveFromMiddle(const int& start, const int& end, const double& startFrac, const double& endFrac, std::vector<double>& Y, std::vector<double>& E)
{
  //Remove bins from middle
  double valPrev = 0;
  double valNext = 0;
  double errPrev = 0;
  double errNext = 0;

  //Values for interpolation
  if (m_interpolate)
  {
    valPrev = Y[start - 1];
    valNext = Y[end];
    errPrev = E[start - 1];
    errNext = E[end];
  }

  const double m = (valNext - valPrev)/(1.0*(end - start) + 2.0); //Gradient
  const double c = valPrev; //Intercept

  double aveE = (errPrev + errNext)/2; //Cheat: will do properly later

  for (int j = start; j < end; ++j)
  {
    if (!m_interpolate && j==start)
    {
      Y[j] *= startFrac;
      E[j] *= startFrac;
    }
    else if (!m_interpolate && j==end-1)
    {
      Y[j] *= endFrac;
      E[j] *= endFrac;
    }
    else
    {
      Y[j] = m * (j - start + 1) + c;
      E[j] = aveE;
    }
  }

  return;
}

} // namespace Algorithm
} // namespace Mantid
