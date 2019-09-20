// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

/*
 * NexusGeometrySave::saveInstrument :
 * Save methods to save geometry and metadata from memory
 * to disk in Nexus file format for Instrument 2.0.
 *
 * @author Takudzwa Makoni, RAL (UKRI), ISIS
 * @date 07/08/2019
 */

#include "MantidNexusGeometry/NexusGeometrySave.h"
#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/ComponentInfoBankHelpers.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/MeshObject2D.h"
#include "MantidGeometry/Objects/MeshObjectCommon.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidNexusGeometry/H5ForwardCompatibility.h"
#include "MantidNexusGeometry/NexusGeometryDefinitions.h"
#include "MantidNexusGeometry/NexusGeometrySave.h"
#include "MantidNexusGeometry/NexusGeometryUtilities.h"

#include <H5Cpp.h>
#include <algorithm>
#include <boost/filesystem/operations.hpp>
#include <cmath>
#include <list>
#include <memory>
#include <regex>
#include <string>
#include <tuple>

namespace Mantid {
namespace NexusGeometry {
namespace NexusGeometrySave {
using namespace Geometry::ComponentInfoBankHelpers;
/*
 * Helper container for spectrum mapping information info
 */
struct SpectraMappings {
  std::vector<int32_t> detector_index;
  std::vector<int32_t> detector_count;
  std::vector<int32_t> detector_list;
  std::vector<int32_t> spectra_ids;
  size_t number_spec = 0;
  size_t number_dets = 0;
};

/** Function tryCreatGroup. will try to create a new child group with the given
 * name inside the parent group. if a child group with that name already exists
 * in the parent group, throws std::invalid_argument. H5 will not allow us to
 * save duplicate groups with the same name, so this provides a utility for an
 * eager check.
 *
 * @param parentGroup : H5 parent group.
 * @param childGroupName : intended name of the child goup.
 * @return : new H5 Group object with name <childGroupName> if did not throw.
 */
H5::Group tryCreateGroup(const H5::Group &parentGroup,
                         const std::string &childGroupName) {
  H5std_string parentGroupName = H5_OBJ_NAME(parentGroup);
  for (hsize_t i = 0; i < parentGroup.getNumObjs(); ++i) {
    if (parentGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
      H5std_string child = parentGroup.getObjnameByIdx(i);
      if (childGroupName == child) {
        throw std::invalid_argument(
            "Cannot create group with name " + childGroupName +
            " inside parent group " + parentGroupName +
            " because a group with this name already exists.");
      }
    }
  }
  return parentGroup.createGroup(childGroupName);
}

/*
produces a vector containing the index of MeshObject 'faces', which is the first
index of the vertex in a winding order that makes a triangle, and the ID (not
index!) of the detector whose position is within that triangle. Returns a vector
of data organied in ascending order of the 'faces' index with their detector ID
as consecutive:

EXAMPLE
key: f - face index, d - detector ID.

return format: {f0, d0, f1, d1, f2, d2, f3, d3... }

*/

std::vector<size_t> findDetectorFaces(const Geometry::ComponentInfo &compInfo,
                                      size_t idx,
                                      const std::vector<int> &detIds) {

  if (!compInfo.hasValidShape(idx)) {
    throw std::invalid_argument("no shape in nxDetector.");
  }

  std::vector<size_t> pairs;
  auto &shapeObj = compInfo.shape(idx);
  const auto childrenDetectors = compInfo.detectorsInSubtree(idx);
  if (auto meshObj = dynamic_cast<const Geometry::MeshObject *>(&shapeObj)) {
    auto triangles = meshObj->getV3Ds(); // V3D vertices of NxDetector shape
    auto nFaces = meshObj->numberOfTriangles();
    auto windingOrder = meshObj->getTriangles();

    for (size_t i = 0; i < nFaces; i += 3) {
      for (const size_t j : childrenDetectors) {

        // position of pixel
        const auto position =
            Kernel::toV3D(offsetFromAncestor(compInfo, idx, j));

        if (Geometry::MeshObjectCommon::isOnTriangle(
                position, triangles[windingOrder[i]],
                triangles[windingOrder[i + 1]],
                triangles[windingOrder[i + 2]])) {
          pairs.push_back(i);
          pairs.push_back(detIds[j]);
        }
      }
    }
  } else if (auto meshObj =
                 dynamic_cast<const Geometry::MeshObject2D *>(&shapeObj)) {
    auto points = meshObj->getVertices();
    auto nFaces = meshObj->numberOfTriangles();
    auto windingOrder = meshObj->getTriangles();

    for (size_t i = 0; i < nFaces; i += 3) {
      for (const size_t j : childrenDetectors) {

        // position of pixel
        const auto position =
            Kernel::toV3D(offsetFromAncestor(compInfo, idx, j));
        std::vector<Kernel::V3D> v3ds = {
            Kernel::V3D{points[i], points[i + 1], points[i + 2]},
            Kernel::V3D{points[i + 3], points[i + 4], points[i + 5]},
            Kernel::V3D{points[i + 6], points[i + 7], points[i + 9]}

        };

        if (Geometry::MeshObjectCommon::isOnTriangle(
                position, v3ds[windingOrder[i]], v3ds[windingOrder[i + 1]],
                v3ds[windingOrder[i + 2]])) {
          pairs.push_back(i);
          pairs.push_back(detIds[j]);
        }
      }
    }
  } else {
    throw std::invalid_argument("no mesh shape in nxdetector.");
  }
  return pairs;
}

Eigen::Vector3d generateOrthogonal(const Eigen::Vector3d &vector) {
  Eigen::Vector3d i, j, k;
  i = {1.0, 0.0, 0.0};
  j = {0.0, 1.0, 0.0};
  k = {0.0, 0.0, 1.0};
  if (vector.normalized().isApprox(i, PRECISION))
    return j;
  if (vector.normalized().isApprox(j, PRECISION))
    return k;
  if (vector.normalized().isApprox(k, PRECISION))
    return i;
  else
    return {1, -(vector[0] / vector[1]), 0};
}
/*
 * Function toStdVector (Overloaded). Store data in Mantid::Kernel::V3D vector
 * into std::vector<double> vector. Used by saveInstrument to write array-type
 * datasets to file.
 *
 * @param data : Mantid::Kernel::V3D vector containing data values
 * @return std::vector<double> vector containing data values in
 * Mantid::Kernel::V3D format.
 */
std::vector<double> toStdVector(const V3D &data) {
  std::vector<double> stdVector;
  stdVector.reserve(3);
  stdVector.push_back(data.X());
  stdVector.push_back(data.Y());
  stdVector.push_back(data.Z());
  return stdVector;
}

/*
 * Function toStdVector (Overloaded). Store data in Eigen::Vector3d vector
 * into std::vector<double> vector. Used by saveInstrument to write array-type
 * datasets to file.
 *
 * @param data : Eigen::Vector3d vector containing data values
 * @return std::vector<double> vector containing data values in
 * Eigen::Vector3d format
 */
std::vector<double> toStdVector(const Eigen::Vector3d &data) {
  return toStdVector(Kernel::toV3D(data));
}

/*
 * Function: isApproxZero. returns true if all values in an variable-sized
 * std-vector container evaluate to zero with a given level of precision. Used
 * by SaveInstrument methods to determine whether or not to write a dataset to
 * file.
 *
 * @param data : std::vector<double> data
 * @param precision : double precision specifier
 * @return true if all elements are approx zero, else false.
 */

bool isApproxZero(const std::vector<double> &data, const double &precision) {
  return std::all_of(data.begin(), data.end(),
                     [&precision](const double &element) {
                       return std::abs(element) < precision;
                     });
}

inline bool isApproxZero(const double data, const double precision) {
  return std::abs(data) < precision;
}

// overload. return true if vector is approx to zero
bool isApproxZero(const Eigen::Vector3d &data, const double &precision) {
  return data.isApprox(Eigen::Vector3d(0, 0, 0), precision);
}

// overload. returns true is angle is approx to zero
bool isApproxZero(const Eigen::Quaterniond &data, const double &precision) {
  return data.isApprox(Eigen::Quaterniond(1, 0, 0, 0), precision);
}

/*
 * Function: strTypeOfSize
 * Produces the HDF StrType of size equal to that of the
 * input string.
 *
 * @param str : std::string
 * @return string datatype of size = length of input string
 */
H5::StrType strTypeOfSize(const std::string &str) {
  H5::StrType stringType(H5::PredType::C_S1, str.size());
  return stringType;
}

/*
 * Function: writeStrDataset
 * writes a StrType HDF dataset and dataset value to a HDF group.
 *
 * @param grp : HDF group object.
 * @param attrname : attribute name.
 * @param attrVal : string attribute value to be stored in attribute.
 */
void writeStrDataset(H5::Group &grp, const std::string &dSetName,
                     const std::string &dSetVal,
                     const H5::DataSpace &dataSpace = SCALAR) {
  // TODO. may need to review if we shoud in fact replace.
  // added safety check for older HDF libraries
  if (dSetVal.empty())
    return;
  if (!utilities::findDataset(grp, dSetName)) {
    H5::StrType dataType = strTypeOfSize(dSetVal);
    H5::DataSet dSet = grp.createDataSet(dSetName, dataType, dataSpace);
    dSet.write(dSetVal, dataType);
  }
}

/*
TODO: TESTS to check empty datasets and attributes are not written.
*/

/** Function: writeStrAttribute
 * writes a StrType HDF attribute and attribute value to a HDF group.
 *
 * @param grp : HDF group object.
 * @param attrname : attribute name.
 * @param attrVal : string attribute value to be stored in attribute.
 */
void writeStrAttribute(H5::Group &grp, const std::string &attrName,
                       const std::string &attrVal,
                       const H5::DataSpace &dataSpace = SCALAR) {
  // added safety check for older HDF libraries
  if (attrVal.empty())
    throw std::invalid_argument("attribute value in " + attrName +
                                " cannot be empty.");
  if (!grp.attrExists(attrName)) {
    H5::StrType dataType = strTypeOfSize(attrVal);
    H5::Attribute attribute =
        grp.createAttribute(attrName, dataType, dataSpace);
    attribute.write(dataType, attrVal);
  }
}

/*
 * Function: writeStrAttribute
 * Overload function which writes a StrType HDF attribute and attribute value
 * to a HDF dataset.
 *
 * @param dSet : HDF dataset object.
 * @param attrname : attribute name.
 * @param attrVal : string attribute value to be stored in attribute.
 */
void writeStrAttribute(H5::DataSet &dSet, const std::string &attrName,
                       const std::string &attrVal,
                       const H5::DataSpace &dataSpace = SCALAR) {
  if (!dSet.attrExists(attrName)) {
    H5::StrType dataType = strTypeOfSize(attrVal);
    auto attribute = dSet.createAttribute(attrName, dataType, dataSpace);
    attribute.write(dataType, attrVal);
  }
}

SpectraMappings makeMappings(const Geometry::ComponentInfo &compInfo,
                             const detid2index_map &detToIndexMap,
                             const Indexing::IndexInfo &indexInfo,
                             const API::SpectrumInfo &specInfo,
                             const std::vector<Mantid::detid_t> &detIds,
                             size_t index) {
  auto childrenDetectors = compInfo.detectorsInSubtree(index);
  size_t nChildDetectors =
      childrenDetectors.size(); // Number of detectors actually considered in
                                // spectra-detector map for this NXdetector
  // local to this nxdetector
  std::map<size_t, int> detector_count_map;
  // We start knowing only the detector index, we have to establish spectra
  // from that.
  for (const auto det_index : childrenDetectors) {
    auto detector_id = detIds[det_index];

    // A detector might not belong to any spectrum at all.
    if (detToIndexMap.find(detector_id) != detToIndexMap.end()) {
      auto spectrum_index = detToIndexMap.at(detector_id);
      detector_count_map[spectrum_index]++; // Attribute detector to a give
                                            // spectrum_index
    } else {
      --nChildDetectors; // Detector is not part of any spectra-detector
                         // mapping. So we have one less detector to consider
                         // recording
    }
  }

  // Sized to spectra in bank
  SpectraMappings mappings;
  mappings.detector_list.resize(nChildDetectors);
  mappings.detector_count.resize(detector_count_map.size(), 0);
  mappings.detector_index.resize(detector_count_map.size() + 1, 0);
  mappings.spectra_ids.resize(detector_count_map.size(), 0);
  mappings.number_dets = nChildDetectors;
  mappings.number_spec = detector_count_map.size();
  size_t specCounter = 0;
  size_t detCounter = 0;
  for (auto &pair : detector_count_map) {
    // using sort order of map to ensure we are ordered by lowest to highest
    // spectrum index
    mappings.detector_count[specCounter] = (pair.second); // Counts
    mappings.detector_index[specCounter + 1] =
        mappings.detector_index[specCounter] + (pair.second);
    mappings.spectra_ids[specCounter] =
        int32_t(indexInfo.spectrumNumber(pair.first));

    // We will list everything by spectrum index, so we need to add the
    // detector ids in the same order.
    const auto &specDefintion = specInfo.spectrumDefinition(pair.first);
    for (const auto &def : specDefintion) {
      mappings.detector_list[detCounter] = detIds[def.first];
      ++detCounter;
    }
    ++specCounter;
  }
  mappings.detector_index.resize(
      detector_count_map.size()); // cut-off last item

  return mappings;
}

void validateInputs(AbstractLogger &logger, const std::string &fullPath,
                    const Geometry::ComponentInfo &compInfo) {
  boost::filesystem::path tmp(fullPath);
  if (!boost::filesystem::is_directory(tmp.root_directory())) {
    throw std::invalid_argument(
        "The path provided for saving the file is invalid: " + fullPath + "\n");
  }

  // check the file extension matches any of the valid extensions defined in
  // nexus_geometry_extensions
  const auto ext = boost::filesystem::path(tmp).extension();
  bool isValidExt = std::any_of(
      nexus_geometry_extensions.begin(), nexus_geometry_extensions.end(),
      [&ext](const std::string &str) { return ext.generic_string() == str; });

  // throw if the file extension is invalid
  if (!isValidExt) {
    // string of valid extensions to output in exception
    std::string extensions;
    std::for_each(
        nexus_geometry_extensions.begin(), nexus_geometry_extensions.end(),
        [&extensions](const std::string &str) { extensions += " " + str; });
    std::string message = "invalid extension for file: '" +
                          ext.generic_string() +
                          "'. Expected any of: " + extensions;
    logger.error(message);
    throw std::invalid_argument(message);
  }

  if (!compInfo.hasDetectorInfo()) {
    logger.error("No detector info was found in the Instrument. Instrument "
                 "not saved.");
    throw std::invalid_argument("No detector info was found in the Instrument");
  }
  if (!compInfo.hasSample()) {
    logger.error(
        "No sample was found in the Instrument. Instrument not saved.");
    throw std::invalid_argument("No sample was found in the Instrument");
  }

  if (Mantid::Kernel::V3D{0, 0, 0} != compInfo.samplePosition()) {
    logger.error("The sample positon is required to be at the origin. "
                 "Instrument not saved.");
    throw std::invalid_argument(
        "The sample positon is required to be at the origin");
  }

  if (!compInfo.hasSource()) {
    logger.error("No source was found in the Instrument. "
                 "Instrument not saved.");
    throw std::invalid_argument("No source was found in the Instrument");
  }
}

/*
 * Function determines if a given index has an ancestor index in the
 * saved_indices list. This allows us to prevent duplicate saving of things
 * that could be considered NXDetectors
 */
template <typename T>
bool isDesiredNXDetector(size_t index, const T &saved_indices,
                         const Geometry::ComponentInfo &compInfo) {
  return saved_indices.end() ==
         std::find_if(saved_indices.begin(), saved_indices.end(),
                      [&compInfo, &index](const size_t idx) {
                        return isAncestorOf(compInfo, idx, index);
                      });
}

/**
 * Internal save implementation. We can either write a new file containing
 * only the geometry, or we might also need to append/merge with an existing
 * file. Knowing the logic for this is important so we build an object around
 * the Mode state.
 */
class NexusGeometrySaveImpl {
public:
  enum class Mode { Trunc, Append };
  std::vector<detid_t> m_detectorIds;
  hid_t m_fileID;

