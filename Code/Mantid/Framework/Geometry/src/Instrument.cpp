#include "MantidGeometry/Instrument.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"

#include <Poco/Path.h>
#include <algorithm>
#include <sstream>
#include <queue>

using namespace Mantid::Kernel;
using Mantid::Kernel::Exception::NotFoundError;
using Mantid::Kernel::Exception::InstrumentDefinitionError;

namespace Mantid {
namespace Geometry {

namespace {
Kernel::Logger g_log("Instrument");
}

/// Default constructor
Instrument::Instrument()
    : CompAssembly(), m_detectorCache(), m_sourceCache(0),
      m_chopperPoints(new std::vector<const ObjComponent *>), m_sampleCache(0),
      m_defaultView("3D"), m_defaultViewAxis("Z+"),
      m_referenceFrame(new ReferenceFrame) {}

/// Constructor with name
Instrument::Instrument(const std::string &name)
    : CompAssembly(name), m_detectorCache(), m_sourceCache(0),
      m_chopperPoints(new std::vector<const ObjComponent *>), m_sampleCache(0),
      m_defaultView("3D"), m_defaultViewAxis("Z+"),
      m_referenceFrame(new ReferenceFrame) {}

/** Constructor to create a parametrized instrument
 *  @param instr :: instrument for parameter inclusion
 *  @param map :: parameter map to include
 */
Instrument::Instrument(const boost::shared_ptr<const Instrument> instr,
                       boost::shared_ptr<ParameterMap> map)
    : CompAssembly(instr.get(), map.get()), m_sourceCache(instr->m_sourceCache),
      m_chopperPoints(instr->m_chopperPoints),
      m_sampleCache(instr->m_sampleCache), m_defaultView(instr->m_defaultView),
      m_defaultViewAxis(instr->m_defaultViewAxis), m_instr(instr),
      m_map_nonconst(map), m_ValidFrom(instr->m_ValidFrom),
      m_ValidTo(instr->m_ValidTo), m_referenceFrame(new ReferenceFrame) {}

/** Copy constructor
 *  This method was added to deal with having distinct neutronic and physical
 * positions
 *  in indirect instruments.
 */
Instrument::Instrument(const Instrument &instr)
    : CompAssembly(instr), m_sourceCache(NULL),
      m_chopperPoints(new std::vector<const ObjComponent *>),
      m_sampleCache(NULL), /* Should only be temporarily null */
      m_logfileCache(instr.m_logfileCache), m_logfileUnit(instr.m_logfileUnit),
      m_monitorCache(instr.m_monitorCache), m_defaultView(instr.m_defaultView),
      m_defaultViewAxis(instr.m_defaultViewAxis), m_instr(),
      m_map_nonconst(), /* Should not be parameterized */
      m_ValidFrom(instr.m_ValidFrom), m_ValidTo(instr.m_ValidTo),
      m_referenceFrame(instr.m_referenceFrame) {
  // Now we need to fill the detector, source and sample caches with pointers
  // into the new instrument
  std::vector<IComponent_const_sptr> children;
  getChildren(children, true);
  std::vector<IComponent_const_sptr>::const_iterator it;
  for (it = children.begin(); it != children.end(); ++it) {
    // First check if the current component is a detector and add to cache if it
    // is
    // N.B. The list of monitors should remain unchanged. As the cache holds
    // detector id
    // numbers rather than pointers, there's no need for special handling
    if (const IDetector *det = dynamic_cast<const Detector *>(it->get())) {
      markAsDetector(det);
      continue;
    }
    // Now check whether the current component is the source or sample.
    // As the majority of components will be detectors, we will rarely get to
    // here
    if (const ObjComponent *obj =
            dynamic_cast<const ObjComponent *>(it->get())) {
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
      for (size_t i = 0; i < instr.m_chopperPoints->size(); ++i) {
        if (objName == (*m_chopperPoints)[i]->getName()) {
          markAsChopperPoint(obj);
          break;
        }
      }
    }
  }
}

/**
 * Destructor
 */
Instrument::~Instrument() {
  if (!m_map) {
    m_chopperPoints->clear(); // CompAssembly will delete them
    delete m_chopperPoints;
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
      // instrument
      return Instrument_const_sptr(
          new Instrument(m_instr->getPhysicalInstrument(), m_map_nonconst));
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
void Instrument::setPhysicalInstrument(
    boost::shared_ptr<const Instrument> physInst) {
  if (!m_map)
    m_physicalInstrument = physInst;
  else
    throw std::runtime_error("Instrument::setPhysicalInstrument() called on a "
                             "parametrized instrument.");
}

//------------------------------------------------------------------------------------------
/**	Fills a copy of the detector cache
* @returns a map of the detectors hold by the instrument
*/
void Instrument::getDetectors(detid2det_map &out_map) const {
  if (m_map) {
    // Get the base instrument detectors
    out_map.clear();
    const detid2det_map &in_dets =
        static_cast<const Instrument *>(m_base)->m_detectorCache;
    // And turn them into parametrized versions
    for (detid2det_map::const_iterator it = in_dets.begin();
         it != in_dets.end(); ++it) {
      out_map.insert(std::pair<detid_t, IDetector_sptr>(
          it->first,
          ParComponentFactory::createDetector(it->second.get(), m_map)));
    }
  } else {
    // You can just return the detector cache directly.
    out_map = m_detectorCache;
  }
}

//------------------------------------------------------------------------------------------
/** Return a vector of detector IDs in this instrument */
std::vector<detid_t> Instrument::getDetectorIDs(bool skipMonitors) const {
  std::vector<detid_t> out;
  if (m_map) {
    const detid2det_map &in_dets =
        static_cast<const Instrument *>(m_base)->m_detectorCache;
    for (detid2det_map::const_iterator it = in_dets.begin();
         it != in_dets.end(); ++it)
      if (!skipMonitors || !it->second->isMonitor())
        out.push_back(it->first);
  } else {
    const detid2det_map &in_dets = m_detectorCache;
    for (detid2det_map::const_iterator it = in_dets.begin();
         it != in_dets.end(); ++it)
      if (!skipMonitors || !it->second->isMonitor())
        out.push_back(it->first);
  }
  return out;
}

/// @return The total number of detector IDs in the instrument */
std::size_t Instrument::getNumberDetectors(bool skipMonitors) const {
  std::size_t numDetIDs(0);

  if (m_map) {
    numDetIDs = static_cast<const Instrument *>(m_base)->m_detectorCache.size();
  } else {
    numDetIDs = m_detectorCache.size();
  }

  if (skipMonitors) // this slow, but gets the right answer
  {
    std::size_t monitors(0);
    if (m_map) {
      const detid2det_map &in_dets =
          static_cast<const Instrument *>(m_base)->m_detectorCache;
      for (detid2det_map::const_iterator it = in_dets.begin();
           it != in_dets.end(); ++it)
        if (it->second->isMonitor())
          monitors += 1;
    } else {
      const detid2det_map &in_dets = m_detectorCache;
      for (detid2det_map::const_iterator it = in_dets.begin();
           it != in_dets.end(); ++it)
        if (it->second->isMonitor())
          monitors += 1;
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
  const detid2det_map *in_dets;
  if (m_map)
    in_dets = &(static_cast<const Instrument *>(m_base)->m_detectorCache);
  else
    in_dets = &this->m_detectorCache;

  if (in_dets->empty())
    throw std::runtime_error(
        "No detectors on this instrument. Can't find min/max ids");
  // Maps are sorted by key. So it is easy to find
  min = in_dets->begin()->first;
  max = in_dets->rbegin()->first;
}

//------------------------------------------------------------------------------------------
/** Fill a vector with all the detectors contained (at any depth) in a named
 *component. For example,
 * you might have a bank10 with 4 tubes with 100 pixels each; this will return
 *the
 * 400 contained Detector objects.
 *
 * @param[out] dets :: vector filled with detector pointers
 * @param bankName :: name of the parent component assembly that contains
 *detectors.
 *        The name must be unique, otherwise the first matching component
 *(getComponentByName)
 *        is used.
 */
void Instrument::getDetectorsInBank(std::vector<IDetector_const_sptr> &dets,
                                    const std::string &bankName) const {
  boost::shared_ptr<const IComponent> comp = this->getComponentByName(bankName);
  boost::shared_ptr<const ICompAssembly> bank =
      boost::dynamic_pointer_cast<const ICompAssembly>(comp);
  if (bank) {
    // Get a vector of children (recursively)
    std::vector<boost::shared_ptr<const IComponent>> children;
    bank->getChildren(children, true);
    std::vector<boost::shared_ptr<const IComponent>>::iterator it;
    for (it = children.begin(); it != children.end(); ++it) {
      IDetector_const_sptr det =
          boost::dynamic_pointer_cast<const IDetector>(*it);
      if (det) {
        dets.push_back(det);
      }
    }
  }
}

//------------------------------------------------------------------------------------------
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

/**
 * Returns the chopper at the given index. Index 0 is defined as closest to the
 * source
 * If there are no choppers defined or the index is out of range then an
 * invalid_argument
 * exception is thrown.
 * @param index :: Defines which chopper to pick, 0 being closest to the source
 * [Default = 0]
 * @return A pointer to the chopper
 */
IObjComponent_const_sptr Instrument::getChopperPoint(const size_t index) const {
  if (index >= getNumberOfChopperPoints()) {
    std::ostringstream os;
    os << "Instrument::getChopperPoint - No chopper point at index '" << index
       << "' defined. Instrument has only " << getNumberOfChopperPoints()
       << " chopper points defined.";
    throw std::invalid_argument(os.str());
  }
  return IObjComponent_const_sptr(m_chopperPoints->at(index), NoDeleting());
}
/**
 * @return The number of chopper points defined by this instrument
 */
size_t Instrument::getNumberOfChopperPoints() const {
  return m_chopperPoints->size();
}

//------------------------------------------------------------------------------------------
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
Kernel::V3D Instrument::getBeamDirection() const {
  V3D retval = getSample()->getPos() - getSource()->getPos();
  retval.normalize();
  return retval;
}

//------------------------------------------------------------------------------------------
/**  Get a shared pointer to a component by its ID, const version
*   @param id :: ID
*   @return A pointer to the component.
*/
boost::shared_ptr<const IComponent>
Instrument::getComponentByID(ComponentID id) const {
  const IComponent *base = (const IComponent *)(id);
  if (m_map)
    return ParComponentFactory::create(
        boost::shared_ptr<const IComponent>(base, NoDeleting()), m_map);
  else
    return boost::shared_ptr<const IComponent>(base, NoDeleting());
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
std::vector<boost::shared_ptr<const IComponent>>
Instrument::getAllComponentsWithName(const std::string &cname) const {
  boost::shared_ptr<const IComponent> node =
      boost::shared_ptr<const IComponent>(this, NoDeleting());
  std::vector<boost::shared_ptr<const IComponent>> retVec;
  // Check the instrument name first
  if (this->getName() == cname) {
    retVec.push_back(node);
  }
  // Same algorithm as used in getComponentByName() but searching the full tree
  std::deque<boost::shared_ptr<const IComponent>> nodeQueue;
  // Need to be able to enter the while loop
  nodeQueue.push_back(node);
  while (!nodeQueue.empty()) {
    node = nodeQueue.front();
    nodeQueue.pop_front();
    int nchildren(0);
    boost::shared_ptr<const ICompAssembly> asmb =
        boost::dynamic_pointer_cast<const ICompAssembly>(node);
    if (asmb) {
      nchildren = asmb->nelements();
    }
    for (int i = 0; i < nchildren; ++i) {
      boost::shared_ptr<const IComponent> comp = (*asmb)[i];
      if (comp->getName() == cname) {
        retVec.push_back(comp);
      } else {
        nodeQueue.push_back(comp);
      }
    }
  } // while-end

  // If we have reached here then the search failed
  return retVec;
}

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
  if (m_map) {
    IDetector_const_sptr baseDet = m_instr->getDetector(detector_id);
    return ParComponentFactory::createDetector(baseDet.get(), m_map);
  } else {
    detid2det_map::const_iterator it = m_detectorCache.find(detector_id);
    if (it == m_detectorCache.end()) {
      std::stringstream readInt;
      readInt << detector_id;
      throw Kernel::Exception::NotFoundError(
          "Instrument: Detector with ID " + readInt.str() + " not found.", "");
    }

    return it->second;
  }
}

/**	Gets a pointer to the base (non-parametrized) detector from its ID
  * returns null if the detector has not been found
  *  @param   detector_id The requested detector ID
  *  @returns A const pointer to the detector object
  */
const IDetector *Instrument::getBaseDetector(const detid_t &detector_id) const {
  detid2det_map::const_iterator it = m_instr->m_detectorCache.find(detector_id);
  if (it == m_instr->m_detectorCache.end()) {
    return NULL;
  }
  return it->second.get();
}

bool Instrument::isMonitor(const detid_t &detector_id) const {
  // Find the (base) detector object in the map.
  detid2det_map::const_iterator it = m_instr->m_detectorCache.find(detector_id);
  if (it == m_instr->m_detectorCache.end())
    return false;
  // This is the detector
  const Detector *det = dynamic_cast<const Detector *>(it->second.get());
  if (det == NULL)
    return false;
  return det->isMonitor();
}

bool Instrument::isMonitor(const std::set<detid_t> &detector_ids) const {
  if (detector_ids.empty())
    return false;

  for (std::set<detid_t>::const_iterator it = detector_ids.begin();
       it != detector_ids.end(); ++it) {
    if (this->isMonitor(*it))
      return true;
  }
  return false;
}

//--------------------------------------------------------------------------
/** Is the detector with the given ID masked?
 *
 * @param detector_id :: detector ID to look for.
 * @return true if masked; false if not masked or if the detector was not found.
 */
bool Instrument::isDetectorMasked(const detid_t &detector_id) const {
  // With no parameter map, then no detector is EVER masked
  if (!isParametrized())
    return false;
  // Find the (base) detector object in the map.
  detid2det_map::const_iterator it = m_instr->m_detectorCache.find(detector_id);
  if (it == m_instr->m_detectorCache.end())
    return false;
  // This is the detector
  const Detector *det = dynamic_cast<const Detector *>(it->second.get());
  if (det == NULL)
    return false;
  // Access the parameter map directly.
  Parameter_sptr maskedParam = m_map->get(det, "masked");
  if (!maskedParam)
    return false;
  // If the parameter is defined, then yes, it is masked.
  return maskedParam->value<bool>();
}

//--------------------------------------------------------------------------
/** Is this group of detectors masked?
 *
 * This returns true (masked) if ALL of the detectors listed are masked.
 * It returns false (not masked) if there are no detectors in the list
 * It returns false (not masked) if any of the detectors are NOT masked.
 *
 * @param detector_ids :: set of detector IDs
 * @return true if masked.
 */
bool Instrument::isDetectorMasked(const std::set<detid_t> &detector_ids) const {
  if (detector_ids.empty())
    return false;

  for (std::set<detid_t>::const_iterator it = detector_ids.begin();
       it != detector_ids.end(); ++it) {
    if (!this->isDetectorMasked(*it))
      return false;
  }
  return true;
}

/**
 * Returns a pointer to the geometrical object for the given set of IDs
 * @param det_ids :: A list of detector ids
 *  @returns A pointer to the detector object
 *  @throw   NotFoundError If no detector is found for the detector ID given
 */
IDetector_const_sptr
Instrument::getDetectorG(const std::vector<detid_t> &det_ids) const {
  const size_t ndets(det_ids.size());
  if (ndets == 1) {
    return this->getDetector(det_ids[0]);
  } else {
    boost::shared_ptr<DetectorGroup> det_group(new DetectorGroup());
    bool warn(false);
    for (size_t i = 0; i < ndets; ++i) {
      det_group->addDetector(this->getDetector(det_ids[i]), warn);
    }
    return det_group;
  }
}

/**
 * Returns a list of Detectors for the given detectors ids
 *
 */
std::vector<IDetector_const_sptr>
Instrument::getDetectors(const std::vector<detid_t> &det_ids) const {
  std::vector<IDetector_const_sptr> dets_ptr;
  dets_ptr.reserve(det_ids.size());
  std::vector<detid_t>::const_iterator it;
  for (it = det_ids.begin(); it != det_ids.end(); ++it) {
    dets_ptr.push_back(this->getDetector(*it));
  }
  return dets_ptr;
}

/**
 * Returns a list of Detectors for the given detectors ids
 *
 */
std::vector<IDetector_const_sptr>
Instrument::getDetectors(const std::set<detid_t> &det_ids) const {
  std::vector<IDetector_const_sptr> dets_ptr;
  dets_ptr.reserve(det_ids.size());
  std::set<detid_t>::const_iterator it;
  for (it = det_ids.begin(); it != det_ids.end(); ++it) {
    dets_ptr.push_back(this->getDetector(*it));
  }
  return dets_ptr;
}

/**
 * Adds a Component which already exists in the instrument to the chopper cache.
 * If
 * the component is not a chopper or it has no name then an invalid_argument
 * expection is thrown
 * @param comp :: A pointer to the component
 */
void Instrument::markAsChopperPoint(const ObjComponent *comp) {
  const std::string name = comp->getName();
  if (name.empty()) {
    throw std::invalid_argument(
        "Instrument::markAsChopper - Chopper component must have a name");
  }
  IComponent_const_sptr source = getSource();
  if (!source) {
    throw Exception::InstrumentDefinitionError("Instrument::markAsChopper - No "
                                               "source is set, cannot defined "
                                               "chopper positions.");
  }
  auto insertPos = m_chopperPoints->begin();
  const double newChopperSourceDist = m_sourceCache->getDistance(*comp);
  for (; insertPos != m_chopperPoints->end(); ++insertPos) {
    const double sourceToChopDist = m_sourceCache->getDistance(**insertPos);
    if (newChopperSourceDist < sourceToChopDist) {
      break; // Found the insertion point
    }
  }
  m_chopperPoints->insert(insertPos, comp);
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

  if (!m_sampleCache) {
    if (comp->getName().empty()) {
      throw Exception::InstrumentDefinitionError(
          "The sample component is required to have a name.");
    }
    m_sampleCache = comp;
  } else {
    g_log.warning(
        "Have already added samplePos component to the _sampleCache.");
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
      throw Exception::InstrumentDefinitionError(
          "The source component is required to have a name.");
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
  std::map<int, IDetector_const_sptr>::iterator it = m_detectorCache.end();
  m_detectorCache.insert(it, std::map<int, IDetector_const_sptr>::value_type(
                                 det->getID(), det_sptr));
}

/** Mark a Component which has already been added to the Instrument class
* as a monitor and add it to the detector cache.
*
* @param det :: Component to be marked (stored for later retrieval) as a
*detector Component
*
* @throw Exception::ExistsError if cannot add detector to cache
*/
void Instrument::markAsMonitor(IDetector *det) {
  if (m_map)
    throw std::runtime_error("Instrument::markAsMonitor() called on a "
                             "parametrized Instrument object.");

  // attempt to add monitor to instrument detector cache
  markAsDetector(det);

  // mark detector as a monitor
  Detector *d = dynamic_cast<Detector *>(det);
  if (d) {
    d->markAsMonitor();
    m_monitorCache.push_back(det->getID());
  } else {
    throw std::invalid_argument(
        "The IDetector pointer does not point to a Detector object");
  }
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
  m_detectorCache.erase(id);
  // Also need to remove from monitor cache if appropriate
  if (det->isMonitor()) {
    std::vector<detid_t>::iterator it =
        std::find(m_monitorCache.begin(), m_monitorCache.end(), id);
    if (it != m_monitorCache.end())
      m_monitorCache.erase(it);
  }

  // Remove it from the parent assembly (and thus the instrument). Evilness
  // required here unfortunately.
  CompAssembly *parentAssembly = dynamic_cast<CompAssembly *>(
      const_cast<IComponent *>(det->getBareParent()));
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
    return static_cast<const Instrument *>(m_base)->m_monitorCache;
  else
    return m_monitorCache;
}

/**
 * Returns the number of monitors attached to this instrument
 * @returns The number of monitors within the instrument
 */
size_t Instrument::numMonitors() const {
  if (m_map) {
    return static_cast<const Instrument *>(m_base)->m_monitorCache.size();
  } else {
    return m_monitorCache.size();
  }
}

/**
 * Get the bounding box for this instrument. It is simply the sum of the
 * bounding boxes of its children excluding the source
 * @param assemblyBox :: [Out] The resulting bounding box is stored here.
 */
void Instrument::getBoundingBox(BoundingBox &assemblyBox) const {
  if (m_map) {
    // Check cache for assembly
    if (m_map->getCachedBoundingBox(this, assemblyBox)) {
      return;
    }
    // Loop over the children and define a box large enough for all of them
    ComponentID sourceID = getSource()->getComponentID();
    assemblyBox =
        BoundingBox(); // this makes the instrument BB always axis aligned
    int nchildren = nelements();
    for (int i = 0; i < nchildren; ++i) {
      IComponent_sptr comp = this->getChild(i);
      if (comp && comp->getComponentID() != sourceID) {
        BoundingBox compBox;
        comp->getBoundingBox(compBox);
        assemblyBox.grow(compBox);
      }
    }
    // Set the cache
    m_map->setCachedBoundingBox(this, assemblyBox);

  } else {

    if (!m_cachedBoundingBox) {
      m_cachedBoundingBox = new BoundingBox();
      ComponentID sourceID = getSource()->getComponentID();
      // Loop over the children and define a box large enough for all of them
      for (const_comp_it it = m_children.begin(); it != m_children.end();
           ++it) {
        BoundingBox compBox;
        IComponent *component = *it;
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

boost::shared_ptr<const std::vector<IObjComponent_const_sptr>>
Instrument::getPlottable() const {
  if (m_map) {
    // Get the 'base' plottable components
    boost::shared_ptr<const std::vector<IObjComponent_const_sptr>> objs =
        m_instr->getPlottable();

    // Get a reference to the underlying vector, casting away the constness so
    // that we
    // can modify it to get our result rather than creating another long vector
    std::vector<IObjComponent_const_sptr> &res =
        const_cast<std::vector<IObjComponent_const_sptr> &>(*objs);
    const std::vector<IObjComponent_const_sptr>::size_type total = res.size();
    for (std::vector<IObjComponent_const_sptr>::size_type i = 0; i < total;
         ++i) {
      res[i] = boost::dynamic_pointer_cast<const Detector>(
          ParComponentFactory::create(objs->at(i), m_map));
    }
    return objs;

  } else {
    // Base instrument
    boost::shared_ptr<std::vector<IObjComponent_const_sptr>> res(
        new std::vector<IObjComponent_const_sptr>);
    res->reserve(m_detectorCache.size() + 10);
    appendPlottable(*this, *res);
    return res;
  }
}

void
Instrument::appendPlottable(const CompAssembly &ca,
                            std::vector<IObjComponent_const_sptr> &lst) const {
  for (int i = 0; i < ca.nelements(); i++) {
    IComponent *c = ca[i].get();
    CompAssembly *a = dynamic_cast<CompAssembly *>(c);
    if (a)
      appendPlottable(*a, lst);
    else {
      Detector *d = dynamic_cast<Detector *>(c);
      ObjComponent *o = dynamic_cast<ObjComponent *>(c);
      if (d)
        lst.push_back(IObjComponent_const_sptr(d, NoDeleting()));
      else if (o)
        lst.push_back(IObjComponent_const_sptr(o, NoDeleting()));
      else
        g_log.error() << "Unknown comp type\n";
    }
  }
}

const double CONSTANT = (PhysicalConstants::h * 1e10) /
                        (2.0 * PhysicalConstants::NeutronMass * 1e6);

//-----------------------------------------------------------------------
/** Calculate the conversion factor (tof -> d-spacing) for a single pixel, i.e.,
 *1/DIFC for that pixel.
 *
 * @param l1 :: Primary flight path.
 * @param beamline: vector = samplePos-sourcePos = a vector pointing from the
 *source to the sample,
 *        the length of the distance between the two.
 * @param beamline_norm: (source to sample distance) * 2.0 (apparently)
 * @param samplePos: position of the sample
 * @param det: Geometry object representing the detector (position of the pixel)
 * @param offset: value (close to zero) that changes the factor := factor *
 *(1+offset).
 */
double Instrument::calcConversion(const double l1, const Kernel::V3D &beamline,
                                  const double beamline_norm,
                                  const Kernel::V3D &samplePos,
                                  const IDetector_const_sptr &det,
                                  const double offset) {
  if (offset <=
      -1.) // not physically possible, means result is negative d-spacing
  {
    std::stringstream msg;
    msg << "Encountered offset of " << offset
        << " which converts data to negative d-spacing\n";
    throw std::logic_error(msg.str());
  }

  // Get the sample-detector distance for this detector (in metres)

  // The scattering angle for this detector (in radians).
  Kernel::V3D detPos;
  detPos = det->getPos();

  // Now detPos will be set with respect to samplePos
  detPos -= samplePos;
  // 0.5*cos(2theta)
  double l2 = detPos.norm();
  double halfcosTwoTheta = detPos.scalar_prod(beamline) / (l2 * beamline_norm);
  // This is sin(theta)
  double sinTheta = sqrt(0.5 - halfcosTwoTheta);
  const double numerator = (1.0 + offset);
  sinTheta *= (l1 + l2);
  return (numerator * CONSTANT) / sinTheta;
}

//-----------------------------------------------------------------------
/** Calculate the conversion factor (tof -> d-spacing)
 * for a LIST of detectors assigned to a single spectrum.
 */
double Instrument::calcConversion(
    const double l1, const Kernel::V3D &beamline, const double beamline_norm,
    const Kernel::V3D &samplePos,
    const boost::shared_ptr<const Instrument> &instrument,
    const std::vector<detid_t> &detectors,
    const std::map<detid_t, double> &offsets) {
  double factor = 0.;
  double offset;
  for (std::vector<detid_t>::const_iterator iter = detectors.begin();
       iter != detectors.end(); ++iter) {
    std::map<detid_t, double>::const_iterator off_iter = offsets.find(*iter);
    if (off_iter != offsets.end()) {
      offset = offsets.find(*iter)->second;
    } else {
      offset = 0.;
    }
    factor += calcConversion(l1, beamline, beamline_norm, samplePos,
                             instrument->getDetector(*iter), offset);
  }
  return factor / static_cast<double>(detectors.size());
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
void Instrument::getInstrumentParameters(double &l1, Kernel::V3D &beamline,
                                         double &beamline_norm,
                                         Kernel::V3D &samplePos) const {
  // Get some positions
  const IComponent_const_sptr sourceObj = this->getSource();
  if (sourceObj == NULL) {
    throw Exception::InstrumentDefinitionError(
        "Failed to get source component from instrument");
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
    throw Exception::InstrumentDefinitionError(
        "Unable to calculate source-sample distance ", this->getName());
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
void Instrument::saveNexus(::NeXus::File *file,
                           const std::string &group) const {
  file->makeGroup(group, "NXinstrument", 1);
  file->putAttr("version", 1);

  file->writeData("name", getName());

  // XML contents of instrument, as a NX note
  file->makeGroup("instrument_xml", "NXnote", true);
  file->writeData("data", getXmlText());
  file->writeData("type", "text/xml"); // mimetype
  file->writeData("description", "XML contents of the instrument IDF file.");
  file->closeGroup();

  file->writeData("instrument_source", Poco::Path(getFilename()).getFileName());

  // Now the parameter map, as a NXnote via its saveNexus method
  if (isParametrized()) {
    const Geometry::ParameterMap &params = *getParameterMap();
    params.saveNexus(file, "instrument_parameter_map");
  }

  // Add physical detector and monitor data
  std::vector<detid_t> detectorIDs;
  std::vector<detid_t> detmonIDs;
  detectorIDs = getDetectorIDs(true);
  detmonIDs = getDetectorIDs(false);
  if (!detmonIDs.empty()) {
    // Add detectors group
    file->makeGroup("physical_detectors", "NXdetector", true);
    file->writeData("number_of_detectors", uint64_t(detectorIDs.size()));
    saveDetectorSetInfoToNexus(file, detectorIDs);
    file->closeGroup(); // detectors

    // Create Monitor IDs vector
    std::vector<IDetector_const_sptr> detmons;
    detmons = getDetectors(detmonIDs);
    std::vector<detid_t> monitorIDs;
    for (size_t i = 0; i < detmonIDs.size(); i++) {
      if (detmons[i]->isMonitor())
        monitorIDs.push_back(detmonIDs[i]);
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
void Instrument::saveDetectorSetInfoToNexus(::NeXus::File *file,
                                            std::vector<detid_t> detIDs) const {

  size_t nDets = detIDs.size();
  if (nDets == 0)
    return;
  std::vector<IDetector_const_sptr> detectors;
  detectors = getDetectors(detIDs);

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
void Instrument::setReferenceFrame(boost::shared_ptr<ReferenceFrame> frame) {
  m_referenceFrame = frame;
}

/**
Getter for the reference frame.
@return : reference frame.
*/
boost::shared_ptr<const ReferenceFrame> Instrument::getReferenceFrame() const {
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
  if (typeUC == "3D" || typeUC == "CYLINDRICAL_X" ||
      typeUC == "CYLINDRICAL_Y" || typeUC == "CYLINDRICAL_Z" ||
      typeUC == "SPHERICAL_X" || typeUC == "SPHERICAL_Y" ||
      typeUC == "SPHERICAL_Z") {
    m_defaultView = typeUC;
  } else {
    m_defaultView = "3D";
    g_log.warning()
        << type
        << " is not allowed as an instrument view type. Default to \"3D\""
        << std::endl;
  }
}

/// Set the date from which the instrument definition begins to be valid.
/// @param val :: date and time
/// @throw InstrumentDefinitionError Thrown if date is earlier than 1900-01-31
/// 23:59:01
void Instrument::setValidFromDate(const Kernel::DateAndTime &val) {
  Kernel::DateAndTime earliestAllowedDate("1900-01-31 23:59:01");
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

  // Add all the direct children of the intrument
  for (int i = 0; i < nelements(); i++)
    compQueue.push(getChild(i));

  bool foundRect = false;
  bool foundNonRect = false;

  IComponent_const_sptr comp;

  while (!compQueue.empty() && !(foundRect && foundNonRect)) {
    comp = compQueue.front();
    compQueue.pop();

    // Skip source, is has one
    if (m_sourceCache &&
        m_sourceCache->getComponentID() == comp->getComponentID())
      continue;

    // Skip sample, if has one
    if (m_sampleCache &&
        m_sampleCache->getComponentID() == comp->getComponentID())
      continue;

    // Skip monitors
    IDetector_const_sptr detector =
        boost::dynamic_pointer_cast<const IDetector>(comp);
    if (detector && detector->isMonitor())
      continue;

    if (dynamic_cast<const RectangularDetector *>(comp.get())) {
      if (!foundRect)
        foundRect = true;
    } else {
      ICompAssembly_const_sptr assembly =
          boost::dynamic_pointer_cast<const ICompAssembly>(comp);

      if (assembly) {
        for (int i = 0; i < assembly->nelements(); i++)
          compQueue.push(assembly->getChild(i));
      } else // Is a non-rectangular component
      {
        if (!foundNonRect)
          foundNonRect = true;
      }
    }

  } // while

  // Found both
  if (foundRect && foundNonRect)
    return Instrument::ContainsState::Partial;
  // Found only rectangular
  else if (foundRect)
    return Instrument::ContainsState::Full;
  // Found only non-rectangular
  else
    return Instrument::ContainsState::None;

} // containsRectDetectors

} // namespace Geometry
} // Namespace Mantid
