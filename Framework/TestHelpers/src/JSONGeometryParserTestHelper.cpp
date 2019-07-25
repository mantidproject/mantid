// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidTestHelpers/JSONGeometryParserTestHelper.h"
#include "json/json.h"
#include <iostream>
#include <numeric>

namespace {
template <class T> std::string getType() {
  std::string type;
  if (std::is_same<T, std::int64_t>::value)
    type = "int64";
  if (std::is_same<T, std::int32_t>::value)
    type = "int32";
  if (std::is_same<T, double>::value)
    type = "double";
  if (std::is_same<T, float>::value)
    type = "float";

  return type;
}

Json::Value createNXAttributes(const std::string &NXClass) {
  Json::Value attributes;
  attributes[0]["name"] = "NX_class";
  attributes[0]["values"] = NXClass;
  return attributes;
}

Json::Value createAttribute(const std::string &name,
                            const std::string &values) {
  Json::Value attribute;
  attribute["name"] = name;
  attribute["values"] = values;
  return attribute;
}

template <class T>
Json::Value createAttribute(const std::string &name,
                            const std::vector<T> &values) {
  Json::Value attribute;
  attribute["name"] = name;
  attribute["type"] = getType<T>();
  for (size_t i = 0; i < values.size(); ++i)
    attribute["values"][static_cast<int>(i)] = values[i];

  return attribute;
}

Json::Value createEmptyDataset(const std::string &name,
                               const std::string &type) {
  Json::Value dataset;
  dataset["type"] = "dataset";
  dataset["name"] = name;

  Json::Value datasetType;
  datasetType["type"] = type;

  dataset["dataset"] = datasetType;

  return dataset;
}

Json::Value createNX(const std::string &name, const std::string &NXClass) {
  Json::Value nx;
  nx["type"] = "group";
  nx["name"] = name;
  nx["children"].resize(0);
  nx["attributes"] = createNXAttributes(NXClass);

  return nx;
}

Json::Value createEmptyBank(int bankNum) {
  std::string bankName = "detector_" + std::to_string(bankNum);
  Json::Value bank = createNX(bankName, "NXdetector");
  return bank;
}

void appendToChildren(Json::Value &parent, const Json::Value &child) {
  Json::Value &children = parent["children"];
  children[children.size()] = child;
}

void resizeValues(Json::Value &values, int32_t size) {
  if (values.size() == 0) {
    values.resize(size);
    for (uint32_t i = 0; i < values.size(); ++i)
      values[i].resize(0);
  } else {
    for (auto &val : values) {
      if (val.size() > 0) {
        for (auto &child : val)
          resizeValues(child, size);
      } else
        resizeValues(val, size);
    }
  }
}

template <class T>
void fillValues(Json::Value &values, const std::vector<T> &fillArray,
                int &start, int size) {
  if (values.size() > 0) {
    for (auto &val : values)
      fillValues(val, fillArray, start, size);
  } else {
    for (int i = 0; i < size; ++i) {
      values[i] = fillArray[start + i];
    }
    start += size;
  }
}

template <class T>
void addDataset(Json::Value &parent, const std::string &name,
                const std::vector<int> &shape, const std::vector<T> &values,
                const std::string &attributesName = "",
                const std::string &attributesValues = "") {
  auto dataset = createEmptyDataset(name, getType<T>());
  auto numVals = 1;
  int i = 0;
  for (; i < static_cast<int>(shape.size() - 1); ++i) {
    auto s = shape[i];
    numVals *= s;
    dataset["dataset"]["size"][i] = s;
    resizeValues(dataset["values"], s);
  }

  auto leafSize = shape[shape.size() - 1];
  dataset["dataset"]["size"][i] = leafSize;
  int start = 0;
  fillValues(dataset["values"], values, start, leafSize);

  if (!attributesName.empty())
    dataset["attributes"][0] =
        createAttribute(attributesName, attributesValues);

  appendToChildren(parent, dataset);
}

void addTransformationChild(
    Json::Value &transformation, const std::string &name,
    const std::string &transformationType, const std::string &dependency,
    const std::string &units, const std::vector<int> &shape,
    const std::vector<double> &values, const std::vector<double> vec) {
  addDataset(transformation, name, shape, values, "units", units);
  auto index = transformation["children"].size() - 1;
  Json::Value &child = transformation["children"][index];
  child["attributes"][1] = createAttribute("vector", vec);
  child["attributes"][2] = createAttribute("depends_on", dependency);
  child["attributes"][3] =
      createAttribute("transformation_type", transformationType);
}

Json::Value &addNX(Json::Value &parent, const std::string &name,
                   const std::string &NXClass) {
  auto &children = parent["children"];
  auto &child = children[children.size()];
  child = createNX(name, NXClass);
  return child;
}

void addStream(Json::Value &parent, const std::string &name,
               const std::string &topic, const std::string &source,
               const std::string &writerModule) {
  Json::Value streamGroup;
  streamGroup["type"] = "group";
  streamGroup["name"] = name;
  Json::Value stream;
  stream["type"] = "stream";
  stream["stream"]["topic"] = topic;
  stream["stream"]["source"] = source;
  stream["stream"]["writer_module"] = writerModule;
  appendToChildren(streamGroup, stream);
  appendToChildren(parent, streamGroup);
}
} // namespace

