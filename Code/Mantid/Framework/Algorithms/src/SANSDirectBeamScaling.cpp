//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SANSDirectBeamScaling.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Histogram1D.h"
#include <iostream>
#include <vector>

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SANSDirectBeamScaling)

/// Sets documentation strings for this algorithm
void SANSDirectBeamScaling::initDocs()
{
  this->setWikiSummary("Computes the scaling factor to get reduced SANS data on an absolute scale.");
  this->setOptionalMessage("Computes the scaling factor to get reduced SANS data on an absolute scale.");
}


using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void SANSDirectBeamScaling::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("Wavelength"));
  wsValidator->add(new HistogramValidator<>);
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator));

  BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
  mustBePositive->setLower(0.0);
  declareProperty("AttenuatorTransmission", 1.0, mustBePositive, "Attenuator transmission for empty direct beam.");
  declareProperty("AttenuatorTransmissionError", 0.0, mustBePositive->clone(), "Uncertainty in attenuator transmission.");
  declareProperty("BeamRadius", 0.03, mustBePositive->clone(), "Radius of the beam stop [m].");

  // Source aperture radius in meters
  declareProperty("SourceApertureRadius", 0.04, mustBePositive->clone(),
      "Source aperture to be used if it is not found in the instrument parameters [m].");

  // Sample aperture radius in meters
  declareProperty("SampleApertureRadius", 0.008, mustBePositive->clone(),
      "Sample aperture to be used if it is not found in the instrument parameters [m].");

  // Detector ID of the monitor
  BoundedValidator<int> *zeroOrMore = new BoundedValidator<int>();
  zeroOrMore->setLower(0);
  declareProperty("BeamMonitor",1,zeroOrMore,"The UDET of the incident beam monitor.");

  declareProperty(new ArrayProperty<double>("ScaleFactor", new NullValidator<std::vector<double> >, Direction::Output),
      "Scale factor value and uncertainty [n/(monitor count)/(cm^2)/steradian].");
}

void SANSDirectBeamScaling::exec()
{
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const double beamRadius = getProperty("BeamRadius");
  const double attTrans = getProperty("AttenuatorTransmission");
  const double attTransErr = getProperty("AttenuatorTransmissionError");

  // Extract the required spectra into separate workspaces
  std::vector<int64_t> udet,index;
  udet.push_back(getProperty("BeamMonitor"));
  // Convert UDETs to workspace indices via spectrum numbers
  const std::vector<int64_t> sampleSpectra = inputWS->spectraMap().getSpectra(udet);
  inputWS->getIndicesFromSpectra(sampleSpectra,index);
  if (index.size() < 1)
  {
    g_log.debug() << "inputWS->getIndicesFromSpectra() returned empty\n";
    throw std::invalid_argument("Could not find the incident beam monitor spectra\n");
  }

  const int64_t numHists = inputWS->getNumberHistograms();
  Progress progress(this,0.0,1.0,numHists);

  // Number of X bins
  const int64_t xLength = inputWS->readY(0).size();

  // Monitor counts
  double monitor = 0.0;
  const MantidVec& MonIn = inputWS->readY(index[0]);
  for (int64_t j = 0; j < xLength; j++)
    monitor += MonIn[j];

  const V3D sourcePos = inputWS->getInstrument()->getSource()->getPos();
  double counts = 0.0;
  double error = 0.0;
  int nPixels = 0;

  // Sample-detector distance for the contributing pixels
  double sdd = 0.0;

  for (int64_t i = 0; i < numHists; ++i)
  {
    IDetector_const_sptr det;
    try {
      det = inputWS->getDetector(i);
    } catch (Exception::NotFoundError&) {
      g_log.warning() << "Spectrum index " << i << " has no detector assigned to it - discarding" << std::endl;
      continue;
    }

    // Skip if we have a monitor or if the detector is masked.
    if ( det->isMonitor() || det->isMasked() ) continue;

    const MantidVec& YIn = inputWS->readY(i);
    const MantidVec& EIn = inputWS->readE(i);

    // Sum up all the counts
    V3D pos = det->getPos() - V3D(sourcePos.X(), sourcePos.Y(), 0.0);
    const double pixelDistance = pos.Z();
    pos.setZ(0.0);
    if (pos.norm() <= beamRadius) {
      // Correct data for all X bins
      for (int64_t j = 0; j < xLength; j++)
      {
        counts += YIn[j];
        error += EIn[j]*EIn[j];
      }
      nPixels += 1;
      sdd += pixelDistance;
    }
    progress.report("Summing beam counts");
  }
  // Get the average SDD for the counted pixels, and transform to mm.
  sdd = sdd/nPixels*1000.0;
  error = std::sqrt(error);

  // Transform from m to mm
  double sourceAperture = getProperty("SourceApertureRadius");
  sourceAperture *= 1000.0;
  // Transform from m to mm
  double sampleAperture = getProperty("SampleApertureRadius");
  sampleAperture *= 1000.0;
  //TODO: replace this by some meaningful value
  const double KCG2FluxPerMon_SUGAR = 1.0;

  // Solid angle correction scale in 1/(cm^2)/steradian
  double solidAngleCorrScale = sdd/(M_PI*sourceAperture*sampleAperture);
  solidAngleCorrScale = solidAngleCorrScale*solidAngleCorrScale*100.0;

  // Scaling factor in n/(monitor count)/(cm^2)/steradian
  double scale = counts/monitor*solidAngleCorrScale/KCG2FluxPerMon_SUGAR;
  double scaleErr = std::abs(error/monitor)*solidAngleCorrScale/KCG2FluxPerMon_SUGAR;

  scaleErr = std::abs(scale/attTrans)*sqrt( (scaleErr/scale)*(scaleErr/scale) +(attTransErr/attTrans)*(attTransErr/attTrans) );
  scale /= attTrans;

  std::vector<double> output;
  output.push_back(scale);
  output.push_back(scaleErr);
  setProperty("ScaleFactor", output);
}

} // namespace Algorithms
} // namespace Mantid

