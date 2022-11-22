// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "ALFAnalysisModel.h"
#include "ALFAnalysisPresenter.h"
#include "ALFAnalysisView.h"

#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"

#include <string>
#include <utility>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace MantidQt::MantidWidgets;

class MockDetector : public IDetector {
public:
  // IDetector
  MOCK_CONST_METHOD1(cloneParameterized, IDetector *(const ParameterMap *map));
  MOCK_CONST_METHOD0(getID, Mantid::detid_t());
  MOCK_CONST_METHOD0(nDets, std::size_t());
  MOCK_CONST_METHOD1(getDistance, double(const IComponent &comp));
  MOCK_CONST_METHOD2(getTwoTheta, double(const Mantid::Kernel::V3D &observer, const Mantid::Kernel::V3D &axis));
  MOCK_CONST_METHOD3(getSignedTwoTheta, double(const Mantid::Kernel::V3D &observer, const Mantid::Kernel::V3D &axis,
                                               const Mantid::Kernel::V3D &instrumentUp));
  MOCK_CONST_METHOD0(getPhi, double());
  MOCK_CONST_METHOD1(getPhiOffset, double(const double &offset));
  MOCK_CONST_METHOD1(getTopology, det_topology(Mantid::Kernel::V3D &center));
  MOCK_CONST_METHOD0(parameterMap, ParameterMap &());
  MOCK_CONST_METHOD0(index, std::size_t());

  // IObjComponent
  MOCK_CONST_METHOD0(clone, IComponent *());
  MOCK_CONST_METHOD1(isValid, bool(const Mantid::Kernel::V3D &point));
  MOCK_CONST_METHOD1(isOnSide, bool(const Mantid::Kernel::V3D &point));
  MOCK_CONST_METHOD1(interceptSurface, int(Track &track));
  MOCK_CONST_METHOD1(solidAngle, double(const Mantid::Kernel::V3D &observer));
  MOCK_CONST_METHOD1(getPointInObject, int(Mantid::Kernel::V3D &point));
  MOCK_CONST_METHOD0(draw, void());
  MOCK_CONST_METHOD0(drawObject, void());
  MOCK_CONST_METHOD0(initDraw, void());
  MOCK_CONST_METHOD0(shape, const std::shared_ptr<const IObject>());
  MOCK_CONST_METHOD0(material, const Mantid::Kernel::Material());