namespace Mantid {
namespace TestHelpers {
namespace JSONInstrumentBuilder {

void initialiseRoot(Json::Value &root, const std::string &name) { root[name]; }

Json::Value &addNXEntry(Json::Value &root, const std::string &name) {
  return addNX(root["nexus_structure"], name, "NXentry");
}

Json::Value &addNXSample(Json::Value &entry, const std::string &name) {
  return addNX(entry, name, "NXsample");
}

Json::Value &addNXInstrument(Json::Value &entry, const std::string &name) {
  return addNX(entry, name, "NXinstrument");
}
Json::Value &addNXMonitor(Json::Value &entry, const std::string &name) {
  return addNX(entry, name, "NXmonitor");
}
void addNXMonitorName(Json::Value &monitor, const std::string &name) {
  auto monitorName = createEmptyDataset("name", "string");
  monitorName["values"] = name;
  appendToChildren(monitor, monitorName);
}

void addNXMonitorDetectorID(Json::Value &monitor, const int64_t detectorID) {
  auto monitorDetID = createEmptyDataset("detector_id", "int64");
  monitorDetID["values"] = detectorID;
  appendToChildren(monitor, monitorDetID);
}

void addNXMonitorEventStreamInfo(Json::Value &monitor, const std::string &topic,
                                 const std::string &source,
                                 const std::string &writerModule) {
  addStream(monitor, "events", topic, source, writerModule);
}

void addNXMonitorWaveformStreamInfo(Json::Value &monitor,
                                    const std::string &topic,
                                    const std::string &source,
                                    const std::string &writerModule) {
  addStream(monitor, "waveforms", topic, source, writerModule);
}

Json::Value &addNXChopper(Json::Value &instrument, const std::string &name) {
  return addNX(instrument, name, "NXdisk_chopper");
}

void addNXChopperName(Json::Value &chopper, const std::string &chopperName) {
  auto chopperFullName = createEmptyDataset("name", "string");
  chopperFullName["values"] = chopperName;
  appendToChildren(chopper, chopperFullName);
}

void addNXChopperRadius(Json::Value &chopper, const double radius) {
  auto chopperRadius = createEmptyDataset("radius", "double");
  chopperRadius["values"] = radius;
  chopperRadius["attributes"][0] = createAttribute("units", "mm");
  appendToChildren(chopper, chopperRadius);
}

void addNXChopperSlitEdges(Json::Value &chopper,
                           const std::vector<double> &edges) {
  addDataset(chopper, "slit_edges", {2}, edges, "units", "mm");
}

void addNXChopperSlitHeight(Json::Value &chopper, const double slitHeight) {
  auto chopperSlitHeight = createEmptyDataset("slit_height", "double");
  chopperSlitHeight["values"] = slitHeight;
  chopperSlitHeight["attributes"][0] = createAttribute("units", "mm");
  appendToChildren(chopper, chopperSlitHeight);
}

void addNXChopperSlits(Json::Value &chopper, const int64_t value) {
  auto chopperFullName = createEmptyDataset("slits", "int64");
  chopperFullName["values"] = value;
  appendToChildren(chopper, chopperFullName);
}

void addNXChopperTopDeadCenter(Json::Value &chopper, const std::string &topic,
                               const std::string &source,
                               const std::string &writerModule) {
  addStream(chopper, "top_dead_center", topic, source, writerModule);
}

Json::Value &addNXDetector(Json::Value &instrument, const std::string &name) {
  return addNX(instrument, name, "NXdetector");
}

void addNXTransformationDependency(Json::Value &nxDetector,
                                   const std::string &dependencyPath) {
  auto transDep = createEmptyDataset("depends_on", "string");
  transDep["values"] = dependencyPath;
  appendToChildren(nxDetector, transDep);
}

Json::Value &addNXTransformation(Json::Value &nxDetector,
                                 const std::string &name) {
  return addNX(nxDetector, name, "NXtransformations");
}

void addNXTransformationBeamDirectionOffset(Json::Value &nxTransformation,
                                            const std::vector<int> &shape,
                                            const std::vector<double> &values,
                                            const std::vector<double> vec) {
  addTransformationChild(
      nxTransformation, "beam_direction_offset", "translation",
      "/entry/instrument/detector_1/transformations/orientation", "m", shape,
      values, vec);
}

void addNXTransformationLocation(Json::Value &nxTransformation,
                                 const std::vector<int> &shape,
                                 const std::vector<double> &values,
                                 const std::vector<double> vec) {
  addTransformationChild(
      nxTransformation, "location", "translation",
      "/entry/instrument/detector_1/transformations/beam_direction_offset", "m",
      shape, values, vec);
}

void addNXTransformationOrientation(Json::Value &nxTransformation,
                                    const std::vector<int> &shape,
                                    const std::vector<double> &values,
                                    const std::vector<double> vec) {
  addTransformationChild(nxTransformation, "orientation", "translation", ".",
                         "degrees", shape, values, vec);
}

void addDetectorNumbers(Json::Value &nxDetector,
                        const std::vector<int32_t> &shape,
                        const std::vector<int64_t> &values) {
  addDataset(nxDetector, "detector_number", shape, values);
}

void addXPixelOffset(Json::Value &nxDetector, const std::vector<int32_t> &shape,
                     const std::vector<double> &values) {
  addDataset(nxDetector, "x_pixel_offset", shape, values, "units", "m");
}
void addYPixelOffset(Json::Value &nxDetector, const std::vector<int32_t> &shape,
                     const std::vector<double> &values) {
  addDataset(nxDetector, "y_pixel_offset", shape, values, "units", "m");
}

Json::Value &addOffPixelShape(Json::Value &nxDetector) {
  return addNX(nxDetector, "pixel_shape", "NXoff_geometry");
}

void addOffPixelShapeFaces(Json::Value &pixelShape,
                           const std::vector<int> &shape,
                           const std::vector<int> &indices) {
  addDataset(pixelShape, "faces", shape, indices, "", "");
}

void addOffPixelShapeVertices(Json::Value &pixelShape,
                              const std::vector<int> &shape,
                              const std::vector<double> &vertices) {
  addDataset(pixelShape, "vertices", shape, vertices, "units", "m");
}

void addOffPixelShapeWindingOrder(Json::Value &pixelShape,
                                  const std::vector<int> &shape,
                                  const std::vector<int> &windingOrder) {
  addDataset(pixelShape, "winding_order", shape, windingOrder, "", "");
}

Json::Value &addCylindricalPixelShape(Json::Value &nxDetector) {
  return addNX(nxDetector, "pixel_shape", "NXcylindrical_geometry");
}

void addCylindricalPixelShapeCylinders(Json::Value &pixelShape,
                                       const std::vector<int> &shape,
                                       const std::vector<int> &indices) {
  addDataset(pixelShape, "cylinders", shape, indices);
}

void addCylindricalPixelShapeVertices(Json::Value &pixelShape,
                                      const std::vector<int> &shape,
                                      const std::vector<double> &vertices) {
  addDataset(pixelShape, "vertices", shape, vertices, "units", "m");
}

const std::string convertToString(const Json::Value &value) {
  return value.toStyledString();
}

} // namespace JSONInstrumentBuilder

std::string getJSONGeometryNoSample() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  JSONInstrumentBuilder::addNXEntry(root, "entry");
  return JSONInstrumentBuilder::convertToString(root);
}

std::string getJSONGeometryNoInstrument() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  auto &entry = JSONInstrumentBuilder::addNXEntry(root, "entry");
  JSONInstrumentBuilder::addNXSample(entry, "sample");

