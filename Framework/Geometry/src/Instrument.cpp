// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/ComponentInfoBankHelpers.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/GridDetectorPixel.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Unit.h"
#include "MantidNexus/NeXusFile.hpp"

#include <algorithm>
#include <memory>
#include <queue>
#include <utility>

using namespace Mantid::Kernel;
using Mantid::Kernel::Exception::InstrumentDefinitionError;
using Mantid::Kernel::Exception::NotFoundError;

namespace Mantid::Geometry {

namespace {
Kernel::Logger g_log("Instrument");

void raiseDuplicateDetectorError(const size_t detectorId) {
  std::stringstream sstream;
  sstream << "Instrument Definition corrupt. Detector with ID " << detectorId << " already exists.";
  throw Exception::InstrumentDefinitionError(sstream.str());
}
} // namespace

/// Default constructor
Instrument::Instrument()
    : CompAssembly(), m_detectorCache(), m_sourceCache(nullptr), m_sampleCache(nullptr), m_defaultView("3D"),
      m_defaultViewAxis("Z+"), m_referenceFrame(new ReferenceFrame) {}

/// Constructor with name
Instrument::Instrument(const std::string &name)
    : CompAssembly(name), m_detectorCache(), m_sourceCache(nullptr), m_sampleCache(nullptr), m_defaultView("3D"),
      m_defaultViewAxis("Z+"), m_referenceFrame(new ReferenceFrame) {}

/** Constructor to create a parametrized instrument
 *  @param instr :: instrument for parameter inclusion
 *  @param map :: parameter map to include
 */
Instrument::Instrument(const std::shared_ptr<const Instrument> &instr, const std::shared_ptr<ParameterMap> &map)
    : CompAssembly(instr.get(), map.get()), m_sourceCache(instr->m_sourceCache), m_sampleCache(instr->m_sampleCache),
      m_defaultView(instr->m_defaultView), m_defaultViewAxis(instr->m_defaultViewAxis), m_instr(instr),
      m_map_nonconst(map), m_ValidFrom(instr->m_ValidFrom), m_ValidTo(instr->m_ValidTo),
      m_referenceFrame(new ReferenceFrame) {
  // Note that we do not copy m_detectorInfo and m_componentInfo into the
  // parametrized instrument since the ParameterMap will make a copy, if
  // applicable.
}

/** Copy constructor
 *  This method was added to deal with having distinct neutronic and physical
 * positions
 *  in indirect instruments.
 */
Instrument::Instrument(const Instrument &instr)
    : CompAssembly(instr), m_sourceCache(nullptr), m_sampleCache(nullptr), /* Should only be temporarily null */
      m_logfileCache(instr.m_logfileCache), m_logfileUnit(instr.m_logfileUnit), m_defaultView(instr.m_defaultView),
      m_defaultViewAxis(instr.m_defaultViewAxis), m_instr(), m_map_nonconst(), /* Should not be parameterized */
      m_ValidFrom(instr.m_ValidFrom), m_ValidTo(instr.m_ValidTo), m_referenceFrame(instr.m_referenceFrame) {
  // Note that we do not copy m_detectorInfo and m_componentInfo into the new
  // instrument since they are only non-NULL for the base instrument, which
  // should usually not be copied.

  // Now we need to fill the detector, source and sample caches with pointers
  // into the new instrument
  std::vector<IComponent_const_sptr> children;
  getChildren(children, true);
  std::vector<IComponent_const_sptr>::const_iterator it;
  for (it = children.begin(); it != children.end(); ++it) {
    // First check if the current component is a detector and add to cache if it
    // is
    if (const IDetector *det = dynamic_cast<const Detector *>(it->get())) {
      if (instr.isMonitor(det->getID()))
        markAsMonitor(det);
      else
        markAsDetector(det);
      continue;
    }
    // Now check whether the current component is the source or sample.
    // As the majority of components will be detectors, we will rarely get to
    // here
    if (const auto *obj = dynamic_cast<const Component *>(it->get())) {
      const std::string objName = obj->getName();
      // This relies on the source and sample having a unique name.
      // I think the way our instrument definition files work ensures this is
      // the case.
      if (objName == instr.m_sourceCache->getName()) {
        markAsSource(obj);
        continue;
      }
      if (objName == instr.m_sampleCache->getName()) {
        markAsSamplePos(obj);
        continue;
      }
    }
  }
}

/// Virtual copy constructor
Instrument *Instrument::clone() const { return new Instrument(*this); }

/// Pointer to the 'real' instrument, for parametrized instruments
Instrument_const_sptr Instrument::baseInstrument() const {
  if (m_map)
    return m_instr;
  else
    throw std::runtime_error("Instrument::baseInstrument() called for a "
                             "non-parametrized instrument.");
}

/**
 * Pointer to the ParameterMap holding the parameters of the modified instrument
 * components.
 * @return parameter map from modified instrument components
 */
ParameterMap_sptr Instrument::getParameterMap() const {
  if (m_map)
    return m_map_nonconst;
  else
    throw std::runtime_error("Instrument::getParameterMap() called for a "
                             "non-parametrized instrument.");
}

/** INDIRECT GEOMETRY INSTRUMENTS ONLY: Returns the physical instrument,
 *  if one has been specified as distinct from the 'neutronic' one.
 *  Otherwise (and most commonly) returns a null pointer, meaning that the
 * holding
 *  instrument is already the physical instrument.
 */
Instrument_const_sptr Instrument::getPhysicalInstrument() const {
  if (m_map) {
    if (m_instr->getPhysicalInstrument()) {
      // A physical instrument should use the same parameter map as the 'main'
      // instrument. This constructor automatically sets the instrument as the
      // owning instrument in the ParameterMap. We need to undo this immediately
      // since the ParameterMap must always be owned by the neutronic
      // instrument.
      return std::make_shared<Instrument>(m_instr->getPhysicalInstrument(), m_map_nonconst);
    } else {
      return Instrument_const_sptr();
    }
  } else {
    return m_physicalInstrument;
  }
}

/** INDIRECT GEOMETRY INSTRUMENTS ONLY: Sets the physical instrument.
 *  The holding instrument is then the 'neutronic' one, and is used in all
 * algorithms.
 *  @param physInst A pointer to the physical instrument object.
 */
void Instrument::setPhysicalInstrument(std::unique_ptr<Instrument> physInst) {
  if (!m_map) {
    physInst->m_isPhysicalInstrument = true;
    m_physicalInstrument = std::move(physInst);
  } else
    throw std::runtime_error("Instrument::setPhysicalInstrument() called on a "
                             "parametrized instrument.");
}

//------------------------------------------------------------------------------------------
/**	Fills a copy of the detector cache
 */
void Instrument::getDetectors(detid2det_map &out_map) const {
  if (m_map) {
    // Get the base instrument detectors
    out_map.clear();
    const auto &in_dets = m_instr->m_detectorCache;
    // And turn them into parametrized versions
    for (const auto &in_det : in_dets) {
      out_map.emplace(std::get<0>(in_det), getDetector(std::get<0>(in_det)));
    }
  } else {
    // You can just return the detector cache directly.
    out_map.clear();
    for (const auto &in_det : m_detectorCache)
      out_map.emplace(std::get<0>(in_det), std::get<1>(in_det));
  }
}

//------------------------------------------------------------------------------------------
/** Return a vector of detector IDs in this instrument */
std::vector<detid_t> Instrument::getDetectorIDs(bool skipMonitors) const {
  std::vector<detid_t> out;
  if (m_map) {
    const auto &in_dets = m_instr->m_detectorCache;
    for (const auto &in_det : in_dets)
      if (!skipMonitors || !std::get<2>(in_det))
        out.emplace_back(std::get<0>(in_det));
  } else {
    const auto &in_dets = m_detectorCache;
    for (const auto &in_det : in_dets)
      if (!skipMonitors || !std::get<2>(in_det))
        out.emplace_back(std::get<0>(in_det));
  }
  return out;
}

/// @return The total number of detector IDs in the instrument */
std::size_t Instrument::getNumberDetectors(bool skipMonitors) const {
  std::size_t numDetIDs(0);

  if (m_map) {
    numDetIDs = m_instr->m_detectorCache.size();
  } else {
    numDetIDs = m_detectorCache.size();
  }

  if (skipMonitors) // this slow, but gets the right answer
  {
    std::size_t monitors(0);
    if (m_map) {
      const auto &in_dets = m_instr->m_detectorCache;
      monitors =
          std::count_if(in_dets.cbegin(), in_dets.cend(), [](const auto &in_det) { return std::get<2>(in_det); });
    } else {
      const auto &in_dets = m_detectorCache;
      monitors =
          std::count_if(in_dets.cbegin(), in_dets.cend(), [](const auto &in_det) { return std::get<2>(in_det); });
    }
    return (numDetIDs - monitors);
  } else {
    return numDetIDs;
  }
}

/** Get the minimum and maximum (inclusive) detector IDs
 *
 * @param min :: set to the min detector ID
 * @param max :: set to the max detector ID
 */
void Instrument::getMinMaxDetectorIDs(detid_t &min, detid_t &max) const {
  const auto *in_dets = m_map ? &m_instr->m_detectorCache : &m_detectorCache;

  if (in_dets->empty())
    throw std::runtime_error("No detectors on this instrument. Can't find min/max ids");
  // Maps are sorted by key. So it is easy to find
  min = std::get<0>(*in_dets->begin());
  max = std::get<0>(*in_dets->rbegin());
}

/** Fill a vector with all the detectors contained (at any depth) in a named
 *component. For example,
 * you might have a bank10 with 4 tubes with 100 pixels each; this will return
 *the
 * 400 contained Detector objects.
 *
 * @param[out] dets :: vector filled with detector pointers
 * @param comp :: the parent component assembly that contains detectors.
 */
void Instrument::getDetectorsInBank(std::vector<IDetector_const_sptr> &dets, const IComponent &comp) const {
  const auto bank = dynamic_cast<const ICompAssembly *>(&comp);
  if (bank) {
    // Get a vector of children (recursively)
    std::vector<std::shared_ptr<const IComponent>> children;
    bank->getChildren(children, true);
    std::vector<std::shared_ptr<const IComponent>>::iterator it;
    for (it = children.begin(); it != children.end(); ++it) {
      IDetector_const_sptr det = std::dynamic_pointer_cast<const IDetector>(*it);
      if (det) {
        dets.emplace_back(det);
      }
    }
  }
}

/** Fill a vector with all the detectors contained (at any depth) in a named
 * component. For example, you might have a bank10 with 4 tubes with 100
 * pixels each; this will return the 400 contained Detector objects.
 *
 * @param[out] dets :: vector filled with detector pointers
 * @param bankName :: name of the parent component assembly that contains
 * detectors. The name must be unique, otherwise the first matching component
 * (getComponentByName) is used.
 * @throws NotFoundError if the given bank does not exist.
 */
void Instrument::getDetectorsInBank(std::vector<IDetector_const_sptr> &dets, const std::string &bankName) const {
  std::shared_ptr<const IComponent> comp = this->getComponentByName(bankName);
  if (!comp) {
    throw Kernel::Exception::NotFoundError("Instrument: Could not find component", bankName);
  }
  getDetectorsInBank(dets, *comp);
}

/** Checks to see if the Instrument has a source.
 *   @returns True if the instrument has a source cache.
 */
bool Instrument::hasSource() const { return m_sourceCache; }

/** Checks to see if the Instrument has a sample.
 *   @returns True if the instrument has a sample cache.
 */
bool Instrument::hasSample() const { return m_sampleCache; }

/** Gets a pointer to the source
 *   @returns a pointer to the source
 */
IComponent_const_sptr Instrument::getSource() const {
  if (!m_sourceCache) {
    g_log.warning("In Instrument::getSource(). No source has been set.");
    return IComponent_const_sptr(m_sourceCache, NoDeleting());
  } else if (m_map) {
    auto sourceCache = static_cast<const Instrument *>(m_base)->m_sourceCache;
    if (dynamic_cast<const ObjComponent *>(sourceCache))
      return IComponent_const_sptr(new ObjComponent(sourceCache, m_map));
    else if (dynamic_cast<const CompAssembly *>(sourceCache))
      return IComponent_const_sptr(new CompAssembly(sourceCache, m_map));
    else if (dynamic_cast<const Component *>(sourceCache))
      return IComponent_const_sptr(new Component(sourceCache, m_map));
    else {
      g_log.error("In Instrument::getSource(). Source is not a recognised "
                  "component type.");
      g_log.error("Try to assume it is a Component.");
      return IComponent_const_sptr(new ObjComponent(sourceCache, m_map));
    }
  } else {
    return IComponent_const_sptr(m_sourceCache, NoDeleting());
  }
}

/** Gets a pointer to the Sample Position
 *  @returns a pointer to the Sample Position
 */
IComponent_const_sptr Instrument::getSample() const {
  if (!m_sampleCache) {
    g_log.warning("In Instrument::getSamplePos(). No SamplePos has been set.");
    return IComponent_const_sptr(m_sampleCache, NoDeleting());
  } else if (m_map) {
    auto sampleCache = static_cast<const Instrument *>(m_base)->m_sampleCache;
    if (dynamic_cast<const ObjComponent *>(sampleCache))
      return IComponent_const_sptr(new ObjComponent(sampleCache, m_map));
    else if (dynamic_cast<const CompAssembly *>(sampleCache))
      return IComponent_const_sptr(new CompAssembly(sampleCache, m_map));
    else if (dynamic_cast<const Component *>(sampleCache))
      return IComponent_const_sptr(new Component(sampleCache, m_map));
    else {
      g_log.error("In Instrument::getSamplePos(). SamplePos is not a "
                  "recognised component type.");
      g_log.error("Try to assume it is a Component.");
      return IComponent_const_sptr(new ObjComponent(sampleCache, m_map));
    }
  } else {
    return IComponent_const_sptr(m_sampleCache, NoDeleting());
  }
}

/** Gets the beam direction (i.e. source->sample direction).
 *  Not virtual because it relies the getSample() & getPos() virtual functions
 *  @returns A unit vector denoting the direction of the beam
 */
Kernel::V3D Instrument::getBeamDirection() const { return normalize(getSample()->getPos() - getSource()->getPos()); }

//------------------------------------------------------------------------------------------
/**  Get a shared pointer to a component by its ID, const version
 *   @param id :: ID
 *   @return A pointer to the component.
 */
std::shared_ptr<const IComponent> Instrument::getComponentByID(const IComponent *id) const {
  const auto *base = static_cast<const IComponent *>(id);
  if (m_map)
    return ParComponentFactory::create(std::shared_ptr<const IComponent>(base, NoDeleting()), m_map);
  else
    return std::shared_ptr<const IComponent>(base, NoDeleting());
}

/** Find all components in an Instrument Definition File (IDF) with a given
 * name. If you know a component
 *  has a unique name use instead getComponentByName(), which is as fast or
 * faster for retrieving a uniquely
 *  named component.
 *  @param cname :: The name of the component. If there are multiple matches,
 * the first one found is returned.
 *  @returns Pointers to components
 */
std::vector<std::shared_ptr<const IComponent>> Instrument::getAllComponentsWithName(const std::string &cname) const {
  std::shared_ptr<const IComponent> node = std::shared_ptr<const IComponent>(this, NoDeleting());
  std::vector<std::shared_ptr<const IComponent>> retVec;
  // Check the instrument name first
  if (this->getName() == cname) {
    retVec.emplace_back(node);
  }
  // Same algorithm as used in getComponentByName() but searching the full tree
  std::deque<std::shared_ptr<const IComponent>> nodeQueue;
  // Need to be able to enter the while loop
  nodeQueue.emplace_back(node);
  while (!nodeQueue.empty()) {
    node = nodeQueue.front();
    nodeQueue.pop_front();
    int nchildren(0);
    std::shared_ptr<const ICompAssembly> asmb = std::dynamic_pointer_cast<const ICompAssembly>(node);
    if (asmb) {
      nchildren = asmb->nelements();
    }
    for (int i = 0; i < nchildren; ++i) {
      std::shared_ptr<const IComponent> comp = (*asmb)[i];
      if (comp->getName() == cname) {
        retVec.emplace_back(comp);
      } else {
        nodeQueue.emplace_back(comp);
      }
    }
  } // while-end

  // If we have reached here then the search failed
  return retVec;
}

namespace {
// Helpers for accessing m_detectorCache, which is a vector of tuples used as a
// map. Lookup is by first element in a tuple. Templated to support const and
// non-const.
template <class T> auto lower_bound(T &map, const detid_t key) -> decltype(map.begin()) {
  return std::lower_bound(map.begin(), map.end(), std::make_tuple(key, IDetector_const_sptr(nullptr), false),
                          [](const typename T::value_type &a, const typename T::value_type &b) -> bool {
                            return std::get<0>(a) < std::get<0>(b);
                          });
}

template <class T> auto find(T &map, const detid_t key) -> decltype(map.begin()) {
  auto it = lower_bound(map, key);
  if ((it != map.end()) && (std::get<0>(*it) == key))
    return it;
  return map.end();
}
} // namespace

/**	Gets a pointer to the detector from its ID
 *  Note that for getting the detector associated with a spectrum, the
 * MatrixWorkspace::getDetector
 *  method should be used rather than this one because it takes account of the
 * possibility of more
 *  than one detector contributing to a single spectrum
 *  @param   detector_id The requested detector ID
 *  @returns A pointer to the detector object
 *  @throw   NotFoundError If no detector is found for the detector ID given
 */
IDetector_const_sptr Instrument::getDetector(const detid_t &detector_id) const {
  const auto &baseInstr = m_map ? *m_instr : *this;
  const auto it = find(baseInstr.m_detectorCache, detector_id);
  if (it == baseInstr.m_detectorCache.end()) {
    std::stringstream readInt;
    readInt << detector_id;
    throw Kernel::Exception::NotFoundError("Instrument: Detector with ID " + readInt.str() + " not found.", "");
  }
  IDetector_const_sptr baseDet = std::get<1>(*it);

  if (!m_map)
    return baseDet;

  auto det = ParComponentFactory::createDetector(baseDet.get(), m_map);
  return det;
}

/**	Gets a pointer to the base (non-parametrized) detector from its ID
 * returns null if the detector has not been found
 *  @param   detector_id The requested detector ID
 *  @returns A const pointer to the detector object
 */
const IDetector *Instrument::getBaseDetector(const detid_t &detector_id) const {
  auto it = find(m_instr->m_detectorCache, detector_id);
  if (it == m_instr->m_detectorCache.end()) {
    return nullptr;
  }
  return std::get<1>(*it).get();
}

bool Instrument::isMonitor(const detid_t &detector_id) const {
  const auto &baseInstr = m_map ? *m_instr : *this;
  const auto it = find(baseInstr.m_detectorCache, detector_id);
  if (it == baseInstr.m_detectorCache.end())
    return false;
  return std::get<2>(*it);
}

bool Instrument::isMonitor(const std::set<detid_t> &detector_ids) const {
  if (detector_ids.empty())
    return false;

  return std::any_of(detector_ids.cbegin(), detector_ids.cend(),
                     [this](const auto detector_id) { return isMonitor(detector_id); });
}

/**
 * Returns a pointer to the geometrical object for the given set of IDs
 * @param det_ids :: A list of detector ids
 *  @returns A pointer to the detector object
 *  @throw   NotFoundError If no detector is found for the detector ID given
 */
IDetector_const_sptr Instrument::getDetectorG(const std::set<detid_t> &det_ids) const {
  const size_t ndets(det_ids.size());
  if (ndets == 1) {
    return this->getDetector(*det_ids.begin());
  } else {
    std::shared_ptr<DetectorGroup> det_group = std::make_shared<DetectorGroup>();
    for (const auto detID : det_ids) {
      det_group->addDetector(this->getDetector(detID));
    }
    return det_group;
  }
}

/**
 * Returns a list of Detectors for the given detectors ids
 *
 */
std::vector<IDetector_const_sptr> Instrument::getDetectors(const std::vector<detid_t> &det_ids) const {
  std::vector<IDetector_const_sptr> dets_ptr;
  dets_ptr.reserve(det_ids.size());
  std::vector<detid_t>::const_iterator it;
  for (it = det_ids.begin(); it != det_ids.end(); ++it) {
    dets_ptr.emplace_back(this->getDetector(*it));
  }
  return dets_ptr;
}

/**
 * Returns a list of Detectors for the given detectors ids
 *
 */
std::vector<IDetector_const_sptr> Instrument::getDetectors(const std::set<detid_t> &det_ids) const {
  std::vector<IDetector_const_sptr> dets_ptr;
  dets_ptr.reserve(det_ids.size());
  std::set<detid_t>::const_iterator it;
  for (it = det_ids.begin(); it != det_ids.end(); ++it) {
    dets_ptr.emplace_back(this->getDetector(*it));
  }
  return dets_ptr;
}

/** Mark a component which has already been added to the Instrument (as a child
 *component)
 *  to be 'the' samplePos component. NOTE THOUGH THAT THIS METHOD DOES NOT
 *VERIFY THAT THIS
 *  IS THE CASE. It is assumed that we have at only one of these.
 *  The component is required to have a name.
 *
 *  @param comp :: Component to be marked (stored for later retrieval) as a
 *"SamplePos" Component
 */
void Instrument::markAsSamplePos(const IComponent *comp) {
  if (m_map)
    throw std::runtime_error("Instrument::markAsSamplePos() called on a "
                             "parametrized Instrument object.");

  auto objComp = dynamic_cast<const IObjComponent *>(comp);
  if (objComp) {
    throw std::runtime_error("Instrument::markAsSamplePos() called on an IObjComponent "
                             "object that supports shape definition. Sample is prevented from "
                             "being this type because the shape must only be stored in "
                             "ExperimentInfo::m_sample.");
  }

  if (!m_sampleCache) {
    if (comp->getName().empty()) {
      throw Exception::InstrumentDefinitionError("The sample component is required to have a name.");
    }
    m_sampleCache = comp;
  } else {
    g_log.warning("Have already added samplePos component to the _sampleCache.");
  }
}

/** Mark a component which has already been added to the Instrument (as a child
 *component)
 *  to be 'the' source component.NOTE THOUGH THAT THIS METHOD DOES NOT VERIFY
 *THAT THIS
 *  IS THE CASE. It is assumed that we have at only one of these.
 *  The component is required to have a name.
 *
 *  @param comp :: Component to be marked (stored for later retrieval) as a
 *"source" Component
 */
void Instrument::markAsSource(const IComponent *comp) {
  if (m_map)
    throw std::runtime_error("Instrument::markAsSource() called on a "
                             "parametrized Instrument object.");

  if (!m_sourceCache) {
    if (comp->getName().empty()) {
      throw Exception::InstrumentDefinitionError("The source component is required to have a name.");
    }
    m_sourceCache = comp;
  } else {
    g_log.warning("Have already added source component to the _sourceCache.");
  }
}

/** Mark a Component which has already been added to the Instrument (as a child
 *component)
 * to be a Detector by adding it to a detector cache.
 *
 * @param det :: Component to be marked (stored for later retrieval) as a
 *detector Component
 *
 */
void Instrument::markAsDetector(const IDetector *det) {
  if (m_map)
    throw std::runtime_error("Instrument::markAsDetector() called on a "
                             "parametrized Instrument object.");

  // Create a (non-deleting) shared pointer to it
  IDetector_const_sptr det_sptr = IDetector_const_sptr(det, NoDeleting());
  auto it = lower_bound(m_detectorCache, det->getID());
  // Duplicate detector ids are forbidden
  if ((it != m_detectorCache.end()) && (std::get<0>(*it) == det->getID())) {
    raiseDuplicateDetectorError(det->getID());
  }
  bool isMonitor = false;
  m_detectorCache.emplace(it, det->getID(), det_sptr, isMonitor);
}

/// As markAsDetector but without the required sorting. Must call
/// markAsDetectorFinalize before accessing detectors.
void Instrument::markAsDetectorIncomplete(const IDetector *det) {
  if (m_map)
    throw std::runtime_error("Instrument::markAsDetector() called on a "
                             "parametrized Instrument object.");

  // Create a (non-deleting) shared pointer to it
  IDetector_const_sptr det_sptr = IDetector_const_sptr(det, NoDeleting());
  bool isMonitor = false;
  m_detectorCache.emplace_back(det->getID(), det_sptr, isMonitor);
}

/// Sorts the detector cache. Called after all detectors have been marked via
/// markAsDetectorIncomplete.
void Instrument::markAsDetectorFinalize() {
  // Detectors (even when different objects) are NOT allowed to have duplicate
  // ids. This method establishes the presence of duplicates.
  std::sort(
      m_detectorCache.begin(), m_detectorCache.end(),
      [](const std::tuple<detid_t, IDetector_const_sptr, bool> &a,
         const std::tuple<detid_t, IDetector_const_sptr, bool> &b) -> bool { return std::get<0>(a) < std::get<0>(b); });

  auto resultIt = std::adjacent_find(m_detectorCache.begin(), m_detectorCache.end(),
                                     [](const std::tuple<detid_t, IDetector_const_sptr, bool> &a,
                                        const std::tuple<detid_t, IDetector_const_sptr, bool> &b) -> bool {
                                       return std::get<0>(a) == std::get<0>(b);
                                     });
  if (resultIt != m_detectorCache.end()) {
    raiseDuplicateDetectorError(std::get<0>(*resultIt));
  }
}

/** Mark a Component which has already been added to the Instrument class
 * as a monitor and add it to the detector cache.
 *
 * @param det :: Component to be marked (stored for later retrieval) as a
 *detector Component
 *
 * @throw Exception::ExistsError if cannot add detector to cache
 */
void Instrument::markAsMonitor(const IDetector *det) {
  if (m_map)
    throw std::runtime_error("Instrument::markAsMonitor() called on a "
                             "parametrized Instrument object.");

  // attempt to add monitor to instrument detector cache
  markAsDetector(det);

  // mark detector as a monitor
  auto it = find(m_detectorCache, det->getID());
  std::get<2>(*it) = true;
}

/** Removes a detector from the instrument and from the detector cache.
 *  The object is deleted.
 *  @param det The detector to remove
 */
void Instrument::removeDetector(IDetector *det) {
  if (m_map)
    throw std::runtime_error("Instrument::removeDetector() called on a "
                             "parameterized Instrument object.");

  const detid_t id = det->getID();
  // Remove the detector from the detector cache
  const auto it = find(m_detectorCache, id);
  m_detectorCache.erase(it);

  // Remove it from the parent assembly (and thus the instrument). Evilness
  // required here unfortunately.
  auto *parentAssembly = dynamic_cast<CompAssembly *>(const_cast<IComponent *>(det->getBareParent()));
  if (parentAssembly) // Should always be true, but check just in case
  {
    parentAssembly->remove(det);
  }
}

/** This method returns monitor detector ids
 *  @return a vector holding detector ids of  monitors
 */
std::vector<detid_t> Instrument::getMonitors() const {
  // Monitors cannot be parametrized. So just return the base.
  if (m_map)
    return m_instr->getMonitors();

  std::vector<detid_t> mons;
  for (const auto &item : m_detectorCache)
    if (std::get<2>(item))
      mons.emplace_back(std::get<0>(item));
  return mons;
}

/**
 * Get the bounding box for this instrument. It is simply the sum of the
 * bounding boxes of its children excluding the source
 * @param assemblyBox :: [Out] The resulting bounding box is stored here.
 */
void Instrument::getBoundingBox(BoundingBox &assemblyBox) const {
  if (m_map) {

    if (m_map->hasComponentInfo(this->baseInstrument().get())) {
      assemblyBox = m_map->componentInfo().boundingBox(index(), &assemblyBox);
      return;
    }

    // Loop over the children and define a box large enough for all of them
    ComponentID sourceID = getSource()->getComponentID();
    assemblyBox = BoundingBox(); // this makes the instrument BB always axis aligned
    int nchildren = nelements();
    for (int i = 0; i < nchildren; ++i) {
      IComponent_sptr comp = this->getChild(i);
      if (comp && comp->getComponentID() != sourceID) {
        BoundingBox compBox;
        comp->getBoundingBox(compBox);
        assemblyBox.grow(compBox);
      }
    }
  } else {

    if (!m_cachedBoundingBox) {
      m_cachedBoundingBox = new BoundingBox();
      ComponentID sourceID = getSource()->getComponentID();
      // Loop over the children and define a box large enough for all of them
      for (const auto component : m_children) {
        BoundingBox compBox;
        if (component && component->getComponentID() != sourceID) {
          component->getBoundingBox(compBox);
          m_cachedBoundingBox->grow(compBox);
        }
      }
    }
    // Use cached box
    assemblyBox = *m_cachedBoundingBox;
  }
}

std::shared_ptr<const std::vector<IObjComponent_const_sptr>> Instrument::getPlottable() const {
  if (m_map) {
    // Get the 'base' plottable components
    std::shared_ptr<const std::vector<IObjComponent_const_sptr>> objs = m_instr->getPlottable();

    // Get a reference to the underlying vector, casting away the constness so
    // that we
    // can modify it to get our result rather than creating another long vector
    auto &res = const_cast<std::vector<IObjComponent_const_sptr> &>(*objs);
    const std::vector<IObjComponent_const_sptr>::size_type total = res.size();
    for (std::vector<IObjComponent_const_sptr>::size_type i = 0; i < total; ++i) {
      res[i] = std::dynamic_pointer_cast<const Detector>(ParComponentFactory::create(objs->at(i), m_map));
    }
    return objs;

  } else {
    // Base instrument
    auto res = std::make_shared<std::vector<IObjComponent_const_sptr>>();
    res->reserve(m_detectorCache.size() + 10);
    appendPlottable(*this, *res);
    return res;
  }
}

void Instrument::appendPlottable(const CompAssembly &ca, std::vector<IObjComponent_const_sptr> &lst) const {
  for (int i = 0; i < ca.nelements(); i++) {
    IComponent *c = ca[i].get();
    const auto *a = dynamic_cast<CompAssembly *>(c);
    if (a)
      appendPlottable(*a, lst);
    else {
      auto *d = dynamic_cast<Detector *>(c);
      auto *o = dynamic_cast<ObjComponent *>(c);
      if (d)
        lst.emplace_back(IObjComponent_const_sptr(d, NoDeleting()));
      else if (o)
        lst.emplace_back(IObjComponent_const_sptr(o, NoDeleting()));
      else
        g_log.error() << "Unknown comp type\n";
    }
  }
}

//------------------------------------------------------------------------------------------------
/** Get several instrument parameters used in tof to D-space conversion
 *
 * @param l1 :: primary flight path (source-sample distance)
 * @param beamline :: vector of the direction and length of the beam (source to
 *samepl)
 * @param beamline_norm :: 2 * the length of beamline
 * @param samplePos :: vector of the position of the sample
 */
void Instrument::getInstrumentParameters(double &l1, Kernel::V3D &beamline, double &beamline_norm,
                                         Kernel::V3D &samplePos) const {
  // Get some positions
  const IComponent_const_sptr sourceObj = this->getSource();
  if (sourceObj == nullptr) {
    throw Exception::InstrumentDefinitionError("Failed to get source component from instrument");
  }
  const Kernel::V3D sourcePos = sourceObj->getPos();
  samplePos = this->getSample()->getPos();
  beamline = samplePos - sourcePos;
  beamline_norm = 2.0 * beamline.norm();

  // Get the distance between the source and the sample (assume in metres)
  IComponent_const_sptr sample = this->getSample();
  try {
    l1 = this->getSource()->getDistance(*sample);
  } catch (Exception::NotFoundError &) {
    throw Exception::InstrumentDefinitionError("Unable to calculate source-sample distance ", this->getName());
  }
}

//--------------------------------------------------------------------------------------------
/** Set the path to the original IDF .xml file that was loaded for this
 * instrument */
void Instrument::setFilename(const std::string &filename) {
  if (m_map)
    m_instr->m_filename = filename;
  else
    m_filename = filename;
}

/** @return the path to the original IDF .xml file that was loaded for this
 * instrument */
const std::string &Instrument::getFilename() const {
  if (m_map)
    return m_instr->getFilename();
  else
    return m_filename;
}

/** Set the Contents of the IDF .xml file that was loaded for this instrument */
void Instrument::setXmlText(const std::string &XmlText) {
  if (m_map)
    m_instr->m_xmlText = XmlText;
  else
    m_xmlText = XmlText;
}

/** @return Contents of the IDF .xml file that was loaded for this instrument */
const std::string &Instrument::getXmlText() const {
  if (m_map)
    return m_instr->getXmlText();
  else
    return m_xmlText;
}

//--------------------------------------------------------------------------------------------
/** Save the instrument to an open NeXus file.
 * This saves the name, valid date, source xml file name, and the full XML text
 * of the definition file.
 * It also saves the parameter map, in the case of a parametrized instrument.
 *
 * @param file :: open NeXus file
 * @param group :: name of the group to create
 */
void Instrument::saveNexus(::NeXus::File *file, const std::string &group) const {
  file->makeGroup(group, "NXinstrument", true);
  file->putAttr("version", 1);

  file->writeData("name", getName());

  // XML contents of instrument, as a NX note
  file->makeGroup("instrument_xml", "NXnote", true);
  const std::string &xmlText = getXmlText();
  if (xmlText.empty())
    g_log.warning() << "Saving Instrument with no XML data. If this was "
                       "instrument data you may not be able to load this data "
                       "back into Mantid, for fitted/analysed data this "
                       "warning can be ignored.\n";
  file->writeData("data", xmlText);
  file->writeData("type", "text/xml"); // mimetype
  file->writeData("description", "XML contents of the instrument IDF file.");
  file->closeGroup();

  // Now the parameter map, as a NXnote via its saveNexus method
  if (isParametrized()) {
    // Map with data extracted from DetectorInfo -> legacy compatible files.
    const auto &params = makeLegacyParameterMap();
    params->saveNexus(file, "instrument_parameter_map");
  }

  // Add physical detector and monitor data
  auto detmonIDs = getDetectorIDs(false);
  if (!detmonIDs.empty()) {
    auto detectorIDs = getDetectorIDs(true);
    // Add detectors group
    file->makeGroup("physical_detectors", "NXdetector", true);
    file->writeData("number_of_detectors", uint64_t(detectorIDs.size()));
    saveDetectorSetInfoToNexus(file, detectorIDs);
    file->closeGroup(); // detectors

    // Create Monitor IDs vector
    std::vector<detid_t> monitorIDs;
    for (size_t i = 0; i < detmonIDs.size(); i++) {
      if (isMonitorViaIndex(i))
        monitorIDs.emplace_back(detmonIDs[i]);
    }

    // Add Monitors group
    file->makeGroup("physical_monitors", "NXmonitor", true);
    file->writeData("number_of_monitors", uint64_t(monitorIDs.size()));
    saveDetectorSetInfoToNexus(file, monitorIDs);
    file->closeGroup(); // monitors
  }

  file->closeGroup();
}

/* A private helper function so save information about a set of detectors to
 * Nexus
 *  @param file :: open Nexus file ready to recieve the info about the set of
 * detectors
 *                 a group must be open that has only one call of this function.
 *  @param detIDs :: the dectector IDs of the detectors belonging to the set
 */
void Instrument::saveDetectorSetInfoToNexus(::NeXus::File *file, const std::vector<detid_t> &detIDs) const {

  size_t nDets = detIDs.size();
  if (nDets == 0)
    return;
  auto detectors = getDetectors(detIDs);

  Geometry::IComponent_const_sptr sample = getSample();
  Kernel::V3D sample_pos;
  if (sample)
    sample_pos = sample->getPos();

  std::vector<double> a_angles(nDets);
  std::vector<double> p_angles(nDets);
  std::vector<double> distances(nDets);

  for (size_t i = 0; i < nDets; i++) {
    if (sample) {
      Kernel::V3D pos = detectors[i]->getPos() - sample_pos;
      pos.getSpherical(distances[i], p_angles[i], a_angles[i]);
    } else {
      a_angles[i] = detectors[i]->getPhi() * 180.0 / M_PI;
    }
  }
  file->writeData("detector_number", detIDs);
  file->writeData("azimuthal_angle", a_angles);
  file->openData("azimuthal_angle");
  file->putAttr("units", "degree");
  file->closeData();
  if (sample) {
    file->writeData("polar_angle", p_angles);
    file->openData("polar_angle");
    file->putAttr("units", "degree");
    file->closeData();
    file->writeData("distance", distances);
    file->openData("distance");
    file->putAttr("units", "metre");
    file->closeData();
  }
}

//--------------------------------------------------------------------------------------------
/** Load the object from an open NeXus file.
 * @param file :: open NeXus file
 * @param group :: name of the group to open
 */
void Instrument::loadNexus(::NeXus::File *file, const std::string &group) {
  file->openGroup(group, "NXinstrument");
  file->closeGroup();
}

/**
Setter for the reference frame.
@param frame : reference frame object to use.
*/
void Instrument::setReferenceFrame(std::shared_ptr<ReferenceFrame> frame) { m_referenceFrame = std::move(frame); }

/**
Getter for the reference frame.
@return : reference frame.
*/
std::shared_ptr<const ReferenceFrame> Instrument::getReferenceFrame() const {
  if (m_map) {
    return m_instr->getReferenceFrame();
  } else {
    return m_referenceFrame;
  }
}

/**
 * Set the default type of the instrument view.
 * @param type :: A string with one of the values:
 *    3D, CYLINDRICAL_X, CYLINDRICAL_Y, CYLINDRICAL_Z, SPHERICAL_X, SPHERICAL_Y,
 * SPHERICAL_Z
 *    Caseless. If a wrong value is given logs a warning and sets the view to
 * "3D"
 */
void Instrument::setDefaultView(const std::string &type) {
  std::string typeUC(type);
  std::transform(typeUC.begin(), typeUC.end(), typeUC.begin(), toupper);
  if (typeUC == "3D" || typeUC == "CYLINDRICAL_X" || typeUC == "CYLINDRICAL_Y" || typeUC == "CYLINDRICAL_Z" ||
      typeUC == "SPHERICAL_X" || typeUC == "SPHERICAL_Y" || typeUC == "SPHERICAL_Z") {
    m_defaultView = typeUC;
  } else {
    m_defaultView = "3D";
    g_log.warning() << type << " is not allowed as an instrument view type. Default to \"3D\"" << '\n';
  }
}

/// Set the date from which the instrument definition begins to be valid.
/// @param val :: date and time
/// @throw InstrumentDefinitionError Thrown if date is earlier than 1900-01-31
/// 23:59:01
void Instrument::setValidFromDate(const Types::Core::DateAndTime &val) {
  Types::Core::DateAndTime earliestAllowedDate("1900-01-31 23:59:01");
  if (val < earliestAllowedDate) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "The valid-from <instrument> tag date must be from 1900-01-31 23:59:01 "
        "or later",
        m_filename);
  }
  m_ValidFrom = val;
}