  explicit NexusGeometrySaveImpl(Mode mode) : m_mode(mode) {}
  NexusGeometrySaveImpl(const NexusGeometrySaveImpl &) =
      delete; // No intention to suport copies

  /*
   * Function: NXInstrument
   * for NXentry parent (root group). Produces an NXinstrument group in the
   * parent group, and writes Nexus compliant datasets and metadata stored in
   * attributes to the new group.
   *
   * @param parent : parent group in which to write the NXinstrument group.
   * @param compInfo : componentinfo
   * @return NXinstrument group, to be passed into children save methods.
   */
  H5::Group instrument(const H5::Group &parent,
                       const Geometry::ComponentInfo &compInfo) {

    std::string nameInCache = compInfo.name(compInfo.root());
    std::string instrName =
        nameInCache.empty() ? "unspecified_instrument" : nameInCache;

    H5::Group childGroup = openOrCreateGroup(parent, instrName, NX_INSTRUMENT);

    writeStrDataset(childGroup, NAME, instrName);
    writeStrAttribute(childGroup, NX_CLASS, NX_INSTRUMENT);

    std::string defaultShortName = instrName.substr(0, 3);
    H5::DataSet name = childGroup.openDataSet(NAME);
    writeStrAttribute(name, SHORT_NAME, defaultShortName);
    return childGroup;
  }

