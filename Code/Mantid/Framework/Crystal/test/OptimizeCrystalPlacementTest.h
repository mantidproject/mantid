/*
 * OptimizeCrystalPlacementTest.h
 *
 *  Created on: Mar 22, 2013
 *      Author: Ruth Mikkelson
 */

#ifndef OPTIMIZECRYSTALPLACEMENTTEST_H_
#define OPTIMIZECRYSTALPLACEMENTTEST_H_
#include <cxxtest/TestSuite.h>
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidCrystal/OptimizeCrystalPlacement.h"
#include "MantidCrystal/PeakHKLErrors.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/IFunction.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidCrystal/IndexPeaks.h"
#include "MantidKernel/Matrix.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidAPI/ITableWorkspace.h"

using namespace Mantid;
using namespace Crystal;

using Mantid::DataObjects::PeaksWorkspace_sptr;
using Mantid::DataObjects::TableWorkspace_sptr;
using Mantid::DataObjects::TableWorkspace_sptr;
using Mantid::Geometry::Goniometer;
using Mantid::Geometry::ParameterMap;
using Mantid::Geometry::IObjComponent_const_sptr;
using Mantid::API::ITableWorkspace;

class OptimizeCrystalPlacementTest: public CxxTest::TestSuite
{

public:

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static OptimizeCrystalPlacementTest *createSuite() { return new OptimizeCrystalPlacementTest(); }
  static void destroySuite( OptimizeCrystalPlacementTest *suite ) { delete suite; }

  OptimizeCrystalPlacementTest()
  {
    initted = false;
  }

  void init()
  {
    if( initted)
      return;
    initted = true;
    LoadIsawPeaks alg;
    alg.initialize();
    alg.setProperty("Filename", "TOPAZ_5637_8.peaks");
    alg.setProperty("OutputWorkspace", "abcd");
    alg.execute();

    alg.setProperty("OutputWorkspace", "abcd");
    API::Workspace_sptr ows=alg.getProperty("OutputWorkspace");
    peaks = boost::dynamic_pointer_cast<DataObjects::PeaksWorkspace>(ows);

    LoadIsawUB loadUB;
    loadUB.initialize();
    loadUB.setProperty("InputWorkspace", alg.getPropertyValue("OutputWorkspace"));
    loadUB.setProperty("Filename", "ls5637.mat");
    loadUB.execute();

    //peaks->setName("abcd");

  }
  void test_basic()
  {
    init();
    OptimizeCrystalPlacement alg;
    alg.initialize();
    alg.setPropertyValue("PeaksWorkspace", "abcd");
    alg.setProperty("PeaksWorkspace", peaks);
    alg.setPropertyValue("ModifiedPeaksWorkspace", "ModPeaks");
    alg.setPropertyValue("FitInfoTable", "FitInfoTable");
    alg.execute();

    alg.setPropertyValue("ModifiedPeaksWorkspace", "ModPeaks");
    peaks1 = alg.getProperty("ModifiedPeaksWorkspace");
    alg.setPropertyValue("FitInfoTable", "FitInfoTable");
    boost::shared_ptr<ITableWorkspace> table = alg.getProperty("FitInfoTable");


    Kernel::Matrix<double> Gon;
    Gon.zeroMatrix();
    Kernel::Matrix<double> ZMat(Gon);

    Kernel::Matrix<double> Rotx = PeakHKLErrors::RotationMatrixAboutRegAxis(1, 'x');
    Kernel::Matrix<double> Roty = PeakHKLErrors::RotationMatrixAboutRegAxis(-2, 'y');

    for (int i = 0; i < peaks1->getNumberPeaks(); ++i)
      if (peaks1->getPeak(i).getRunNumber() == 5638)
      {
        API::IPeak& peak = peaks1->getPeak(i);
        if (Gon == ZMat)
        {
          origGon5638 = peak.getGoniometerMatrix();
          Gon = Rotx * Roty * origGon5638;

        }
        peak.setGoniometerMatrix(Gon);

      }
    Geometry::Goniometer GonInst(Gon);
    std::vector<double> chiphiOmega = GonInst.getEulerAngles("YZY");

    OptimizeCrystalPlacement alg1;
    alg1.initialize();
    API::AnalysisDataService::Instance().addOrReplace("abcd1", peaks1);

    alg1.setPropertyValue( "PeaksWorkspace", "abcd1" );
    alg1.setPropertyValue( "ModifiedPeaksWorkspace", "ModPeaks" );
    alg1.setPropertyValue ("FitInfoTable", "FitInfoTable1" );
    alg1.setProperty( "KeepGoniometerFixedfor", "5637" );
    alg1.execute();

    alg1.setPropertyValue( "FitInfoTable", "FitInfoTable1" );
    table = alg1.getProperty( "FitInfoTable" );


    Geometry::Goniometer IGon(origGon5638);
    std::vector<double> GonAngles5638 = IGon.getEulerAngles("YZY");

    for (size_t i = 0; i < table->rowCount(); ++i )
    {
      std::string nm = table->String(i, 0);
      double d = 0.0;
      if (nm == "chi5638")
        d = GonAngles5638[1] - table->Double(i, 1);
      else if (nm == "phi5638")
        d = GonAngles5638[2] - table->Double(i, 1);
      else if (nm == "omega5638")
        d = GonAngles5638[0] - table->Double(i, 1);

      TS_ASSERT_DELTA(d, 0, .3);

    }
  }