Instrument::ContainsState Instrument::containsRectDetectors() const {
  std::queue<IComponent_const_sptr> compQueue; // Search queue
  addInstrumentChildrenToQueue(compQueue);

  bool foundRect = false;
  bool foundNonRect = false;

  IComponent_const_sptr comp;

  while (!compQueue.empty() && !(foundRect && foundNonRect)) {
    comp = compQueue.front();
    compQueue.pop();

    if (!validateComponentProperties(comp))
      continue;

    if (dynamic_cast<const RectangularDetector *>(comp.get())) {
      foundRect = true;
    } // If component isn't a ComponentAssembly, we know it is a non-rectangular detector. Otherwise check its children
    else if (!addAssemblyChildrenToQueue(compQueue, comp)) {
      foundNonRect = true;
    }
  }

  // Found both
  if (foundRect && foundNonRect)
    return Instrument::ContainsState::Partial;
  // Found only rectangular
  else if (foundRect)
    return Instrument::ContainsState::Full;
  // Found only non-rectangular
  else
    return Instrument::ContainsState::None;
}

std::vector<RectangularDetector_const_sptr> Instrument::findRectDetectors() const {
  std::queue<IComponent_const_sptr> compQueue; // Search queue
  addInstrumentChildrenToQueue(compQueue);

  std::vector<RectangularDetector_const_sptr> detectors;

  IComponent_const_sptr comp;

  while (!compQueue.empty()) {
    comp = compQueue.front();
    compQueue.pop();

    if (!validateComponentProperties(comp))
      continue;

    if (auto const detector = std::dynamic_pointer_cast<const RectangularDetector>(comp)) {
      detectors.push_back(detector);
    } else {
      // If component is a ComponentAssembly, we add its children to the queue to check if they're Rectangular Detectors
      addAssemblyChildrenToQueue(compQueue, comp);
    }
  }
  return detectors;
}