  /*
   * Function: saveNXSample
   * For NXentry parent (root group). Produces an NXsample group in the parent
   * group, and writes the Nexus compliant datasets and metadata stored in
   * attributes to the new group.
   *
   * @param parent : parent group in which to write the NXinstrument group.
   * @param compInfo : componentInfo object.
   */
  void sample(const H5::Group &parentGroup,
              const Geometry::ComponentInfo &compInfo) {

    std::string nameInCache = compInfo.name(compInfo.sample());
    std::string sampleName =
        nameInCache.empty() ? "unspecified_sample" : nameInCache;

    H5::Group childGroup =
        openOrCreateGroup(parentGroup, sampleName, NX_SAMPLE);
    writeStrAttribute(childGroup, NX_CLASS, NX_SAMPLE);
    writeStrDataset(childGroup, NAME, sampleName);
  }

  /*
   * Function: saveNXSource
   * For NXentry (root group). Produces an NXsource group in the parent group,
   * and writes the Nexus compliant datasets and metadata stored in attributes
   * to the new group.
   *
   * @param parent : parent group in which to write the NXinstrument group.
   * @param compInfo : componentInfo object.
   */
  void source(const H5::Group &parentGroup,
              const Geometry::ComponentInfo &compInfo) {

    size_t index = compInfo.source();

    std::string nameInCache = compInfo.name(index);
    std::string sourceName =
        nameInCache.empty() ? "unspecified_source" : nameInCache;

    std::string dependency = NO_DEPENDENCY;

    Eigen::Vector3d position =
        Mantid::Kernel::toVector3d(compInfo.position(index));
    Eigen::Quaterniond rotation =
        Mantid::Kernel::toQuaterniond(compInfo.rotation(index));

    bool locationIsOrigin = isApproxZero(position, PRECISION);
    bool orientationIsZero = isApproxZero(rotation, PRECISION);

    H5::Group childGroup =
        openOrCreateGroup(parentGroup, sourceName, NX_SOURCE);
    writeStrAttribute(childGroup, NX_CLASS, NX_SOURCE);

    // do not write NXtransformations if there is no translation or rotation
    if (!(locationIsOrigin && orientationIsZero)) {
      H5::Group transformations =
          simpleNXSubGroup(childGroup, TRANSFORMATIONS, NX_TRANSFORMATIONS);

      // self, ".", is the default first NXsource dependency in the chain.
      // first check translation in NXsource is non-zero, and set dependency
      // to location if true and write location. Then check if orientation in
      // NXsource is non-zero, replace dependency with orientation if true. If
      // neither orientation nor location are non-zero, NXsource is self
      // dependent.
      if (!locationIsOrigin) {
        dependency = H5_OBJ_NAME(transformations) + "/" + LOCATION;
        writeLocation(transformations, position);
      }
      if (!orientationIsZero) {
        dependency = H5_OBJ_NAME(transformations) + "/" + ORIENTATION;

        // If location dataset is written to group also, then dependency for
        // orientation dataset containg the rotation transformation will be
        // location. Else dependency for orientation is self.
        std::string rotationDependency =
            locationIsOrigin ? NO_DEPENDENCY
                             : H5_OBJ_NAME(transformations) + "/" + LOCATION;
        writeOrientation(transformations, rotation, rotationDependency);
      }
    }

    writeStrDataset(childGroup, NAME, sourceName);
    writeStrDataset(childGroup, DEPENDS_ON, dependency);
  }

  /*
   * Function: monitor
   * For NXinstrument parent (component info root). Produces an NXmonitor
   * groups from Component info, and saves it in the parent
   * group, along with the Nexus compliant datasets, and metadata stored in
   * attributes to the new group.
   *
   * @param parentGroup : parent group in which to write the NXinstrument
   * group.
   * @param compInfo : componentInfo object.
   * @param monitorID :  ID of the specific monitor.
   * @param index :  index of the specific monitor in the Instrument cache.
   * @return child group for further additions
   */
  H5::Group monitor(const H5::Group &parentGroup,
                    const Geometry::ComponentInfo &compInfo,
                    const size_t index) {

    const detid_t monitorId = m_detectorIds[index];
    // if the component is unnamed sets the name as unspecified with the
    // location of the component in the cache
    std::string nameInCache = compInfo.name(index);
    std::string monitorName =
        nameInCache.empty() ? "unspecified_monitor_" + std::to_string(index)
                            : nameInCache;

    Eigen::Vector3d position =
        Mantid::Kernel::toVector3d(compInfo.position(index));
    Eigen::Quaterniond rotation =
        Mantid::Kernel::toQuaterniond(compInfo.rotation(index));

    std::string dependency = NO_DEPENDENCY; // dependency initialiser

    bool locationIsOrigin = isApproxZero(position, PRECISION);
    bool orientationIsZero = isApproxZero(rotation, PRECISION);

    H5::Group childGroup =
        openOrCreateGroup(parentGroup, monitorName, NX_MONITOR);
    writeStrAttribute(childGroup, NX_CLASS, NX_MONITOR);

    // do not write NXtransformations if there is no translation or rotation
    if (!(locationIsOrigin && orientationIsZero)) {
      H5::Group transformations =
          simpleNXSubGroup(childGroup, TRANSFORMATIONS, NX_TRANSFORMATIONS);

      // self, ".", is the default first NXmonitor dependency in the chain.
      // first check translation in NXmonitor is non-zero, and set dependency
      // to location if true and write location. Then check if orientation in
      // NXmonitor is non-zero, replace dependency with orientation if true.
      // If neither orientation nor location are non-zero, NXmonitor is self
      // dependent.
      if (!locationIsOrigin) {
        dependency = H5_OBJ_NAME(transformations) + "/" + LOCATION;
        writeLocation(transformations, position);
      }
      if (!orientationIsZero) {
        dependency = H5_OBJ_NAME(transformations) + "/" + ORIENTATION;

        // If location dataset is written to group also, then dependency for
        // orientation dataset containg the rotation transformation will be
        // location. Else dependency for orientation is self.
        std::string rotationDependency =
            locationIsOrigin ? NO_DEPENDENCY
                             : H5_OBJ_NAME(transformations) + "/" + LOCATION;
        writeOrientation(transformations, rotation, rotationDependency);
      }
    }

    H5::StrType dependencyStrType = strTypeOfSize(dependency);
    writeNXMonitorNumber(childGroup, monitorId);
    // write either INDIVIDUAL mesh or cylinder
    writeMonitorShape(compInfo, childGroup, index);

    writeStrDataset(childGroup, BANK_NAME, monitorName);
    writeStrDataset(childGroup, DEPENDS_ON, dependency);
    return childGroup;
  }

  /* For NXinstrument parent (component info root). Produces an NXmonitor
   * groups from Component info, and saves it in the parent
   * group, along with the Nexus compliant datasets, and metadata stored in
   * attributes to the new group.
   *
   * Saves detector-spectra mappings too
   *
   * @param parentGroup : parent group in which to write the NXinstrument
   * group.
   * @param compInfo : componentInfo object.
   * @param monitorId :  ID of the specific monitor.
   * @param index :  index of the specific monitor in the Instrument cache.
   * @param mappings : Spectra to detector mappings
   */
  void monitor(const H5::Group &parentGroup,
               const Geometry::ComponentInfo &compInfo, const size_t index,
               SpectraMappings &mappings) {

    auto childGroup = monitor(parentGroup, compInfo, index);
    // Additional mapping information written.
    writeDetectorCount(childGroup, mappings);
    // Note that the detector list is the same as detector_number, but it is
    // ordered by spectrum index 0 - N, whereas detector_number is just
    // written out in the order the detectors are encountered in the bank.
    writeDetectorList(childGroup, mappings);
    writeDetectorIndex(childGroup, mappings);
    writeSpectra(childGroup, mappings);
  }