  return JSONInstrumentBuilder::convertToString(root);
}

std::string getJSONGeometryNoDetectors() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  auto &entry = JSONInstrumentBuilder::addNXEntry(root, "entry");
  JSONInstrumentBuilder::addNXSample(entry, "sample");
  JSONInstrumentBuilder::addNXInstrument(entry, "instrument");

  return JSONInstrumentBuilder::convertToString(root);
}

std::string getJSONGeometryNoDetectorIDs() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  auto &entry = JSONInstrumentBuilder::addNXEntry(root, "entry");
  JSONInstrumentBuilder::addNXSample(entry, "sample");
  auto &instrument =
      JSONInstrumentBuilder::addNXInstrument(entry, "instrument");
  JSONInstrumentBuilder::addNXDetector(instrument, "detector_1");

  return JSONInstrumentBuilder::convertToString(root);
}

std::string getJSONGeometryNoXPixelOffset() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  auto &entry = JSONInstrumentBuilder::addNXEntry(root, "entry");
  JSONInstrumentBuilder::addNXSample(entry, "sample");
  auto &instrument =
      JSONInstrumentBuilder::addNXInstrument(entry, "instrument");
  auto &detectorBank =
      JSONInstrumentBuilder::addNXDetector(instrument, "detector_1");

  JSONInstrumentBuilder::addDetectorNumbers(detectorBank, {2, 2},
                                            std::vector<int64_t>{1, 2, 3, 4});

  return JSONInstrumentBuilder::convertToString(root);
}