bool Instrument::validateComponentProperties(IComponent_const_sptr component) const {
  // Skip source, if has one
  if (m_sourceCache && m_sourceCache->getComponentID() == component->getComponentID())
    return false;

  // Skip sample, if has one
  if (m_sampleCache && m_sampleCache->getComponentID() == component->getComponentID())
    return false;

  // Skip monitors
  IDetector_const_sptr detector = std::dynamic_pointer_cast<const IDetector>(component);
  if (detector && isMonitor(detector->getID()))
    return false;

  // skip choppers, slits and supermirrors - HACK!
  const auto &name = component->getName();
  if (name == "chopper-position" || name.substr(0, 4) == "slit" || name == "supermirror") {
    return false;
  }

  return true;
}

void Instrument::addInstrumentChildrenToQueue(std::queue<IComponent_const_sptr> &queue) const {
  // Add all the direct children of the instrument
  for (int i = 0; i < nelements(); i++)
    queue.push(getChild(i));
}

/// If component is a ComponentAssembly, we add its children to the queue to check if they're Rectangular Detectors and
/// returns true. Otherwise, it returns false.
bool Instrument::addAssemblyChildrenToQueue(std::queue<IComponent_const_sptr> &queue,
                                            IComponent_const_sptr component) const {
  if (auto const assembly = std::dynamic_pointer_cast<const ICompAssembly>(component)) {
    for (int i = 0; i < assembly->nelements(); i++)
      queue.push(assembly->getChild(i));
    return true;
  }
  return false;
}