  /*
   * Function: detectors
   * For NXinstrument parent (component info root). Save method which produces
   * a set of NXdetctor groups from Component info detector banks, and saves
   * it in the parent group, along with the Nexus compliant datasets, and
   * metadata stored in attributes to the new group.
   *
   * @param parentGroup : parent group in which to write the NXinstrument
   * group.
   * @param compInfo : componentInfo object.
   * @param detIDs : global detector IDs, from which those specific to the
   * NXdetector will be extracted.
   * @return childGroup for futher additions
   */
  H5::Group detector(const H5::Group &parentGroup,
                     const Geometry::ComponentInfo &compInfo,
                     const size_t index) {

    // if the component is unnamed sets the name as unspecified with the
    // location of the component in the cache
    std::string nameInCache = compInfo.name(index);
    std::string detectorName =
        nameInCache.empty() ? "unspecified_detector_at_" + std::to_string(index)
                            : nameInCache;

    Eigen::Vector3d position =
        Mantid::Kernel::toVector3d(compInfo.position(index));
    Eigen::Quaterniond rotation =
        Mantid::Kernel::toQuaterniond(compInfo.rotation(index));

    std::string dependency = NO_DEPENDENCY; // dependency initialiser

    bool locationIsOrigin = isApproxZero(position, PRECISION);
    bool orientationIsZero = isApproxZero(rotation, PRECISION);

    H5::Group childGroup =
        openOrCreateGroup(parentGroup, detectorName, NX_DETECTOR);
    writeStrAttribute(childGroup, NX_CLASS, NX_DETECTOR);

    // do not write NXtransformations if there is no translation or rotation
    if (!(locationIsOrigin && orientationIsZero)) {
      H5::Group transformations =
          simpleNXSubGroup(childGroup, TRANSFORMATIONS, NX_TRANSFORMATIONS);

      // self, ".", is the default first NXdetector dependency in the chain.
      // first check translation in NXdetector is non-zero, and set dependency
      // to location if true and write location. Then check if orientation in
      // NXdetector is non-zero, replace dependency with orientation if true.
      // If neither orientation nor location are non-zero, NXdetector is self
      // dependent.
      if (!locationIsOrigin) {
        dependency = H5_OBJ_NAME(transformations) + "/" + LOCATION;
        writeLocation(transformations, position);
      }
      if (!orientationIsZero) {
        dependency = H5_OBJ_NAME(transformations) + "/" + ORIENTATION;

        // If location dataset is written to group also, then dependency for
        // orientation dataset containg the rotation transformation will be
        // location. Else dependency for orientation is self.
        std::string rotationDependency =
            locationIsOrigin ? NO_DEPENDENCY
                             : H5_OBJ_NAME(transformations) + "/" + LOCATION;
        writeOrientation(transformations, rotation, rotationDependency);
      }
    }

    H5::StrType dependencyStrType = strTypeOfSize(dependency);
    writePixelOffsets(childGroup, compInfo, index);
    writeNXDetectorNumber(childGroup, compInfo, m_detectorIds, index);
    // write either meshes or cyinders
    writePixelShapes(compInfo, m_detectorIds, childGroup, index);

    writeStrDataset(childGroup, BANK_NAME, detectorName);
    writeStrDataset(childGroup, DEPENDS_ON, dependency);
    return childGroup;
  }

  /*
   * Function: detectors
   * For NXinstrument parent (component info root). Save method which produces
   * a set of NXdetctor groups from Component info detector banks, and saves
   * it in the parent group, along with the Nexus compliant datasets, and
   * metadata stored in attributes to the new group.
   *
   * @param parentGroup : parent group in which to write the NXinstrument
   * group.
   * @param compInfo : componentInfo object.
   * @param detIDs : global detector IDs, from which those specific to the
   * @param index : current component index
   * @param mappings : Spectra to detector mappings
   * NXdetector will be extracted.
   */
  void detector(const H5::Group &parentGroup,
                const Geometry::ComponentInfo &compInfo, const size_t index,
                SpectraMappings &mappings) {

    auto childGroup = detector(parentGroup, compInfo, index);

    // Additional mapping information written.
    writeDetectorCount(childGroup, mappings);
    // Note that the detector list is the same as detector_number, but it is
    // ordered by spectrum index 0 - N, whereas detector_number is just
    // written out in the order the detectors are encountered in the bank.
    writeDetectorList(childGroup, mappings);
    writeDetectorIndex(childGroup, mappings);
    writeSpectra(childGroup, mappings);
  }

  /*
    TODO:
    */
  void writeMesh(H5::Group &grp, const std::vector<double> &vertices,
                 const std::vector<uint32_t> windingOrder, const size_t nFaces,
                 const size_t nVertices, std::vector<size_t> detFaces = {}) {

    H5::DataSet dFaces, dVertices, dWindingOrder;

    // vertices

    int vrank = 2;
    hsize_t vdims[static_cast<hsize_t>(2)];
    vdims[0] = static_cast<hsize_t>(nVertices);
    vdims[1] = static_cast<hsize_t>(3);
    H5::DataSpace vspace = H5Screate_simple(vrank, vdims, nullptr);

    // Modify dataset creation property to enable chunking
    hsize_t vchunks[2] = {static_cast<hsize_t>(nVertices), 3};
    H5::DSetCreatPropList vplist;
    vplist.setChunk(2, vchunks);
    vplist.setDeflate(6);

    dVertices = grp.createDataSet(VERTICES, H5::PredType::NATIVE_DOUBLE, vspace,
                                  vplist);
    writeToDataset(dVertices, vertices, H5::PredType::NATIVE_DOUBLE, vspace);
    writeStrAttribute(dVertices, UNITS, METRES);

    // winding order

    int wrank = 1;
    auto wsize = static_cast<hsize_t>(windingOrder.size());
    hsize_t wdims[static_cast<hsize_t>(1)];
    wdims[0] = wsize;
    H5::DataSpace wspace = H5Screate_simple(wrank, wdims, nullptr);

    // Modify dataset creation property to enable chunking
    hsize_t wchunks[1] = {wsize};
    H5::DSetCreatPropList wplist;
    wplist.setChunk(1, wchunks);
    wplist.setDeflate(6);

    dWindingOrder = grp.createDataSet(WINDING_ORDER, H5::PredType::NATIVE_INT,
                                      wspace, wplist);
    writeToDataset(dWindingOrder, windingOrder, H5::PredType::NATIVE_INT,
                   wspace);

    // faces

    // fixed to 3 for triangles.
    int jumps = 3;

    std::vector<int> facesData;
    for (int i = 0; static_cast<hsize_t>(i) <= nFaces; ++i)
      facesData.push_back(i * jumps);

    int frank = 1;
    hsize_t fdims[static_cast<hsize_t>(1)];
    fdims[0] = static_cast<hsize_t>(nFaces);
    H5::DataSpace fspace = H5Screate_simple(frank, fdims, nullptr);

    // Modify dataset creation property to enable chunking
    hsize_t fchunks[1] = {static_cast<hsize_t>(nFaces)};
    H5::DSetCreatPropList fplist;
    fplist.setChunk(1, fchunks);
    fplist.setDeflate(6);

    dFaces = grp.createDataSet(FACES, H5::PredType::NATIVE_INT, fspace, fplist);
    writeToDataset(dFaces, facesData, H5::PredType::NATIVE_INT, fspace);

    // only if the mesh describes the shape of the NXdetector should
    // 'detector_faces' be written, such as when the pixels are
    // non-homogeneous
    if (!detFaces.empty()) {

      // detector faces
      int dfrank = 2;
      hsize_t dfdims[static_cast<hsize_t>(2)];
      dfdims[0] = static_cast<hsize_t>(detFaces.size() / 2);
      dfdims[1] = static_cast<hsize_t>(2);
      H5::DataSpace dfspace = H5Screate_simple(dfrank, dfdims, nullptr);
      H5::DataSet dDetectorFaces =
          grp.createDataSet(DETECTOR_FACES, H5::PredType::NATIVE_INT, dfspace);
      writeToDataset(dDetectorFaces, detFaces, H5::PredType::NATIVE_INT,
                     dfspace);
    }
  }

  /*
  TODO:
  */
  void writeCylinder(H5::Group &grp, size_t nCylinders,
                     const std::vector<double> &vertexCoordinates) {

    H5::DataSet cylinders, vertices;
    // prepare the data
    std::vector<int> cylinderData(nCylinders * 3);
    for (size_t i = 0; i < nCylinders * 3; i += 3) {
      for (int j = 0; j < 3; ++j) {
        cylinderData[i + j] = static_cast<int>(i + j); // testing output
      }
    }

    int crank = 2;
    hsize_t cdims[static_cast<hsize_t>(2)];
    cdims[0] = static_cast<hsize_t>(nCylinders); // nCylinders;
    cdims[1] = static_cast<hsize_t>(3);
    H5::DataSpace cspace = H5Screate_simple(crank, cdims, nullptr);

    // Modify dataset creation property to enable chunking
    if (nCylinders > 1) {
      // cylinder dataset can be chunked if describing more than cylinder only
      hsize_t cchunks[2] = {nCylinders, 3};
      H5::DSetCreatPropList cplist;
      cplist.setChunk(2, cchunks);
      cplist.setDeflate(6);
      cylinders = grp.createDataSet(CYLINDERS, H5::PredType::NATIVE_INT, cspace,
                                    cplist);
    } else {
      // cylinder dataset is contiguous if describing only one pixel shape.
      cylinders =
          grp.createDataSet(CYLINDERS, H5::PredType::NATIVE_INT, cspace);
    }
    writeToDataset(cylinders, cylinderData, H5::PredType::NATIVE_INT, cspace);

    int vrank = 2;
    hsize_t vdims[static_cast<hsize_t>(2)];
    vdims[0] = static_cast<hsize_t>(3 * nCylinders);
    vdims[1] = static_cast<hsize_t>(3);
    H5::DataSpace vspace = H5Screate_simple(vrank, vdims, nullptr);

    // Modify dataset creation property to enable chunking
    hsize_t vchunks[2] = {static_cast<hsize_t>(3 * nCylinders), 3};
    H5::DSetCreatPropList vplist;
    vplist.setChunk(2, vchunks);
    vplist.setDeflate(6);

    vertices = grp.createDataSet(VERTICES, H5::PredType::NATIVE_DOUBLE, vspace,
                                 vplist);
    writeToDataset(vertices, vertexCoordinates, H5::PredType::NATIVE_DOUBLE,
                   vspace);
    writeStrAttribute(vertices, UNITS, METRES);
  }

