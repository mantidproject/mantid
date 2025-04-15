// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/AnyShapeAbsorption.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAlgorithms/BeamProfileFactory.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Rasterize.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AnyShapeAbsorption)

using namespace Kernel;
using namespace Geometry;
using namespace API;

AnyShapeAbsorption::AnyShapeAbsorption() : AbsorptionCorrection(), m_cubeSide(0.0) {}

void AnyShapeAbsorption::defineProperties() {
  auto moreThanZero = std::make_shared<BoundedValidator<double>>();
  moreThanZero->setLower(0.001);
  declareProperty("ElementSize", 1.0, moreThanZero, "The size of one side of an integration element cube in mm");
}

/// Fetch the properties and set the appropriate member variables
void AnyShapeAbsorption::retrieveProperties() {
  m_cubeSide = getProperty("ElementSize"); // in mm
  m_cubeSide *= 0.001;                     // now in m
}

std::string AnyShapeAbsorption::sampleXML() {
  // Returning an empty string signals to the base class that it should
  // use the object already attached to the sample.
  return std::string();
}

/// Calculate the distances for L1 and element size for each element in the
/// sample
void AnyShapeAbsorption::initialiseCachedDistances() {
  // First, check if a 'gauge volume' has been defined. If not, it's the same as
  // the sample.
  IObject_const_sptr integrationVolume;
  if (m_inputWS->run().hasProperty("GaugeVolume")) {
    integrationVolume = constructGaugeVolume();
  } else {
    try {
      auto beamProfile =
          Mantid::Algorithms::BeamProfileFactory::createBeamProfile(*m_inputWS->getInstrument(), Mantid::API::Sample());
      integrationVolume = beamProfile->getIntersectionWithSample(*m_sampleObject);
    } catch (...) {
      // If the beam profile is not defined, use the sample object
      integrationVolume = std::shared_ptr<const IObject>(m_sampleObject->clone());
    }
  }

  auto raster = Geometry::Rasterize::calculate(m_beamDirection, *m_sampleObject, *integrationVolume, m_cubeSide);
  m_sampleVolume = raster.totalvolume;
  if (raster.l1.size() == 0)
    throw std::runtime_error("Failed to rasterize shape");
  // move over the information
  m_numVolumeElements = raster.l1.size();
  m_L1s = std::move(raster.l1);
  m_elementPositions = std::move(raster.position);
  m_elementVolumes = std::move(raster.volume);
}

std::shared_ptr<const Geometry::IObject> AnyShapeAbsorption::constructGaugeVolume() {
  g_log.information("Calculating scattering within the gauge volume defined on "
                    "the input workspace");

  // Retrieve and create the gauge volume shape
  std::shared_ptr<const Geometry::IObject> volume =
      ShapeFactory().createShape(m_inputWS->run().getProperty("GaugeVolume")->value());

  return volume;
}

} // namespace Mantid::Algorithms