  void test_tilt()
  {
    init();
    Kernel::Matrix<double> tilt = PeakHKLErrors::RotationMatrixAboutRegAxis(1, 'x')
        * PeakHKLErrors::RotationMatrixAboutRegAxis(-2, 'y')
        * PeakHKLErrors::RotationMatrixAboutRegAxis(1.3, 'z');

    origGon5637.zeroMatrix();
    Kernel::Matrix<double> ZMat;
    ZMat.zeroMatrix();

    for (int i = 0; i < peaks1->getNumberPeaks(); ++i)
    {
      API::IPeak & peak = peaks1->getPeak(i);
      int RunNum = peak.getRunNumber();

      Kernel::Matrix<double> GG;
      if (RunNum == 5637)
      {
        if (origGon5637 == ZMat)
          origGon5637 = peak.getGoniometerMatrix();

        peak.setGoniometerMatrix(tilt * origGon5637);
      }
      else
        peak.setGoniometerMatrix(tilt * origGon5638);

    }

    Geometry::Goniometer IGon(origGon5638);
    std::vector<double> GonAngles5638 = IGon.getEulerAngles("YZY");

    Geometry::Goniometer IGon2(origGon5637);
    std::vector<double> GonAngles5637 = IGon2.getEulerAngles("YZY");

    OptimizeCrystalPlacement alg;
    alg.initialize();
    API::AnalysisDataService::Instance().addOrReplace("abcd2", peaks1);

    alg.setPropertyValue("PeaksWorkspace", "abcd2");
    alg.setPropertyValue("ModifiedPeaksWorkspace", "ModPeaks");
    alg.setPropertyValue("FitInfoTable", "FitInfoTable2");
    alg.setProperty("KeepGoniometerFixedfor", "5637,5638");
    alg.setProperty("OptimizeGoniometerTilt", true);
    alg.execute();
    alg.setPropertyValue("FitInfoTable", "FitInfoTable2");
    boost::shared_ptr<ITableWorkspace> table = alg.getProperty("FitInfoTable");

    Kernel::V3D Rotxyz;
    for (size_t i = 0; i < table->rowCount(); ++i)
    {
      std::string nm = table->String(i, 0);

      if (nm == "GonRotx")
        Rotxyz[0] = table->Double(i, 1);

      else if (nm == "GonRoty")
        Rotxyz[1] = table->Double(i, 1);

      else if (nm == "GonRotz")
        Rotxyz[2] = table->Double(i, 1);

    }

    Kernel::Matrix<double> tilt2 = PeakHKLErrors::RotationMatrixAboutRegAxis(Rotxyz[0], 'x')
        * PeakHKLErrors::RotationMatrixAboutRegAxis(Rotxyz[1], 'y')
        * PeakHKLErrors::RotationMatrixAboutRegAxis(Rotxyz[2], 'z');

    Kernel::Matrix<double> Change = tilt2 * tilt;

    Geometry::Goniometer IGon3(Change * origGon5637);
    std::vector<double> GonAngles5637a = IGon3.getEulerAngles("YZY");

    Geometry::Goniometer IGon4(Change * origGon5638);
    std::vector<double> GonAngles5638a = IGon4.getEulerAngles("YZY");

    for (int i = 0; i < 3; i++)
    {
      TS_ASSERT_DELTA(GonAngles5637[i], GonAngles5637a[i], .2);
      TS_ASSERT_DELTA(GonAngles5638[i], GonAngles5638a[i], .15);
    }

  }

  void test_SamplePosition()
  {
    init();
    API::IPeak & peak = peaks1->getPeak(0);
    boost::shared_ptr<const Geometry::Instrument> Inst = peak.getInstrument();
    Kernel::V3D SampPos(.0003, -.00025, .00015);

    boost::shared_ptr<Geometry::ParameterMap> pmap = Inst->getParameterMap();//check if parameterized.
    Geometry::IObjComponent_const_sptr sample = Inst->getSample();
    Kernel::V3D oldSampPos = sample->getPos();// Should reset Inst

    pmap->addPositionCoordinate(sample.get(), "x", SampPos.X());
    pmap->addPositionCoordinate(sample.get(), "y", SampPos.Y());
    pmap->addPositionCoordinate(sample.get(), "z", SampPos.Z());
    boost::shared_ptr<const Geometry::Instrument> newInstr(new Geometry::Instrument(
        Inst->baseInstrument(), pmap));

    for (int i = 0; i < peaks1->getNumberPeaks(); ++i)
      peaks1->getPeak(i).setInstrument(newInstr);
    OptimizeCrystalPlacement alg;
    alg.initialize();


    API::AnalysisDataService::Instance().addOrReplace("abcd3", peaks1);

    alg.setPropertyValue("PeaksWorkspace", "abcd3");
    alg.setPropertyValue("ModifiedPeaksWorkspace", "ModPeaks");
    alg.setPropertyValue("FitInfoTable", "FitInfoTable2");
    alg.setProperty("KeepGoniometerFixedfor", "5637,5638");
    alg.setProperty("AdjustSampleOffsets", true);
    alg.execute();

    alg.setPropertyValue("FitInfoTable", "FitInfoTable2");
    boost::shared_ptr<ITableWorkspace> table = alg.getProperty("FitInfoTable");


    TS_ASSERT_DELTA(table->Double(0, 1), 0, .00024);
    TS_ASSERT_DELTA(table->Double(1, 1), 0, .00024);
    TS_ASSERT_DELTA(table->Double(2, 1), 0, .00024);

  /*  for (size_t i = 0; i < table->rowCount(); ++i)
     {
     std::string nm = table->String(i, 0);
     std::cout<<nm<<","<<table->Double(i,1)<<","<<table->Double(i,2)<<std::endl;
     }
*/
  }
private:
  DataObjects::PeaksWorkspace_sptr peaks;
  DataObjects::PeaksWorkspace_sptr peaks1;
  Kernel::Matrix<double> origGon5637, origGon5638;
  bool initted;
};

#endif /* OPTIMIZECRYSTALPLACEMENTTEST_H_ */