  // IComponent
  MOCK_CONST_METHOD0(getComponentID, ComponentID());
  MOCK_CONST_METHOD0(getBaseComponent, IComponent const *());
  MOCK_METHOD1(setParent, void(IComponent *));
  MOCK_CONST_METHOD0(getParent, std::shared_ptr<const IComponent>());
  MOCK_CONST_METHOD0(getBareParent, const IComponent *());
  MOCK_CONST_METHOD0(getAncestors, std::vector<std::shared_ptr<const IComponent>>());
  MOCK_METHOD1(setName, void(const std::string &));
  MOCK_CONST_METHOD0(getName, std::string());
  MOCK_CONST_METHOD0(getFullName, std::string());
  MOCK_METHOD3(setPos, void(double, double, double));
  MOCK_METHOD1(setPos, void(const Mantid::Kernel::V3D &));
  MOCK_METHOD1(setRot, void(const Mantid::Kernel::Quat &));
  MOCK_METHOD1(translate, void(const Mantid::Kernel::V3D &));
  MOCK_METHOD3(translate, void(double, double, double));
  MOCK_METHOD1(rotate, void(const Mantid::Kernel::Quat &));
  MOCK_METHOD2(rotate, void(double, const Mantid::Kernel::V3D &));
  MOCK_CONST_METHOD0(getRelativePos, Mantid::Kernel::V3D());
  MOCK_CONST_METHOD0(getPos, Mantid::Kernel::V3D());
  MOCK_CONST_METHOD0(getRelativeRot, Mantid::Kernel::Quat());
  MOCK_CONST_METHOD0(getRotation, Mantid::Kernel::Quat());
  MOCK_CONST_METHOD1(getBoundingBox, void(BoundingBox &boundingBox));
  MOCK_CONST_METHOD1(getParameterNames, std::set<std::string>(bool recursive));
  MOCK_CONST_METHOD0(getParameterNamesByComponent, std::map<std::string, ComponentID>());
  MOCK_CONST_METHOD2(hasParameter, bool(const std::string &name, bool recursive));
  MOCK_CONST_METHOD2(getParameterType, std::string(const std::string &pname, bool recursive));
  MOCK_CONST_METHOD2(getNumberParameter, std::vector<double>(const std::string &pname, bool recursive));
  MOCK_CONST_METHOD2(getPositionParameter, std::vector<Mantid::Kernel::V3D>(const std::string &pname, bool recursive));
  MOCK_CONST_METHOD2(getRotationParameter, std::vector<Mantid::Kernel::Quat>(const std::string &pname, bool recursive));
  MOCK_CONST_METHOD2(getStringParameter, std::vector<std::string>(const std::string &pname, bool recursive));
  MOCK_CONST_METHOD2(getIntParameter, std::vector<int>(const std::string &pname, bool recursive));
  MOCK_CONST_METHOD2(getBoolParameter, std::vector<bool>(const std::string &pname, bool recursive));
  MOCK_CONST_METHOD2(getParameterAsString, std::string(const std::string &pname, bool recursive));
  MOCK_CONST_METHOD2(getParameterVisible, bool(const std::string &pname, bool recursive));
  MOCK_CONST_METHOD1(printSelf, void(std::ostream &));
  MOCK_CONST_METHOD0(isParametrized, bool());
  MOCK_CONST_METHOD1(registerContents, std::size_t(class ComponentVisitor &component));
};

class MockInstrumentActor : public IInstrumentActor {
public:
  MOCK_CONST_METHOD1(draw, void(bool picking));
  MOCK_CONST_METHOD3(getBoundingBox,
                     void(Mantid::Kernel::V3D &minBound, Mantid::Kernel::V3D &maxBound, const bool excludeMonitors));

  MOCK_METHOD1(setComponentVisible, void(std::size_t componentIndex));
  MOCK_METHOD1(setAllComponentsVisibility, void(bool /*on*/));
  MOCK_CONST_METHOD0(hasChildVisible, bool());

  MOCK_CONST_METHOD0(getInstrument, std::shared_ptr<const Mantid::Geometry::Instrument>());
  MOCK_CONST_METHOD0(getWorkspace, std::shared_ptr<const Mantid::API::MatrixWorkspace>());
  MOCK_CONST_METHOD0(componentInfo, const Mantid::Geometry::ComponentInfo &());
  MOCK_CONST_METHOD0(detectorInfo, const Mantid::Geometry::DetectorInfo &());

  MOCK_CONST_METHOD0(getMaskMatrixWorkspace, std::shared_ptr<Mantid::API::MatrixWorkspace>());
  MOCK_CONST_METHOD1(setMaskMatrixWorkspace, void(Mantid::API::MatrixWorkspace_sptr wsMask));
  MOCK_CONST_METHOD0(invertMaskWorkspace, void());
  MOCK_CONST_METHOD0(getMaskWorkspace, std::shared_ptr<Mantid::API::IMaskWorkspace>());
  MOCK_CONST_METHOD0(getMaskWorkspaceIfExists, std::shared_ptr<Mantid::API::IMaskWorkspace>());
  MOCK_METHOD0(applyMaskWorkspace, void());
  MOCK_METHOD1(addMaskBinsData, void(const std::vector<size_t> &indices));
  MOCK_CONST_METHOD0(extractCurrentMask, Mantid::API::MatrixWorkspace_sptr());
  MOCK_METHOD0(clearMasks, void());

  MOCK_CONST_METHOD0(isInitialized, bool());

  MOCK_CONST_METHOD0(getColorMap, const ColorMap &());
  MOCK_METHOD2(loadColorMap, void(const std::pair<QString, bool> & /*cmap*/, bool reset_colors));