std::string getJSONGeometryNoYPixelOffset() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  auto &entry = JSONInstrumentBuilder::addNXEntry(root, "entry");
  JSONInstrumentBuilder::addNXSample(entry, "sample");
  auto &instrument =
      JSONInstrumentBuilder::addNXInstrument(entry, "instrument");
  auto &detectorBank =
      JSONInstrumentBuilder::addNXDetector(instrument, "detector_1");

  JSONInstrumentBuilder::addDetectorNumbers(detectorBank, {2, 2},
                                            std::vector<int64_t>{1, 2, 3, 4});
  JSONInstrumentBuilder::addXPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.297, -0.299, -0.297});

  return JSONInstrumentBuilder::convertToString(root);
}

std::string getJSONGeometryNoPixelShape() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  auto &entry = JSONInstrumentBuilder::addNXEntry(root, "entry");
  JSONInstrumentBuilder::addNXSample(entry, "sample");
  auto &instrument =
      JSONInstrumentBuilder::addNXInstrument(entry, "instrument");
  auto &detectorBank =
      JSONInstrumentBuilder::addNXDetector(instrument, "detector_1");

  JSONInstrumentBuilder::addDetectorNumbers(detectorBank, {2, 2},
                                            std::vector<int64_t>{1, 2, 3, 4});
  JSONInstrumentBuilder::addXPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.297, -0.299, -0.297});
  JSONInstrumentBuilder::addYPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.299, -0.297, -0.297});
  return JSONInstrumentBuilder::convertToString(root);
}

std::string getJSONGeometryEmptyOffGeometry() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  auto &entry = JSONInstrumentBuilder::addNXEntry(root, "entry");
  JSONInstrumentBuilder::addNXSample(entry, "sample");
  auto &instrument =
      JSONInstrumentBuilder::addNXInstrument(entry, "instrument");
  auto &detectorBank =
      JSONInstrumentBuilder::addNXDetector(instrument, "detector_1");

  JSONInstrumentBuilder::addDetectorNumbers(detectorBank, {2, 2},
                                            std::vector<int64_t>{1, 2, 3, 4});
  JSONInstrumentBuilder::addXPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.297, -0.299, -0.297});
  JSONInstrumentBuilder::addYPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.299, -0.297, -0.297});
  JSONInstrumentBuilder::addOffPixelShape(detectorBank);
  return JSONInstrumentBuilder::convertToString(root);
}

std::string getJSONGeometryInvalidOffGeometry() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  auto &entry = JSONInstrumentBuilder::addNXEntry(root, "entry");
  JSONInstrumentBuilder::addNXSample(entry, "sample");
  auto &instrument =
      JSONInstrumentBuilder::addNXInstrument(entry, "instrument");
  auto &detectorBank =
      JSONInstrumentBuilder::addNXDetector(instrument, "detector_1");

  JSONInstrumentBuilder::addDetectorNumbers(detectorBank, {2, 2},
                                            std::vector<int64_t>{1, 2, 3, 4});
  JSONInstrumentBuilder::addXPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.297, -0.299, -0.297});
  JSONInstrumentBuilder::addYPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.299, -0.297, -0.297});
  auto &pixelShape = JSONInstrumentBuilder::addOffPixelShape(detectorBank);
  JSONInstrumentBuilder::addOffPixelShapeFaces(pixelShape);
  JSONInstrumentBuilder::addOffPixelShapeVertices(pixelShape);
  JSONInstrumentBuilder::addOffPixelShapeWindingOrder(pixelShape, {3},
                                                      {0, 1, 2}); // invalid

  return JSONInstrumentBuilder::convertToString(root);
}

