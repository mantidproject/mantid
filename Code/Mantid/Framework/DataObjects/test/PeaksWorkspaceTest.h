#ifndef MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_
#define MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdio.h>
#include <cmath>
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidTestHelpers/NexusTestHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/LogManager.h"

#include <Poco/File.h>


using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class PeaksWorkspaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeaksWorkspaceTest *createSuite() { return new PeaksWorkspaceTest(); }
  static void destroySuite( PeaksWorkspaceTest *suite ) { delete suite; }

  /** Build a test PeaksWorkspace with one peak (others peaks can be added)
   *
   * @return PeaksWorkspace
   */
  PeaksWorkspace_sptr buildPW()
  {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, 10);
    inst->setName("SillyInstrument");
    auto pw = PeaksWorkspace_sptr(new PeaksWorkspace);
    pw->setInstrument(inst);
    std::string val = "value";
    pw->mutableRun().addProperty("TestProp", val);
    Peak p(inst, 1, 3.0);
    pw->addPeak(p);
    return pw;
  }

  /** Check that the PeaksWorkspace build by buildPW() is correct */
  void checkPW(const PeaksWorkspace & pw)
  {
    TS_ASSERT_EQUALS( pw.columnCount(), 17);
    TS_ASSERT_EQUALS( pw.rowCount(), 1);
    TS_ASSERT_EQUALS( pw.getNumberPeaks(), 1);
    if (pw.getNumberPeaks() != 1) return;
    TS_ASSERT_DELTA( pw.getPeak(0).getWavelength(), 3.0, 1e-4);
    // Experiment info stuff got copied
    TS_ASSERT_EQUALS( pw.getInstrument()->getName(), "SillyInstrument");
    TS_ASSERT( pw.run().hasProperty("TestProp") );
  }

  void test_defaultConstructor()
  {
    auto pw = buildPW();
    checkPW(*pw);
  }

  void test_copyConstructor()
  {
    auto pw = buildPW();
    auto pw2 = PeaksWorkspace_sptr(new PeaksWorkspace(*pw));
    checkPW(*pw2);
  }

  void test_clone()
  {
    auto pw = buildPW();
    auto pw2 = pw->clone();
    checkPW(*pw2);
  }

  void test_sort()
  {
    auto pw = buildPW();
    Instrument_const_sptr inst = pw->getInstrument();
    Peak p0 = Peak(pw->getPeak(0)); //Peak(inst, 1, 3.0)
    Peak p1(inst, 1, 4.0);
    Peak p2(inst, 1, 5.0);
    Peak p3(inst, 2, 3.0);
    Peak p4(inst, 3, 3.0);
    pw->addPeak(p1);
    pw->addPeak(p2);
    pw->addPeak(p3);
    pw->addPeak(p4);

    std::vector< std::pair<std::string, bool> > criteria;
    // Sort by detector ID then descending wavelength
    criteria.push_back( std::pair<std::string, bool>("detid", true) );
    criteria.push_back( std::pair<std::string, bool>("wavelength", false) );
    pw->sort(criteria);
    TS_ASSERT_EQUALS( pw->getPeak(0).getDetectorID(), 1);
    TS_ASSERT_DELTA(  pw->getPeak(0).getWavelength(), 5.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(1).getDetectorID(), 1);
    TS_ASSERT_DELTA(  pw->getPeak(1).getWavelength(), 4.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(2).getDetectorID(), 1);
    TS_ASSERT_DELTA(  pw->getPeak(2).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(3).getDetectorID(), 2);
    TS_ASSERT_DELTA(  pw->getPeak(3).getWavelength(), 3.0, 1e-5);

    // Sort by wavelength ascending then detID
    criteria.clear();
    criteria.push_back( std::pair<std::string, bool>("wavelength", true) );
    criteria.push_back( std::pair<std::string, bool>("detid", true) );
    pw->sort(criteria);
    TS_ASSERT_EQUALS( pw->getPeak(0).getDetectorID(), 1);
    TS_ASSERT_DELTA(  pw->getPeak(0).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(1).getDetectorID(), 2);
    TS_ASSERT_DELTA(  pw->getPeak(1).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(2).getDetectorID(), 3);
    TS_ASSERT_DELTA(  pw->getPeak(2).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(3).getDetectorID(), 1);
    TS_ASSERT_DELTA(  pw->getPeak(3).getWavelength(), 4.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(4).getDetectorID(), 1);
    TS_ASSERT_DELTA(  pw->getPeak(4).getWavelength(), 5.0, 1e-5);

  }

  void test_Save_Unmodified_PeaksWorkspace_Nexus()
  {

    const std::string filename = "test_Save_Unmodified_PeaksWorkspace_Nexus.nxs";
    auto testPWS = createSaveTestPeaksWorkspace();
    NexusTestHelper nexusHelper(true);
    nexusHelper.createFile("testSavePeaksWorkspace.nxs");

    testPWS->saveNexus(nexusHelper.file);
    nexusHelper.reopenFile();

    // Verify that this test_entry has a peaks_workspace entry
    TS_ASSERT_THROWS_NOTHING(nexusHelper.file->openGroup("peaks_workspace","NXentry") );

    // Check detector IDs
    TS_ASSERT_THROWS_NOTHING(nexusHelper.file->openData("column_1") );
    std::vector<int> detIDs;
    TS_ASSERT_THROWS_NOTHING(nexusHelper.file->getData(detIDs));
    nexusHelper.file->closeData();
    TS_ASSERT_EQUALS( detIDs.size(), 5);  // We have 5 detectors
    if( detIDs.size() >= 5) 
    {
      TS_ASSERT_EQUALS( detIDs[0], 1);
      TS_ASSERT_EQUALS( detIDs[1], 10);
      TS_ASSERT_EQUALS( detIDs[2], 10);
      TS_ASSERT_EQUALS( detIDs[3], 20);
      TS_ASSERT_EQUALS( detIDs[4], 50);
    }

    // Check wavelengths
    TS_ASSERT_THROWS_NOTHING(nexusHelper.file->openData("column_10") );
    std::vector<double> waveLengths;
    TS_ASSERT_THROWS_NOTHING(nexusHelper.file->getData(waveLengths));
    nexusHelper.file->closeData();
    TS_ASSERT_EQUALS( waveLengths.size(), 5);  // We have 5 wave lengths
    if( waveLengths.size() >= 5)
    {
      TS_ASSERT_DELTA( waveLengths[0], 3.0, 1e-5);
      TS_ASSERT_DELTA( waveLengths[1], 4.0, 1e-5);
      TS_ASSERT_DELTA( waveLengths[2], 5.0, 1e-5);
      TS_ASSERT_DELTA( waveLengths[3], 3.0, 1e-5);
      TS_ASSERT_DELTA( waveLengths[4], 3.0, 1e-5);
    }

  }

  void test_getSetLogAccess()
  {
    bool trueSwitch(true);
    auto pw = buildPW();

    LogManager_const_sptr props = pw->getLogs();
    std::string existingVal;

    TS_ASSERT_THROWS_NOTHING(existingVal=props->getPropertyValueAsType<std::string>("TestProp"));
    TS_ASSERT_EQUALS("value",existingVal);

    // define local scope;
    if(trueSwitch)
    {
      // get mutable pointer to existing values;
      LogManager_sptr mprops = pw->logs();

      TS_ASSERT_THROWS_NOTHING(mprops->addProperty<std::string>("TestProp2","value2"));

      TS_ASSERT(mprops->hasProperty("TestProp2"));
      TS_ASSERT(!props->hasProperty("TestProp2"));
      TS_ASSERT(pw->run().hasProperty("TestProp2"));
    }
    // nothing terrible happened and workspace still have this property
    TS_ASSERT(pw->run().hasProperty("TestProp2"));

    auto pw1 = pw->clone();
    if(trueSwitch)
    {
      // get mutable pointer to existing values, which would be taken from the cash
      LogManager_sptr mprops1 = pw->logs();
      // and in ideal world this should cause CowPtr to diverge but it does not
      TS_ASSERT_THROWS_NOTHING(mprops1->addProperty<std::string>("TestProp1-3","value1-3"));
      TS_ASSERT(mprops1->hasProperty("TestProp1-3"));
      //THE CHANGES TO PW ARE APPLIED TO the COPY (PW1 too!!!!)
      TS_ASSERT(pw->run().hasProperty("TestProp1-3"));
      TS_ASSERT(pw1->run().hasProperty("TestProp1-3"));
    }
    TS_ASSERT(pw1->run().hasProperty("TestProp1-3"));
    if(trueSwitch)
    {
      // but this will cause it to diverge
      LogManager_sptr mprops2 = pw1->logs();
      // and this  causes CowPtr to diverge
      TS_ASSERT_THROWS_NOTHING(mprops2->addProperty<std::string>("TestProp2-3","value2-3"));
      TS_ASSERT(mprops2->hasProperty("TestProp2-3"));
      TS_ASSERT(!pw->run().hasProperty("TestProp2-3"));
      TS_ASSERT(pw1->run().hasProperty("TestProp2-3"));
    }

  }

   void test_hasIntegratedPeaks_without_property()
   {
     PeaksWorkspace ws;
     TSM_ASSERT("Should not indicate that there are integrated peaks without property.", !ws.hasIntegratedPeaks());
   }

   void test_hasIntegratedPeaks_with_property_when_false()
   {
     PeaksWorkspace ws;
     bool hasIntegratedPeaks(false);
     ws.mutableRun().addProperty("PeaksIntegrated", hasIntegratedPeaks);
     TS_ASSERT_EQUALS(hasIntegratedPeaks, ws.hasIntegratedPeaks());
   }

   void test_hasIntegratedPeaks_with_property_when_true()
   {
     PeaksWorkspace ws;
     bool hasIntegratedPeaks(true);
     ws.mutableRun().addProperty("PeaksIntegrated", hasIntegratedPeaks);
     TS_ASSERT_EQUALS(hasIntegratedPeaks, ws.hasIntegratedPeaks());
   }

   void test_createDetectorTable_With_SinglePeak_And_Centre_Det_Has_Single_Row()
   {
     auto pw = buildPW(); // single peak with single detector
     auto detTable = pw->createDetectorTable();
     TSM_ASSERT("No table has been created",detTable);
     if(!detTable) return;
     check_Detector_Table_Metadata(*detTable, 1);

     auto column0 = detTable->getColumn(0);
     auto column1 = detTable->getColumn(1);
     // Contents
     TS_ASSERT_EQUALS(0, column0->cell<int>(0));
     TS_ASSERT_EQUALS(1, column1->cell<int>(0));
   }

   void test_createDetectorTable_With_SinglePeak_And_Multiple_Det_Has_Same_Num_Rows_As_Dets()
   {
     auto pw = buildPW(); // 1 peaks each with single detector
     // Add a detector to the peak
     Mantid::API::IPeak & ipeak = pw->getPeak(0);
     auto & peak = static_cast<Peak&>(ipeak);
     peak.addContributingDetID(2);
     peak.addContributingDetID(3);

     auto detTable = pw->createDetectorTable();
     TSM_ASSERT("No table has been created",detTable);
     if(!detTable) return;
     check_Detector_Table_Metadata(*detTable, 3);

     auto column0 = detTable->getColumn(0);
     auto column1 = detTable->getColumn(1);
     // Contents
     // Peak 1
     TS_ASSERT_EQUALS(0, column0->cell<int>(0)); // Index 0
     TS_ASSERT_EQUALS(1, column1->cell<int>(0)); // Id 1
     TS_ASSERT_EQUALS(0, column0->cell<int>(1)); // Index 0
     TS_ASSERT_EQUALS(2, column1->cell<int>(1)); // Id 2
     TS_ASSERT_EQUALS(0, column0->cell<int>(2)); // Index 0
     TS_ASSERT_EQUALS(3, column1->cell<int>(2)); // Id 3
   }

   void test_createDetectorTable_With_Many_Peaks_And_Multiple_Dets()
   {
     auto pw = createSaveTestPeaksWorkspace(); // 5 peaks each with single detector

     // Add some detectors
     Mantid::API::IPeak & ipeak3 = pw->getPeak(2);
     auto & peak3 = static_cast<Peak&>(ipeak3);
     peak3.addContributingDetID(11);
     Mantid::API::IPeak & ipeak5 = pw->getPeak(4);
     auto & peak5 = static_cast<Peak&>(ipeak5);
     peak5.addContributingDetID(51);
     peak5.addContributingDetID(52);

     auto detTable = pw->createDetectorTable();
     TSM_ASSERT("No table has been created",detTable);
     if(!detTable) return;
     check_Detector_Table_Metadata(*detTable, 8);

     auto column0 = detTable->getColumn(0);
     auto column1 = detTable->getColumn(1);
     // Contents -- Be verbose, it's easier to understand
     // Peak 1 
     TS_ASSERT_EQUALS(0, column0->cell<int>(0)); // Index 0
     TS_ASSERT_EQUALS(1, column1->cell<int>(0)); // Id 1
     // Peak 2
     TS_ASSERT_EQUALS(1, column0->cell<int>(1)); // Index 1
     TS_ASSERT_EQUALS(10, column1->cell<int>(1)); // Id 10

     // Peak 3
     TS_ASSERT_EQUALS(2, column0->cell<int>(2)); // Index 2
     TS_ASSERT_EQUALS(10, column1->cell<int>(2)); // Id 10
     TS_ASSERT_EQUALS(2, column0->cell<int>(3)); // Index 2
     TS_ASSERT_EQUALS(11, column1->cell<int>(3)); // Id 11

     // Peak 4
     TS_ASSERT_EQUALS(3, column0->cell<int>(4)); // Index 3
     TS_ASSERT_EQUALS(20, column1->cell<int>(4)); // Id 20

     // Peak 5
     TS_ASSERT_EQUALS(4, column0->cell<int>(5)); // Index 4
     TS_ASSERT_EQUALS(50, column1->cell<int>(5)); // Id 50
     TS_ASSERT_EQUALS(4, column0->cell<int>(6)); // Index 4
     TS_ASSERT_EQUALS(51, column1->cell<int>(6)); // Id 51
     TS_ASSERT_EQUALS(4, column0->cell<int>(7)); // Index 4
     TS_ASSERT_EQUALS(52, column1->cell<int>(7)); // Id 52
   }

   void test_default_getSpecialCoordinates()
   {
     auto pw = PeaksWorkspace_sptr(new PeaksWorkspace);
     TS_ASSERT_EQUALS(None, pw->getSpecialCoordinateSystem());
   }

   void test_setSpecialCoordinates()
   {
     auto pw = PeaksWorkspace_sptr(new PeaksWorkspace);
     SpecialCoordinateSystem coordSystem = Mantid::Kernel::HKL;
     pw->setCoordinateSystem(coordSystem);
     TS_ASSERT_EQUALS(coordSystem, pw->getSpecialCoordinateSystem());
   }

   void test_createPeakHKL()
   {
       // Create simple fictional instrument
       const V3D source(0,0,0);
       const V3D sample(15, 0, 0);
       const V3D detectorPos(20, 5, 0);
       const V3D beam1 = sample - source;
       const V3D beam2 = detectorPos - sample;
       auto minimalInstrument = ComponentCreationHelper::createMinimalInstrument( source, sample, detectorPos );

       // Derive distances and angles
       const double l1 = beam1.norm();
       const double l2 = beam2.norm();
       const V3D qLabDir = (beam1/l1) - (beam2/l2);

       const double microSecsInSec = 1e6;

       // Derive QLab for diffraction
       const double wavenumber_in_angstrom_times_tof_in_microsec =
           (Mantid::PhysicalConstants::NeutronMass * (l1 + l2) * 1e-10 * microSecsInSec) /
            Mantid::PhysicalConstants::h_bar;
       V3D qLab = qLabDir * wavenumber_in_angstrom_times_tof_in_microsec;

       Mantid::Geometry::OrientedLattice orientedLattice(1, 1, 1, 90, 90, 90); // U is identity, real and reciprocal lattice vectors are identical.
       Mantid::Geometry::Goniometer goniometer; // identity
       V3D hkl = qLab / (2 * M_PI); // Given our settings above, this is the simplified relationship between qLab and hkl.

       // Now create a peaks workspace around the simple fictional instrument
       PeaksWorkspace ws;
       ws.setInstrument(minimalInstrument);
       ws.mutableSample().setOrientedLattice(&orientedLattice);
       ws.mutableRun().setGoniometer(goniometer, false);

       // Create the peak
       Peak* peak = ws.createPeakHKL(hkl);

       /*
        Now we check we have made a self - consistent peak
        */
       TSM_ASSERT_EQUALS("New peak should have HKL we demanded.", hkl, peak->getHKL());
       TSM_ASSERT_EQUALS("New peak should have QLab we expected.", qLab, peak->getQLabFrame());
       TSM_ASSERT_EQUALS("QSample and QLab should be identical given the identity goniometer settings.", peak->getQLabFrame(), peak->getQSampleFrame());
       auto detector = peak->getDetector();
       TSM_ASSERT("No detector", detector);
       TSM_ASSERT_EQUALS("This detector id does not match what we expect from the instrument definition", 1, detector->getID());
       TSM_ASSERT_EQUALS("Thie detector position is wrong", detectorPos, detector->getPos());
       TSM_ASSERT_EQUALS("Goniometer has not been set properly", goniometer.getR(), peak->getGoniometerMatrix());

       // Clean up.
       delete peak;
   }

private:

   PeaksWorkspace_sptr createSaveTestPeaksWorkspace()
   {
     // Create peak workspace
     auto pw = buildPW();
     Instrument_const_sptr inst = pw->getInstrument();

     // Add peaks (one peak already at detector ID 1)
     Peak p1(inst, 10, 4.0);
     Peak p2(inst, 10, 5.0);
     Peak p3(inst, 20, 3.0);
     Peak p4(inst, 50, 3.0);
     pw->addPeak(p1);
     pw->addPeak(p2);
     pw->addPeak(p3);
     pw->addPeak(p4);

     return pw;
   }

   void check_Detector_Table_Metadata(const Mantid::API::ITableWorkspace & detTable, const size_t expectedNRows)
   {
     TS_ASSERT_EQUALS(expectedNRows, detTable.rowCount());
     TS_ASSERT_EQUALS(2, detTable.columnCount());
     if(detTable.columnCount() != 2) return;

     auto column0 = detTable.getColumn(0);
     auto column1 = detTable.getColumn(1);
     TS_ASSERT_EQUALS("Index", column0->name());
     TS_ASSERT_EQUALS("DetectorID", column1->name());
   }
};


#endif /* MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_ */