  MOCK_METHOD1(changeScaleType, void(int /*type*/));
  MOCK_METHOD1(changeNthPower, void(double /*nth_power*/));
  MOCK_CONST_METHOD0(getCurrentColorMap, std::pair<QString, bool>());
  MOCK_METHOD1(setAutoscaling, void(bool /*on*/));
  MOCK_CONST_METHOD0(autoscaling, bool());

  MOCK_METHOD2(setIntegrationRange, void(const double &xmin, const double &xmax));
  MOCK_CONST_METHOD0(minValue, double());
  MOCK_CONST_METHOD0(maxValue, double());
  MOCK_METHOD1(setMinValue, void(double value));
  MOCK_METHOD1(setMaxValue, void(double value));
  MOCK_METHOD2(setMinMaxRange, void(double vmin, double vmax));
  MOCK_CONST_METHOD0(minPositiveValue, double());
  MOCK_CONST_METHOD0(minBinValue, double());
  MOCK_CONST_METHOD0(maxBinValue, double());
  MOCK_CONST_METHOD0(minWkspBinValue, double());
  MOCK_CONST_METHOD0(maxWkspBinValue, double());
  MOCK_CONST_METHOD0(wholeRange, bool());

  MOCK_CONST_METHOD0(ndetectors, std::size_t());
  MOCK_CONST_METHOD1(getDetectorByDetID, std::size_t(Mantid::detid_t detID));
  MOCK_CONST_METHOD1(getDetID, Mantid::detid_t(std::size_t pickID));
  MOCK_CONST_METHOD1(getDetIDs, std::vector<Mantid::detid_t>(const std::vector<size_t> &dets));
  MOCK_CONST_METHOD1(getComponentID, Mantid::Geometry::ComponentID(std::size_t pickID));
  MOCK_CONST_METHOD1(getDetPos, const Mantid::Kernel::V3D(std::size_t pickID));
  MOCK_CONST_METHOD0(getAllDetIDs, const std::vector<Mantid::detid_t> &());
  MOCK_CONST_METHOD1(getWorkspaceIndex, std::size_t(std::size_t index));
  MOCK_CONST_METHOD1(getWorkspaceIndices, std::vector<size_t>(const std::vector<size_t> &dets));
  MOCK_CONST_METHOD1(getIntegratedCounts, double(std::size_t index));
  MOCK_CONST_METHOD3(getBinMinMaxIndex, void(std::size_t wi, std::size_t &imin, std::size_t &imax));

  MOCK_METHOD0(updateColors, void());
  MOCK_METHOD1(showGuides, void(bool /*on*/));
  MOCK_CONST_METHOD0(areGuidesShown, bool());

  MOCK_CONST_METHOD0(initMaskHelper, void());
  MOCK_CONST_METHOD0(hasMaskWorkspace, bool());
  MOCK_CONST_METHOD0(hasBinMask, bool());
  MOCK_CONST_METHOD1(getParameterInfo, QString(std::size_t index));
  MOCK_CONST_METHOD0(getDefaultAxis, std::string());
  MOCK_CONST_METHOD0(getDefaultView, std::string());
  MOCK_CONST_METHOD0(getInstrumentName, std::string());
  MOCK_CONST_METHOD2(getStringParameter, std::vector<std::string>(const std::string &name, bool recursive));

  MOCK_METHOD1(loadFromProject, void(const std::string &lines));
  MOCK_CONST_METHOD0(saveToProject, std::string());
  MOCK_CONST_METHOD0(components, const std::vector<size_t> &());

  MOCK_CONST_METHOD0(hasGridBank, bool());
  MOCK_CONST_METHOD0(getNumberOfGridLayers, std::size_t());
  MOCK_CONST_METHOD2(setGridLayer, void(bool isUsingLayer, int layer));
  MOCK_CONST_METHOD0(getInstrumentRenderer, const InstrumentRenderer &());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
