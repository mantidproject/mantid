// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//-------------------------------------------------------------------------------------------------
// This set of tests has been placed in DataObjects because it really needs to
// use a real workspace
//-------------------------------------------------------------------------------------------------
#ifndef REFAXISTEST_H_
#define REFAXISTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/RefAxis.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class RefAxisTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RefAxisTest *createSuite() { return new RefAxisTest(); }
  static void destroySuite(RefAxisTest *suite) { delete suite; }

  RefAxisTest() {
    // Set up two small workspaces for these tests
    m_space.reset(new Mantid::DataObjects::Workspace2D);
    m_space->initialize(5, 5, 5);
    m_space2.reset(new Mantid::DataObjects::Workspace2D);
    m_space2->initialize(1, 5, 5);

    // Fill them
    Mantid::MantidVec a(25);
    for (size_t i = 0; i < a.size(); ++i) {
      a[i] = static_cast<double>(i) + 0.1;
    }
    for (int j = 0; j < 5; ++j) {
      m_space->mutableX(j) =
          Mantid::MantidVec(a.cbegin() + (5 * j), a.cbegin() + (5 * j) + 5);
    }

    // Create the axis that the tests will be performed on
    m_refAxis.reset(new RefAxis(m_space.get()));
    m_refAxis->title() = "test axis";
    m_refAxis->unit() = UnitFactory::Instance().create("TOF");
  }

  void testConstructor() {
    TS_ASSERT_EQUALS(m_refAxis->title(), "test axis")
    TS_ASSERT(m_refAxis->isNumeric())
    TS_ASSERT(!m_refAxis->isSpectra())
    TS_ASSERT_EQUALS(m_refAxis->unit()->unitID(), "TOF")
    TS_ASSERT_THROWS(m_refAxis->spectraNo(0), std::domain_error)
  }

  void testClone() {
    std::unique_ptr<Axis> clonedAxis(m_refAxis->clone(m_space2.get()));
    TS_ASSERT_DIFFERS(clonedAxis.get(), m_refAxis.get())
    TS_ASSERT(clonedAxis)
    TS_ASSERT_EQUALS(clonedAxis->title(), "test axis")
    TS_ASSERT_EQUALS(clonedAxis->unit()->unitID(), "TOF")
    TS_ASSERT(clonedAxis->isNumeric())
    TS_ASSERT_EQUALS((*clonedAxis)(0, 0), 1.0)
    TS_ASSERT_THROWS((*clonedAxis)(0, 1), std::range_error)
  }

  void testCloneDifferentLength() {
    std::unique_ptr<Axis> newRefAxis(m_refAxis->clone(5, m_space2.get()));
    TS_ASSERT_DIFFERS(newRefAxis.get(), m_refAxis.get());
    TS_ASSERT(newRefAxis->isNumeric());
    TS_ASSERT_EQUALS(newRefAxis->title(), "test axis");
    TS_ASSERT_EQUALS(newRefAxis->unit()->unitID(), "TOF");
    TS_ASSERT_EQUALS(newRefAxis->length(), 5);
    m_space2->dataX(0)[1] = 9.9;
    TS_ASSERT_EQUALS((*newRefAxis)(1), 9.9);
  }

  void testOperatorBrackets() {
    TS_ASSERT_EQUALS((*m_refAxis)(4, 4), 24.1)
    TS_ASSERT_EQUALS((*m_refAxis)(0, 2), 10.1)
    TS_ASSERT_EQUALS((*m_refAxis)(2, 0), 2.1)

    TS_ASSERT_THROWS((*m_refAxis)(-1, 0), Exception::IndexError)
    TS_ASSERT_THROWS((*m_refAxis)(5, 0), Exception::IndexError)
    TS_ASSERT_THROWS((*m_refAxis)(0, -1), std::range_error)
    TS_ASSERT_THROWS((*m_refAxis)(0, 5), std::range_error)
  }

  void testSetValue() {
    TS_ASSERT_THROWS(m_refAxis->setValue(0, 9.9), std::domain_error)
  }

  void testGetMin() {
    std::unique_ptr<Axis> newRefAxis(m_refAxis->clone(5, m_space2.get()));
    TS_ASSERT_THROWS(newRefAxis->getMin(), std::runtime_error)
  }

  void testGetMax() {
    std::unique_ptr<Axis> newRefAxis(m_refAxis->clone(5, m_space2.get()));
    TS_ASSERT_THROWS(newRefAxis->getMax(), std::runtime_error)
  }

private:
  std::unique_ptr<MatrixWorkspace> m_space, m_space2;
  std::unique_ptr<RefAxis> m_refAxis;
};

#endif /*REFAXISTEST_H_*/