std::string getJSONGeometryEmptyCylindricalGeometry() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  auto &entry = JSONInstrumentBuilder::addNXEntry(root, "entry");
  JSONInstrumentBuilder::addNXSample(entry, "sample");
  auto &instrument =
      JSONInstrumentBuilder::addNXInstrument(entry, "instrument");
  auto &detectorBank =
      JSONInstrumentBuilder::addNXDetector(instrument, "detector_1");

  JSONInstrumentBuilder::addDetectorNumbers(detectorBank, {2, 2},
                                            std::vector<int64_t>{1, 2, 3, 4});
  JSONInstrumentBuilder::addXPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.297, -0.299, -0.297});
  JSONInstrumentBuilder::addYPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.299, -0.297, -0.297});
  JSONInstrumentBuilder::addCylindricalPixelShape(detectorBank);
  return JSONInstrumentBuilder::convertToString(root);
}

std::string getJSONGeometryInvalidCylindricalGeometry() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  auto &entry = JSONInstrumentBuilder::addNXEntry(root, "entry");
  JSONInstrumentBuilder::addNXSample(entry, "sample");
  auto &instrument =
      JSONInstrumentBuilder::addNXInstrument(entry, "instrument");
  auto &detectorBank =
      JSONInstrumentBuilder::addNXDetector(instrument, "detector_1");

  JSONInstrumentBuilder::addDetectorNumbers(detectorBank, {2, 2},
                                            std::vector<int64_t>{1, 2, 3, 4});
  JSONInstrumentBuilder::addXPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.297, -0.299, -0.297});
  JSONInstrumentBuilder::addYPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.299, -0.297, -0.297});
  auto &pixelShape =
      JSONInstrumentBuilder::addCylindricalPixelShape(detectorBank);
  JSONInstrumentBuilder::addCylindricalPixelShapeCylinders(pixelShape);
  // invalid
  JSONInstrumentBuilder::addCylindricalPixelShapeVertices(
      pixelShape, {3, 2}, {-0.001, 0, -0.001, 0.0045, 0.001, 0});

  return JSONInstrumentBuilder::convertToString(root);
}

std::string getJSONGeometryMissingTransformations() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  auto &entry = JSONInstrumentBuilder::addNXEntry(root, "entry");
  JSONInstrumentBuilder::addNXSample(entry, "sample");
  auto &instrument =
      JSONInstrumentBuilder::addNXInstrument(entry, "instrument");
  auto &detectorBank =
      JSONInstrumentBuilder::addNXDetector(instrument, "detector_1");

  JSONInstrumentBuilder::addDetectorNumbers(detectorBank, {2, 2},
                                            std::vector<int64_t>{1, 2, 3, 4});
  JSONInstrumentBuilder::addXPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.297, -0.299, -0.297});
  JSONInstrumentBuilder::addYPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.299, -0.297, -0.297});
  auto &pixelShape = JSONInstrumentBuilder::addOffPixelShape(detectorBank);
  JSONInstrumentBuilder::addOffPixelShapeFaces(pixelShape);
  JSONInstrumentBuilder::addOffPixelShapeVertices(pixelShape);
  JSONInstrumentBuilder::addOffPixelShapeWindingOrder(pixelShape);

  // Add dependency but no transformations
  JSONInstrumentBuilder::addNXTransformationDependency(
      detectorBank, "/entry/instrument/detector_1/transformations/location");

  return JSONInstrumentBuilder::convertToString(root);
}

std::string getJSONGeometryMissingBeamDirectionOffset() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  auto &entry = JSONInstrumentBuilder::addNXEntry(root, "entry");
  JSONInstrumentBuilder::addNXSample(entry, "sample");
  auto &instrument =
      JSONInstrumentBuilder::addNXInstrument(entry, "instrument");
  auto &detectorBank =
      JSONInstrumentBuilder::addNXDetector(instrument, "detector_1");

  JSONInstrumentBuilder::addDetectorNumbers(detectorBank, {2, 2},
                                            std::vector<int64_t>{1, 2, 3, 4});
  JSONInstrumentBuilder::addXPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.297, -0.299, -0.297});
  JSONInstrumentBuilder::addYPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.299, -0.297, -0.297});
  auto &pixelShape = JSONInstrumentBuilder::addOffPixelShape(detectorBank);
  JSONInstrumentBuilder::addOffPixelShapeFaces(pixelShape);
  JSONInstrumentBuilder::addOffPixelShapeVertices(pixelShape);
  JSONInstrumentBuilder::addOffPixelShapeWindingOrder(pixelShape);

  // Add dependency but no transformations
  JSONInstrumentBuilder::addNXTransformationDependency(
      detectorBank, "/entry/instrument/detector_1/transformations/location");

  auto &transformation = JSONInstrumentBuilder::addNXTransformation(
      detectorBank, "transformations");
  JSONInstrumentBuilder::addNXTransformationLocation(transformation);
  return JSONInstrumentBuilder::convertToString(root);
}

