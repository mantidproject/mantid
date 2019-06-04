// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SumNeighbours.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/BoundedValidator.h"

#include <boost/algorithm/string.hpp>

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SumNeighbours)

using namespace Kernel;
using namespace Geometry;
using namespace API;
using namespace DataObjects;

/** Initialisation method.
 *
 */
void SumNeighbours::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Mantid::API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<InstrumentValidator>()),
                  "A workspace containing one or more rectangular area "
                  "detectors. Each spectrum needs to correspond to only one "
                  "pixelID (e.g. no grouping or previous calls to "
                  "SumNeighbours).");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of "
                  "the algorithm.");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(1);

  declareProperty("SumX", 4, mustBePositive,
                  "The number of X (horizontal) pixels to sum together. This "
                  "must evenly divide the number of X pixels in a detector.");

  declareProperty("SumY", 4, mustBePositive,
                  "The number of Y (vertical) pixels to sum together. This "
                  "must evenly divide the number of Y pixels in a detector");
}

/** Executes the algorithm
 *
 */
void SumNeighbours::exec() {
  // Try and retrieve the optional properties
  int SumX = getProperty("SumX");
  int SumY = getProperty("SumY");

  // Get the input workspace
  Mantid::API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  const auto &spectrumInfo = inWS->spectrumInfo();
  const auto &det = spectrumInfo.detector(0);
  // Check if grandparent is rectangular detector
  boost::shared_ptr<const Geometry::IComponent> parent = det.getParent();
  boost::shared_ptr<const RectangularDetector> rect;

  if (parent) {
    rect = boost::dynamic_pointer_cast<const RectangularDetector>(
        parent->getParent());
  }

  Mantid::API::MatrixWorkspace_sptr outWS;

  Progress progress(this, 0.0, 1.0, 2);

  progress.report("Smoothing Neighbours...");

  IAlgorithm_sptr smooth = createChildAlgorithm("SmoothNeighbours");
  smooth->setProperty("InputWorkspace", inWS);
  if (rect) {
    smooth->setProperty("SumPixelsX", SumX);
    smooth->setProperty("SumPixelsY", SumY);
  } else {
    smooth->setProperty<std::string>("RadiusUnits", "NumberOfPixels");
    smooth->setProperty("Radius",
                        static_cast<double>(SumX * SumY * SumX * SumY));
    smooth->setProperty("NumberOfNeighbours", SumX * SumY * SumX * SumY * 4);
    smooth->setProperty("SumNumberOfNeighbours", SumX * SumY);
  }
  smooth->executeAsChildAlg();

  progress.report();
  // Get back the result
  outWS = smooth->getProperty("OutputWorkspace");
  // Cast to the matrixOutputWS and save it
  this->setProperty("OutputWorkspace", outWS);
}

} // namespace Algorithms
} // namespace Mantid
