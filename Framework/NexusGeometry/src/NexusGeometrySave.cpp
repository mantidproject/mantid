// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidNexusGeometry/H5ForwardCompatibility.h"
#include "MantidNexusGeometry/NexusGeometryDefinitions.h"
#include "MantidNexusGeometry/NexusGeometryUtilities.h"
#include <H5Cpp.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <list>
#include <memory>
#include <string>

namespace Mantid::NexusGeometry::NexusGeometrySave {
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
inline H5::Group tryCreateGroup(const H5::Group &parentGroup, const std::string &childGroupName) {
  H5std_string parentGroupName = H5_OBJ_NAME(parentGroup);
  for (hsize_t i = 0; i < parentGroup.getNumObjs(); ++i) {
    if (parentGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
      H5std_string child = parentGroup.getObjnameByIdx(i);
      if (childGroupName == child) {
        // TODO: runtime error instead?
        throw std::invalid_argument("Cannot create group with name " + childGroupName + " inside parent group " +
                                    parentGroupName + " because a group with this name already exists.");
      }
    }
  }
  return parentGroup.createGroup(childGroupName);
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
  stdVector.emplace_back(data.X());
  stdVector.emplace_back(data.Y());
  stdVector.emplace_back(data.Z());
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
std::vector<double> toStdVector(const Eigen::Vector3d &data) { return toStdVector(Kernel::toV3D(data)); }

/*
 * Function: isApproxZero. returns true if all values in an variable-sized
 * std-vector container evaluate to zero with a given level of precision. Used
 * by SaveInstrument methods to determine whether or not to write a dataset to
 * file.
 *
 * @param data : std::vector<T> data
 * @param precision : double precision specifier
 * @return true if all elements are approx zero, else false.
 */
bool isApproxZero(const std::vector<double> &data, const double &precision) {

  return std::all_of(data.begin(), data.end(),
                     [&precision](const double &element) { return std::abs(element) < precision; });
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
void writeStrDataset(H5::Group &grp, const std::string &dSetName, const std::string &dSetVal,
                     const H5::DataSpace &dataSpace = SCALAR) {
  // TODO. may need to review if we shoud in fact replace.
  if (!utilities::findDataset(grp, dSetName)) {
    H5::StrType dataType = strTypeOfSize(dSetVal);
    H5::DataSet dSet = grp.createDataSet(dSetName, dataType, dataSpace);
    dSet.write(dSetVal, dataType);
  }
}

/*
 * Function: writeStrAttribute
 * writes a StrType HDF attribute and attribute value to a HDF group.
 *
 * @param grp : HDF group object.
 * @param attrname : attribute name.
 * @param attrVal : string attribute value to be stored in attribute.
 */
void writeStrAttribute(H5::Group &grp, const std::string &attrName, const std::string &attrVal,
                       const H5::DataSpace &dataSpace = SCALAR) {
  if (!grp.attrExists(attrName)) {
    H5::StrType dataType = strTypeOfSize(attrVal);
    H5::Attribute attribute = grp.createAttribute(attrName, dataType, dataSpace);
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
void writeStrAttribute(H5::DataSet &dSet, const std::string &attrName, const std::string &attrVal,
                       const H5::DataSpace &dataSpace = SCALAR) {
  if (!dSet.attrExists(attrName)) {
    H5::StrType dataType = strTypeOfSize(attrVal);
    auto attribute = dSet.createAttribute(attrName, dataType, dataSpace);
    attribute.write(dataType, attrVal);
  }
}

/*
 * Function: writeXYZPixeloffset
 * write the x, y, and z offset of the pixels from the parent detector bank as
 * HDF5 datasets to HDF5 group. If all of the pixel offsets in either x, y, or z
 * are approximately zero, skips writing that dataset to file.
 * @param grp : HDF5 parent group
 * @param compInfo : Component Info Instrument cache
 * @param idx : index of bank in cache.
 */
void writeXYZPixeloffset(H5::Group &grp, const Geometry::ComponentInfo &compInfo, const size_t idx) {

  H5::DataSet xPixelOffset, yPixelOffset, zPixelOffset;
  auto childrenDetectors = compInfo.detectorsInSubtree(idx);

  std::vector<double> posx;
  std::vector<double> posy;
  std::vector<double> posz;

  posx.reserve(childrenDetectors.size());
  posy.reserve(childrenDetectors.size());
  posz.reserve(childrenDetectors.size());

  for (const size_t &i : childrenDetectors) {

    auto offset = Geometry::ComponentInfoBankHelpers::offsetFromAncestor(compInfo, idx, i);

    posx.emplace_back(offset[0]);
    posy.emplace_back(offset[1]);
    posz.emplace_back(offset[2]);
  }

  bool xIsZero = isApproxZero(posx, PRECISION);
  bool yIsZero = isApproxZero(posy, PRECISION);
  bool zIsZero = isApproxZero(posz, PRECISION);

  const auto nDetectorsInBank = static_cast<hsize_t>(posx.size());

  int rank = 1;
  hsize_t dims[static_cast<hsize_t>(1)];
  dims[0] = nDetectorsInBank;

  H5::DataSpace space = H5Screate_simple(rank, dims, nullptr);

  if (!xIsZero) {
    xPixelOffset = grp.createDataSet(X_PIXEL_OFFSET, H5::PredType::NATIVE_DOUBLE, space);
    xPixelOffset.write(posx.data(), H5::PredType::NATIVE_DOUBLE, space);
    writeStrAttribute(xPixelOffset, UNITS, METRES);
  }

  if (!yIsZero) {
    yPixelOffset = grp.createDataSet(Y_PIXEL_OFFSET, H5::PredType::NATIVE_DOUBLE, space);
    yPixelOffset.write(posy.data(), H5::PredType::NATIVE_DOUBLE);
    writeStrAttribute(yPixelOffset, UNITS, METRES);
  }

  if (!zIsZero) {
    zPixelOffset = grp.createDataSet(Z_PIXEL_OFFSET, H5::PredType::NATIVE_DOUBLE, space);
    zPixelOffset.write(posz.data(), H5::PredType::NATIVE_DOUBLE);
    writeStrAttribute(zPixelOffset, UNITS, METRES);
  }
}

template <typename T>
void write1DIntDataset(H5::Group &grp, const H5std_string &name, const std::vector<T> &container) {
  const int rank = 1;
  hsize_t dims[1] = {static_cast<hsize_t>(container.size())};

  H5::DataSpace space = H5Screate_simple(rank, dims, nullptr);

  auto dataset = grp.createDataSet(name, H5::PredType::NATIVE_INT, space);
  if (!container.empty())
    dataset.write(container.data(), H5::PredType::NATIVE_INT, space);
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
void writeNXDetectorNumber(H5::Group &grp, const Geometry::ComponentInfo &compInfo, const std::vector<int> &detectorIDs,
                           const size_t idx) {

  H5::DataSet detectorNumber;

  std::vector<int> bankDetIDs;                                          // IDs of detectors beloning to bank
  std::vector<size_t> bankDetectors = compInfo.detectorsInSubtree(idx); // Indexes of children detectors in bank
  bankDetIDs.reserve(bankDetectors.size());

  // write the ID for each child detector to std::vector to be written to
  // dataset
  std::for_each(bankDetectors.begin(), bankDetectors.end(),
                [&bankDetIDs, &detectorIDs](const size_t index) { bankDetIDs.emplace_back(detectorIDs[index]); });

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

// Write the detector indexes. This provides offsets into the detector_list and
// is sized to the number of spectra
void writeDetectorIndex(H5::Group &grp, const SpectraMappings &mappings) {
  write1DIntDataset(grp, DETECTOR_INDEX, mappings.detector_index);
}

// Write the spectra numbers for each spectra
void writeSpectra(H5::Group &grp, const SpectraMappings &mappings) {
  write1DIntDataset(grp, SPECTRA_NUMBERS, mappings.spectra_ids);
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
  if (!utilities::findDataset(grp, DETECTOR_IDS)) {
    detectorNumber = grp.createDataSet(DETECTOR_IDS, H5::PredType::NATIVE_INT, space);
    detectorNumber.write(&monitorID, H5::PredType::NATIVE_INT, space);
  }
  if (!utilities::findDataset(grp, DETECTOR_ID)) {

    detector_id = grp.createDataSet(DETECTOR_ID, H5::PredType::NATIVE_INT, space);
    detector_id.write(&monitorID, H5::PredType::NATIVE_INT, space);
  }
}

/*
 * Function: writeLocation
 * For use with NXdetector group. Writes absolute position of detector bank to
 * dataset and metadata as attributes.
 *
 * @param grp : NXdetector group : (HDF group)
 * @param position : Eigen::Vector3d position of component in instrument cache.
 */
inline void writeLocation(H5::Group &grp, const Eigen::Vector3d &position) {

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

  norm = position.norm();                                // norm od the position vector
  auto unitVec = position.normalized();                  // unit vector of the position vector
  std::vector<double> stdNormPos = toStdVector(unitVec); // convert to std::vector

  dspace = H5Screate_simple(drank, ddims, nullptr); // dataspace for dataset
  location = grp.createDataSet(LOCATION, H5::PredType::NATIVE_DOUBLE,
                               dspace); // dataset location
  location.write(&norm, H5::PredType::NATIVE_DOUBLE,
                 dspace); // write norm to location

  int arank = 1;                          // rank of attribute
  hsize_t adims[static_cast<hsize_t>(3)]; // dimensions of attribute
  adims[0] = 3;                           // datapoints in attribute dimension 0

  aspace = H5Screate_simple(arank, adims, nullptr); // dataspace for attribute
  vector = location.createAttribute(VECTOR, H5::PredType::NATIVE_DOUBLE,
                                    aspace); // attribute vector
  vector.write(H5::PredType::NATIVE_DOUBLE,
               stdNormPos.data()); // write unit vector to vector

  // units attribute
  strSize = strTypeOfSize(METRES);
  units = location.createAttribute(UNITS, strSize, SCALAR);
  units.write(strSize, METRES);

  // transformation-type attribute
  strSize = strTypeOfSize(TRANSLATION);
  transformationType = location.createAttribute(TRANSFORMATION_TYPE, strSize, SCALAR);
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
inline void writeOrientation(H5::Group &grp, const Eigen::Quaterniond &rotation, const std::string &dependency) {

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

  angle = std::acos(rotation.w()) * (360.0 / M_PI);              // angle magnitude
  Eigen::Vector3d axisOfRotation = rotation.vec().normalized();  // angle axis
  std::vector<double> stdNormAxis = toStdVector(axisOfRotation); // convert to std::vector

  dspace = H5Screate_simple(drank, ddims, nullptr); // dataspace for dataset
  orientation = grp.createDataSet(ORIENTATION, H5::PredType::NATIVE_DOUBLE,
                                  dspace); // dataset orientation
  orientation.write(&angle, H5::PredType::NATIVE_DOUBLE,
                    dspace); // write angle magnitude to orientation

  int arank = 1;                          // rank of attribute
  hsize_t adims[static_cast<hsize_t>(3)]; // dimensions of attribute
  adims[0] = static_cast<hsize_t>(3);     // datapoints in attibute dimension 0

  aspace = H5Screate_simple(arank, adims, nullptr); // dataspace for attribute
  vector = orientation.createAttribute(VECTOR, H5::PredType::NATIVE_DOUBLE,
                                       aspace); // attribute vector
  vector.write(H5::PredType::NATIVE_DOUBLE,
               stdNormAxis.data()); // write angle axis to vector

  // units attribute
  strSize = strTypeOfSize(DEGREES);
  units = orientation.createAttribute(UNITS, strSize, SCALAR);
  units.write(strSize, DEGREES);

  // transformation-type attribute
  strSize = strTypeOfSize(ROTATION);
  transformationType = orientation.createAttribute(TRANSFORMATION_TYPE, strSize, SCALAR);
  transformationType.write(strSize, ROTATION);

  // dependency attribute
  strSize = strTypeOfSize(dependency);
  dependsOn = orientation.createAttribute(DEPENDS_ON, strSize, SCALAR);
  dependsOn.write(strSize, dependency);
}

SpectraMappings makeMappings(const Geometry::ComponentInfo &compInfo, const detid2index_map &detToIndexMap,
                             const Indexing::IndexInfo &indexInfo, const API::SpectrumInfo &specInfo,
                             const std::vector<Mantid::detid_t> &detIds, size_t index) {
  auto childrenDetectors = compInfo.detectorsInSubtree(index);
  size_t nChildDetectors = childrenDetectors.size(); // Number of detectors actually considered in
                                                     // spectra-detector map for this NXdetector
  // local to this nxdetector
  std::map<size_t, int> detector_count_map;
  // We start knowing only the detector index, we have to establish spectra from
  // that.
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
  for (const auto &pair : detector_count_map) {
    // using sort order of map to ensure we are ordered by lowest to highest
    // spectrum index
    mappings.detector_count[specCounter] = (pair.second); // Counts
    mappings.detector_index[specCounter + 1] = mappings.detector_index[specCounter] + (pair.second);
    mappings.spectra_ids[specCounter] = int32_t(indexInfo.spectrumNumber(pair.first));

    // We will list everything by spectrum index, so we need to add the detector
    // ids in the same order.
    const auto &specDefintion = specInfo.spectrumDefinition(pair.first);
    for (const auto &def : specDefintion) {
      mappings.detector_list[detCounter] = detIds[def.first];
      ++detCounter;
    }
    ++specCounter;
  }
  mappings.detector_index.resize(detector_count_map.size()); // cut-off last item

  return mappings;
}

void validateInputs(AbstractLogger &logger, const std::string &fullPath, const Geometry::ComponentInfo &compInfo) {
  std::filesystem::path tmp(fullPath);
  if (!std::filesystem::is_directory(tmp.root_directory())) {
    throw std::invalid_argument("The path provided for saving the file is invalid: " + fullPath + "\n");
  }

  // check the file extension matches any of the valid extensions defined in
  // nexus_geometry_extensions
  const auto ext = std::filesystem::path(tmp).extension();
  bool isValidExt = std::any_of(nexus_geometry_extensions.begin(), nexus_geometry_extensions.end(),
                                [&ext](const std::string &str) { return ext.generic_string() == str; });

  // throw if the file extension is invalid
  if (!isValidExt) {
    // string of valid extensions to output in exception
    std::string extensions;
    std::for_each(nexus_geometry_extensions.begin(), nexus_geometry_extensions.end(),
                  [&extensions](const std::string &str) { extensions += " " + str; });
    std::string message = "invalid extension for file: '" + ext.generic_string() + "'. Expected any of: " + extensions;
    logger.error(message);
    throw std::invalid_argument(message);
  }

  if (!compInfo.hasDetectorInfo()) {
    logger.error("No detector info was found in the Instrument. Instrument not saved.");
    throw std::invalid_argument("No detector info was found in the Instrument");
  }
  if (!compInfo.hasSample()) {
    logger.error("No sample was found in the Instrument. Instrument not saved.");
    throw std::invalid_argument("No sample was found in the Instrument");
  }

  if (Mantid::Kernel::V3D{0, 0, 0} != compInfo.samplePosition()) {
    logger.error("The sample positon is required to be at the origin. "
                 "Instrument not saved.");
    throw std::invalid_argument("The sample positon is required to be at the origin");
  }

  if (!compInfo.hasSource()) {
    logger.error("No source was found in the Instrument. "
                 "Instrument not saved.");
    throw std::invalid_argument("No source was found in the Instrument");
  }
}

/*
 * Function determines if a given index has an ancestor index in the
 * saved_indices list. This allows us to prevent duplicate saving of things that
 * could be considered NXDetectors
 */
template <typename T>
bool isDesiredNXDetector(size_t index, const T &saved_indices, const Geometry::ComponentInfo &compInfo) {
  return saved_indices.end() ==
         std::find_if(saved_indices.begin(), saved_indices.end(),
                      [&compInfo, &index](const size_t idx) { return isAncestorOf(compInfo, idx, index); });
}

/**
 * Internal save implementation. We can either write a new file containing only
 * the geometry, or we might also need to append/merge with an existing file.
 * Knowing the logic for this is important so we build an object around the Mode
 * state.
 */
class NexusGeometrySaveImpl {
public:
  enum class Mode { Trunc, Append };

  explicit NexusGeometrySaveImpl(Mode mode) : m_mode(mode) {}
  NexusGeometrySaveImpl(const NexusGeometrySaveImpl &) = delete; // No intention to suport copies

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
  H5::Group instrument(const H5::Group &parent, const Geometry::ComponentInfo &compInfo) {

    std::string nameInCache = compInfo.name(compInfo.root());
    std::string instrName = nameInCache.empty() ? "unspecified_instrument" : nameInCache;

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
  void sample(const H5::Group &parentGroup, const Geometry::ComponentInfo &compInfo) {

    std::string nameInCache = compInfo.name(compInfo.sample());
    std::string sampleName = nameInCache.empty() ? "unspecified_sample" : nameInCache;

    H5::Group childGroup = openOrCreateGroup(parentGroup, sampleName, NX_SAMPLE);
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
  void source(const H5::Group &parentGroup, const Geometry::ComponentInfo &compInfo) {

    size_t index = compInfo.source();

    std::string nameInCache = compInfo.name(index);
    std::string sourceName = nameInCache.empty() ? "unspecified_source" : nameInCache;

    std::string dependency = NO_DEPENDENCY;

    Eigen::Vector3d position = Mantid::Kernel::toVector3d(compInfo.position(index));
    Eigen::Quaterniond rotation = Mantid::Kernel::toQuaterniond(compInfo.rotation(index));

    bool locationIsOrigin = isApproxZero(position, PRECISION);
    bool orientationIsZero = isApproxZero(rotation, PRECISION);

    H5::Group childGroup = openOrCreateGroup(parentGroup, sourceName, NX_SOURCE);
    writeStrAttribute(childGroup, NX_CLASS, NX_SOURCE);

    // do not write NXtransformations if there is no translation or rotation
    if (!(locationIsOrigin && orientationIsZero)) {
      H5::Group transformations = simpleNXSubGroup(childGroup, TRANSFORMATIONS, NX_TRANSFORMATIONS);

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
            locationIsOrigin ? NO_DEPENDENCY : H5_OBJ_NAME(transformations) + "/" + LOCATION;
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
  H5::Group monitor(const H5::Group &parentGroup, const Geometry::ComponentInfo &compInfo, const int monitorId,
                    const size_t index) {

    // if the component is unnamed sets the name as unspecified with the
    // location of the component in the cache
    std::string nameInCache = compInfo.name(index);
    std::string monitorName = nameInCache.empty() ? "unspecified_monitor_" + std::to_string(index) : nameInCache;

    Eigen::Vector3d position = Mantid::Kernel::toVector3d(compInfo.position(index));
    Eigen::Quaterniond rotation = Mantid::Kernel::toQuaterniond(compInfo.rotation(index));

    std::string dependency = NO_DEPENDENCY; // dependency initialiser

    bool locationIsOrigin = isApproxZero(position, PRECISION);
    bool orientationIsZero = isApproxZero(rotation, PRECISION);

    H5::Group childGroup = openOrCreateGroup(parentGroup, monitorName, NX_MONITOR);
    writeStrAttribute(childGroup, NX_CLASS, NX_MONITOR);

    // do not write NXtransformations if there is no translation or rotation
    if (!(locationIsOrigin && orientationIsZero)) {
      H5::Group transformations = simpleNXSubGroup(childGroup, TRANSFORMATIONS, NX_TRANSFORMATIONS);

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
            locationIsOrigin ? NO_DEPENDENCY : H5_OBJ_NAME(transformations) + "/" + LOCATION;
        writeOrientation(transformations, rotation, rotationDependency);
      }
    }

    writeNXMonitorNumber(childGroup, monitorId);

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
  void monitor(const H5::Group &parentGroup, const Geometry::ComponentInfo &compInfo, const int monitorId,
               const size_t index, const SpectraMappings &mappings) {

    auto childGroup = monitor(parentGroup, compInfo, monitorId, index);
    // Additional mapping information written.
    writeDetectorCount(childGroup, mappings);
    // Note that the detector list is the same as detector_number, but it is
    // ordered by spectrum index 0 - N, whereas detector_number is just written
    // out in the order the detectors are encountered in the bank.
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
  H5::Group detector(const H5::Group &parentGroup, const Geometry::ComponentInfo &compInfo,
                     const std::vector<int> &detIds, const size_t index) {

    // if the component is unnamed sets the name as unspecified with the
    // location of the component in the cache
    std::string nameInCache = compInfo.name(index);
    std::string detectorName = nameInCache.empty() ? "unspecified_detector_at_" + std::to_string(index) : nameInCache;

    Eigen::Vector3d position = Mantid::Kernel::toVector3d(compInfo.position(index));
    Eigen::Quaterniond rotation = Mantid::Kernel::toQuaterniond(compInfo.rotation(index));

    std::string dependency = NO_DEPENDENCY; // dependency initialiser

    bool locationIsOrigin = isApproxZero(position, PRECISION);
    bool orientationIsZero = isApproxZero(rotation, PRECISION);

    H5::Group childGroup = openOrCreateGroup(parentGroup, detectorName, NX_DETECTOR);
    writeStrAttribute(childGroup, NX_CLASS, NX_DETECTOR);

    // do not write NXtransformations if there is no translation or rotation
    if (!(locationIsOrigin && orientationIsZero)) {
      H5::Group transformations = simpleNXSubGroup(childGroup, TRANSFORMATIONS, NX_TRANSFORMATIONS);

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
            locationIsOrigin ? NO_DEPENDENCY : H5_OBJ_NAME(transformations) + "/" + LOCATION;
        writeOrientation(transformations, rotation, rotationDependency);
      }
    }

    writeXYZPixeloffset(childGroup, compInfo, index);
    writeNXDetectorNumber(childGroup, compInfo, detIds, index);

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
  void detector(const H5::Group &parentGroup, const Geometry::ComponentInfo &compInfo, const std::vector<int> &detIds,
                const size_t index, const SpectraMappings &mappings) {

    auto childGroup = detector(parentGroup, compInfo, detIds, index);

    // Additional mapping information written.
    writeDetectorCount(childGroup, mappings);
    // Note that the detector list is the same as detector_number, but it is
    // ordered by spectrum index 0 - N, whereas detector_number is just written
    // out in the order the detectors are encountered in the bank.
    writeDetectorList(childGroup, mappings);
    writeDetectorIndex(childGroup, mappings);
    writeSpectra(childGroup, mappings);
  }

private:
  const Mode m_mode;

  H5::Group openOrCreateGroup(const H5::Group &parent, const std::string &name, const std::string &classType) {

    if (m_mode == Mode::Append) {
      // Find by class and by name
      auto results = utilities::findGroups(parent, classType);
      const auto it = std::find_if(results.cbegin(), results.cend(),
                                   [&name](const auto &result) { return utilities::isNamed(result, name); });
      if (it != results.cend())
        return *it;
    }
    // We can't find it, or we are writing from scratch anyway
    return tryCreateGroup(parent, name);
  }

  // function to create a simple sub-group that has a nexus class attribute,
  // inside a parent group.
  H5::Group simpleNXSubGroup(const H5::Group &parent, const std::string &name, const std::string &nexusAttribute) {
    H5::Group subGroup = openOrCreateGroup(parent, name, nexusAttribute);
    writeStrAttribute(subGroup, NX_CLASS, nexusAttribute);
    return subGroup;
  }
}; // class NexusGeometrySaveImpl

/*
 * Function: saveInstrument
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
void saveInstrument(const Geometry::ComponentInfo &compInfo, const Geometry::DetectorInfo &detInfo,
                    const std::string &fullPath, const std::string &rootName, AbstractLogger &logger, bool append,
                    Kernel::ProgressBase *reporter) {

  validateInputs(logger, fullPath, compInfo);
  // IDs of all detectors in Instrument
  H5::Group rootGroup;
  H5::H5File file;
  if (append) {
    file = H5::H5File(fullPath, H5F_ACC_RDWR); // open file
    rootGroup = file.openGroup(rootName);
  } else {
    file = H5::H5File(fullPath, H5F_ACC_TRUNC); // open file
    rootGroup = file.createGroup(rootName);
  }

  writeStrAttribute(rootGroup, NX_CLASS, NX_ENTRY);

  using Mode = NexusGeometrySaveImpl::Mode;
  NexusGeometrySaveImpl writer(append ? Mode::Append : Mode::Trunc);
  // save and capture NXinstrument (component root)
  H5::Group instrument = writer.instrument(rootGroup, compInfo);

  // save NXsource
  writer.source(instrument, compInfo);

  // save NXsample
  writer.sample(rootGroup, compInfo);

  const auto &detIds = detInfo.detectorIDs();
  // save NXdetectors
  std::list<size_t> saved_indices;
  // Looping from highest to lowest component index is critical
  for (size_t index = compInfo.root() - 1; index >= detInfo.size(); --index) {
    if (Geometry::ComponentInfoBankHelpers::isSaveableBank(compInfo, detInfo, index)) {
      if (isDesiredNXDetector(index, saved_indices, compInfo)) {
        if (reporter != nullptr)
          reporter->report();
        writer.detector(instrument, compInfo, detIds, index);
        saved_indices.emplace_back(index); // Now record the fact that children of
                                           // this are not needed as NXdetectors
      }
    }
  }

  // save NXmonitors
  for (size_t index = 0; index < detInfo.size(); ++index) {
    if (detInfo.isMonitor(index)) {
      if (reporter != nullptr)
        reporter->report();
      writer.monitor(instrument, compInfo, detIds[index], index);
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
    const std::pair<std::unique_ptr<Geometry::ComponentInfo>, std::unique_ptr<Geometry::DetectorInfo>> &instrPair,
    const std::string &fullPath, const std::string &rootName, AbstractLogger &logger, bool append,
    Kernel::ProgressBase *reporter) {

  const Geometry::ComponentInfo &compInfo = (*instrPair.first);
  const Geometry::DetectorInfo &detInfo = (*instrPair.second);

  return saveInstrument(compInfo, detInfo, fullPath, rootName, logger, append, reporter);
}

void saveInstrument(const Mantid::API::MatrixWorkspace &ws, const std::string &fullPath, const std::string &rootName,
                    AbstractLogger &logger, bool append, Kernel::ProgressBase *reporter) {

  const auto &detInfo = ws.detectorInfo();
  const auto &compInfo = ws.componentInfo();

  // Exception handling.
  validateInputs(logger, fullPath, compInfo);
  // IDs of all detectors in Instrument
  const auto &detIds = detInfo.detectorIDs();

  H5::Group rootGroup;
  H5::H5File file;
  if (append) {
    file = H5::H5File(fullPath, H5F_ACC_RDWR); // open file
    rootGroup = file.openGroup(rootName);
  } else {
    file = H5::H5File(fullPath, H5F_ACC_TRUNC); // open file
    rootGroup = file.createGroup(rootName);
  }

  writeStrAttribute(rootGroup, NX_CLASS, NX_ENTRY);

  using Mode = NexusGeometrySaveImpl::Mode;
  NexusGeometrySaveImpl writer(append ? Mode::Append : Mode::Trunc);
  // save and capture NXinstrument (component root)
  H5::Group instrument = writer.instrument(rootGroup, compInfo);

  // save NXsource
  writer.source(instrument, compInfo);

  // save NXsample
  writer.sample(rootGroup, compInfo);

  // save NXdetectors
  auto detToIndexMap = ws.getDetectorIDToWorkspaceIndexMap(false /*do not throw if multiples*/);
  std::list<size_t> saved_indices;
  // Looping from highest to lowest component index is critical
  for (size_t index = compInfo.root() - 1; index >= detInfo.size(); --index) {
    if (Geometry::ComponentInfoBankHelpers::isSaveableBank(compInfo, detInfo, index)) {

      if (isDesiredNXDetector(index, saved_indices, compInfo)) {
        // Make spectra detector mappings that can be used
        SpectraMappings mappings =
            makeMappings(compInfo, detToIndexMap, ws.indexInfo(), ws.spectrumInfo(), detIds, index);

        if (reporter != nullptr)
          reporter->report();
        writer.detector(instrument, compInfo, detIds, index, mappings);
        saved_indices.emplace_back(index); // Now record the fact that children of
                                           // this are not needed as NXdetectors
      }
    }
  }

  // save NXmonitors
  for (size_t index = 0; index < detInfo.size(); ++index) {
    if (detInfo.isMonitor(index)) {
      // Make spectra detector mappings that can be used
      SpectraMappings mappings =
          makeMappings(compInfo, detToIndexMap, ws.indexInfo(), ws.spectrumInfo(), detIds, index);

      if (reporter != nullptr)
        reporter->report();
      writer.monitor(instrument, compInfo, detIds[index], index, mappings);
    }
  }

  file.close(); // close file
}

// saveInstrument

} // namespace Mantid::NexusGeometry::NexusGeometrySave