std::string getJSONGeometryMissingOrientation() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  auto &entry = JSONInstrumentBuilder::addNXEntry(root, "entry");
  JSONInstrumentBuilder::addNXSample(entry, "sample");
  auto &instrument =
      JSONInstrumentBuilder::addNXInstrument(entry, "instrument");
  auto &detectorBank =
      JSONInstrumentBuilder::addNXDetector(instrument, "detector_1");

  JSONInstrumentBuilder::addDetectorNumbers(detectorBank, {2, 2},
                                            std::vector<int64_t>{1, 2, 3, 4});
  JSONInstrumentBuilder::addXPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.297, -0.299, -0.297});
  JSONInstrumentBuilder::addYPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.299, -0.297, -0.297});
  auto &pixelShape = JSONInstrumentBuilder::addOffPixelShape(detectorBank);
  JSONInstrumentBuilder::addOffPixelShapeFaces(pixelShape);
  JSONInstrumentBuilder::addOffPixelShapeVertices(pixelShape);
  JSONInstrumentBuilder::addOffPixelShapeWindingOrder(pixelShape);

  // Add dependency but no transformations
  JSONInstrumentBuilder::addNXTransformationDependency(
      detectorBank, "/entry/instrument/detector_1/transformations/location");

  auto &transformation = JSONInstrumentBuilder::addNXTransformation(
      detectorBank, "transformations");
  JSONInstrumentBuilder::addNXTransformationLocation(transformation);
  JSONInstrumentBuilder::addNXTransformationBeamDirectionOffset(transformation);
  return JSONInstrumentBuilder::convertToString(root);
}

std::string getJSONGeometryMissingMonitorInformation() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  auto &entry = JSONInstrumentBuilder::addNXEntry(root, "entry");
  JSONInstrumentBuilder::addNXSample(entry, "sample");
  auto &instrument =
      JSONInstrumentBuilder::addNXInstrument(entry, "instrument");
  auto &detectorBank =
      JSONInstrumentBuilder::addNXDetector(instrument, "detector_1");

  JSONInstrumentBuilder::addNXMonitor(instrument, "monitor_1");
  JSONInstrumentBuilder::addDetectorNumbers(detectorBank, {2, 2},
                                            std::vector<int64_t>{1, 2, 3, 4});
  JSONInstrumentBuilder::addXPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.297, -0.299, -0.297});
  JSONInstrumentBuilder::addYPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.299, -0.297, -0.297});
  auto &pixelShape = JSONInstrumentBuilder::addOffPixelShape(detectorBank);
  JSONInstrumentBuilder::addOffPixelShapeFaces(pixelShape);
  JSONInstrumentBuilder::addOffPixelShapeVertices(pixelShape);
  JSONInstrumentBuilder::addOffPixelShapeWindingOrder(pixelShape);

  // Add dependency but no transformations
  JSONInstrumentBuilder::addNXTransformationDependency(
      detectorBank, "/entry/instrument/detector_1/transformations/location");

  auto &transformation = JSONInstrumentBuilder::addNXTransformation(
      detectorBank, "transformations");
  JSONInstrumentBuilder::addNXTransformationLocation(transformation);
  JSONInstrumentBuilder::addNXTransformationBeamDirectionOffset(transformation);
  JSONInstrumentBuilder::addNXTransformationOrientation(transformation);
  return JSONInstrumentBuilder::convertToString(root);
}

std::string getJSONGeometryMissingChopperInformation() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  auto &entry = JSONInstrumentBuilder::addNXEntry(root, "entry");
  JSONInstrumentBuilder::addNXSample(entry, "sample");
  auto &instrument =
      JSONInstrumentBuilder::addNXInstrument(entry, "instrument");
  auto &detectorBank =
      JSONInstrumentBuilder::addNXDetector(instrument, "detector_1");

  JSONInstrumentBuilder::addNXChopper(instrument, "chopper_1");
  JSONInstrumentBuilder::addDetectorNumbers(detectorBank, {2, 2},
                                            std::vector<int64_t>{1, 2, 3, 4});
  JSONInstrumentBuilder::addXPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.297, -0.299, -0.297});
  JSONInstrumentBuilder::addYPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.299, -0.297, -0.297});
  auto &pixelShape = JSONInstrumentBuilder::addOffPixelShape(detectorBank);
  JSONInstrumentBuilder::addOffPixelShapeFaces(pixelShape);
  JSONInstrumentBuilder::addOffPixelShapeVertices(pixelShape);
  JSONInstrumentBuilder::addOffPixelShapeWindingOrder(pixelShape);

  // Add dependency but no transformations
  JSONInstrumentBuilder::addNXTransformationDependency(
      detectorBank, "/entry/instrument/detector_1/transformations/location");

  auto &transformation = JSONInstrumentBuilder::addNXTransformation(
      detectorBank, "transformations");
  JSONInstrumentBuilder::addNXTransformationLocation(transformation);
  JSONInstrumentBuilder::addNXTransformationBeamDirectionOffset(transformation);
  JSONInstrumentBuilder::addNXTransformationOrientation(transformation);
  return JSONInstrumentBuilder::convertToString(root);
}