  template <typename T>
  void writeMeshObjShapeToPixels(const Geometry::ComponentInfo &compInfo,
                                 const std::vector<int> &detIds, H5::Group &grp,
                                 std::vector<size_t> &meshes, size_t bankIdx) {

    // shape type of the first detector in children detectors
    auto &firstShapeObj = compInfo.shape(meshes.front());
    auto fMeshObj = dynamic_cast<const T *>(&firstShapeObj);
    auto fVertices = fMeshObj->getVertices();
    auto fWindingOrder = fMeshObj->getTriangles();

    // prevents undefined behaviour when passing empty container into
    // std::all_of. If empty, there is no data to write. No associated group
    // will be created in file.

    // check if cylinders are homogeneous
    bool meshesAreHomogeneous =
        std::all_of(meshes.begin(), meshes.end(), [&](const size_t &idx) {
          auto &shapeObj = compInfo.shape(idx);
          auto meshObj = dynamic_cast<const T *>(&shapeObj);
          auto vertices = meshObj->getVertices();
          auto windingOrder = meshObj->getTriangles();

          // by extension, if the vertices in both first and current mesh are
          // equal, then the number of triangles are also equal
          return (fVertices == vertices && fWindingOrder == windingOrder);
        });
    if (meshesAreHomogeneous) {
      // homogeneous pixels can be optimised so as to only write the shape of
      // one pixel, the group of which is called the 'pixel_shape'
      H5::Group pixelShapeGroup = simpleNXSubGroup(grp, PIXEL_SHAPE, NX_OFF);
      // write first mesh

      auto &shapeObj = compInfo.shape(meshes.front());
      auto meshObj = dynamic_cast<const T *>(&shapeObj);

      size_t nFaces = meshObj->numberOfTriangles();
      size_t nVertices = meshObj->numberOfVertices();

      auto windingOrder = meshObj->getTriangles();
      std::vector<double> vertices = meshObj->getVertices();

      writeMesh(pixelShapeGroup, vertices, windingOrder, nFaces, nVertices);

    } else {
      // non-homogeneous pixels will describe the entire shape of the
      // detector, the group for which is called 'detector_shape'
      H5::Group pixelShapeGroup = simpleNXSubGroup(grp, DETECTOR_SHAPE, NX_OFF);
      std::vector<double> vertices;
      std::vector<int> detFaces;
      std::vector<uint32_t> windingOrder;
      size_t totalNumOfVertices = 0;
      size_t totalNumOfFaces = 0;
      for (std::vector<size_t>::iterator it = meshes.begin();
           it != meshes.end(); ++it) {

        auto &shapeObj = compInfo.shape(*it);
        auto meshObj = dynamic_cast<const T *>(&shapeObj);

        size_t nFaces = meshObj->numberOfTriangles();
        size_t nVertices = meshObj->numberOfVertices();

        std::vector<uint32_t> currentWindingOrder = meshObj->getTriangles();
        std::vector<double> currentVertices = meshObj->getVertices();

        // append the data
        vertices.insert(vertices.end(), currentVertices.begin(),
                        currentVertices.end());
        windingOrder.insert(windingOrder.end(), currentWindingOrder.begin(),
                            currentWindingOrder.end());
        totalNumOfFaces += nFaces;
        totalNumOfVertices += nVertices;
      }
      const std::vector<size_t> detFacesData =
          findDetectorFaces(compInfo, bankIdx, detIds);
      writeMesh(pixelShapeGroup, vertices, windingOrder, totalNumOfFaces,
                totalNumOfVertices, detFacesData);
    }
  }

  /*
  TODO: DOC
  */
  void writeCylinderShapeToPixels(const Geometry::ComponentInfo &compInfo,
                                  H5::Group &grp,
                                  std::vector<size_t> &cylinders) {

    // shape type of the first detector in children detectors
    auto &firstShape = compInfo.shape(cylinders.front());
    auto firstShapeInfo = firstShape.shapeInfo();
    auto fType = firstShapeInfo.shape();
    auto fHeight = firstShapeInfo.height();
    auto fRadius = firstShapeInfo.radius();

    // check if cylinders are homogeneous
    bool cylindersAreHomogeneous =
        std::all_of(cylinders.begin(), cylinders.end(), [&](const size_t &idx) {
          auto &shapeObj = compInfo.shape(idx);
          auto shapeInfo = shapeObj.shapeInfo();
          auto type = shapeInfo.shape();
          auto height = shapeInfo.height();
          auto radius = shapeInfo.radius();
          return (type == fType && height == fHeight && fRadius == radius);
        });
    if (cylindersAreHomogeneous) {
      // homogeneous pixels can be optimised so as to only write the shape of
      // one pixel, the group of which is called the 'pixel_shape'
      H5::Group pixelShapeGroup =
          simpleNXSubGroup(grp, PIXEL_SHAPE, NX_CYLINDER);
      // write cylinder only once, using the first index
      auto geometry = firstShapeInfo.cylinderGeometry();
      const Eigen::Vector3d &base =
          Kernel::toVector3d(geometry.centreOfBottomBase);
      const Eigen::Vector3d &axis = Kernel::toVector3d(geometry.axis);
      double &height = geometry.height;
      double &radius = geometry.radius;

      double a, b, c;
      a = axis[0];
      b = axis[1];
      c = axis[2];
      // A vector orthogonal to the axis and lying on plane z.
      Eigen::Vector3d orthogonalToAxis = generateOrthogonal(axis);
      if (!isApproxZero(axis.dot(orthogonalToAxis), PRECISION))
        throw;
      orthogonalToAxis.normalize();
      Eigen::Vector3d edge = base + (radius * orthogonalToAxis);
      Eigen::Vector3d top = base + (height * axis.normalized());

      std::vector<double> vertices{base[0], base[1], base[2], edge[0], edge[1],
                                   edge[2], top[0],  top[1],  top[2]};
      writeCylinder(pixelShapeGroup, 1, vertices);
    } else {
      // non-homogeneous pixels will describe the entire shape of the
      // detector, the group for which is called 'detector_shape'
      H5::Group pixelShapeGroup =
          simpleNXSubGroup(grp, DETECTOR_SHAPE, NX_CYLINDER);
      std::vector<double> vertices;
      for (std::vector<size_t>::iterator it = cylinders.begin();
           it != cylinders.end(); ++it) {

        auto shapeInfo = compInfo.shape(*it).shapeInfo();
        auto geometry = shapeInfo.cylinderGeometry();
        const Eigen::Vector3d &base =
            Kernel::toVector3d(geometry.centreOfBottomBase);
        const Eigen::Vector3d &axis = Kernel::toVector3d(geometry.axis);
        double &height = geometry.height;
        double &radius = geometry.radius;

        Eigen::Vector3d orthogonalToBase{base[1], base[0], base[2]};
        if (!isApproxZero(orthogonalToBase.dot(orthogonalToBase), PRECISION))
          throw;
        orthogonalToBase.normalize();
        Eigen::Vector3d edge = base + (radius * orthogonalToBase);
        Eigen::Vector3d top = base + (height * axis.normalized());

        std::vector<double> additionalVertices{base[0], base[1], base[2],
                                               edge[0], edge[1], edge[2],
                                               top[0],  top[1],  top[2]};
        vertices.insert(vertices.end(), additionalVertices.begin(),
                        additionalVertices.end());
      }
      writeCylinder(pixelShapeGroup, cylinders.size(), vertices);
    }
  }