/// Temporary helper for refactoring. Argument is index, *not* ID!
bool Instrument::isMonitorViaIndex(const size_t index) const {
  if (m_map)
    return std::get<2>(m_instr->m_detectorCache[index]);
  else
    return std::get<2>(m_detectorCache[index]);
}

bool Instrument::isEmptyInstrument() const { return this->nelements() == 0; }

int Instrument::add(IComponent *component) {
  // invalidate cache
  return CompAssembly::add(component);
}

/// Returns the index for a detector ID. Used for accessing DetectorInfo.
size_t Instrument::detectorIndex(const detid_t detID) const {
  const auto &baseInstr = m_map ? *m_instr : *this;
  const auto it = find(baseInstr.m_detectorCache, detID);
  return std::distance(baseInstr.m_detectorCache.cbegin(), it);
}

/// Returns a legacy ParameterMap, containing information that is now stored in
/// DetectorInfo (masking, positions, rotations, scale factors).
std::shared_ptr<ParameterMap> Instrument::makeLegacyParameterMap() const {
  auto pmap = std::make_shared<ParameterMap>(*getParameterMap());
  // Instrument is only needed for DetectorInfo access so it is not needed. This
  // also clears DetectorInfo and ComponentInfo (information will be stored
  // directly in pmap so we do not need them).
  pmap->setInstrument(nullptr);

  const auto &baseInstr = m_map ? *m_instr : *this;

  if (!getParameterMap()->hasComponentInfo(&baseInstr))
    return pmap;

  // Tolerance 1e-9 m with rotation center at a distance of L = 1000 m as in
  // Beamline::DetectorInfo::isEquivalent.
  constexpr double d_max = 1e-9;
  constexpr double L = 1000.0;
  constexpr double safety_factor = 2.0;
  const double imag_norm_max = sin(d_max / (2.0 * L * safety_factor));

  auto transformation = Eigen::Affine3d::Identity();
  int64_t oldParentIndex = -1;

  const auto &componentInfo = getParameterMap()->componentInfo();
  const auto &detectorInfo = getParameterMap()->detectorInfo();
  for (size_t i = 0; i < componentInfo.size(); ++i) {

    const int64_t parentIndex = componentInfo.parent(i);
    const bool makeTransform = parentIndex != oldParentIndex;
    bool isDetFixedInBank = false;

    if (makeTransform) {
      oldParentIndex = parentIndex;
      const auto parentPos = toVector3d(componentInfo.position(parentIndex));
      const auto invParentRot = toQuaterniond(componentInfo.rotation(parentIndex)).conjugate();

      transformation = invParentRot;
      transformation.translate(-parentPos);
    }

    if (componentInfo.isDetector(i)) {

      const std::shared_ptr<const IDetector> &baseDet = std::get<1>(baseInstr.m_detectorCache[i]);

      isDetFixedInBank = ComponentInfoBankHelpers::isDetectorFixedInBank(componentInfo, i);
      if (detectorInfo.isMasked(i)) {
        pmap->forceUnsafeSetMasked(baseDet.get(), true);
      }

      if (makeTransform) {
        // Special case: scaling for GridDetectorPixel.
        if (isDetFixedInBank) {

          size_t panelIndex = componentInfo.parent(parentIndex);
          const auto panelID = componentInfo.componentID(panelIndex);

          Eigen::Vector3d scale(1, 1, 1);
          if (auto scalex = pmap->get(panelID, "scalex"))
            scale[0] = 1.0 / scalex->value<double>();
          if (auto scaley = pmap->get(panelID, "scaley"))
            scale[1] = 1.0 / scaley->value<double>();
          transformation.prescale(scale);
        }
      }
    }

    const auto componentId = componentInfo.componentID(i);
    const IComponent *baseComponent = componentId->getBaseComponent();
    // Generic sca scale factors
    const auto newScaleFactor = Kernel::toVector3d(componentInfo.scaleFactor(i));
    if ((newScaleFactor - toVector3d(baseComponent->getScaleFactor())).norm() >= 1e-9) {
      pmap->addV3D(componentId, ParameterMap::scale(), componentInfo.scaleFactor(i));
    }

    // Undo parent transformation to obtain relative position/rotation.
    Eigen::Vector3d relPos = transformation * toVector3d(componentInfo.position(i));
    Eigen::Quaterniond relRot = toQuaterniond(componentInfo.relativeRotation(i));

    // Tolerance 1e-9 m as in Beamline::DetectorInfo::isEquivalent.
    if ((relPos - toVector3d(baseComponent->getRelativePos())).norm() >= 1e-9) {
      if (isDetFixedInBank) {
        throw std::runtime_error("Cannot create legacy ParameterMap: Position "
                                 "parameters for GridDetectorPixel are "
                                 "not supported");
      }
      pmap->addV3D(componentId, ParameterMap::pos(), Kernel::toV3D(relPos));
    }
    if ((relRot * toQuaterniond(baseComponent->getRelativeRot()).conjugate()).vec().norm() >= imag_norm_max) {
      pmap->addQuat(componentId, ParameterMap::rot(), Kernel::toQuat(relRot));
    }
  }

  return pmap;
}