std::string getFullJSONInstrumentSimpleOFF() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  auto &entry = JSONInstrumentBuilder::addNXEntry(root, "entry");
  JSONInstrumentBuilder::addNXSample(entry, "sample");
  auto &instrument =
      JSONInstrumentBuilder::addNXInstrument(entry, "instrument");
  auto &detectorBank =
      JSONInstrumentBuilder::addNXDetector(instrument, "detector_1");

  JSONInstrumentBuilder::addDetectorNumbers(detectorBank, {2, 2},
                                            std::vector<int64_t>{1, 2, 3, 4});
  JSONInstrumentBuilder::addXPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.297, -0.299, -0.297});
  JSONInstrumentBuilder::addYPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.299, -0.297, -0.297});
  auto &pixelShape = JSONInstrumentBuilder::addOffPixelShape(detectorBank);
  JSONInstrumentBuilder::addOffPixelShapeFaces(pixelShape);
  JSONInstrumentBuilder::addOffPixelShapeVertices(pixelShape);
  JSONInstrumentBuilder::addOffPixelShapeWindingOrder(pixelShape);

  // Add dependency but no transformations
  JSONInstrumentBuilder::addNXTransformationDependency(
      detectorBank, "/entry/instrument/detector_1/transformations/location");

  auto &transformation = JSONInstrumentBuilder::addNXTransformation(
      detectorBank, "transformations");
  JSONInstrumentBuilder::addNXTransformationLocation(transformation);
  JSONInstrumentBuilder::addNXTransformationBeamDirectionOffset(transformation);
  JSONInstrumentBuilder::addNXTransformationOrientation(transformation);
  return JSONInstrumentBuilder::convertToString(root);
}

std::string getFullJSONInstrumentSimpleCylindrical() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  auto &entry = JSONInstrumentBuilder::addNXEntry(root, "entry");
  JSONInstrumentBuilder::addNXSample(entry, "sample");
  auto &instrument =
      JSONInstrumentBuilder::addNXInstrument(entry, "instrument");
  auto &detectorBank =
      JSONInstrumentBuilder::addNXDetector(instrument, "detector_1");

  JSONInstrumentBuilder::addDetectorNumbers(detectorBank, {2, 2},
                                            std::vector<int64_t>{1, 2, 3, 4});
  JSONInstrumentBuilder::addXPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.297, -0.299, -0.297});
  JSONInstrumentBuilder::addYPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.299, -0.297, -0.297});
  auto &pixelShape =
      JSONInstrumentBuilder::addCylindricalPixelShape(detectorBank);
  JSONInstrumentBuilder::addCylindricalPixelShapeCylinders(pixelShape);
  JSONInstrumentBuilder::addCylindricalPixelShapeVertices(pixelShape);

  // Add dependency but no transformations
  JSONInstrumentBuilder::addNXTransformationDependency(
      detectorBank, "/entry/instrument/detector_1/transformations/location");

  auto &transformation = JSONInstrumentBuilder::addNXTransformation(
      detectorBank, "transformations");
  JSONInstrumentBuilder::addNXTransformationLocation(transformation);
  JSONInstrumentBuilder::addNXTransformationBeamDirectionOffset(transformation);
  JSONInstrumentBuilder::addNXTransformationOrientation(transformation);
  return JSONInstrumentBuilder::convertToString(root);
}

