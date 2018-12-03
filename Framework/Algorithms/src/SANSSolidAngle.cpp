// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/SANSSolidAngle.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SANSSolidAngle)

void SANSSolidAngle::init() {

  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<InstrumentValidator>()),
                  "This workspace is used to identify the instrument to use "
                  "and also which\n"
                  "spectra to create a solid angle for. If the Max and Min "
                  "spectra values are\n"
                  "not provided one solid angle will be created for each "
                  "spectra in the input\n"
                  "workspace");
  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of "
                  "the algorithm.  A workspace of this name will be created "
                  "and stored in the Analysis Data Service.");

  std::vector<std::string> exp_options{"Normal", "Tube", "Wing"};
  declareProperty("Type", "Normal",
                  boost::make_shared<StringListValidator>(exp_options),
                  "Select the method to calculate the Solid Angle.\n"
                  "Normal: cos^3(theta); Tube: cons(alpha)*cos^3(theta); "
                  "Wing: cos^3(alpha);");
}

void SANSSolidAngle::exec() {

  // Get the workspaces
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const int numberOfSpectra = static_cast<int>(inputWS->getNumberHistograms());
  API::MatrixWorkspace_sptr outputWS =
      WorkspaceFactory::Instance().create(inputWS, numberOfSpectra, 2, 1);
  // The result of this will be a distribution
  outputWS->setDistribution(true);
  outputWS->setYUnit("");
  outputWS->setYUnitLabel("Steradian");
  setProperty("OutputWorkspace", outputWS);

  const auto &spectrumInfo = inputWS->spectrumInfo();
  const auto &componentInfo = inputWS->componentInfo();
  const double pixelSizeX =
      inputWS->getInstrument()->getNumberParameter("x-pixel-size")[0];
  const double pixelSizeY =
      inputWS->getInstrument()->getNumberParameter("y-pixel-size")[0];

  g_log.debug() << "Pixel sizes (mm) X=" << pixelSizeX << " Y=" << pixelSizeY
                << '\n';

  Progress prog(this, 0.0, 1.0, numberOfSpectra);

  // Loop over the histograms (detector spectra)
  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS, *inputWS))
  for (int i = 0; i < numberOfSpectra; ++i) {
    PARALLEL_START_INTERUPT_REGION

    if (spectrumInfo.hasDetectors(i) && !spectrumInfo.isMonitor(i)) {
      // Copy over the spectrum number & detector IDs
      outputWS->getSpectrum(i).copyInfoFrom(inputWS->getSpectrum(i));

      double solidAngle = calculateSolidAngle(i, spectrumInfo, componentInfo,
                                              pixelSizeX, pixelSizeY);

      outputWS->mutableX(i)[0] = inputWS->x(i).front();
      outputWS->mutableX(i)[1] = inputWS->x(i).back();
      outputWS->mutableY(i)[0] = solidAngle;
      outputWS->mutableE(i)[0] = 0;
    } else {
      outputWS->mutableX(i) = 0;
      outputWS->mutableY(i) = 0;
      outputWS->mutableE(i) = 0;
    }

    prog.report();
    PARALLEL_END_INTERUPT_REGION
  } // loop over spectra
  PARALLEL_CHECK_INTERUPT_REGION
}

/*
 * Returns the angle between the sample-to-pixel vector and its
 * projection on the X-Z plane.
 * */
double SANSSolidAngle::getYTubeAngle(const SpectrumInfo &spectrumInfo,
                                     size_t index) {
  const V3D samplePos = spectrumInfo.samplePosition();

  // Get the vector from the sample position to the detector pixel
  V3D sampleDetVec = spectrumInfo.position(index) - samplePos;

  // Get the projection of that vector on the X-Z plane
  V3D inPlane = V3D(sampleDetVec);
  inPlane.setY(0.0);

  // This is the angle between the sample-to-detector vector
  // and its project on the X-Z plane.
  return sampleDetVec.angle(inPlane);
}

/**
 * Compute the solid angle
 * */
double SANSSolidAngle::calculateSolidAngle(int histogramIndex,
                                           const SpectrumInfo &spectrumInfo,
                                           const ComponentInfo &componentInfo,
                                           const double pixelSizeX,
                                           const double pixelSizeY) {

  const std::string type = getProperty("Type");
  double pixelSizeXScaled =
      pixelSizeX * componentInfo.scaleFactor(histogramIndex)[0];
  double pixelSizeYScaled =
      pixelSizeY * componentInfo.scaleFactor(histogramIndex)[1];
  double pixelArea = pixelSizeXScaled * pixelSizeYScaled;
  double distance = spectrumInfo.l2(histogramIndex) * 1e3; // mm

  double angularTerm = 0;
  if (type == "Normal") {
    double cosTheta = cos(spectrumInfo.twoTheta(histogramIndex));
    angularTerm = cosTheta * cosTheta * cosTheta;
  } else if (type == "Tube") {
    double cosTheta = cos(spectrumInfo.twoTheta(histogramIndex));
    double cosAlpha = cos(getYTubeAngle(spectrumInfo, histogramIndex));
    angularTerm = cosTheta * cosTheta * cosAlpha;
  } else if (type == "Wing") {
    double cosAlpha = cos(getYTubeAngle(spectrumInfo, histogramIndex));
    angularTerm = cosAlpha * cosAlpha * cosAlpha;
  } else {
    throw std::runtime_error("Invalid type of correction");
  }
  return (pixelArea * angularTerm) / (distance * distance);
}

} // namespace Algorithms
} // namespace Mantid