/** Parse the instrument tree and create ComponentInfo and DetectorInfo.
 *
 * This can be called for the base instrument once it is completely created, in
 * particular when it is stored in the InstrumentDataService for reusing it
 * later and avoiding repeated tree walks if several workspaces with the same
 * instrument are loaded. */
void Instrument::parseTreeAndCacheBeamline() {
  if (isParametrized())
    throw std::logic_error("Instrument::parseTreeAndCacheBeamline must be "
                           "called with the base instrument, not a "
                           "parametrized instrument");
  std::tie(m_componentInfo, m_detectorInfo) = InstrumentVisitor::makeWrappers(*this);
}

/** Return ComponentInfo and DetectorInfo for instrument given by pmap.
 *
 * If suitable ComponentInfo and DetectorInfo are found in this or the
 * (optional) `source` pmap they are simply copied, otherwise the instrument
 * tree is parsed. */
std::pair<std::unique_ptr<ComponentInfo>, std::unique_ptr<DetectorInfo>>
Instrument::makeBeamline(ParameterMap &pmap, const ParameterMap *source) const {
  // If we have source and it has Beamline objects just copy them
  if (source && source->hasComponentInfo(this))
    return makeWrappers(pmap, source->componentInfo(), source->detectorInfo());
  // If pmap is empty and base instrument has Beamline objects just copy them
  if (pmap.empty() && m_componentInfo)
    return makeWrappers(pmap, *m_componentInfo, *m_detectorInfo);
  // pmap not empty and/or no cached Beamline objects found
  return InstrumentVisitor::makeWrappers(*this, &pmap);
}