std::string getFullJSONInstrumentSimpleWithChopper() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  auto &entry = JSONInstrumentBuilder::addNXEntry(root, "entry");
  JSONInstrumentBuilder::addNXSample(entry, "sample");
  auto &instrument =
      JSONInstrumentBuilder::addNXInstrument(entry, "instrument");

  auto &chopper = JSONInstrumentBuilder::addNXChopper(instrument, "chopper_1");
  JSONInstrumentBuilder::addNXChopperName(
      chopper, "Airbus, Source Chopper, ESS Pulse, Disc 1");
  JSONInstrumentBuilder::addNXChopperRadius(chopper);
  JSONInstrumentBuilder::addNXChopperSlitEdges(chopper);
  JSONInstrumentBuilder::addNXChopperSlitHeight(chopper);
  JSONInstrumentBuilder::addNXChopperSlits(chopper, 1);
  JSONInstrumentBuilder::addNXChopperTopDeadCenter(
      chopper, "V20_choppers", "HZB-V20:Chop-Drv-0401:TDC_array", "senv");

  auto &detectorBank =
      JSONInstrumentBuilder::addNXDetector(instrument, "detector_1");

  JSONInstrumentBuilder::addDetectorNumbers(detectorBank, {2, 2},
                                            std::vector<int64_t>{1, 2, 3, 4});
  JSONInstrumentBuilder::addXPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.297, -0.299, -0.297});
  JSONInstrumentBuilder::addYPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.299, -0.297, -0.297});
  auto &pixelShape =
      JSONInstrumentBuilder::addCylindricalPixelShape(detectorBank);
  JSONInstrumentBuilder::addCylindricalPixelShapeCylinders(pixelShape);
  JSONInstrumentBuilder::addCylindricalPixelShapeVertices(pixelShape);

  // Add dependency but no transformations
  JSONInstrumentBuilder::addNXTransformationDependency(
      detectorBank, "/entry/instrument/detector_1/transformations/location");

  auto &transformation = JSONInstrumentBuilder::addNXTransformation(
      detectorBank, "transformations");
  JSONInstrumentBuilder::addNXTransformationLocation(transformation);
  JSONInstrumentBuilder::addNXTransformationBeamDirectionOffset(transformation);
  JSONInstrumentBuilder::addNXTransformationOrientation(transformation);
  return JSONInstrumentBuilder::convertToString(root);
}

std::string getFullJSONInstrumentSimpleWithMonitor() {
  Json::Value root;
  JSONInstrumentBuilder::initialiseRoot(root, "nexus_structure");
  auto &entry = JSONInstrumentBuilder::addNXEntry(root, "entry");
  JSONInstrumentBuilder::addNXSample(entry, "sample");
  auto &instrument =
      JSONInstrumentBuilder::addNXInstrument(entry, "instrument");

  auto &monitor = JSONInstrumentBuilder::addNXMonitor(instrument, "monitor_1");
  JSONInstrumentBuilder::addNXMonitorName(monitor, "Helium-3 monitor");
  JSONInstrumentBuilder::addNXMonitorDetectorID(monitor, 90000);
  JSONInstrumentBuilder::addNXMonitorEventStreamInfo(
      monitor, "monitor", "Monitor_Adc0_Ch1", "ev42");
  JSONInstrumentBuilder::addNXMonitorWaveformStreamInfo(
      monitor, "monitor", "Monitor_Adc0_Ch1", "senv");

  auto &monitorTransformation =
      JSONInstrumentBuilder::addNXTransformation(monitor, "transformations");
  JSONInstrumentBuilder::addNXTransformationLocation(monitorTransformation, {1},
                                                     {-3.298}, {0, 0, 1});
  JSONInstrumentBuilder::addNXTransformationOrientation(monitorTransformation,
                                                        {1}, {45}, {0, 1, 0});
  JSONInstrumentBuilder::addNXTransformationDependency(
      monitor, "/entry/monitor_1/transformations/location");

  auto &detectorBank =
      JSONInstrumentBuilder::addNXDetector(instrument, "detector_1");

  JSONInstrumentBuilder::addDetectorNumbers(detectorBank, {2, 2},
                                            std::vector<int64_t>{1, 2, 3, 4});
  JSONInstrumentBuilder::addXPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.297, -0.299, -0.297});
  JSONInstrumentBuilder::addYPixelOffset(detectorBank, {2, 2},
                                         {-0.299, -0.299, -0.297, -0.297});
  auto &pixelShape =
      JSONInstrumentBuilder::addCylindricalPixelShape(detectorBank);
  JSONInstrumentBuilder::addCylindricalPixelShapeCylinders(pixelShape);
  JSONInstrumentBuilder::addCylindricalPixelShapeVertices(pixelShape);

  // Add dependency but no transformations
  JSONInstrumentBuilder::addNXTransformationDependency(
      detectorBank, "/entry/instrument/detector_1/transformations/location");

  auto &transformation = JSONInstrumentBuilder::addNXTransformation(
      detectorBank, "transformations");
  JSONInstrumentBuilder::addNXTransformationLocation(transformation);
  JSONInstrumentBuilder::addNXTransformationBeamDirectionOffset(transformation);
  JSONInstrumentBuilder::addNXTransformationOrientation(transformation);
  return JSONInstrumentBuilder::convertToString(root);
}

} // namespace TestHelpers
} // namespace Mantid