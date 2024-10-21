// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//-------------------------------------------------------------
// Includes
//-------------------------------------------------------------
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/V3D.h"
#include <deque>
#include <iterator>
#include <utility>

namespace Mantid::Geometry {

using Kernel::V3D;

//-------------------------------------------------------------
// Public member functions
//-------------------------------------------------------------

/**
 * Constructor specifying the instrument involved in the tracing. The instrument
 * must have defined a source
 * component.
 * @param instrument :: The instrument to perform the ray tracings on. It must
 * have a defined source.
 */
InstrumentRayTracer::InstrumentRayTracer(Instrument_const_sptr instrument) : m_instrument(std::move(instrument)) {
  if (!m_instrument) {
    std::ostringstream lexer;
    lexer << "Cannot create a InstrumentRayTracer, invalid instrument given. "
             "Input = "
          << m_instrument.get() << "\n";
    throw std::invalid_argument(lexer.str());
  }
  if (!m_instrument->getSource()) {
    std::string errorMsg = "Cannot create InstrumentRayTracer, instrument has "
                           "no defined source.\n";
    throw std::invalid_argument(errorMsg);
  }
}

/**
 * Trace a given track from the instrument source in the given direction. For
 * performance reasons the
 * results are accumulated within the object and can be returned using
 * getResults.
 * @param dir :: A directional vector. The starting point is defined by
 * the instrument source.
 */
void InstrumentRayTracer::trace(const V3D &dir) const {
  // Define the track with the source position and the given direction.
  m_resultsTrack.reset(m_instrument->getSource()->getPos(), dir);
  // m_resultsTrack.reset(m_instrument->getSample()->getPos() +
  // V3D(1.0,0.0,0.0), dir);
  // The intersection results are accumulated within the ray object
  fireRay(m_resultsTrack);
}

/**
 * Trace a given track from the sample position in the given direction. For
 * performance reasons the
 * results are accumulated within the object and can be returned using
 * getResults.
 * @param dir :: A directional vector. The starting point is defined by
 * the instrument source.
 */
void InstrumentRayTracer::traceFromSample(const V3D &dir) const {
  // Define the track with the sample position and the given direction.
  m_resultsTrack.reset(m_instrument->getSample()->getPos(), dir);
  // The intersection results are accumulated within the ray object
  fireRay(m_resultsTrack);
}

/**
 * Return the results of any trace() calls since the last call the getResults.
 * @returns A collection of links defining intersection information
 */
Links InstrumentRayTracer::getResults() const {
  Links results(m_resultsTrack.cbegin(), m_resultsTrack.cend());
  m_resultsTrack.clearIntersectionResults();
  return results;
}

//----------------------------------------------------------------------------------
/** Gets the results of the trace, then returns the first detector
 * (that is NOT a monitor) found in the results.
 * @return sptr to IDetector, or an invalid sptr if not found
 */
IDetector_const_sptr InstrumentRayTracer::getDetectorResult() const {
  Links results = this->getResults();

  // Go through all results
  Links::const_iterator resultItr = results.begin();
  for (; resultItr != results.end(); ++resultItr) {
    IComponent_const_sptr component = m_instrument->getComponentByID(resultItr->componentID);
    IDetector_const_sptr det = std::dynamic_pointer_cast<const IDetector>(component);
    if (det) {
      if (!m_instrument->isMonitor(det->getID())) {
        return det;
      }
    } // (is a detector)
  } // each ray tracer result
  return IDetector_const_sptr();
}

//-------------------------------------------------------------
// Private member functions
//-------------------------------------------------------------
/**
 * Fire the test ray at the instrument and perform a bread-first search of the
 * object tree to find the objects that were intersected.
 * @param testRay :: An input/output parameter that defines the track and
 * accumulates the
 *        intersection results
 */
void InstrumentRayTracer::fireRay(Track &testRay) const {
  // Go through the instrument tree and see if we get any hits by
  // (a) first testing the bounding box and if we're inside that then
  // (b) test the lower components.
  std::deque<IComponent_const_sptr> nodeQueue;

  // Start at the root of the tree
  nodeQueue.emplace_back(m_instrument);

  IComponent_const_sptr node;
  while (!nodeQueue.empty()) {
    node = nodeQueue.front();
    nodeQueue.pop_front();
    BoundingBox bbox;
    auto it = m_boxCache.find(node->getComponentID());
    if (it != m_boxCache.end()) {
      bbox = it->second;
    } else {
      node->getBoundingBox(bbox);
      std::lock_guard<std::mutex> lock(m_mutex);
      m_boxCache[node->getComponentID()] = bbox;
    }

    // Quick test. If this suceeds moved on to test the children
    if (bbox.doesLineIntersect(testRay)) {
      if (ICompAssembly_const_sptr assembly = std::dynamic_pointer_cast<const ICompAssembly>(node)) {
        assembly->testIntersectionWithChildren(testRay, nodeQueue);
      } else {
        throw Kernel::Exception::NotImplementedError("Implement non-comp assembly interactions");
      }
    }
  }
}

///**
// * Perform a quick check as to whether the ray passes through the component
// * @param component :: The test component
// */
// bool InstrumentRayTracer::quickIntersectCheck(std::shared_ptr<IComponent>
// component, const Track & testRay) const
// {
//
// }
//
// /**
//  * Perform a proper intersection test of the physical object and accumulate
//  the results if necessary
//  * @param testRay :: An input/output parameter that defines the track and
//  accumulates the
//  * intersection results
//  */
// void slowIntersectCheck(std::shared_ptr<IComponent> component, Track &
// testRay) const;
} // namespace Mantid::Geometry
