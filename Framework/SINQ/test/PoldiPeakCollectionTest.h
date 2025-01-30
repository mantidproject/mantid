// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidSINQ/PoldiUtilities/PoldiPeak.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"
#include "MantidSINQ/PoldiUtilities/UncertainValueIO.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Crystal/BraggScattererFactory.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/ReflectionGenerator.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"

#include <stdexcept>

using namespace Mantid::Poldi;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace std::placeholders;

class PoldiPeakCollectionTest;

class TestablePoldiPeakCollection : public PoldiPeakCollection {
  friend class PoldiPeakCollectionTest;

  TestablePoldiPeakCollection() : PoldiPeakCollection() {}

  TestablePoldiPeakCollection(const TableWorkspace_sptr &workspace) : PoldiPeakCollection(workspace) {}
};

class PoldiPeakCollectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiPeakCollectionTest *createSuite() { return new PoldiPeakCollectionTest(); }
  static void destroySuite(PoldiPeakCollectionTest *suite) { delete suite; }

  PoldiPeakCollectionTest() {
    m_dummyData = std::dynamic_pointer_cast<TableWorkspace>(WorkspaceFactory::Instance().createTable());
    m_dummyData->addColumn("str", "HKL");
    m_dummyData->addColumn("double", "d");
    m_dummyData->addColumn("double", "delta d");
    m_dummyData->addColumn("double", "Q");
    m_dummyData->addColumn("double", "delta Q");
    m_dummyData->addColumn("double", "Intensity");
    m_dummyData->addColumn("double", "delta Intensity");
    m_dummyData->addColumn("double", "FWHM (rel.)");
    m_dummyData->addColumn("double", "delta FWHM (rel.)");

    TableRow first = m_dummyData->appendRow();
    first << "1 0 0" << 0.5 << 0.001 << 12.566370 << 0.02513274 << 2000. << 3. << 0.5 << 0.02;

    TableRow second = m_dummyData->appendRow();
    second << "1 1 0" << 0.8 << 0.004 << 7.853981 << 0.039269905 << 200. << 14. << 0.9 << 0.1;
  }

  void testConstruction() {
    TS_ASSERT_THROWS_NOTHING(PoldiPeakCollection newCollection);

    TS_ASSERT_THROWS_NOTHING(PoldiPeakCollection fromTable(m_dummyData));
  }

  void testTableImportExport() {
    PoldiPeakCollection fromTable(m_dummyData);
    TS_ASSERT_EQUALS(fromTable.peakCount(), 2);

    PoldiPeak_sptr first = fromTable.peak(0);
    TS_ASSERT_EQUALS(first->d(), 0.5);
    TS_ASSERT_EQUALS(first->d().error(), 0.001);
    TS_ASSERT_EQUALS(first->q(), 2.0 * M_PI / 0.5);
    TS_ASSERT_EQUALS(first->fwhm(PoldiPeak::Relative), 0.5);
    TS_ASSERT_EQUALS(first->fwhm(PoldiPeak::AbsoluteD), 0.25);

    TableWorkspace_sptr exported = fromTable.asTableWorkspace();
    TS_ASSERT_EQUALS(exported->columnCount(), 9);
    TS_ASSERT_EQUALS(exported->rowCount(), 2);

    TableRow secondRowReference = m_dummyData->getRow(1);
    TableRow secondRow = exported->getRow(1);

    TS_ASSERT_EQUALS(secondRow.cell<std::string>(0), secondRowReference.cell<std::string>(0));

    for (size_t i = 1; i < exported->columnCount(); ++i) {
      TS_ASSERT_DELTA(secondRow.cell<double>(i), secondRowReference.cell<double>(i), 1e-6);
    }
  }

  void testProfileFunctionName() {
    TestablePoldiPeakCollection collection;

    TS_ASSERT(collection.m_profileFunctionName.empty());

    collection.setProfileFunctionName("Gaussian");

    TS_ASSERT_EQUALS(collection.m_profileFunctionName, "Gaussian");
  }

  void testProfileFunctionRevovery() {
    TestablePoldiPeakCollection collection;
    collection.setProfileFunctionName("Gaussian");

    TableWorkspace_sptr table = collection.asTableWorkspace();

    TestablePoldiPeakCollection other(table);

    TS_ASSERT_EQUALS(other.getProfileFunctionName(), "Gaussian");
    TS_ASSERT(other.hasProfileFunctionName());
  }

  void testMissingProfileFunction() {
    TestablePoldiPeakCollection collection(m_dummyData);
    TS_ASSERT(!collection.hasProfileFunctionName());
    TS_ASSERT(collection.getProfileFunctionName().empty());
  }

  void testIntensityTypeFromString() {
    TestablePoldiPeakCollection collection;

    TS_ASSERT_EQUALS(collection.intensityTypeFromString("Maximum"), PoldiPeakCollection::Maximum);
    TS_ASSERT_EQUALS(collection.intensityTypeFromString("maximum"), PoldiPeakCollection::Maximum);
    TS_ASSERT_EQUALS(collection.intensityTypeFromString("mAxIMuM"), PoldiPeakCollection::Maximum);

    TS_ASSERT_EQUALS(collection.intensityTypeFromString("Integral"), PoldiPeakCollection::Integral);
    TS_ASSERT_EQUALS(collection.intensityTypeFromString("integral"), PoldiPeakCollection::Integral);
    TS_ASSERT_EQUALS(collection.intensityTypeFromString("InTEgrAl"), PoldiPeakCollection::Integral);

    TS_ASSERT_EQUALS(collection.intensityTypeFromString("Garbage"), PoldiPeakCollection::Maximum);
    TS_ASSERT_EQUALS(collection.intensityTypeFromString(""), PoldiPeakCollection::Maximum);
  }

  void testIntensityTypeToString() {
    TestablePoldiPeakCollection collection;
    TS_ASSERT_EQUALS(collection.intensityTypeToString(PoldiPeakCollection::Maximum), "Maximum");
    TS_ASSERT_EQUALS(collection.intensityTypeToString(PoldiPeakCollection::Integral), "Integral");
  }

  void testIntensityTypeRecovery() {
    PoldiPeakCollection collection(m_dummyData);

    TS_ASSERT_EQUALS(collection.intensityType(), PoldiPeakCollection::Maximum);

    ITableWorkspace_sptr newDummy(m_dummyData->clone().release());
    newDummy->logs()->addProperty<std::string>("IntensityType", "Integral");

    PoldiPeakCollection otherCollection(std::static_pointer_cast<TableWorkspace>(newDummy));
    TS_ASSERT_EQUALS(otherCollection.intensityType(), PoldiPeakCollection::Integral);
  }

  void testIntensityTypeRecoveryConversion() {
    ITableWorkspace_sptr newDummy(m_dummyData->clone().release());
    newDummy->logs()->addProperty<std::string>("IntensityType", "Integral");

    PoldiPeakCollection collection(std::static_pointer_cast<TableWorkspace>(newDummy));

    TableWorkspace_sptr compare = collection.asTableWorkspace();

    TS_ASSERT(compare->logs()->hasProperty("IntensityType"));
    TS_ASSERT_EQUALS(compare->logs()->getPropertyValueAsType<std::string>("IntensityType"), "Integral");

    PoldiPeakCollection otherCollection(compare);

    TS_ASSERT_EQUALS(otherCollection.intensityType(), PoldiPeakCollection::Integral);
  }

  void testPointGroup() {
    PoldiPeakCollection peaks;
    TS_ASSERT_EQUALS(peaks.pointGroup()->getSymbol(), "1");

    PointGroup_sptr m3m = PointGroupFactory::Instance().createPointGroup("m-3m");

    peaks.setPointGroup(m3m);
    TS_ASSERT_EQUALS(peaks.pointGroup()->getName(), m3m->getName());

    // It should not be the same instance.
    TS_ASSERT_DIFFERS(peaks.pointGroup(), m3m);

    PointGroup_sptr invalid;
    TS_ASSERT_THROWS(peaks.setPointGroup(invalid), const std::invalid_argument &);
  }

  void testUnitCell() {
    PoldiPeakCollection peaks;

    UnitCell defaultCell;
    TS_ASSERT_EQUALS(unitCellToStr(peaks.unitCell()), unitCellToStr(defaultCell));

    UnitCell cell(1, 2, 3, 90, 91, 92);
    peaks.setUnitCell(cell);

    UnitCell newCell = peaks.unitCell();
    TS_ASSERT_EQUALS(unitCellToStr(newCell), unitCellToStr(cell));
  }

  void testUnitCellFromLogs() {
    ITableWorkspace_sptr newDummy(m_dummyData->clone().release());

    UnitCell cell(1, 2, 3, 90, 91, 92);
    newDummy->logs()->addProperty<std::string>("UnitCell", unitCellToStr(cell));

    PoldiPeakCollection collection(std::static_pointer_cast<TableWorkspace>(newDummy));
    TS_ASSERT_EQUALS(unitCellToStr(collection.unitCell()), unitCellToStr(cell));
  }

  void testPointGroupStringConversion() {
    TestablePoldiPeakCollection peaks;
    PointGroup_sptr m3m = PointGroupFactory::Instance().createPointGroup("m-3m");

    TS_ASSERT(peaks.pointGroupFromString(peaks.pointGroupToString(m3m)));
    TS_ASSERT_EQUALS(m3m->getName(), peaks.pointGroupFromString(peaks.pointGroupToString(m3m))->getName());

    PointGroup_sptr one = PointGroupFactory::Instance().createPointGroup("1");
    TS_ASSERT_EQUALS(peaks.pointGroupFromString("DoesNotExist")->getSymbol(), one->getSymbol());
  }

  void testGetPointGroupStringFromLog() {
    ITableWorkspace_sptr newDummy(m_dummyData->clone().release());
    newDummy->logs()->addProperty<std::string>("PointGroup", "SomeString");

    TestablePoldiPeakCollection peaks;
    TS_ASSERT_EQUALS(peaks.getPointGroupStringFromLog(newDummy->logs()), "SomeString");
  }

  void testAddPeak() {
    PoldiPeakCollection peaks;
    peaks.addPeak(PoldiPeak::create(2.0));

    TS_ASSERT_EQUALS(peaks.peakCount(), 1);
  }

  void testPeakAccess() {
    PoldiPeakCollection peaks;
    PoldiPeak_sptr newPeak = PoldiPeak::create(2.0);
    peaks.addPeak(newPeak);

    PoldiPeak_sptr peak = peaks.peak(0);
    TS_ASSERT_EQUALS(peak, newPeak);

    TS_ASSERT_THROWS(peaks.peak(1), const std::range_error &);
    TS_ASSERT_THROWS(peaks.peak(-2), const std::range_error &);
  }

  void testPeaksVector() {
    PoldiPeakCollection fromTable(m_dummyData);
    std::vector<PoldiPeak_sptr> peaks = fromTable.peaks();

    // make sure that a copy of the vector is created
    peaks.clear();
    TS_ASSERT_EQUALS(fromTable.peakCount(), 2);
  }

  void testColumnCheckConsistency() {
    TestablePoldiPeakCollection peaks;

    auto newTable = std::make_shared<TableWorkspace>();
    peaks.prepareTable(newTable);

    TS_ASSERT(peaks.checkColumns(newTable));
  }

  void testClone() {
    PoldiPeakCollection_sptr peaks(new PoldiPeakCollection);
    peaks->setProfileFunctionName("Test");
    peaks->addPeak(PoldiPeak::create(2.0));
    peaks->addPeak(PoldiPeak::create(3.0));

    PoldiPeakCollection_sptr clone = peaks->clonePeakCollection();

    // make sure those are different instances
    TS_ASSERT(clone != peaks);

    // everything else should be identical
    TS_ASSERT_EQUALS(clone->getProfileFunctionName(), peaks->getProfileFunctionName());
    TS_ASSERT_EQUALS(clone->intensityType(), peaks->intensityType());
    TS_ASSERT_EQUALS(clone->peakCount(), peaks->peakCount());
    TS_ASSERT_EQUALS(unitCellToStr(clone->unitCell()), unitCellToStr(peaks->unitCell()));
    TS_ASSERT_EQUALS(clone->pointGroup()->getSymbol(), peaks->pointGroup()->getSymbol());

    for (size_t i = 0; i < clone->peakCount(); ++i) {
      PoldiPeak_sptr clonePeak = clone->peak(i);
      PoldiPeak_sptr peaksPeak = peaks->peak(i);

      TS_ASSERT(clonePeak != peaksPeak);
      TS_ASSERT_EQUALS(clonePeak->d(), peaksPeak->d());
    }
  }

  void testStructureConstructor() {
    CrystalStructure structure = getCsClStructure();

    double dMin = 0.55;
    double dMax = 5.0;

    /* The peak collection should contain all allowed symmetry independent HKLs
     * between 0.55 and 5.0 Angstrom, for the unit cell of CsCl
     * (Primitive cubic cell with a = 4.126 Angstrom, Pointgroup m-3m).
     */
    PoldiPeakCollection p(structure, dMin, dMax);

    TS_ASSERT_EQUALS(p.peakCount(), 69);

    PoldiPeak_sptr peak1 = p.peak(0);
    TS_ASSERT_EQUALS(peak1->hkl(), MillerIndices(1, 0, 0));
    TS_ASSERT_EQUALS(peak1->d(), 4.126);

    PoldiPeak_sptr peak68 = p.peak(68);
    TS_ASSERT_EQUALS(peak68->hkl(), MillerIndices(7, 2, 1));
    TS_ASSERT_DELTA(peak68->d(), 0.5615, 1e-4);

    std::vector<PoldiPeak_sptr> poldiPeaks = p.peaks();

    // sort peak list and check that all peaks are within the limits
    std::sort(poldiPeaks.begin(), poldiPeaks.end(), std::bind(&PoldiPeak::greaterThan, _1, _2, &PoldiPeak::d));

    TS_ASSERT_LESS_THAN_EQUALS(poldiPeaks[0]->d(), 5.0);
    TS_ASSERT_LESS_THAN_EQUALS(0.55, poldiPeaks[68]->d());
    TS_ASSERT_LESS_THAN(poldiPeaks[68]->d(), poldiPeaks[0]->d());
  }

  void testSetPeaks() {
    CrystalStructure structure = getCsClStructure();
    ReflectionGenerator generator(structure);

    double dMin = 0.55;
    double dMax = 5.0;

    std::vector<V3D> hkls = generator.getUniqueHKLs(dMin, dMax);
    std::vector<double> dValues = generator.getDValues(hkls);
    std::vector<double> fSquared(dValues.size(), 0.0);

    TestablePoldiPeakCollection p;

    p.setPointGroup(structure.spaceGroup()->getPointGroup());
    TS_ASSERT_THROWS_NOTHING(p.setPeaks(hkls, dValues, fSquared));

    dValues.pop_back();
    TS_ASSERT_THROWS(p.setPeaks(hkls, dValues, fSquared), const std::invalid_argument &);
  }

private:
  CrystalStructure getCsClStructure() {
    UnitCell CsCl(4.126, 4.126, 4.126);
    SpaceGroup_const_sptr Pm3m = SpaceGroupFactory::Instance().createSpaceGroup("P m -3 m");

    BraggScatterer_sptr cs = BraggScattererFactory::Instance().createScatterer(
        "IsotropicAtomBraggScatterer", R"({"Element":"Cs","Position":"0.5,0.5,0.5","U":"0.005"})");
    BraggScatterer_sptr cl = BraggScattererFactory::Instance().createScatterer(
        "IsotropicAtomBraggScatterer", R"({"Element":"Cl","Position":"0,0,0","U":"0.005"})");

    CompositeBraggScatterer_sptr atoms = CompositeBraggScatterer::create();
    atoms->addScatterer(cs);
    atoms->addScatterer(cl);

    return CrystalStructure(CsCl, Pm3m, atoms);
  }

  TableWorkspace_sptr m_dummyData;
};