  /*
  optimisation for
  */
  void writeMonitorShape(const Geometry::ComponentInfo &compInfo,
                         H5::Group &grp, size_t idx) {

    if (compInfo.hasValidShape(idx)) {
      auto &shapeObj = compInfo.shape(idx);
      if (const auto mesh =
              dynamic_cast<const Geometry::MeshObject *>(&shapeObj)) {

        size_t nFaces = mesh->numberOfTriangles();
        size_t nVertices = mesh->numberOfVertices();
        auto windingOrder = mesh->getTriangles();
        std::vector<double> vertices = mesh->getVertices();
        H5::Group shapeGroup = simpleNXSubGroup(grp, SHAPE, NX_OFF);
        writeMesh(shapeGroup, vertices, windingOrder, nFaces, nVertices);
      } else if (const auto mesh =
                     dynamic_cast<const Geometry::MeshObject2D *>(&shapeObj)) {
        size_t nFaces = mesh->numberOfTriangles();
        size_t nVertices = mesh->numberOfVertices();
        auto windingOrder = mesh->getTriangles();
        std::vector<double> vertices = mesh->getVertices();
        H5::Group shapeGroup = simpleNXSubGroup(grp, SHAPE, NX_OFF);
        writeMesh(shapeGroup, vertices, windingOrder, nFaces, nVertices);

      } else {

        auto shapeInfo = shapeObj.shapeInfo();
        auto type = shapeObj.shapeInfo().shape();
        // this makes the assumption that if either dynamic casts above fail
        // then the shape must be of type CSGObject, and has implementation
        // for shabeObj.ShapeInfo().
        if (static_cast<int>(type) == 4 /*CYLINDER*/) {

          H5::Group shapeGroup = simpleNXSubGroup(grp, SHAPE, NX_CYLINDER);

          auto geometry = shapeInfo.cylinderGeometry();
          const Eigen::Vector3d &base =
              Kernel::toVector3d(geometry.centreOfBottomBase);
          const Eigen::Vector3d &axis = Kernel::toVector3d(geometry.axis);
          double &height = geometry.height;
          double &radius = geometry.radius;

          Eigen::Vector3d orthogonalToBase{base[1], base[0], base[2]};
          if (!isApproxZero(orthogonalToBase.dot(orthogonalToBase), PRECISION))
            throw;
          orthogonalToBase.normalize();
          Eigen::Vector3d edge = base + (radius * orthogonalToBase);
          Eigen::Vector3d top = base + (height * axis.normalized());

          std::vector<double> vertices{base[0], base[1], base[2],
                                       edge[0], edge[1], edge[2],
                                       top[0],  top[1],  top[2]};
          writeCylinder(shapeGroup, 1, vertices);
        }
      }
    }
  }

  void writePixelShapes(const Geometry::ComponentInfo &compInfo,
                        const std::vector<int> &detIds, H5::Group &grp,
                        size_t idx) {

    auto pixels =
        compInfo.detectorsInSubtree(idx); // indices of all pixels in bank
    // If children detectors empty, there is no data to write to bank. exits
    // here.
    if (pixels.empty())
      return;

    std::vector<size_t> meshes;
    std::vector<size_t> meshes2D;
    std::vector<size_t> cylinders;

    // shape type of the first detector in children detectors

    // return true if all shape types, heights and radii are equal to the
    // first shape in children detectors.

    const Geometry::MeshObject *mesh;
    const Geometry::MeshObject2D *mesh2D;

    std::for_each(pixels.begin(), pixels.end(), [&](const size_t &idx) {
      if (compInfo.hasValidShape(idx)) {
        auto &shapeObj = compInfo.shape(idx);
        if (mesh = dynamic_cast<const Geometry::MeshObject *>(&shapeObj))
          meshes.push_back(idx);
        else if (mesh2D =
                     dynamic_cast<const Geometry::MeshObject2D *>(&shapeObj))
          meshes2D.push_back(idx);
        else {
          // this makes the assumption that if either dynamic casts above fail
          // then the shape must be of type CSGObject, and has implementation
          // for shabeObj.ShapeInfo().
          auto type = shapeObj.shapeInfo().shape();
          if (static_cast<int>(type) == 4 /*CYLINDER*/)
            cylinders.push_back(idx);
        }
      }
    });

    // prevents undefined behaviour when passing empty container into
    // std::all_of. If empty, there is no data to write. No associated group
    // will be created in file.
    if (meshes.size() == pixels.size())
      writeMeshObjShapeToPixels<Geometry::MeshObject>(compInfo, detIds, grp,
                                                      meshes, idx);
    else if (meshes2D.size() == pixels.size())
      writeMeshObjShapeToPixels<Geometry::MeshObject2D>(compInfo, detIds, grp,
                                                        meshes2D, idx);
    else if (cylinders.size() == pixels.size())
      writeCylinderShapeToPixels(compInfo, grp, cylinders);
  }

private:
  const Mode m_mode;

  H5::Group openOrCreateGroup(const H5::Group &parent, const std::string &name,
                              const std::string &classType) {

    if (m_mode == Mode::Append) {
      // Find by class and by name
      auto results = utilities::findGroups(parent, classType);
      for (auto &result : results) {
        auto resultName = H5_OBJ_NAME(result);
        // resultName gives full path. We match the last name on the path
        if (std::regex_match(resultName, std::regex(".*/" + name + "$"))) {
          return result;
        }
      }
    }
    // We can't find it, or we are writing from scratch anyway
    return tryCreateGroup(parent, name);
  }

  // function to create a simple sub-group that has a nexus class attribute,
  // inside a parent group.
  H5::Group simpleNXSubGroup(H5::Group &parent, const std::string &name,
                             const std::string &nexusAttribute) {
    H5::Group subGroup = openOrCreateGroup(parent, name, nexusAttribute);
    writeStrAttribute(subGroup, NX_CLASS, nexusAttribute);
    return subGroup;
  }

  template <typename T>
  void writeToDataset(H5::DataSet &dSet, const std::vector<T> &data,
                      const H5::PredType &predType, const H5::DataSpace space) {

    if (!data.empty()) {

      dSet.write(data.data(), predType, space);
    }
  }

  template <typename T>
  void writeToAttribute(H5::Attribute &attr, const std::vector<T> &data,
                        const H5::PredType &predType) {
    if (!data.empty())
      attr.write(predType, data.data());
  }

  /*
   * Function: writePixelOffsets
   * write the x, y, and z offset of the pixels from the parent detector bank as
   * HDF5 datasets to HDF5 group. If all of the pixel offsets in either x, y, or
   * z are approximately zero, skips writing that dataset to file.
   *
   * @param grp : HDF5 parent group
   * @param compInfo : Component Info Instrument cache
   * @param idx : index of bank in cache.
   */
  void writePixelOffsets(H5::Group &grp,
                         const Geometry::ComponentInfo &compInfo,
                         const size_t idx) {

    H5::DataSet xPixelOffset, yPixelOffset,
        zPixelOffset; // pixel offset datasets
    auto pixels =
        compInfo.detectorsInSubtree(idx); // indices of all pixels in bank
    const auto nPixels = pixels.size();   // number of pixels

    // If children detectors empty, there is no data to write to bank. exits
    // here.
    if (pixels.empty())
      return;

    std::vector<double> posx;
    std::vector<double> posy;
    std::vector<double> posz;

    posx.reserve(nPixels);
    posy.reserve(nPixels);
    posz.reserve(nPixels);

    bool xIsZero{true}; // becomes false when atleast 1 non-zero x found
    bool yIsZero{true}; // becomes false when atleast 1 non-zero y found
    bool zIsZero{true}; // becomes false when atleast 1 non-zero z found
    // get pixel offset data
    for (std::vector<size_t>::iterator it = pixels.begin(); it != pixels.end();
         ++it) {
      Eigen::Vector3d offset =
          Geometry::ComponentInfoBankHelpers::offsetFromAncestor(compInfo, idx,
                                                                 *it);
      if (!isApproxZero(offset[0], PRECISION))
        xIsZero = false;
      if (!isApproxZero(offset[1], PRECISION))
        yIsZero = false;
      if (!isApproxZero(offset[2], PRECISION))
        zIsZero = false;

      posx.push_back(offset[0]); // x pixel offset
      posy.push_back(offset[1]); // y pixel offset
      posz.push_back(offset[2]); // z pixel offset
    }

    auto bankName = compInfo.name(idx);
    const auto nDetectorsInBank = static_cast<hsize_t>(posx.size());

    int rank = 1;
    hsize_t dims[static_cast<hsize_t>(1)];
    dims[0] = nDetectorsInBank;
    H5::DataSpace space = H5Screate_simple(rank, dims, nullptr);

    // Modify dataset creation property to enable chunking
    hsize_t chunks[1] = {nDetectorsInBank};
    H5::DSetCreatPropList plist;
    plist.setChunk(1, chunks);
    plist.setDeflate(6);

    if (!xIsZero) {
      xPixelOffset = grp.createDataSet(
          X_PIXEL_OFFSET, H5::PredType::NATIVE_DOUBLE, space, plist);
      writeToDataset(xPixelOffset, posx, H5::PredType::NATIVE_DOUBLE, space);
      writeStrAttribute(xPixelOffset, UNITS, METRES);
    }

    if (!yIsZero) {
      yPixelOffset = grp.createDataSet(
          Y_PIXEL_OFFSET, H5::PredType::NATIVE_DOUBLE, space, plist);
      writeToDataset(yPixelOffset, posy, H5::PredType::NATIVE_DOUBLE, space);
      writeStrAttribute(yPixelOffset, UNITS, METRES);
    }

    if (!zIsZero) {
      zPixelOffset = grp.createDataSet(
          Z_PIXEL_OFFSET, H5::PredType::NATIVE_DOUBLE, space, plist);
      writeToDataset(zPixelOffset, posz, H5::PredType::NATIVE_DOUBLE, space);
      writeStrAttribute(zPixelOffset, UNITS, METRES);
    }
  }

