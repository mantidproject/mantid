// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_TESTHELPERS_JSONGEOMETRYPARSERTESTHELPER_H_
#define MANTID_TESTHELPERS_JSONGEOMETRYPARSERTESTHELPER_H_

#include <string>
#include <vector>

namespace Json {
class Value;
}

namespace Mantid {

namespace Kernel {
class V3D;
}

namespace TestHelpers {

namespace JSONInstrumentBuilder {
void initialiseRoot(Json::Value &root, const std::string &name);
Json::Value &addNXEntry(Json::Value &root, const std::string &name);
Json::Value &addNXSample(Json::Value &entry, const std::string &name);
Json::Value &addNXInstrument(Json::Value &entry, const std::string &name);
Json::Value &addNXMonitor(Json::Value &entry, const std::string &name);
void addNXMonitorName(Json::Value &monitor, const std::string &name);
void addNXMonitorDetectorID(Json::Value &monitor, const int64_t detectorID);
void addNXMonitorEventStreamInfo(Json::Value &monitor, const std::string &topic,
                                 const std::string &source,
                                 const std::string &writerModule);
void addNXMonitorWaveformStreamInfo(Json::Value &monitor,
                                    const std::string &topic,
                                    const std::string &source,
                                    const std::string &writerModule);

Json::Value &addNXChopper(Json::Value &instrument,
                                   const std::string &name);
void addNXChopperName(Json::Value &chopper, const std::string &chopperName);
void addNXChopperRadius(Json::Value &chopper, const double radius = 350);
void addNXChopperSlitEdges(Json::Value &chopper,
                           const std::vector<double> &edges = {0.0, 23.0});
void addNXChopperSlitHeight(Json::Value &chopper,
                            const double slitHeight = 150);
void addNXChopperSlits(Json::Value &chopper, const int64_t value);
void addNXChopperTopDeadCenter(Json::Value &chopper, const std::string &topic,
                               const std::string &source,
                               const std::string &writerModule);

Json::Value &addNXDetector(Json::Value &instrument, const std::string &name);
void addNXTransformationDependency(Json::Value &nxDetector,
                                   const std::string &dependencyPath);
Json::Value &addNXTransformation(Json::Value &nxDetector,
                                 const std::string &name);
void addNXTransformationBeamDirectionOffset(
    Json::Value &nxTransformation, const std::vector<int> &shape = {1},
    const std::vector<double> &values = {0.049},
    const std::vector<double> vec = {0, 0, -1});

void addNXTransformationLocation(Json::Value &nxTransformation,
                                 const std::vector<int> &shape = {1},
                                 const std::vector<double> &values = {0.971},
                                 const std::vector<double> vec = {1, 0, 0});

void addNXTransformationOrientation(Json::Value &nxTransformation,
                                    const std::vector<int> &shape = {1},
                                    const std::vector<double> &values = {90},
                                    const std::vector<double> vec = {0, 1, 0});

void addDetectorNumbers(Json::Value &nxDetector, const std::vector<int> &shape,
                        const std::vector<int64_t> &values);
void addXPixelOffset(Json::Value &nxDetector, const std::vector<int> &shape,
                     const std::vector<double> &values);
void addYPixelOffset(Json::Value &nxDetector, const std::vector<int> &shape,
                     const std::vector<double> &values);

Json::Value &addOffPixelShape(Json::Value &nxDetector);
void addOffPixelShapeFaces(Json::Value &pixelShape,
                           const std::vector<int> &shape = {1},
                           const std::vector<int> &faces = {0});
void addOffPixelShapeVertices(Json::Value &pixelShape,
                              const std::vector<int> &shape = {4, 3},
                              const std::vector<double> &vertices = {
                                  -0.001, -0.001, 0, 0.001, -0.001, 0, 0.001,
                                  0.001, 0, -0.001, 0.001, 0});
void addOffPixelShapeWindingOrder(Json::Value &pixelShape,
                                  const std::vector<int> &shape = {4},
                                  const std::vector<int> &windingOrder = {
                                      0, 1, 2, 3});

Json::Value &addCylindricalPixelShape(Json::Value &nxDetector);
void addCylindricalPixelShapeCylinders(Json::Value &pixelShape,
                                       const std::vector<int> &shape = {3},
                                       const std::vector<int> &indices = {0, 1,
                                                                          2});
void addCylindricalPixelShapeVertices(Json::Value &pixelShape,
                                      const std::vector<int> &shape = {3, 3},
                                      const std::vector<double> &vertices = {
                                          -0.001, 0, 0, 0.001, 0.00405, 0,
                                          0.001, 0, 0});

const std::string convertToString(const Json::Value &value);
} // namespace JSONInstrumentBuilder

std::string getJSONGeometryNoSample();
std::string getJSONGeometryNoInstrument();
std::string getJSONGeometryNoDetectors();
std::string getJSONGeometryNoDetectorIDs();
std::string getJSONGeometryNoXPixelOffset();
std::string getJSONGeometryNoYPixelOffset();
std::string getJSONGeometryNoPixelShape();
std::string getJSONGeometryEmptyOffGeometry();
std::string getJSONGeometryInvalidOffGeometry();
std::string getJSONGeometryEmptyCylindricalGeometry();
std::string getJSONGeometryInvalidCylindricalGeometry();
std::string getJSONGeometryMissingTransformations();
std::string getJSONGeometryMissingBeamDirectionOffset();
std::string getJSONGeometryMissingOrientation();
std::string getJSONGeometryMissingMonitorInformation();
std::string getJSONGeometryMissingChopperInformation();
std::string getFullJSONInstrumentSimpleOFF();
std::string getFullJSONInstrumentSimpleCylindrical();
std::string getFullJSONInstrumentSimpleWithChopper();
std::string getFullJSONInstrumentSimpleWithMonitor();

} // namespace TestHelpers
} // namespace Mantid

#endif // MANTID_TESTHELPERS_JSONGEOMETRYPARSERTESTHELPER_H_