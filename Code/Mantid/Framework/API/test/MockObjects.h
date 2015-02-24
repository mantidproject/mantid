/*
 * MockObjects.h
 *
 *  Created on: Apr 8, 2014
 *      Author: spu92482
 */

#ifndef MANTIDAPITEST_MOCKOBJECTS_H_
#define MANTIDAPITEST_MOCKOBJECTS_H_

#include "MantidAPI/PeakTransform.h"
#include "MantidAPI/PeakTransformFactory.h"
#include "MantidAPI/IPeak.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include <boost/regex.hpp>
#include <gmock/gmock.h>

using namespace Mantid::API;
using namespace Mantid;
using boost::regex;

namespace
{

  /*------------------------------------------------------------
   Mock Peak Transform
   ------------------------------------------------------------*/
  class MockPeakTransform: public PeakTransform
  {
  public:
    MockPeakTransform() :
        PeakTransform("H (Lattice)", "K (Lattice)", regex("^H.*$"), regex("^K.*$"), regex("^L.*$"))
    {
    }
    ~MockPeakTransform()
    {
    }
    MOCK_CONST_METHOD0(clone, PeakTransform_sptr());
    MOCK_CONST_METHOD1(transform, Mantid::Kernel::V3D(const Mantid::Kernel::V3D&));
    MOCK_CONST_METHOD1(transformPeak, Mantid::Kernel::V3D(const Mantid::API::IPeak&));
    MOCK_CONST_METHOD0(getFriendlyName, std::string());
    MOCK_CONST_METHOD0(getCoordinateSystem, Mantid::Kernel::SpecialCoordinateSystem());
  };

  /*------------------------------------------------------------
   Mock Peak Transform Factory
   ------------------------------------------------------------*/
  class MockPeakTransformFactory: public PeakTransformFactory
  {
  public:
    MOCK_CONST_METHOD0(createDefaultTransform, PeakTransform_sptr());
    MOCK_CONST_METHOD2(createTransform, PeakTransform_sptr(const std::string&, const std::string&));
  };



  /*------------------------------------------------------------
  Mock IPeak
  ------------------------------------------------------------*/
  class MockIPeak : public Mantid::API::IPeak
  {
  public:
    MOCK_METHOD1(setInstrument,
      void(Geometry::Instrument_const_sptr inst));
    MOCK_CONST_METHOD0(getDetectorID,
      int());
    MOCK_METHOD1(setDetectorID,
      void(int m_DetectorID));
    MOCK_CONST_METHOD0(getDetector,
      Geometry::IDetector_const_sptr());
    MOCK_CONST_METHOD0(getInstrument,
      Geometry::Instrument_const_sptr());
    MOCK_CONST_METHOD0(getRunNumber,
      int());
    MOCK_METHOD1(setRunNumber,
      void(int m_RunNumber));
    MOCK_CONST_METHOD0(getMonitorCount,
      double());
    MOCK_METHOD1(setMonitorCount,
      void(double m_MonitorCount));
    MOCK_CONST_METHOD0(getH,
      double());
    MOCK_CONST_METHOD0(getK,
      double());
    MOCK_CONST_METHOD0(getL,
      double());
    MOCK_CONST_METHOD0(getHKL,
      Mantid::Kernel::V3D());
    MOCK_METHOD1(setH,
      void(double m_H));
    MOCK_METHOD1(setK,
      void(double m_K));
    MOCK_METHOD1(setL,
      void(double m_L));
    MOCK_METHOD3(setHKL,
      void(double H, double K, double L));
    MOCK_METHOD1(setHKL,
      void(Mantid::Kernel::V3D HKL));
    MOCK_CONST_METHOD0(getQLabFrame,
      Mantid::Kernel::V3D());
    MOCK_CONST_METHOD0(getQSampleFrame,
      Mantid::Kernel::V3D());
    MOCK_METHOD0(findDetector,
      bool());
    MOCK_METHOD2(setQSampleFrame,
      void(Mantid::Kernel::V3D QSampleFrame, boost::optional<double> detectorDistance));
    MOCK_METHOD2(setQLabFrame,
      void(Mantid::Kernel::V3D QLabFrame, boost::optional<double> detectorDistance));
    MOCK_METHOD1(setWavelength,
      void(double wavelength));
    MOCK_CONST_METHOD0(getWavelength,
      double());
    MOCK_CONST_METHOD0(getScattering,
      double());
    MOCK_CONST_METHOD0(getDSpacing,
      double());
    MOCK_CONST_METHOD0(getTOF,
      double());
    MOCK_CONST_METHOD0(getInitialEnergy,
      double());
    MOCK_CONST_METHOD0(getFinalEnergy,
      double());
    MOCK_METHOD1(setInitialEnergy,
      void(double m_InitialEnergy));
    MOCK_METHOD1(setFinalEnergy,
      void(double m_FinalEnergy));
    MOCK_CONST_METHOD0(getIntensity,
      double());
    MOCK_CONST_METHOD0(getSigmaIntensity,
      double());
    MOCK_METHOD1(setIntensity,
      void(double m_Intensity));
    MOCK_METHOD1(setSigmaIntensity,
      void(double m_SigmaIntensity));
    MOCK_CONST_METHOD0(getBinCount,
      double());
    MOCK_METHOD1(setBinCount,
      void(double m_BinCount));
    MOCK_CONST_METHOD0(getGoniometerMatrix,
      Mantid::Kernel::Matrix<double>());
    MOCK_METHOD1(setGoniometerMatrix,
      void(Mantid::Kernel::Matrix<double> m_GoniometerMatrix));
    MOCK_CONST_METHOD0(getBankName,
      std::string());
    MOCK_CONST_METHOD0(getRow,
      int());
    MOCK_CONST_METHOD0(getCol,
      int());
    MOCK_CONST_METHOD0(getDetPos,
      Mantid::Kernel::V3D());
    MOCK_CONST_METHOD0(getL1,
      double());
    MOCK_CONST_METHOD0(getL2,
      double());
    MOCK_CONST_METHOD0(getDetectorPosition,
      Mantid::Kernel::V3D());
    MOCK_CONST_METHOD0(getDetectorPositionNoCheck,
          Mantid::Kernel::V3D());
    MOCK_METHOD0(getPeakShape, const Mantid::Geometry::PeakShape&());
  };


}
#endif /* MANTIDAPITEST_MOCKOBJECTS_H_ */