  /*
   * Function: writeNXMonitorNumber
   * For use with NXmonitor group. write 'detector_id's of an NXmonitor, which
   * is a specific type of pixel, to its group.
   *
   * @param grp : NXmonitor group (HDF group)
   * @param monitorID : monitor ID to be
   * stored into dataset 'detector_id' (or 'detector_number'. naming convention
   * inconsistency?).
   */
  void writeNXMonitorNumber(H5::Group &grp, const int monitorID) {

    // these DataSets are duplicates of each other. written to the NXmonitor
    // group to handle the naming inconsistency. probably temporary.
    H5::DataSet detectorNumber, detector_id;

    int rank = 1;
    hsize_t dims[static_cast<hsize_t>(1)];
    dims[0] = static_cast<hsize_t>(1);

    H5::DataSpace space = H5Screate_simple(rank, dims, nullptr);

    // these DataSets are duplicates of each other. written to the group to
    // handle the naming inconsistency. probably temporary.

    // monitorIds are always singular, and can be contiguous.
    if (!utilities::findDataset(grp, DETECTOR_IDS)) {

      detectorNumber =
          grp.createDataSet(DETECTOR_IDS, H5::PredType::NATIVE_INT, space);
      detectorNumber.write(&monitorID, H5::PredType::NATIVE_INT, space);
    }
    if (!utilities::findDataset(grp, DETECTOR_ID)) {

      detector_id =
          grp.createDataSet(DETECTOR_ID, H5::PredType::NATIVE_INT, space);
      detector_id.write(&monitorID, H5::PredType::NATIVE_INT, space);
    }
  }

  /*
   * Function: writeLocation
   * For use with NXdetector group. Writes absolute position of detector bank to
   * dataset and metadata as attributes.
   *
   * @param grp : NXdetector group : (HDF group)
   * @param position : Eigen::Vector3d position of component in instrument
   * cache.
   */
  void writeLocation(H5::Group &grp, const Eigen::Vector3d &position) {

    std::string dependency = NO_DEPENDENCY; // self dependent

    double norm;

    H5::DataSet location;
    H5::DataSpace dspace;
    H5::DataSpace aspace;

    H5::Attribute vector;
    H5::Attribute units;
    H5::Attribute transformationType;
    H5::Attribute dependsOn;

    H5::StrType strSize;

    int drank = 1;                          // rank of dataset
    hsize_t ddims[static_cast<hsize_t>(1)]; // dimensions of dataset
    ddims[0] = static_cast<hsize_t>(1);     // datapoints in dataset dimension 0

    norm = position.norm();               // norm of the position vector
    auto unitVec = position.normalized(); // unit vector of the position vector
    std::vector<double> stdNormPos =
        toStdVector(unitVec); // convert to std::vector

    dspace = H5Screate_simple(drank, ddims, nullptr); // dataspace for dataset
    location = grp.createDataSet(LOCATION, H5::PredType::NATIVE_DOUBLE,
                                 dspace); // dataset location
    location.write(&norm, H5::PredType::NATIVE_DOUBLE,
                   dspace); // write norm to location

    int arank = 1;                          // rank of attribute
    hsize_t adims[static_cast<hsize_t>(3)]; // dimensions of attribute
    adims[0] = 3; // datapoints in attribute dimension 0

    aspace = H5Screate_simple(arank, adims, nullptr); // dataspace for attribute
    vector = location.createAttribute(VECTOR, H5::PredType::NATIVE_DOUBLE,
                                      aspace); // attribute vector
    writeToAttribute(
        vector, stdNormPos,
        H5::PredType::NATIVE_DOUBLE); // write unit vector to vector

    // units attribute
    strSize = strTypeOfSize(METRES);
    units = location.createAttribute(UNITS, strSize, SCALAR);
    units.write(strSize, METRES);

    // transformation-type attribute
    strSize = strTypeOfSize(TRANSLATION);
    transformationType =
        location.createAttribute(TRANSFORMATION_TYPE, strSize, SCALAR);
    transformationType.write(strSize, TRANSLATION);

    // dependency attribute
    strSize = strTypeOfSize(dependency);
    dependsOn = location.createAttribute(DEPENDS_ON, strSize, SCALAR);
    dependsOn.write(strSize, dependency);
  }

  /*
   * Function: writeOrientation
   * For use with NXdetector group. Writes the absolute rotation of detector
   * bank to dataset and metadata as attributes.
   *
   * @param grp : NXdetector group : (HDF group)
   * @param rotation : Eigen::Quaterniond rotation of component in instrument
   * cache.
   * @param dependency : dependency of the orientation dataset:
   * Compliant to the Mantid Instrument Definition file, if a translation
   * exists, it precedes a rotation.
   * https://docs.mantidproject.org/nightly/concepts/InstrumentDefinitionFile.html
   */
  void writeOrientation(H5::Group &grp, const Eigen::Quaterniond &rotation,
                        const std::string &dependency) {

    // dependency for orientation defaults to self-dependent. If Location
    // dataset exists, the orientation will depend on it instead.

    double angle;

    H5::DataSet orientation;
    H5::DataSpace dspace;
    H5::DataSpace aspace;

    H5::Attribute vector;
    H5::Attribute units;
    H5::Attribute transformationType;
    H5::Attribute dependsOn;

    H5::StrType strSize;

    int drank = 1;                          // rank of dataset
    hsize_t ddims[static_cast<hsize_t>(1)]; // dimensions of dataset
    ddims[0] = static_cast<hsize_t>(1);     // datapoints in dataset dimension 0

    angle = std::acos(rotation.w()) * (360.0 / M_PI); // angle magnitude
    Eigen::Vector3d axisOfRotation = rotation.vec().normalized(); // angle axis
    std::vector<double> stdNormAxis =
        toStdVector(axisOfRotation); // convert to std::vector

    dspace = H5Screate_simple(drank, ddims, nullptr); // dataspace for dataset
    orientation = grp.createDataSet(ORIENTATION, H5::PredType::NATIVE_DOUBLE,
                                    dspace); // dataset orientation
    orientation.write(&angle, H5::PredType::NATIVE_DOUBLE,
                      dspace); // write angle magnitude to orientation

    int arank = 1;                          // rank of attribute
    hsize_t adims[static_cast<hsize_t>(3)]; // dimensions of attribute
    adims[0] = static_cast<hsize_t>(3); // datapoints in attibute dimension 0

    aspace = H5Screate_simple(arank, adims, nullptr); // dataspace for attribute
    vector = orientation.createAttribute(VECTOR, H5::PredType::NATIVE_DOUBLE,
                                         aspace); // attribute vector
    writeToAttribute(vector, stdNormAxis,
                     H5::PredType::NATIVE_DOUBLE); // write angle axis to vector

    // units attribute
    strSize = strTypeOfSize(DEGREES);
    units = orientation.createAttribute(UNITS, strSize, SCALAR);
    units.write(strSize, DEGREES);

    // transformation-type attribute
    strSize = strTypeOfSize(ROTATION);
    transformationType =
        orientation.createAttribute(TRANSFORMATION_TYPE, strSize, SCALAR);
    transformationType.write(strSize, ROTATION);

    // dependency attribute
    strSize = strTypeOfSize(dependency);
    dependsOn = orientation.createAttribute(DEPENDS_ON, strSize, SCALAR);
    dependsOn.write(strSize, dependency);
  }

  template <typename T>
  void write1DIntDataset(H5::Group &grp, const H5std_string &name,
                         const std::vector<T> &container) {

    auto csize = static_cast<hsize_t>(container.size());
    H5::DataSet dataset;
    const int rank = 1;
    hsize_t dims[1] = {csize};

    H5::DataSpace space = H5Screate_simple(rank, dims, nullptr);

    // dataset can be chunked if datasize is > 1. else is contiguous.
    if (csize > 1) {
      hsize_t chunks[1] = {csize};
      H5::DSetCreatPropList plist;
      plist.setChunk(1, chunks);
      plist.setDeflate(6);
      dataset = grp.createDataSet(name, H5::PredType::NATIVE_INT, space, plist);
    } else {
      dataset = grp.createDataSet(name, H5::PredType::NATIVE_INT, space);
    }
    writeToDataset(dataset, container, H5::PredType::NATIVE_INT, space);
  }

