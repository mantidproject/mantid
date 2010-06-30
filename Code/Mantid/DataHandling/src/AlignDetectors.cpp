//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/AlignDetectors.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/FileProperty.h"
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
  declareProperty(
    new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input,wsValidator),
    "A workspace with units of TOF" );
  declareProperty(
    new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "The name to use for the output workspace" );
  declareProperty(new FileProperty("CalibrationFile", "", FileProperty::Load, ".cal"),
     "The CalFile containing the position correction factors");
}

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

  progress(0.0,"Reading calibration file");

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
  const double constant = (PhysicalConstants::h * 1e10) / (2.0 * PhysicalConstants::NeutronMass * 1e6);

  // Calculate the number of spectra in this workspace
  const int numberOfSpectra = inputWS->size() / inputWS->blocksize();

  // Get some positions
  const Geometry::V3D sourcePos = inputWS->getInstrument()->getSource()->getPos();
  const Geometry::V3D samplePos = inputWS->getInstrument()->getSample()->getPos();
  const Geometry::V3D beamline = samplePos-sourcePos;
  const double beamline_norm=2.0*beamline.norm();

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
      // Loop over the detectors that contribute to this spectrum and calculate the average correction
      const int ndets = specMap.ndet(spec);
      const std::vector<int> dets = specMap.getDetectors(spec);
      double factor = 0.0;
      for (int j = 0; j < ndets; ++j)
      {
        const int detsj=dets[j];
        Geometry::IDetector_const_sptr det = instrument->getDetector(detsj);
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
        const double numerator = (1.0+offsets[detsj]);
        //
        sinTheta*= (l1+l2);
        factor += numerator / sinTheta;
      }
      // Now average the factor and multiplies by the prefactor.
      factor*= constant/ndets;
      // Get references to the x data
      const MantidVec& xIn = inputWS->readX(i);
      MantidVec& xOut = outputWS->dataX(i);
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
