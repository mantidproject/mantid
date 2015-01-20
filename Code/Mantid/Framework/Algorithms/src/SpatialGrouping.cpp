//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SpatialGrouping.h"

#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/BoundingBox.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FileProperty.h"

#include <map>

#include <fstream>
#include <iostream>

#include <algorithm>
#include "MantidAPI/ISpectrum.h"

using Mantid::API::ISpectrum;

namespace {
/*
* Comparison operator for use in std::sort when dealing with a vector of
* std::pair<int,double> where int is DetectorID and double is distance from
* centre point.
* Needs to be outside of SpatialGrouping class because of the way STL handles
* passing functions as arguments.
* @param left :: element to compare
* @param right :: element to compare
* @return true if left should come before right in the order
*/
static bool
compareIDPair(const std::pair<int64_t, Mantid::Kernel::V3D> &left,
              const std::pair<int64_t, Mantid::Kernel::V3D> &right) {
  return (left.second.norm() < right.second.norm());
}
}

namespace Mantid {
namespace Algorithms {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SpatialGrouping)

/**
* init() method implemented from Algorithm base class
*/
void SpatialGrouping::init() {
  declareProperty(new Mantid::API::WorkspaceProperty<>(
                      "InputWorkspace", "", Mantid::Kernel::Direction::Input),
                  "Name of the input workspace, which is used only as a means "
                  "of retrieving the instrument geometry.");
  declareProperty(new Mantid::API::FileProperty(
                      "Filename", "", Mantid::API::FileProperty::Save, ".xml"),
                  "Name (and location) in which to save the file. Having a "
                  "suffix of ''.xml'' is recommended.");
  declareProperty("SearchDistance", 2.5, "The number of pixel widths in which "
                                         "to search for neighbours of the "
                                         "detector.");
  declareProperty("GridSize", 3, "The size of the grid that should be grouped. "
                                 "i.e, 3 (the default) will select a group of "
                                 "nine detectors centred in a 3 by 3 grid.");
}

/**
* exec() method implemented from Algorithm base class
*/
void SpatialGrouping::exec() {
  inputWorkspace = getProperty("InputWorkspace");
  double searchDist = getProperty("SearchDistance");
  m_pix = searchDist;
  int gridSize = getProperty("GridSize");
  size_t nNeighbours = (gridSize * gridSize) - 1;

  // Make a map key = spectrum number, value = detector at that spectrum
  m_detectors.clear();
  for (size_t i = 0; i < inputWorkspace->getNumberHistograms(); i++) {
    const ISpectrum *spec = inputWorkspace->getSpectrum(i);
    m_detectors[spec->getSpectrumNo()] = inputWorkspace->getDetector(i);
  }

  // TODO: There is a confusion in this algorithm between detector IDs and
  // spectrum numbers!

  Mantid::API::Progress prog(this, 0.0, 1.0, m_detectors.size());

  for (std::map<specid_t, Geometry::IDetector_const_sptr>::iterator detIt =
           m_detectors.begin();
       detIt != m_detectors.end(); ++detIt) {
    prog.report();

    // The detector
    Mantid::Geometry::IDetector_const_sptr det = detIt->second;
    // The spectrum number of the detector
    specid_t specNo = detIt->first;

    // We are not interested in Monitors and we don't want them to be included
    // in
    // any of the other lists
    if (det->isMonitor()) {
      m_included.insert(specNo);
      continue;
    }

    // Or detectors already flagged as included in a group
    std::set<specid_t>::iterator inclIt = m_included.find(specNo);
    if (inclIt != m_included.end()) {
      continue;
    }

    std::map<detid_t, Mantid::Kernel::V3D> nearest;

    const double empty = EMPTY_DBL();
    Mantid::Geometry::BoundingBox bbox(empty, empty, empty, empty, empty,
                                       empty);

    createBox(det, bbox);

    bool extend = true;
    while ((nNeighbours > nearest.size()) && extend) {
      extend = expandNet(nearest, specNo, nNeighbours, bbox);
    }

    if (nearest.size() != nNeighbours)
      continue;

    // if we've gotten to this point, we want to go and make the group list.
    std::vector<int> group;
    m_included.insert(specNo);
    // Add the central spectrum
    group.push_back(specNo);

    // Add all the nearest neighbors
    std::map<specid_t, Mantid::Kernel::V3D>::iterator nrsIt;
    for (nrsIt = nearest.begin(); nrsIt != nearest.end(); ++nrsIt) {
      m_included.insert(nrsIt->first);
      group.push_back(nrsIt->first);
    }
    m_groups.push_back(group);
  }

  if (m_groups.size() == 0) {
    g_log.warning() << "No groups generated." << std::endl;
    return;
  }

  // Create grouping XML file
  g_log.information() << "Creating XML Grouping File." << std::endl;
  std::vector<std::vector<detid_t>>::iterator grpIt;
  std::ofstream xml;
  std::string fname = getPropertyValue("Filename");

  // Check to see whether we need to append .xml to the name.
  size_t fnameXMLappend = fname.find(".xml");
  if (fnameXMLappend == std::string::npos) {
    fnameXMLappend = fname.find(".XML"); // check both 'xml' and 'XML'
    if (fnameXMLappend == std::string::npos)
      fname = fname + ".xml";
  }

  // set the property again so the user can retrieve the stored result.
  setPropertyValue("Filename", fname);

  xml.open(fname.c_str());

  xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
      << "<!-- XML Grouping File created by SpatialGrouping Algorithm -->\n"
      << "<detector-grouping>\n";

  int grpID = 1;
  for (grpIt = m_groups.begin(); grpIt != m_groups.end(); ++grpIt) {
    xml << "<group name=\"group" << grpID++ << "\"><detids val=\""
        << (*grpIt)[0];
    for (size_t i = 1; i < (*grpIt).size(); i++) {
      // The following lines replace: std::vector<detid_t> detIds =
      // smap.getDetectors((*grpIt)[i]);
      size_t workspaceIndex =
          inputWorkspace->getIndexFromSpectrumNumber((*grpIt)[i]);
      const std::set<detid_t> &detIds =
          inputWorkspace->getSpectrum(workspaceIndex)->getDetectorIDs();
      for (std::set<detid_t>::const_iterator it = detIds.begin();
           it != detIds.end(); ++it) {
        xml << "," << (*it);
      }
    }
    xml << "\"/></group>\n";
  }

  xml << "</detector-grouping>";

  xml.close();

  g_log.information() << "Finished creating XML Grouping File." << std::endl;
}

/**
* This method will, using the NearestNeighbours methods, expand our view on the
* nearby detectors from
* the standard eight closest that are recorded in the graph.
* @param nearest :: neighbours found in previous requests
* @param spec :: pointer to the central detector, for calculating distances
* @param noNeighbours :: number of neighbours that must be found (in total,
* including those already found)
* @param bbox :: BoundingBox object representing the search region
* @return true if neighbours were found matching the parameters, false otherwise
*/
bool
SpatialGrouping::expandNet(std::map<specid_t, Mantid::Kernel::V3D> &nearest,
                           specid_t spec, const size_t &noNeighbours,
                           const Mantid::Geometry::BoundingBox &bbox) {
  const size_t incoming = nearest.size();

  Mantid::Geometry::IDetector_const_sptr det = m_detectors[spec];

  std::map<specid_t, Mantid::Kernel::V3D> potentials;

  // Special case for first run for this detector
  if (incoming == 0) {
    potentials = inputWorkspace->getNeighbours(det.get());
  } else {
    for (std::map<specid_t, Mantid::Kernel::V3D>::iterator nrsIt =
             nearest.begin();
         nrsIt != nearest.end(); ++nrsIt) {
      std::map<specid_t, Mantid::Kernel::V3D> results;
      results = inputWorkspace->getNeighbours(m_detectors[nrsIt->first].get());
      for (std::map<specid_t, Mantid::Kernel::V3D>::iterator resIt =
               results.begin();
           resIt != results.end(); ++resIt) {
        potentials[resIt->first] = resIt->second;
      }
    }
  }

  for (std::map<specid_t, Mantid::Kernel::V3D>::iterator potIt =
           potentials.begin();
       potIt != potentials.end(); ++potIt) {
    // We do not want to include the detector in it's own list of nearest
    // neighbours
    if (potIt->first == spec) {
      continue;
    }

    // Or detectors that are already in the nearest list passed into this
    // function
    std::map<detid_t, Mantid::Kernel::V3D>::iterator nrsIt =
        nearest.find(potIt->first);
    if (nrsIt != nearest.end()) {
      continue;
    }

    // We should not include detectors already included in a group (or monitors
    // for that matter)
    std::set<specid_t>::iterator inclIt = m_included.find(potIt->first);
    if (inclIt != m_included.end()) {
      continue;
    }

    // If we get this far, we need to determine if the detector is of a suitable
    // distance
    Mantid::Kernel::V3D pos = m_detectors[potIt->first]->getPos();
    if (!bbox.isPointInside(pos)) {
      continue;
    }

    // Elsewise, it's all good.
    nearest[potIt->first] = potIt->second;
  }

  if (nearest.size() == incoming) {
    return false;
  }

  if (nearest.size() > noNeighbours) {
    sortByDistance(nearest, noNeighbours);
  }

  return true;
}

/**
* This method will trim the result set down to the specified number required by
* sorting
* the results and removing those that are the greatest distance away.
* @param nearest :: map of values that need to be sorted, will be modified by
* the method
* @param noNeighbours :: number of elements that should be kept
*/
void
SpatialGrouping::sortByDistance(std::map<detid_t, Mantid::Kernel::V3D> &nearest,
                                const size_t &noNeighbours) {
  std::vector<std::pair<detid_t, Mantid::Kernel::V3D>> order(nearest.begin(),
                                                             nearest.end());

  std::sort(order.begin(), order.end(), compareIDPair);

  size_t current = order.size();
  size_t lose = current - noNeighbours;
  if (lose < 1)
    return;

  for (size_t i = 1; i <= lose; i++) {
    nearest.erase(order[current - i].first);
  }
}
/**
* Creates a bounding box representing the area in which to search for
* neighbours, and a scaling vector representing the dimensions
* of the detector
* @param det :: input detector
* @param bndbox :: reference to BoundingBox object (changed by this function)
*/
void
SpatialGrouping::createBox(boost::shared_ptr<const Geometry::IDetector> det,
                           Geometry::BoundingBox &bndbox) {

  // We may have DetectorGroups here
  // Unfortunately, IDetector doesn't contain the
  // boost::shared_ptr<Mantid::Geometry::Detector> detector =
  // boost::dynamic_pointer_cast<Mantid::Geometry::Detector>(det);

  Mantid::Geometry::BoundingBox bbox;
  det->getBoundingBox(bbox);

  double xmax = bbox.xMax();
  double ymax = bbox.yMax();
  double zmax = bbox.zMax();
  double xmin = bbox.xMin();
  double ymin = bbox.yMin();
  double zmin = bbox.zMin();

  double factor = 2.0 * m_pix;

  growBox(xmin, xmax, factor);
  growBox(ymin, ymax, factor);
  growBox(zmin, zmax, factor);

  bndbox = Mantid::Geometry::BoundingBox(xmax, ymax, zmax, xmin, ymin, zmin);
}

/**
* Enlarges values given by a certain factor. Used to "grow" the BoundingBox
* object.
* @param min :: min value (changed by this function)
* @param max :: max value (changed by this function)
* @param factor :: factor by which to grow the values
*/
void SpatialGrouping::growBox(double &min, double &max, const double &factor) {
  double rng = max - min;
  double mid = (max + min) / 2.0;
  double halfwid = rng / 2.0;
  min = mid - (factor * halfwid);
  max = mid + (factor * halfwid);
}

} // namespace Algorithms
} // namespace Mantid