  /*
   * Function: writeNXDetectorNumber
   * For use with NXdetector group. Writes the detector numbers for all detector
   * pixels in compInfo to a new dataset in the group.
   *
   * @param detectorIDs : std::vector<int> container of all detectorIDs to be
   * stored into dataset 'detector_number'.
   * @param compInfo : instrument cache with component info.
   * @idx : size_t index of bank in compInfo.
   */
  void writeNXDetectorNumber(H5::Group &grp,
                             const Geometry::ComponentInfo &compInfo,
                             const std::vector<int> &detectorIDs,
                             const size_t idx) {

    H5::DataSet detectorNumber;

    std::vector<int> bankDetIDs; // IDs of detectors beloning to bank
    std::vector<size_t> bankDetectors = compInfo.detectorsInSubtree(
        idx); // Indexes of children detectors in bank
    bankDetIDs.reserve(bankDetectors.size());

    // write the ID for each child detector to std::vector to be written to
    // dataset
    std::for_each(bankDetectors.begin(), bankDetectors.end(),
                  [&bankDetIDs, &detectorIDs](const size_t index) {
                    bankDetIDs.push_back(detectorIDs[index]);
                  });

    write1DIntDataset(grp, DETECTOR_IDS, bankDetIDs);
  }

  // Write the count of how many detectors contribute to each spectra
  void writeDetectorCount(H5::Group &grp, const SpectraMappings &mappings) {
    write1DIntDataset(grp, SPECTRA_COUNTS, mappings.detector_count);
  }

  // Write the detectors ids ordered by spectra index 0 - N for each NXDetector
  void writeDetectorList(H5::Group &grp, const SpectraMappings &mappings) {
    write1DIntDataset(grp, DETECTOR_LIST, mappings.detector_list);
  }

  // Write the detector indexes. This provides offsets into the detector_list
  // and is sized to the number of spectra
  void writeDetectorIndex(H5::Group &grp, const SpectraMappings &mappings) {
    write1DIntDataset(grp, DETECTOR_INDEX, mappings.detector_index);
  }

  // Write the spectra numbers for each spectra
  void writeSpectra(H5::Group &grp, const SpectraMappings &mappings) {
    write1DIntDataset(grp, SPECTRA_NUMBERS, mappings.spectra_ids);
  }

}; // class NexusGeometrySaveImpl

/**
 * calls the save methods to write components to file after exception
 * checking. Produces a Nexus format file containing the Instrument geometry
 * and metadata.
 *
 * @param compInfo : componentInfo object.
 * @param detInfo : detectorInfo object.
 * @param fullPath : save destination as full path.
 * @param rootName : name of root entry
 * @param logger : logging object
 * @param append : append mode, means openting and appending to existing file.
 * If false, creates new file.
 * @param reporter : (optional) report to progressBase.
 */
void saveInstrument(const Geometry::ComponentInfo &compInfo,
                    const Geometry::DetectorInfo &detInfo,
                    const std::string &fullPath, const std::string &rootName,
                    AbstractLogger &logger, bool append,
                    Kernel::ProgressBase *reporter) {

  validateInputs(logger, fullPath, compInfo);
  // IDs of all detectors in Instrument
  const auto &detIds = detInfo.detectorIDs();

  H5::Group rootGroup;
  H5::H5File file;
  hid_t fileID;

  if (append) {
    file = H5::H5File(fullPath, H5F_ACC_RDWR); // open file
    rootGroup = file.openGroup(rootName);
  } else {
    file = H5::H5File(fullPath, H5F_ACC_TRUNC); // open file
    rootGroup = file.createGroup(rootName);
  }

  fileID = H5Fcreate(fullPath.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

  writeStrAttribute(rootGroup, NX_CLASS, NX_ENTRY);

  using Mode = NexusGeometrySaveImpl::Mode;
  NexusGeometrySaveImpl writer(append ? Mode::Append : Mode::Trunc);

  // save and capture NXinstrument (component root)
  H5::Group instrument = writer.instrument(rootGroup, compInfo);

  // save NXsource
  writer.source(instrument, compInfo);

  // save NXsample
  writer.sample(rootGroup, compInfo);

  writer.m_fileID = fileID;
  writer.m_detectorIds = detIds;

  // save NXdetectors
  std::list<size_t> saved_indices;
  // Looping from highest to lowest component index is critical
  for (size_t index = compInfo.root() - 1; index >= detInfo.size(); --index) {
    if (Geometry::ComponentInfoBankHelpers::isSaveableBank(compInfo, index)) {
      if (isDesiredNXDetector(index, saved_indices, compInfo)) {
        if (reporter != nullptr)
          reporter->report();
        writer.detector(instrument, compInfo, index);
        saved_indices.push_back(index); // Now record the fact that children of
                                        // this are not needed as NXdetectors
      }
    }
  }

  // save NXmonitors
  for (size_t index = 0; index < detInfo.size(); ++index) {
    if (detInfo.isMonitor(index)) {
      if (reporter != nullptr)
        reporter->report();
      writer.monitor(instrument, compInfo, index);
    }
  }

  file.close(); // close file

} // saveInstrument

/**
 * Function: saveInstrument (overload)
 * calls the save methods to write components to file after exception
 * checking. Produces a Nexus format file containing the Instrument geometry
 * and metadata.
 *
 * @param instrPair : instrument 2.0  object.
 * @param fullPath : save destination as full path.
 * @param rootName : name of root entry
 * @param logger : logging object
 * @param append : append mode, means openting and appending to existing file.
 * If false, creates new file.
 * @param reporter : (optional) report to progressBase.
 */
void saveInstrument(
    const std::pair<std::unique_ptr<Geometry::ComponentInfo>,
                    std::unique_ptr<Geometry::DetectorInfo>> &instrPair,
    const std::string &fullPath, const std::string &rootName,
    AbstractLogger &logger, bool append, Kernel::ProgressBase *reporter) {

  const Geometry::ComponentInfo &compInfo = (*instrPair.first);
  const Geometry::DetectorInfo &detInfo = (*instrPair.second);

  return saveInstrument(compInfo, detInfo, fullPath, rootName, logger, append,
                        reporter);
}

void saveInstrument(const Mantid::API::MatrixWorkspace &ws,
                    const std::string &fullPath, const std::string &rootName,
                    AbstractLogger &logger, bool append,
                    Kernel::ProgressBase *reporter) {

  const auto &detInfo = ws.detectorInfo();
  const auto &compInfo = ws.componentInfo();

  // Exception handling.
  validateInputs(logger, fullPath, compInfo);
  // IDs of all detectors in Instrument
  const auto &detIds = detInfo.detectorIDs();

  H5::Group rootGroup;
  H5::H5File file;
  hid_t fileID;

  if (append) {
    file = H5::H5File(fullPath, H5F_ACC_RDWR); // open file
    rootGroup = file.openGroup(rootName);
  } else {
    file = H5::H5File(fullPath, H5F_ACC_TRUNC); // open file
    rootGroup = file.createGroup(rootName);
  }

  fileID = H5Fcreate(fullPath.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

  writeStrAttribute(rootGroup, NX_CLASS, NX_ENTRY);

  using Mode = NexusGeometrySaveImpl::Mode;
  NexusGeometrySaveImpl writer(append ? Mode::Append : Mode::Trunc);
  // save and capture NXinstrument (component root)
  H5::Group instrument = writer.instrument(rootGroup, compInfo);

  // save NXsource
  writer.source(instrument, compInfo);

  // save NXsample
  writer.sample(rootGroup, compInfo);

  writer.m_fileID = fileID;
  writer.m_detectorIds = detIds;

  // save NXdetectors
  auto detToIndexMap =
      ws.getDetectorIDToWorkspaceIndexMap(false /*do not throw if multiples*/);
  std::list<size_t> saved_indices;
  // Looping from highest to lowest component index is critical
  for (size_t index = compInfo.root() - 1; index >= detInfo.size(); --index) {
    if (Geometry::ComponentInfoBankHelpers::isSaveableBank(compInfo, index)) {

      if (isDesiredNXDetector(index, saved_indices, compInfo)) {
        // Make spectra detector mappings that can be used
        SpectraMappings mappings =
            makeMappings(compInfo, detToIndexMap, ws.indexInfo(),
                         ws.spectrumInfo(), detIds, index);

        if (reporter != nullptr)
          reporter->report();
        writer.detector(instrument, compInfo, index, mappings);
        saved_indices.push_back(index); // Now record the fact that children of
                                        // this are not needed as NXdetectors
      }
    }
  }

  // save NXmonitors
  for (size_t index = 0; index < detInfo.size(); ++index) {
    if (detInfo.isMonitor(index)) {
      // Make spectra detector mappings that can be used
      SpectraMappings mappings =
          makeMappings(compInfo, detToIndexMap, ws.indexInfo(),
                       ws.spectrumInfo(), detIds, index);

      if (reporter != nullptr)
        reporter->report();
      writer.monitor(instrument, compInfo, index, mappings);
    }
  }

  file.close(); // close file
}

// saveInstrument

} // namespace NexusGeometrySave
} // namespace NexusGeometry
} // namespace Mantid
