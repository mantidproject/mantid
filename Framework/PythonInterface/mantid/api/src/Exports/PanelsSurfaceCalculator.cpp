// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/PanelsSurfaceCalculator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/core/Converters/PyObjectToV3D.h"
#include <boost/python.hpp>
#include <boost/python/class.hpp>
#include <boost/python/list.hpp>

// boost::python generates this warning in gcc
GNU_DIAG_OFF("maybe-uninitialized")

using namespace boost::python;

namespace {
using Mantid::API::PanelsSurfaceCalculator;
using Mantid::PythonInterface::Converters::PyObjectToV3D;

void setupBasisAxes(const PanelsSurfaceCalculator &self, object &xAxis, object &yAxis, const object &zAxis) {
  auto x = PyObjectToV3D(xAxis)();
  auto y = PyObjectToV3D(yAxis)();
  const auto z = PyObjectToV3D(zAxis)();
  self.setupBasisAxes(z, x, y);
  for (size_t i = 0; i < 3; i++) {
    xAxis[i] = x[i];
    yAxis[i] = y[i];
  }
}

list retrievePanelCorners(const PanelsSurfaceCalculator &self, const object &componentInfo, const size_t rootIndex) {
  const std::shared_ptr<ComponentInfo> cInfoSharedPtr = extract<std::shared_ptr<ComponentInfo>>(componentInfo);
  const auto panelCorners = self.retrievePanelCorners(*(cInfoSharedPtr.get()), rootIndex);
  list panelCornersList;
  for (size_t i = 0; i < panelCorners.size(); i++) {
    list corner(panelCorners[i]);
    panelCornersList.append(corner);
  }
  return panelCornersList;
}

list calculatePanelNormal(const PanelsSurfaceCalculator &self, const object &panelCorners) {
  if (len(panelCorners) != 4) {
    throw std::invalid_argument("Must be 4 panel corners");
  }
  std::vector<V3D> panelCornersVec{4};
  for (size_t i = 0; i < 4; i++) {
    const object corner = panelCorners[i];
    panelCornersVec[i] = PyObjectToV3D(panelCorners[i])();
  }
  const auto panelNormal = self.calculatePanelNormal(panelCornersVec);
  list panelNormalList(panelNormal);
  return panelNormalList;
}

bool isBankFlat(PanelsSurfaceCalculator &self, const object &componentInfo, const size_t bankIndex, const list &tubes,
                const object &normal) {
  const std::shared_ptr<ComponentInfo> cInfoSharedPtr = extract<std::shared_ptr<ComponentInfo>>(componentInfo);
  std::vector<size_t> tubesVector;
  const size_t lenTubes = len(tubes);
  for (size_t i = 0; i < lenTubes; i++) {
    tubesVector.push_back(extract<size_t>(tubes[i]));
  }
  const auto normalV3D = PyObjectToV3D(normal)();
  return self.isBankFlat(*(cInfoSharedPtr.get()), bankIndex, tubesVector, normalV3D);
}

list calculateBankNormal(PanelsSurfaceCalculator &self, const object &componentInfo, const list &tubes) {
  const std::shared_ptr<ComponentInfo> cInfoSharedPtr = extract<std::shared_ptr<ComponentInfo>>(componentInfo);
  std::vector<size_t> tubesVector;
  const size_t lenTubes = len(tubes);
  for (size_t i = 0; i < lenTubes; i++) {
    tubesVector.push_back(extract<size_t>(tubes[i]));
  }
  const auto normal = self.calculateBankNormal(*(cInfoSharedPtr.get()), tubesVector);
  list normalList{normal};
  return normalList;
}

void setBankVisited(const PanelsSurfaceCalculator &self, const object &componentInfo, const size_t bankIndex,
                    list &visitedComponents) {
  const std::shared_ptr<ComponentInfo> cInfoSharedPtr = extract<std::shared_ptr<ComponentInfo>>(componentInfo);
  std::vector<bool> visitedComponentsVector;
  const size_t lenVisitedComponents = len(visitedComponents);
  for (size_t i = 0; i < lenVisitedComponents; i++) {
    visitedComponentsVector.push_back(extract<bool>(visitedComponents[i]));
  }
  self.setBankVisited(*(cInfoSharedPtr.get()), bankIndex, visitedComponentsVector);
  for (size_t i = 0; i < lenVisitedComponents; i++) {
    // A bool is 8 bytes because a byte is the smallest addressable unit of memory, but
    // an std::vector<bool> uses bits to store bools as a memory optimisation. Hence
    // the objects in a vector<bool> are not actually bools, and boost::python
    // doesn't know what to do with them
    visitedComponents[i] = visitedComponentsVector[i] ? true : false;
  }
}

size_t findNumDetectors(const PanelsSurfaceCalculator &self, const object &componentInfo, const list &components) {
  const std::shared_ptr<ComponentInfo> cInfoSharedPtr = extract<std::shared_ptr<ComponentInfo>>(componentInfo);
  std::vector<size_t> componentsVector;
  const size_t lenComponents = len(components);
  for (size_t i = 0; i < lenComponents; i++) {
    componentsVector.push_back(extract<size_t>(components[i]));
  }
  return self.findNumDetectors(*(cInfoSharedPtr.get()), componentsVector);
}

list calcBankRotation(const PanelsSurfaceCalculator &self, const object &detPos, object normal, const object &zAxis,
                      const object &yAxis, const object &samplePosition) {
  const auto bankRotation =
      self.calcBankRotation(PyObjectToV3D(detPos)(), PyObjectToV3D(normal)(), PyObjectToV3D(zAxis)(),
                            PyObjectToV3D(yAxis)(), PyObjectToV3D(samplePosition)());

  list quat;
  quat.append(bankRotation.real());
  quat.append(bankRotation.imagI());
  quat.append(bankRotation.imagJ());
  quat.append(bankRotation.imagK());
  return quat;
}

list transformedBoundingBoxPoints(const PanelsSurfaceCalculator &self, const object &componentInfo,
                                  size_t detectorIndex, const object &refPos, const list &rotation, const object &xaxis,
                                  const object &yaxis) {
  const std::shared_ptr<ComponentInfo> cInfoSharedPtr = extract<std::shared_ptr<ComponentInfo>>(componentInfo);
  Mantid::Kernel::Quat quatRotation{extract<double>(rotation[0]), extract<double>(rotation[1]),
                                    extract<double>(rotation[2]), extract<double>(rotation[3])};
  const auto referencePosition = PyObjectToV3D(refPos)();
  const auto xAxisVec = PyObjectToV3D(xaxis)();
  const auto yAxisVec = PyObjectToV3D(yaxis)();
  const auto boundingBoxPoints = self.transformedBoundingBoxPoints(*(cInfoSharedPtr.get()), detectorIndex,
                                                                   referencePosition, quatRotation, xAxisVec, yAxisVec);
  list pointA, pointB;
  pointA.append(boundingBoxPoints[0].X());
  pointA.append(boundingBoxPoints[0].Y());
  pointB.append(boundingBoxPoints[1].X());
  pointB.append(boundingBoxPoints[1].Y());
  list result;
  result.append(pointA);
  result.append(pointB);
  return result;
}

list getAllTubeDetectorFlatGroupParents(PanelsSurfaceCalculator &self, const object &componentInfo) {
  const std::shared_ptr<ComponentInfo> cInfoSharedPtr = extract<std::shared_ptr<ComponentInfo>>(componentInfo);
  const auto allTubeGroupParents =
      self.examineAllComponents(*(cInfoSharedPtr.get()), [&](const auto &cinfo, auto root, auto &visited) {
        return self.tubeDetectorParentIDs(cinfo, root, visited);
      });
  list pyAllTubeGroupParents;
  for (size_t groupIndex = 0; groupIndex < allTubeGroupParents.size(); groupIndex++) {
    const auto tubeGroupParents = allTubeGroupParents[groupIndex];
    list pyTubeGroupParents;
    for (size_t tubeParentIndex = 0; tubeParentIndex < tubeGroupParents.size(); tubeParentIndex++) {
      pyTubeGroupParents.append(tubeGroupParents[tubeParentIndex]);
    }
    pyAllTubeGroupParents.append(pyTubeGroupParents);
  }
  return pyAllTubeGroupParents;
}

tuple getSideBySideViewPos(const PanelsSurfaceCalculator &self, const object &componentInfo, const object &instrument,
                           const size_t componentIndex) {
  const std::shared_ptr<ComponentInfo> cInfoSharedPtr = extract<std::shared_ptr<ComponentInfo>>(componentInfo);
  const std::shared_ptr<Instrument> instrumentSharedPtr = extract<std::shared_ptr<Instrument>>(instrument);
  const auto sideBySidePos = self.getSideBySideViewPos(*(cInfoSharedPtr).get(), instrumentSharedPtr, componentIndex);
  list position;
  list result;
  if (!sideBySidePos.has_value()) {
    position.append(0);
    position.append(0);
    result.append(false);
  } else {
    position.append(sideBySidePos->X());
    position.append(sideBySidePos->Y());
    result.append(true);
  }
  result.append(position);
  return tuple(result);
}

} // namespace