/// Sets up links between m_detectorInfo, m_componentInfo, and m_instrument.
std::pair<std::unique_ptr<ComponentInfo>, std::unique_ptr<DetectorInfo>>
Instrument::makeWrappers(ParameterMap &pmap, const ComponentInfo &componentInfo,
                         const DetectorInfo &detectorInfo) const {
  auto compInfo = componentInfo.cloneWithoutDetectorInfo();
  auto detInfo = std::make_unique<DetectorInfo>(detectorInfo);
  compInfo->m_componentInfo->setDetectorInfo(detInfo->m_detectorInfo.get());
  const auto parInstrument = ParComponentFactory::createInstrument(
      std::shared_ptr<const Instrument>(this, NoDeleting()), std::shared_ptr<ParameterMap>(&pmap, NoDeleting()));
  detInfo->m_instrument = parInstrument;
  return {std::move(compInfo), std::move(detInfo)};
}

namespace Conversion {

/**
 * Calculate and return conversion factor from tof to d-spacing.
 * @param l1
 * @param l2
 * @param twoTheta scattering angle
 * @param offset
 * @return
 */
double tofToDSpacingFactor(const double l1, const double l2, const double twoTheta, const double offset) {
  return Kernel::Units::tofToDSpacingFactor(l1, l2, twoTheta, offset);
}

double calculateDIFCCorrection(const double l1, const double l2, const double twoTheta, const double offset,
                               const double binWidth) {
  return Kernel::Units::calculateDIFCCorrection(l1, l2, twoTheta, offset, binWidth);
}

} // namespace Conversion
} // namespace Mantid::Geometry
