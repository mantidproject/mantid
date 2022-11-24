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
#include "MantidKernel/Material.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/WarningSuppressions.h"

#include <string>
#include <utility>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

using namespace Mantid::API;
using namespace Mantid::Geometry;

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

GNU_DIAG_ON_SUGGEST_OVERRIDE
