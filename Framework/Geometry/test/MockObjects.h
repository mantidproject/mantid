// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * MockObjects.h
 *
 *  Created on: Apr 8, 2014
 *      Author: spu92482
 */

#pragma once

#include "MantidFrameworkTestHelpers/FallbackBoostOptionalIO.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Crystal/PeakTransform.h"
#include "MantidGeometry/Crystal/PeakTransformFactory.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/WarningSuppressions.h"
#include <boost/regex.hpp>
#include <gmock/gmock.h>

using namespace Mantid::Geometry;
using namespace Mantid;
using boost::regex;

namespace {

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/*------------------------------------------------------------
 Mock Peak Transform
 ------------------------------------------------------------*/
class MockPeakTransform : public PeakTransform {
public:
  MockPeakTransform() : PeakTransform("H (Lattice)", "K (Lattice)", regex("^H.*$"), regex("^K.*$"), regex("^L.*$")) {}
  ~MockPeakTransform() override = default;
  MOCK_CONST_METHOD0(clone, PeakTransform_sptr());
  MOCK_CONST_METHOD1(transform, Mantid::Kernel::V3D(const Mantid::Kernel::V3D &));
  MOCK_CONST_METHOD1(transformPeak, Mantid::Kernel::V3D(const Mantid::Geometry::IPeak &));
  MOCK_CONST_METHOD0(getFriendlyName, std::string());
  MOCK_CONST_METHOD0(getCoordinateSystem, Mantid::Kernel::SpecialCoordinateSystem());
};

/*------------------------------------------------------------
 Mock Peak Transform Factory
 ------------------------------------------------------------*/
class MockPeakTransformFactory : public PeakTransformFactory {
public:
  MOCK_CONST_METHOD0(createDefaultTransform, PeakTransform_sptr());
  MOCK_CONST_METHOD2(createTransform, PeakTransform_sptr(const std::string &, const std::string &));
};

/*------------------------------------------------------------
Mock IPeak
------------------------------------------------------------*/
class MockIPeak : public Mantid::Geometry::IPeak {
public:
  MOCK_CONST_METHOD0(getReferenceFrame, std::shared_ptr<const Geometry::ReferenceFrame>());
  MOCK_CONST_METHOD0(getRunNumber, int());
  MOCK_CONST_METHOD0(getPeakNumber, int());
  MOCK_CONST_METHOD0(getIntMNP, Mantid::Kernel::V3D());
  MOCK_METHOD1(setRunNumber, void(int m_RunNumber));
  MOCK_METHOD1(setPeakNumber, void(int m_PeakNumber));
  MOCK_METHOD1(setIntMNP, void(const Mantid::Kernel::V3D &m_modStru));
  MOCK_CONST_METHOD0(getMonitorCount, double());
  MOCK_METHOD1(setMonitorCount, void(double m_MonitorCount));
  MOCK_CONST_METHOD0(getH, double());
  MOCK_CONST_METHOD0(getK, double());
  MOCK_CONST_METHOD0(getL, double());
  MOCK_CONST_METHOD0(getHKL, Mantid::Kernel::V3D());
  MOCK_CONST_METHOD0(isIndexed, bool());
  MOCK_CONST_METHOD0(getIntHKL, Mantid::Kernel::V3D());
  MOCK_CONST_METHOD0(getSamplePos, Mantid::Kernel::V3D());
  MOCK_METHOD1(setH, void(double m_H));
  MOCK_METHOD1(setK, void(double m_K));
  MOCK_METHOD1(setL, void(double m_L));
  MOCK_METHOD3(setHKL, void(double H, double K, double L));
  MOCK_METHOD1(setHKL, void(const Mantid::Kernel::V3D &HKL));
  MOCK_METHOD1(setIntHKL, void(const Mantid::Kernel::V3D &HKL));
  MOCK_METHOD3(setSamplePos, void(double samX, double samY, double samZ));
  MOCK_METHOD1(setSamplePos, void(const Mantid::Kernel::V3D &XYZ));
  MOCK_CONST_METHOD0(getQLabFrame, Mantid::Kernel::V3D());
  MOCK_CONST_METHOD0(getQSampleFrame, Mantid::Kernel::V3D());
  MOCK_METHOD2(setQSampleFrame, void(const Mantid::Kernel::V3D &QSampleFrame, std::optional<double> detectorDistance));
  MOCK_METHOD2(setQLabFrame, void(const Mantid::Kernel::V3D &QLabFrame, std::optional<double> detectorDistance));
  MOCK_METHOD1(setWavelength, void(double wavelength));
  MOCK_CONST_METHOD0(getWavelength, double());
  MOCK_CONST_METHOD0(getScattering, double());
  MOCK_CONST_METHOD0(getAzimuthal, double());
  MOCK_CONST_METHOD0(getDSpacing, double());
  MOCK_CONST_METHOD0(getTOF, double());
  MOCK_CONST_METHOD0(getInitialEnergy, double());
  MOCK_CONST_METHOD0(getFinalEnergy, double());
  MOCK_CONST_METHOD0(getEnergyTransfer, double());
  MOCK_METHOD1(setInitialEnergy, void(double m_InitialEnergy));
  MOCK_METHOD1(setFinalEnergy, void(double m_FinalEnergy));
  MOCK_CONST_METHOD0(getIntensity, double());
  MOCK_CONST_METHOD0(getSigmaIntensity, double());
  MOCK_CONST_METHOD0(getIntensityOverSigma, double());
  MOCK_CONST_METHOD0(getAbsorptionWeightedPathLength, double());
  MOCK_METHOD1(setIntensity, void(double m_Intensity));
  MOCK_METHOD1(setSigmaIntensity, void(double m_SigmaIntensity));
  MOCK_METHOD1(setAbsorptionWeightedPathLength, void(double m_AbsorptionWeightedPathLength));
  MOCK_CONST_METHOD0(getBinCount, double());
  MOCK_METHOD1(setBinCount, void(double m_BinCount));
  MOCK_CONST_METHOD0(getGoniometerMatrix, Mantid::Kernel::Matrix<double>());
  MOCK_METHOD1(setGoniometerMatrix, void(const Mantid::Kernel::Matrix<double> &m_GoniometerMatrix));
  MOCK_CONST_METHOD0(getDetectorID, int());
  MOCK_CONST_METHOD0(getRow, int());
  MOCK_CONST_METHOD0(getCol, int());
  MOCK_CONST_METHOD0(getL1, double());
  MOCK_CONST_METHOD0(getL2, double());
  MOCK_CONST_METHOD0(getPeakShape, const Mantid::Geometry::PeakShape &());
  MOCK_METHOD1(setPeakShape, void(Mantid::Geometry::PeakShape *shape));
  MOCK_METHOD1(setPeakShape, void(Mantid::Geometry::PeakShape_const_sptr shape));
  MOCK_CONST_METHOD0(getSourceDirectionSampleFrame, Mantid::Kernel::V3D());
  MOCK_CONST_METHOD0(getDetectorDirectionSampleFrame, Mantid::Kernel::V3D());
};
} // namespace
GNU_DIAG_ON_SUGGEST_OVERRIDE
