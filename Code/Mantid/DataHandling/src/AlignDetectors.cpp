//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/AlignDetectors.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include <fstream>

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(AlignDetectors)

using namespace Kernel;
using namespace API;

// Initialise the logger
Logger& AlignDetectors::g_log = Logger::get("AlignDetectors");

/// (Empty) Constructor
AlignDetectors::AlignDetectors()
{}

/// Destructor
AlignDetectors::~AlignDetectors()
{}

void AlignDetectors::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("TOF"));
  wsValidator->add(new RawCountValidator<>);
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input,wsValidator));
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",Direction::Output));
  declareProperty("CalibrationFile","",new FileValidator(std::vector<std::string>(1,"cal")));
}

/** Executes the algorithm
 *  @throw Exception::NotImplementedError If the workspace is not TOF, with raw counts for data
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
  if ( ! this->readCalFile(calFileName, offsets) )
  {
    throw Exception::FileError("Problem reading file", calFileName);
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
  SpectraMap_const_sptr specMap = inputWS->getSpectraMap();

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
  Geometry::V3D samplePos = sample->getPos();
  const double constant = (PhysicalConstants::h * 1e10) / (2.0 * PhysicalConstants::NeutronMass * 1e6);

  // Calculate the number of spectra in this workspace
  const int numberOfSpectra = inputWS->size() / inputWS->blocksize();
  // Loop over the histograms (detector spectra)
  for (int i = 0; i < numberOfSpectra; ++i)
  {
    try {
      // Get the spectrum number for this histogram
      const int spec = inputWS->getAxis(1)->spectraNo(i);
      // Loop over the detectors that contribute to this spectrum and calculate the average correction
      const int ndets = specMap->ndet(spec);
      std::vector<int> dets = specMap->getDetectors(spec);
      double factor = 0.0;
      for (int j = 0; j < ndets; ++j)
      {
        Geometry::IDetector_const_sptr det = instrument->getDetector(dets[j]);
        Geometry::V3D detPos = det->getPos();
        // Get the sample-detector distance for this detector (in metres)
        const double l2 = detPos.distance(samplePos);
        // The scattering angle for this detector (in radians).
        const double twoTheta = inputWS->detectorTwoTheta(det);
        // Get the correction for this detector
        const double offset = offsets[dets[j]];
        const double numerator = constant * (1.0+offset);
        const double denom = ((l1+l2)*sin(twoTheta/2.0));
        factor += numerator / denom;
      }
      // Now average the factor
      factor = factor / ndets;
      // Get a reference to the x data
      const std::vector<double>& xIn = inputWS->dataX(i);
      std::vector<double>& xOut = outputWS->dataX(i);
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
  }
}

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