void export_PanelsSurfaceCalculator() {
  using namespace boost::python;
  using Mantid::API::PanelsSurfaceCalculator;

  class_<PanelsSurfaceCalculator, boost::noncopyable>("PanelsSurfaceCalculator",
                                                      init<>("Make a side by side projection calculator"))
      .def("setupBasisAxes", &setupBasisAxes, (arg("self"), arg("xaxis"), arg("yaxis"), arg("zaxis")),
           "Sets up the basis axes for the projection.")
      .def("retrievePanelCorners", &retrievePanelCorners, (arg("self"), arg("componentInfo"), arg("rootIndex")),
           "Retrieves the corners of the panel.")
      .def("calculatePanelNormal", &calculatePanelNormal, (arg("self"), arg("panelCorners")),
           "Calculates the normal vector of the panel.")
      .def("isBankFlat", &isBankFlat,
           (arg("self"), arg("componentInfo"), arg("bankIndex"), arg("tubes"), arg("normal")),
           "Checks if a bank is flat based on its normal vector.")
      .def("calculateBankNormal", &calculateBankNormal, (arg("self"), arg("componentInfo"), arg("tubes")),
           "Calculates the normal vector of a bank.")
      .def("setBankVisited", &setBankVisited,
           (arg("self"), arg("componentInfo"), arg("bankIndex"), arg("visitedComponents")),
           "Marks a bank as visited in the visitedComponents vector")
      .def("findNumDetectors", &findNumDetectors, (arg("componentInfo"), arg("components")),
           "Finds the number of detectors in a component.")
      .def("calcBankRotation", &calcBankRotation,
           (arg("self"), arg("detPos"), arg("normal"), arg("zAxis"), arg("yAxis"), arg("samplePosition")),
           "Calculates the rotation quaternion for a bank based on its position and normal vector.")
      .def("transformedBoundingBoxPoints", &transformedBoundingBoxPoints,
           (arg("self"), arg("componentInfo"), arg("detectorIndex"), arg("refPos"), arg("rotation"), arg("xaxis"),
            arg("yaxis")),
           "Transforms a component's bounding box based on reference position and rotation. The rotation should be "
           "provided as a list containing the real and imaginary parts of a quaternion (w, i, j, k).")
      .def("getAllTubeDetectorFlatGroupParents", &getAllTubeDetectorFlatGroupParents,
           (arg("self"), arg("componentInfo")),
           "Returns the parent component indices of detectors of all groups of tubes arranged in flat banks")
      .def("getSideBySideViewPos", &getSideBySideViewPos,
           (arg("self"), arg("componentInfo"), arg("instrument"), arg("componentIndex")),
           "Returns a tuple indicating whether the bank side-by-side projection position has been specified in the "
           "IDF, and what it is.");
}
