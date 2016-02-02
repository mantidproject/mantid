#ifndef MANTID_DATAOBJECTS_MDHISTOWORKSPACETEST_H_
#define MANTID_DATAOBJECTS_MDHISTOWORKSPACETEST_H_

#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/VMD.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspaceIterator.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/scoped_array.hpp>
#include <boost/scoped_ptr.hpp>
#include <cxxtest/TestSuite.h>
#include "MantidAPI/ExperimentInfo.h"
#include "MantidKernel/Strings.h"
#include "PropertyManagerHelper.h"

using namespace Mantid::DataObjects;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid;

class MDHistoWorkspaceTest : public CxxTest::TestSuite {
private:
  /// Helper function to return the number of masked bins in a workspace. TODO:
  /// move helper into test helpers
  size_t getNumberMasked(Mantid::API::IMDWorkspace_sptr ws) {
    Mantid::API::IMDIterator *it = ws->createIterator(NULL);
    size_t numberMasked = 0;
    size_t counter = 0;
    for (; counter < it->getDataSize(); ++counter) {
      if (it->getIsMasked()) {
        ++numberMasked;
      }
      it->next(1);
    }
    delete it;
    return numberMasked;
  }
  /// Helper method returns the size of an element in the MDHistoWorkspace
  size_t sizeOfElement() { return (sizeof(double) * 3 + sizeof(bool)); }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDHistoWorkspaceTest *createSuite() {
    return new MDHistoWorkspaceTest();
  }
  static void destroySuite(MDHistoWorkspaceTest *suite) { delete suite; }

  MDHistoWorkspace_sptr two;
  MDHistoWorkspace_sptr three;

  /** Create some fake workspace, 5x5, with 2.0 and 3.0 each */
  MDHistoWorkspaceTest() {
    two = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.0, 2, 5, 10.0, 2.0);
    three = MDEventsTestHelper::makeFakeMDHistoWorkspace(3.0, 2, 5, 10.0, 3.0);
  }

  /** Check that a workspace has the right signal/error*/
  void checkWorkspace(MDHistoWorkspace_sptr ws, double expectedSignal,
                      double expectedErrorSquared,
                      double expectedNumEvents = 1.0) {
    for (size_t i = 0; i < ws->getNPoints(); i++) {
      TS_ASSERT_DELTA(ws->getSignalAt(i), expectedSignal, 1e-5);
      TS_ASSERT_DELTA(ws->getErrorAt(i), std::sqrt(expectedErrorSquared), 1e-5);
      TS_ASSERT_DELTA(ws->getNumEventsAt(i), expectedNumEvents, 1e-5);
    }
  }

  //--------------------------------------------------------------------------------------
  void test_constructor() {
    Mantid::Geometry::GeneralFrame frame("m", "m");
    MDHistoDimension_sptr dimX(
        new MDHistoDimension("X", "x", frame, -10, 10, 5));
    MDHistoDimension_sptr dimY(
        new MDHistoDimension("Y", "y", frame, -10, 10, 5));
    MDHistoDimension_sptr dimZ(
        new MDHistoDimension("Z", "z", frame, -10, 10, 5));
    MDHistoDimension_sptr dimT(
        new MDHistoDimension("T", "t", frame, -10, 10, 5));

    MDHistoWorkspace ws(dimX, dimY, dimZ, dimT);

    TS_ASSERT_EQUALS(ws.getNumDims(), 4);
    TS_ASSERT_EQUALS(ws.getNPoints(), 5 * 5 * 5 * 5);
    TS_ASSERT_EQUALS(ws.getMemorySize(), 5 * 5 * 5 * 5 * sizeOfElement());
    TS_ASSERT_EQUALS(ws.getXDimension(), dimX);
    TS_ASSERT_EQUALS(ws.getYDimension(), dimY);
    TS_ASSERT_EQUALS(ws.getZDimension(), dimZ);
    TS_ASSERT_EQUALS(ws.getTDimension(), dimT);

    // The values are cleared at the start
    for (size_t i = 0; i < ws.getNPoints(); i++) {
      TS_ASSERT(boost::math::isnan(ws.getSignalAt(i)));
      TS_ASSERT(boost::math::isnan(ws.getErrorAt(i)));
      TS_ASSERT(boost::math::isnan(ws.getSignalNormalizedAt(i)));
      TS_ASSERT(boost::math::isnan(ws.getErrorNormalizedAt(i)));
      TS_ASSERT(!ws.getIsMaskedAt(i));
    }

    // Setting and getting
    ws.setSignalAt(5, 2.3456);
    TS_ASSERT_DELTA(ws.getSignalAt(5), 2.3456, 1e-5);
    TS_ASSERT_DELTA(ws.getSignalNormalizedAt(5), 2.3456 / 256.0,
                    1e-5); // Cell volume is 256

    ws.setErrorSquaredAt(5, 1.234);
    TS_ASSERT_DELTA(ws.getErrorAt(5), sqrt(1.234), 1e-5);
    TS_ASSERT_DELTA(ws.getErrorNormalizedAt(5), sqrt(1.234) / 256.0,
                    1e-5); // Cell volume is 256

    std::vector<signal_t> data = ws.getSignalDataVector();
    TS_ASSERT_EQUALS(data.size(), 5 * 5 * 5 * 5);
    TS_ASSERT_DELTA(data[5], 2.3456, 1e-5);

    // Set a different value at every point
    for (size_t i = 0; i < ws.getNPoints(); ++i) {
      ws.setSignalAt(i, (signal_t)i);
      ws.setErrorSquaredAt(i, (signal_t)i);
    }

    // Test the 4 overloads of each method. Phew!
    TS_ASSERT_DELTA(ws.getSignalAt(1), 1.0, 1e-4);
    TS_ASSERT_DELTA(ws.getSignalAt(1, 2), 1.0 + 2 * 5.0, 1e-4);
    TS_ASSERT_DELTA(ws.getSignalAt(1, 2, 3), 1.0 + 2 * 5.0 + 3 * 25.0, 1e-4);
    TS_ASSERT_DELTA(ws.getSignalAt(1, 2, 3, 4),
                    1.0 + 2 * 5.0 + 3 * 25.0 + 4 * 125.0, 1e-4);
    TS_ASSERT_DELTA(ws.getErrorAt(1), sqrt(1.0), 1e-4);
    TS_ASSERT_DELTA(ws.getErrorAt(1, 2), sqrt(1.0 + 2 * 5.0), 1e-4);
    TS_ASSERT_DELTA(ws.getErrorAt(1, 2, 3), sqrt(1.0 + 2 * 5.0 + 3 * 25.0),
                    1e-4);
    TS_ASSERT_DELTA(ws.getErrorAt(1, 2, 3, 4),
                    sqrt(1.0 + 2 * 5.0 + 3 * 25.0 + 4 * 125.0), 1e-4);
    TS_ASSERT_DELTA(ws.getSignalNormalizedAt(1) * 256.0, 1.0, 1e-4);
    TS_ASSERT_DELTA(ws.getSignalNormalizedAt(1, 2) * 256.0, 1.0 + 2 * 5.0,
                    1e-4);
    TS_ASSERT_DELTA(ws.getSignalNormalizedAt(1, 2, 3) * 256.0,
                    1.0 + 2 * 5.0 + 3 * 25.0, 1e-4);
    TS_ASSERT_DELTA(ws.getSignalNormalizedAt(1, 2, 3, 4) * 256.0,
                    1.0 + 2 * 5.0 + 3 * 25.0 + 4 * 125.0, 1e-4);
    TS_ASSERT_DELTA(ws.getErrorNormalizedAt(1) * 256.0, sqrt(1.0), 1e-4);
    TS_ASSERT_DELTA(ws.getErrorNormalizedAt(1, 2) * 256.0, sqrt(1.0 + 2 * 5.0),
                    1e-4);
    TS_ASSERT_DELTA(ws.getErrorNormalizedAt(1, 2, 3) * 256.0,
                    sqrt(1.0 + 2 * 5.0 + 3 * 25.0), 1e-4);
    TS_ASSERT_DELTA(ws.getErrorNormalizedAt(1, 2, 3, 4) * 256.0,
                    sqrt(1.0 + 2 * 5.0 + 3 * 25.0 + 4 * 125.0), 1e-4);
  }

  //---------------------------------------------------------------------------------------------------
  /** Create a dense histogram with only 2 dimensions */
  void test_constructor_fewerDimensions() {
    Mantid::Geometry::GeneralFrame frame("m", "m");
    MDHistoDimension_sptr dimX(
        new MDHistoDimension("X", "x", frame, -10, 10, 5));
    MDHistoDimension_sptr dimY(
        new MDHistoDimension("Y", "y", frame, -10, 10, 5));

    MDHistoWorkspace ws(dimX, dimY);

    TS_ASSERT_EQUALS(ws.getNumDims(), 2);
    TS_ASSERT_EQUALS(ws.getNPoints(), 5 * 5);
    TS_ASSERT_EQUALS(ws.getMemorySize(), 5 * 5 * sizeOfElement());
    TS_ASSERT_EQUALS(ws.getXDimension(), dimX);
    TS_ASSERT_EQUALS(ws.getYDimension(), dimY);
    TS_ASSERT_THROWS_ANYTHING(ws.getZDimension());
    TS_ASSERT_THROWS_ANYTHING(ws.getTDimension());

    // Setting and getting
    ws.setSignalAt(5, 2.3456);
    TS_ASSERT_DELTA(ws.getSignalAt(5), 2.3456, 1e-5);

    ws.setErrorSquaredAt(5, 1.234);
    TS_ASSERT_DELTA(ws.getErrorAt(5), sqrt(1.234), 1e-5);

    std::vector<signal_t> data = ws.getSignalDataVector();
    TS_ASSERT_EQUALS(data.size(), 5 * 5);
    TS_ASSERT_DELTA(data[5], 2.3456, 1e-5);
  }

  //---------------------------------------------------------------------------------------------------
  /** Create a dense histogram with 7 dimensions */
  void test_constructor_MoreThanFourDimensions() {
    std::vector<MDHistoDimension_sptr> dimensions;
    Mantid::Geometry::GeneralFrame frame("m", "m");
    for (size_t i = 0; i < 7; i++) {
      dimensions.push_back(MDHistoDimension_sptr(
          new MDHistoDimension("Dim", "Dim", frame, -10, 10, 3)));
    }

    MDHistoWorkspace ws(dimensions);

    TS_ASSERT_EQUALS(ws.getNumDims(), 7);
    TS_ASSERT_EQUALS(ws.getNPoints(), 3 * 3 * 3 * 3 * 3 * 3 * 3);
    TS_ASSERT_EQUALS(ws.getMemorySize(), ws.getNPoints() * sizeOfElement());

    // Setting and getting
    ws.setSignalAt(5, 2.3456);
    TS_ASSERT_DELTA(ws.getSignalAt(5), 2.3456, 1e-5);

    ws.setErrorSquaredAt(5, 1.234);
    TS_ASSERT_DELTA(ws.getErrorAt(5), sqrt(1.234), 1e-5);

    std::vector<signal_t> data = ws.getSignalDataVector();
    TS_ASSERT_EQUALS(data.size(), 3 * 3 * 3 * 3 * 3 * 3 * 3);
    TS_ASSERT_DELTA(data[5], 2.3456, 1e-5);
  }

  class TestableMDHistoWorkspace : public MDHistoWorkspace {
  public:
    TestableMDHistoWorkspace(const MDHistoWorkspace &other)
        : MDHistoWorkspace(other) {}
  };

  //--------------------------------------------------------------------------------------
  void test_copy_constructor() {
    MDHistoWorkspace_sptr a =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 3.234);
    a->addExperimentInfo(ExperimentInfo_sptr(new ExperimentInfo()));
    for (size_t i = 0; i < a->getNPoints(); i++)
      a->setNumEventsAt(i, 123.);
    MDHistoWorkspace_sptr b(new TestableMDHistoWorkspace(*a));
    TS_ASSERT_EQUALS(b->getNumDims(), a->getNumDims());
    TS_ASSERT_EQUALS(b->getNPoints(), a->getNPoints());
    TS_ASSERT_EQUALS(b->getNumExperimentInfo(), a->getNumExperimentInfo());
    TS_ASSERT_EQUALS(b->displayNormalization(), a->displayNormalization());
    checkWorkspace(b, 1.23, 3.234, 123.);
  }

  //--------------------------------------------------------------------------------------
  void test_array_operator() {
    MDHistoWorkspace_sptr a =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 3.234);
    TS_ASSERT_DELTA((*a)[0], 1.23, 1e-5);
    TS_ASSERT_THROWS_ANYTHING((*a)[25]);
    TS_ASSERT_THROWS_ANYTHING((*a)[-1]);
  }

  //---------------------------------------------------------------------------------------------------
  void test_getVertexesArray_1D() {
    Mantid::Geometry::GeneralFrame frame("m", "m");
    MDHistoDimension_sptr dimX(
        new MDHistoDimension("X", "x", frame, -10, 10, 5));
    MDHistoWorkspace ws(dimX);
    size_t numVertices;
    coord_t *v1 = ws.getVertexesArray(0, numVertices);
    TS_ASSERT_EQUALS(numVertices, 2);
    TS_ASSERT_DELTA(v1[0], -10.0, 1e-5);
    TS_ASSERT_DELTA(v1[1], -6.0, 1e-5);
    delete[] v1;

    coord_t *v2 = ws.getVertexesArray(4, numVertices);
    TS_ASSERT_DELTA(v2[0], 6.0, 1e-5);
    TS_ASSERT_DELTA(v2[1], 10.0, 1e-5);
    delete[] v2;
  }

  //---------------------------------------------------------------------------------------------------
  void test_getVertexesArray_2D() {
    Mantid::Geometry::GeneralFrame frame("m", "m");
    MDHistoDimension_sptr dimX(
        new MDHistoDimension("X", "x", frame, -10, 10, 5));
    MDHistoDimension_sptr dimY(
        new MDHistoDimension("Y", "y", frame, -10, 10, 5));
    MDHistoWorkspace ws(dimX, dimY);
    size_t numVertices, i;

    boost::scoped_array<coord_t> v1(ws.getVertexesArray(0, numVertices));
    TS_ASSERT_EQUALS(numVertices, 4);
    i = 0 * 2;
    TS_ASSERT_DELTA(v1[i + 0], -10.0, 1e-5);
    TS_ASSERT_DELTA(v1[i + 1], -10.0, 1e-5);
    i = 3 * 2;
    TS_ASSERT_DELTA(v1[i + 0], -6.0, 1e-5);
    TS_ASSERT_DELTA(v1[i + 1], -6.0, 1e-5);
    // The opposite corner
    boost::scoped_array<coord_t> v2(ws.getVertexesArray(24, numVertices));
    i = 0 * 2;
    TS_ASSERT_DELTA(v2[i + 0], 6.0, 1e-5);
    TS_ASSERT_DELTA(v2[i + 1], 6.0, 1e-5);
    i = 3 * 2;
    TS_ASSERT_DELTA(v2[i + 0], 10.0, 1e-5);
    TS_ASSERT_DELTA(v2[i + 1], 10.0, 1e-5);
  }

  //---------------------------------------------------------------------------------------------------
  void test_getVertexesArray_3D() {
    Mantid::Geometry::GeneralFrame frame("m", "m");
    MDHistoDimension_sptr dimX(
        new MDHistoDimension("X", "x", frame, -10, 10, 5));
    MDHistoDimension_sptr dimY(
        new MDHistoDimension("Y", "y", frame, -9, 10, 5));
    MDHistoDimension_sptr dimZ(
        new MDHistoDimension("Z", "z", frame, -8, 10, 5));
    MDHistoWorkspace ws(dimX, dimY, dimZ);
    size_t numVertices, i;

    boost::scoped_array<coord_t> v(ws.getVertexesArray(0, numVertices));
    TS_ASSERT_EQUALS(numVertices, 8);
    i = 0;
    TS_ASSERT_DELTA(v[i + 0], -10.0, 1e-5);
    TS_ASSERT_DELTA(v[i + 1], -9.0, 1e-5);
    TS_ASSERT_DELTA(v[i + 2], -8.0, 1e-5);
  }

  //---------------------------------------------------------------------------------------------------
  void test_getCenter_3D() {
    Mantid::Geometry::GeneralFrame frame("m", "m");
    MDHistoDimension_sptr dimX(
        new MDHistoDimension("X", "x", frame, -10, 10, 20));
    MDHistoDimension_sptr dimY(
        new MDHistoDimension("Y", "y", frame, -9, 10, 19));
    MDHistoDimension_sptr dimZ(
        new MDHistoDimension("Z", "z", frame, -8, 10, 18));
    MDHistoWorkspace ws(dimX, dimY, dimZ);
    VMD v = ws.getCenter(0);
    TS_ASSERT_DELTA(v[0], -9.5, 1e-5);
    TS_ASSERT_DELTA(v[1], -8.5, 1e-5);
    TS_ASSERT_DELTA(v[2], -7.5, 1e-5);
  }

  //---------------------------------------------------------------------------------------------------
  /** Test for a possible seg-fault if nx != ny etc. */
  void test_uneven_numbers_of_bins() {
    Mantid::Geometry::GeneralFrame frame("m", "m");
    MDHistoDimension_sptr dimX(
        new MDHistoDimension("X", "x", frame, -10, 10, 5));
    MDHistoDimension_sptr dimY(
        new MDHistoDimension("Y", "y", frame, -10, 10, 10));
    MDHistoDimension_sptr dimZ(
        new MDHistoDimension("Z", "z", frame, -10, 10, 20));
    MDHistoDimension_sptr dimT(
        new MDHistoDimension("T", "t", frame, -10, 10, 10));

    MDHistoWorkspace ws(dimX, dimY, dimZ, dimT);

    TS_ASSERT_EQUALS(ws.getNumDims(), 4);
    TS_ASSERT_EQUALS(ws.getNPoints(), 5 * 10 * 20 * 10);
    TS_ASSERT_EQUALS(ws.getMemorySize(), 5 * 10 * 20 * 10 * sizeOfElement());

    // Setting and getting
    size_t index = 5 * 10 * 20 * 10 - 1; // The last point
    ws.setSignalAt(index, 2.3456);
    TS_ASSERT_DELTA(ws.getSignalAt(index), 2.3456, 1e-5);

    // Getter with all indices
    TS_ASSERT_DELTA(ws.getSignalAt(4, 9, 19, 9), 2.3456, 1e-5);

    // check shapes
    TS_ASSERT_EQUALS(5, ws.getDimension(0)->getNBins());
    TS_ASSERT_EQUALS(10, ws.getDimension(1)->getNBins());
    TS_ASSERT_EQUALS(20, ws.getDimension(2)->getNBins());
    TS_ASSERT_EQUALS(10, ws.getDimension(3)->getNBins());

    auto *binWidth = ws.getBinWidths();
    TS_ASSERT_DELTA(20. / 5, *(binWidth + 0), 1.e-5);
    TS_ASSERT_DELTA(20. / 10, *(binWidth + 1), 1.e-5);
    TS_ASSERT_DELTA(20. / 20, *(binWidth + 2), 1.e-5);
    TS_ASSERT_DELTA(20. / 10, *(binWidth + 3), 1.e-5);
  }

  //---------------------------------------------------------------------------------------------------
  void test_createIterator() {
    Mantid::Geometry::GeneralFrame frame("m", "m");
    MDHistoDimension_sptr dimX(
        new MDHistoDimension("X", "x", frame, -10, 10, 10));
    MDHistoDimension_sptr dimY(
        new MDHistoDimension("Y", "y", frame, -9, 10, 10));
    MDHistoDimension_sptr dimZ(
        new MDHistoDimension("Z", "z", frame, -8, 10, 10));
    MDHistoWorkspace ws(dimX, dimY, dimZ);
    IMDIterator *it = ws.createIterator();
    TS_ASSERT(it);
    MDHistoWorkspaceIterator *hwit =
        dynamic_cast<MDHistoWorkspaceIterator *>(it);
    TS_ASSERT(hwit);
    TS_ASSERT(it->next());
    delete it;
    boost::scoped_ptr<MDImplicitFunction> mdfunction(new MDImplicitFunction);
    it = ws.createIterator(mdfunction.get());
    TS_ASSERT(it);
    delete it;
  }

  //---------------------------------------------------------------------------------------------------
  // Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetNonIntegratedDimensions() {
    Mantid::Geometry::GeneralFrame frame("m", "m");
    MDHistoDimension_sptr dimX(
        new MDHistoDimension("X", "x", frame, -10, 10, 1)); // Integrated.
    MDHistoDimension_sptr dimY(
        new MDHistoDimension("Y", "y", frame, -10, 10, 10));
    MDHistoDimension_sptr dimZ(
        new MDHistoDimension("Z", "z", frame, -10, 10, 20));
    MDHistoDimension_sptr dimT(
        new MDHistoDimension("T", "t", frame, -10, 10, 10));

    MDHistoWorkspace ws(dimX, dimY, dimZ, dimT);
    Mantid::Geometry::VecIMDDimension_const_sptr vecNonIntegratedDims =
        ws.getNonIntegratedDimensions();
    TSM_ASSERT_EQUALS("Only 3 of the 4 dimensions should be non-integrated", 3,
                      vecNonIntegratedDims.size());
    TSM_ASSERT_EQUALS("First non-integrated dimension should be Y", "y",
                      vecNonIntegratedDims[0]->getDimensionId());
    TSM_ASSERT_EQUALS("Second non-integrated dimension should be Z", "z",
                      vecNonIntegratedDims[1]->getDimensionId());
    TSM_ASSERT_EQUALS("Third non-integrated dimension should be T", "t",
                      vecNonIntegratedDims[2]->getDimensionId());
  }

  //---------------------------------------------------------------------------------------------------
  void test_getGeometryXML() {
    // If POCO xml supported schema validation, we wouldn't need to check xml
    // outputs like this.
    std::string expectedXML =
        std::string("<DimensionSet>") + "<Dimension ID=\"x\">" +
        "<Name>X</Name>" + "<Units>m</Units>" +
        "<Frame>My General Frame</Frame>" +
        "<UpperBounds>10.0000</UpperBounds>" +
        "<LowerBounds>-10.0000</LowerBounds>" +
        "<NumberOfBins>5</NumberOfBins>" + "</Dimension>" +
        "<Dimension ID=\"y\">" + "<Name>Y</Name>" + "<Units>m</Units>" +
        "<Frame>My General Frame</Frame>" +
        "<UpperBounds>10.0000</UpperBounds>" +
        "<LowerBounds>-10.0000</LowerBounds>" +
        "<NumberOfBins>5</NumberOfBins>" + "</Dimension>" +
        "<Dimension ID=\"z\">" + "<Name>Z</Name>" + "<Units>m</Units>" +
        "<Frame>My General Frame</Frame>" +
        "<UpperBounds>10.0000</UpperBounds>" +
        "<LowerBounds>-10.0000</LowerBounds>" +
        "<NumberOfBins>5</NumberOfBins>" + "</Dimension>" +
        "<Dimension ID=\"t\">" + "<Name>T</Name>" + "<Units>m</Units>" +
        "<Frame>My General Frame</Frame>" +
        "<UpperBounds>10.0000</UpperBounds>" +
        "<LowerBounds>-10.0000</LowerBounds>" +
        "<NumberOfBins>5</NumberOfBins>" + "</Dimension>" + "<XDimension>" +
        "<RefDimensionId>x</RefDimensionId>" + "</XDimension>" +
        "<YDimension>" + "<RefDimensionId>y</RefDimensionId>" +
        "</YDimension>" + "<ZDimension>" +
        "<RefDimensionId>z</RefDimensionId>" + "</ZDimension>" +
        "<TDimension>" + "<RefDimensionId>t</RefDimensionId>" +
        "</TDimension>" + "</DimensionSet>";
    Mantid::Geometry::GeneralFrame frame("My General Frame", "m");
    MDHistoDimension_sptr dimX(
        new MDHistoDimension("X", "x", frame, -10, 10, 5));
    MDHistoDimension_sptr dimY(
        new MDHistoDimension("Y", "y", frame, -10, 10, 5));
    MDHistoDimension_sptr dimZ(
        new MDHistoDimension("Z", "z", frame, -10, 10, 5));
    MDHistoDimension_sptr dimT(
        new MDHistoDimension("T", "t", frame, -10, 10, 5));

    MDHistoWorkspace ws(dimX, dimY, dimZ, dimT);

    std::string actualXML = ws.getGeometryXML();
    TS_ASSERT_EQUALS(expectedXML, actualXML);
  }

  //---------------------------------------------------------------------------------------------------
  void test_getNumEvents() {
    MDHistoWorkspace_sptr ws =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2, 10);
    ws->setNumEventsAt(0, 123);
    ws->setNumEventsAt(1, 345);
    TS_ASSERT_DELTA(ws->getNumEventsAt(0), 123, 1e-6);
    TS_ASSERT_DELTA(ws->getNumEventsAt(1), 345, 1e-6);
  }

  //---------------------------------------------------------------------------------------------------
  void test_getSignalAtCoord() {
    // 2D workspace with signal[i] = i (linear index)
    MDHistoWorkspace_sptr ws =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2, 10);
    for (size_t i = 0; i < 100; i++)
      ws->setSignalAt(i, double(i));
    IMDWorkspace_sptr iws(ws);
    TS_ASSERT_DELTA(iws->getSignalAtVMD(VMD(0.5, 0.5)), 0.0, 1e-6);
    TS_ASSERT_DELTA(iws->getSignalAtVMD(VMD(1.5, 0.5)), 1.0, 1e-6);
    TS_ASSERT_DELTA(iws->getSignalAtVMD(VMD(1.5, 1.5)), 11.0, 1e-6);
    TS_ASSERT_DELTA(iws->getSignalAtVMD(VMD(9.5, 9.5)), 99.0, 1e-6);
    // Out of range = NaN
    TS_ASSERT(boost::math::isnan(iws->getSignalAtVMD(VMD(-0.01, 2.5))));
    TS_ASSERT(boost::math::isnan(iws->getSignalAtVMD(VMD(3.5, -0.02))));
    TS_ASSERT(boost::math::isnan(iws->getSignalAtVMD(VMD(10.01, 2.5))));
    TS_ASSERT(boost::math::isnan(iws->getSignalAtVMD(VMD(3.5, 10.02))));
  }

  //---------------------------------------------------------------------------------------------------
  void test_getSignalAtCoord_withNormalization() {
    // 2D workspace with signal[i] = i (linear index)
    MDHistoWorkspace_sptr ws =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2, 10, 20);
    for (size_t i = 0; i < 100; i++) {
      ws->setSignalAt(i, double(i));
      ws->setNumEventsAt(i, 10.0);
    }
    IMDWorkspace_sptr iws(ws);
    TS_ASSERT_DELTA(iws->getSignalAtVMD(VMD(0.5, 0.5)), 0.0, 1e-6);
    TS_ASSERT_DELTA(iws->getSignalAtVMD(VMD(3.5, 0.5), NoNormalization), 1.0,
                    1e-6);
    TS_ASSERT_DELTA(iws->getSignalAtVMD(VMD(3.5, 0.5), VolumeNormalization),
                    0.25, 1e-6);
    TS_ASSERT_DELTA(iws->getSignalAtVMD(VMD(3.5, 0.5), NumEventsNormalization),
                    0.1, 1e-6);
  }

  //---------------------------------------------------------------------------------------------------
  void test_getSignalWithMaskAtVMD() {
    // 2D workspace with signal[i] = i (linear index)
    MDHistoWorkspace_sptr ws =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2, 10, 20);
    for (size_t i = 0; i < 100; i++) {
      ws->setSignalAt(i, double(i));
      ws->setNumEventsAt(i, 10.0);
    }

    std::vector<coord_t> min;
    std::vector<coord_t> max;
    min.push_back(0);
    min.push_back(0);
    max.push_back(5);
    max.push_back(5);

    // Mask part of the workspace
    MDImplicitFunction *function = new MDBoxImplicitFunction(min, max);
    ws->setMDMasking(function);

    IMDWorkspace_sptr iws(ws);

    // Testing with isnan() as following commented line doesn't work
    // when MDMaskValue is NaN.
    // TS_ASSERT_DELTA(iws->getSignalWithMaskAtVMD(VMD(0.5, 0.5)), MDMaskValue,
    // 1e-6);
    TS_ASSERT(boost::math::isnan(iws->getSignalAtVMD(VMD(0.5, 0.5))));
    TS_ASSERT(boost::math::isnan(iws->getSignalWithMaskAtVMD(VMD(0.5, 0.5))));

    TS_ASSERT(boost::math::isnan(
        iws->getSignalAtVMD(VMD(3.5, 0.5), VolumeNormalization)));
    TS_ASSERT(boost::math::isnan(
        iws->getSignalWithMaskAtVMD(VMD(3.5, 0.5), VolumeNormalization)));
  }

  //---------------------------------------------------------------------------------------------------
  /** Line along X, going positive */
  void test_getLinePlot_horizontal() {
    MDHistoWorkspace_sptr ws =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2, 10);
    for (size_t i = 0; i < 100; i++)
      ws->setSignalAt(i, double(i));
    VMD start(0.5, 0.5);
    VMD end(9.5, 0.5);
    std::vector<coord_t> x;
    std::vector<signal_t> y;
    std::vector<signal_t> e;
    ws->getLinePlot(start, end, NoNormalization, x, y, e);
    TS_ASSERT_EQUALS(x.size(), 11);
    TS_ASSERT_DELTA(x[0], 0.0, 1e-5);
    TS_ASSERT_DELTA(x[1], 0.5, 1e-5);
    TS_ASSERT_DELTA(x[2], 1.5, 1e-5);
    TS_ASSERT_DELTA(x[10], 9.0, 1e-5);

    TS_ASSERT_EQUALS(y.size(), 10);
    TS_ASSERT_DELTA(y[0], 0.0, 1e-5);
    TS_ASSERT_DELTA(y[1], 1.0, 1e-5);
    TS_ASSERT_DELTA(y[2], 2.0, 1e-5);
  }

  //---------------------------------------------------------------------------------------------------
  /** Line along X, going positive */
  void test_getLinePlot_horizontal_withMask() {
    MDHistoWorkspace_sptr ws =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2, 10);
    for (size_t i = 0; i < 100; i++)
      ws->setSignalAt(i, double(i));

    std::vector<coord_t> min{0, 0};
    std::vector<coord_t> max{5, 5};

    // Mask part of the workspace
    MDImplicitFunction *function = new MDBoxImplicitFunction(min, max);
    ws->setMDMasking(function);

    VMD start(0.5, 0.5);
    VMD end(9.5, 0.5);
    std::vector<coord_t> x;
    std::vector<signal_t> y;
    std::vector<signal_t> e;
    ws->getLinePlot(start, end, NoNormalization, x, y, e);

    // Masked values should be omitted
    TS_ASSERT_EQUALS(y.size(), 5);

    // Unmasked values
    TS_ASSERT_DELTA(y[0], 5.0, 1e-5);
    TS_ASSERT_DELTA(y[1], 6.0, 1e-5);
    TS_ASSERT_DELTA(y[2], 7.0, 1e-5);
    TS_ASSERT_DELTA(y[3], 8.0, 1e-5);
    TS_ASSERT_DELTA(y[4], 9.0, 1e-5);

    TS_ASSERT_EQUALS(x.size(), 6);
    TS_ASSERT_DELTA(x[0], 4.5, 1e-5);
    TS_ASSERT_DELTA(x[1], 5.5, 1e-5);
    TS_ASSERT_DELTA(x[2], 6.5, 1e-5);
  }

  //---------------------------------------------------------------------------------------------------
  /** Line along X, going positive */
  void test_getLinePlot_3D() {
    MDHistoWorkspace_sptr ws =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3, 10);
    for (size_t i = 0; i < 1000; i++)
      ws->setSignalAt(i, double(i));
    VMD start(0.5, 0.5, 0.5);
    VMD end(9.5, 0.5, 0.5);
    std::vector<coord_t> x;
    std::vector<signal_t> y;
    std::vector<signal_t> e;
    ws->getLinePlot(start, end, NoNormalization, x, y, e);
    TS_ASSERT_EQUALS(x.size(), 11);
    TS_ASSERT_DELTA(x[0], 0.0, 1e-5);
    TS_ASSERT_DELTA(x[1], 0.5, 1e-5);
    TS_ASSERT_DELTA(x[2], 1.5, 1e-5);
    TS_ASSERT_DELTA(x[10], 9.0, 1e-5);

    TS_ASSERT_EQUALS(y.size(), 10);
    TS_ASSERT_DELTA(y[0], 0.0, 1e-5);
    TS_ASSERT_DELTA(y[1], 1.0, 1e-5);
    TS_ASSERT_DELTA(y[2], 2.0, 1e-5);
  }

  //---------------------------------------------------------------------------------------------------
  /** Line along X, going negative */
  void test_getLinePlot_horizontal_backwards() {
    MDHistoWorkspace_sptr ws =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2, 10);
    for (size_t i = 0; i < 100; i++)
      ws->setSignalAt(i, double(i));
    VMD start(9.5, 0.5);
    VMD end(0.5, 0.5);
    std::vector<coord_t> x;
    std::vector<signal_t> y;
    std::vector<signal_t> e;
    ws->getLinePlot(start, end, NoNormalization, x, y, e);
    TS_ASSERT_EQUALS(x.size(), 11);
    TS_ASSERT_DELTA(x[0], 0.0, 1e-5);
    TS_ASSERT_DELTA(x[1], 0.5, 1e-5);
    TS_ASSERT_DELTA(x[2], 1.5, 1e-5);
    TS_ASSERT_DELTA(x[10], 9.0, 1e-5);

    TS_ASSERT_EQUALS(y.size(), 10);
    TS_ASSERT_DELTA(y[0], 9.0, 1e-5);
    TS_ASSERT_DELTA(y[1], 8.0, 1e-5);
    TS_ASSERT_DELTA(y[2], 7.0, 1e-5);
  }

  //---------------------------------------------------------------------------------------------------
  /** Diagonal line at 45 degrees crosses through 3 bins */
  void test_getLinePlot_diagonal() {
    MDHistoWorkspace_sptr ws =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2, 10);
    for (size_t i = 0; i < 100; i++)
      ws->setSignalAt(i, double(i));
    VMD start(0.9, 0.5);
    VMD end(1.9, 1.5);
    std::vector<coord_t> x;
    std::vector<signal_t> y;
    std::vector<signal_t> e;
    ws->getLinePlot(start, end, NoNormalization, x, y, e);
    std::cout << "X\n" << Strings::join(x.begin(), x.end(), ",") << std::endl;
    std::cout << "Y\n" << Strings::join(y.begin(), y.end(), ",") << std::endl;

    TS_ASSERT_EQUALS(x.size(), 4);
    TS_ASSERT_DELTA(x[0], 0.0, 1e-5);
    TS_ASSERT_DELTA(x[1], 0.1 * sqrt(2.0), 1e-5);
    TS_ASSERT_DELTA(x[2], 0.5 * sqrt(2.0), 1e-5);
    TS_ASSERT_DELTA(x[3], 1.0 * sqrt(2.0), 1e-5);

    TS_ASSERT_EQUALS(y.size(), 3);
    TS_ASSERT_DELTA(y[0], 0.0, 1e-5);
    TS_ASSERT_DELTA(y[1], 1.0, 1e-5);
    TS_ASSERT_DELTA(y[2], 11.0, 1e-5);
  }

  //---------------------------------------------------------------------------------------------------
  /** Line along X, going positive, starting before and ending after limits */
  void test_getLinePlot_horizontal_pastEdges() {
    MDHistoWorkspace_sptr ws =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2, 10);
    for (size_t i = 0; i < 100; i++)
      ws->setSignalAt(i, double(i));
    VMD start(-0.5, 0.5);
    VMD end(10.5, 0.5);
    std::vector<coord_t> x;
    std::vector<signal_t> y;
    std::vector<signal_t> e;
    ws->getLinePlot(start, end, NoNormalization, x, y, e);
    TS_ASSERT_EQUALS(x.size(), 11);
    TS_ASSERT_DELTA(x[0], 0.5, 1e-5);
    TS_ASSERT_DELTA(x[1], 1.5, 1e-5);
    TS_ASSERT_DELTA(x[2], 2.5, 1e-5);
    TS_ASSERT_DELTA(x[10], 10.5, 1e-5);

    TS_ASSERT_EQUALS(y.size(), 10);
    TS_ASSERT_DELTA(y[0], 0.0, 1e-5);
    TS_ASSERT_DELTA(y[1], 1.0, 1e-5);
    TS_ASSERT_DELTA(y[2], 2.0, 1e-5);
  }

  //---------------------------------------------------------------------------------------------------
  /** Line that completely misses the workspace */
  void test_getLinePlot_totallyOutOfBounds() {
    MDHistoWorkspace_sptr ws =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2, 10);
    for (size_t i = 0; i < 100; i++)
      ws->setSignalAt(i, double(i));
    VMD start(-5, 5);
    VMD end(1, 20.5);
    std::vector<coord_t> x;
    std::vector<signal_t> y;
    std::vector<signal_t> e;
    ws->getLinePlot(start, end, NoNormalization, x, y, e);
    TS_ASSERT_EQUALS(x.size(), 2);
    TS_ASSERT_DELTA(x[0], 0, 1e-5);
    // NAN for Y
    TS_ASSERT_EQUALS(y.size(), 1);
    TS_ASSERT(y[0] != y[0]);
  }

  //--------------------------------------------------------------------------------------
  void test_plus_ws() {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        2.0, 2, 5, 10.0, 2.5 /*errorSquared*/);
    MDHistoWorkspace_sptr b = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        3.0, 2, 5, 10.0, 3.5 /*errorSquared*/);
    *a += *b;
    checkWorkspace(a, 5.0, 6.0, 2.0);
  }

  void test_plus_scalar() {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        2.0, 2, 5, 10.0, 2.5 /*errorSquared*/);
    a->add(3.0, sqrt(3.5));
    checkWorkspace(a, 5.0, 6.0, 1.0);
  }

  //--------------------------------------------------------------------------------------
  void test_minus_ws() {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        3.0, 2, 5, 10.0, 2.5 /*errorSquared*/);
    MDHistoWorkspace_sptr b = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        2.0, 2, 5, 10.0, 3.5 /*errorSquared*/);
    *a -= *b;
    checkWorkspace(a, 1.0, 6.0, 2.0);
  }

  void test_minus_scalar() {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        3.0, 2, 5, 10.0, 2.5 /*errorSquared*/);
    a->subtract(2.0, sqrt(3.5));
    checkWorkspace(a, 1.0, 6.0, 1.0);
  }

  //--------------------------------------------------------------------------------------
  void test_times_ws() {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        2.0, 2, 5, 10.0, 2.0 /*errorSquared*/, "", 2.0);
    MDHistoWorkspace_sptr b = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        3.0, 2, 5, 10.0, 3.0 /*errorSquared*/, "", 3.0);
    *a *= *b;
    checkWorkspace(a, 6.0, 36. * (.5 + 1. / 3.), 2.0);
  }

  //--------------------------------------------------------------------------------------
  void test_times_scalar() {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        2.0, 2, 5, 10.0, 2.0 /*errorSquared*/);
    a->multiply(3.0, sqrt(3.0));
    checkWorkspace(a, 6.0, 36. * (.5 + 1. / 3.), 1.0);
    // Scalar without error
    MDHistoWorkspace_sptr d = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        2.0, 2, 5, 10.0, 2.0 /*errorSquared*/);
    WorkspaceSingleValue e(3.0, 0);
    d->multiply(3.0, 0);
    checkWorkspace(d, 6.0, 9 * 2.0, 1.0);
  }

  //--------------------------------------------------------------------------------------
  void test_divide_ws() {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        3.0, 2, 5, 10.0, 3.0 /*errorSquared*/);
    MDHistoWorkspace_sptr b = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        2.0, 2, 5, 10.0, 2.0 /*errorSquared*/);
    *a /= *b;
    checkWorkspace(a, 1.5, 1.5 * 1.5 * (.5 + 1. / 3.));
  }

  //--------------------------------------------------------------------------------------
  void test_divide_scalar() {
    MDHistoWorkspace_sptr a = MDEventsTestHelper::makeFakeMDHistoWorkspace(
        3.0, 2, 5, 10.0, 3.0 /*errorSquared*/);
    a->divide(2.0, sqrt(2.0));
    checkWorkspace(a, 1.5, 1.5 * 1.5 * (.5 + 1. / 3.), 1.0);
  }

  //--------------------------------------------------------------------------------------
  void test_exp() {
    MDHistoWorkspace_sptr a =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(2.0, 2, 5, 10.0, 3.0);
    a->exp();
    checkWorkspace(a, std::exp(2.0), std::exp(2.0) * std::exp(2.0) * 3.0, 1.0);
  }

  //--------------------------------------------------------------------------------------
  void test_log() {
    MDHistoWorkspace_sptr a =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(2.71828, 2, 5, 10.0, 3.0);
    a->log();
    checkWorkspace(a, 1.0, 3.0 / (2.71828 * 2.71828), 1.0);
  }

  //--------------------------------------------------------------------------------------
  void test_log10() {
    MDHistoWorkspace_sptr a =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(10.0, 2, 5, 10.0, 3.0);
    a->log10();
    checkWorkspace(a, 1.0, 0.1886117 * 3. / 100., 1.0);
  }

  //--------------------------------------------------------------------------------------
  void test_power() {
    MDHistoWorkspace_sptr a =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(2.0, 2, 5, 10.0, 3.0);
    a->power(2.);
    checkWorkspace(a, 4.0, 16 * 4 * 3. / 4., 1.0);
  }

  //--------------------------------------------------------------------------------------
  void test_boolean_and() {
    MDHistoWorkspace_sptr a =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 3.0);
    MDHistoWorkspace_sptr b =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(2.34, 2, 5, 10.0, 2.0);
    MDHistoWorkspace_sptr c =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(0.00, 2, 5, 10.0, 2.0);
    *a &= *b;
    checkWorkspace(a, 1.0, 0.0);
    *b &= *c;
    checkWorkspace(b, 0.0, 0.0);
  }

  //--------------------------------------------------------------------------------------
  void test_boolean_or() {
    MDHistoWorkspace_sptr a =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 3.0);
    MDHistoWorkspace_sptr b =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(2.34, 2, 5, 10.0, 2.0);
    MDHistoWorkspace_sptr c =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(0.00, 2, 5, 10.0, 2.0);
    *a |= *b;
    checkWorkspace(a, 1.0, 0.0);
    *b |= *c;
    checkWorkspace(b, 1.0, 0.0);
    *c |= *c;
    checkWorkspace(c, 0.0, 0.0);
  }

  //--------------------------------------------------------------------------------------
  void test_boolean_xor() {
    MDHistoWorkspace_sptr a =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 3.0);
    MDHistoWorkspace_sptr b =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(2.34, 2, 5, 10.0, 2.0);
    MDHistoWorkspace_sptr c =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(0.00, 2, 5, 10.0, 2.0);
    *a ^= *b;
    checkWorkspace(a, 0.0, 0.0);
    *b ^= *c;
    checkWorkspace(b, 1.0, 0.0);
    *c ^= *c;
    checkWorkspace(c, 0.0, 0.0);
  }

  //--------------------------------------------------------------------------------------
  void test_boolean_operatorNot() {
    MDHistoWorkspace_sptr a =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 3.0);
    MDHistoWorkspace_sptr b =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(0.00, 2, 5, 10.0, 2.0);
    a->operatorNot();
    checkWorkspace(a, 0.0, 0.0);
    b->operatorNot();
    checkWorkspace(b, 1.0, 0.0);
  }

  //--------------------------------------------------------------------------------------
  void test_boolean_lessThan() {
    MDHistoWorkspace_sptr a, b, c;
    a = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 3.0);
    b = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.34, 2, 5, 10.0, 2.0);
    a->lessThan(*b);
    checkWorkspace(a, 1.0, 0.0);
    a = MDEventsTestHelper::makeFakeMDHistoWorkspace(4.56, 2, 5, 10.0, 3.0);
    b = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.34, 2, 5, 10.0, 2.0);
    a->lessThan(*b);
    checkWorkspace(a, 0.0, 0.0);
    a = MDEventsTestHelper::makeFakeMDHistoWorkspace(4.56, 2, 5, 10.0, 3.0);
    a->lessThan(4.57);
    checkWorkspace(a, 1.0, 0.0);
    a = MDEventsTestHelper::makeFakeMDHistoWorkspace(4.56, 2, 5, 10.0, 3.0);
    a->lessThan(4.55);
    checkWorkspace(a, 0.0, 0.0);
  }

  //--------------------------------------------------------------------------------------
  void test_boolean_greaterThan() {
    MDHistoWorkspace_sptr a, b, c;
    a = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 3.0);
    b = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.34, 2, 5, 10.0, 2.0);
    a->greaterThan(*b);
    checkWorkspace(a, 0.0, 0.0);
    a = MDEventsTestHelper::makeFakeMDHistoWorkspace(4.56, 2, 5, 10.0, 3.0);
    b = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.34, 2, 5, 10.0, 2.0);
    a->greaterThan(*b);
    checkWorkspace(a, 1.0, 0.0);
    a = MDEventsTestHelper::makeFakeMDHistoWorkspace(4.56, 2, 5, 10.0, 3.0);
    a->greaterThan(4.57);
    checkWorkspace(a, 0.0, 0.0);
    a = MDEventsTestHelper::makeFakeMDHistoWorkspace(4.56, 2, 5, 10.0, 3.0);
    a->greaterThan(4.55);
    checkWorkspace(a, 1.0, 0.0);
  }

  //--------------------------------------------------------------------------------------
  void test_boolean_equalTo() {
    MDHistoWorkspace_sptr a, b, c;
    a = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 3.0);
    b = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23000001, 2, 5, 10.0,
                                                     2.0);
    a->equalTo(*b);
    checkWorkspace(a, 1.0, 0.0);

    a = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.12, 2, 5, 10.0, 3.0);
    a->equalTo(*b);
    checkWorkspace(a, 0.0, 0.0);

    a = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 3.0);
    a->equalTo(1.2300001);
    checkWorkspace(a, 1.0, 0.0);

    a = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 3.0);
    a->equalTo(2.34, 1e-4);
    checkWorkspace(a, 0.0, 0.0);

    a = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 3.0);
    a->equalTo(2.34, 3 /* large tolerance */);
    checkWorkspace(a, 1.0, 0.0);
  }

  //--------------------------------------------------------------------------------------
  void test_setUsingMask() {
    MDHistoWorkspace_sptr a, mask, c;
    a = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 3.0);
    mask = MDEventsTestHelper::makeFakeMDHistoWorkspace(0.00, 2, 5, 10.0,
                                                        0.0); // mask
    c = MDEventsTestHelper::makeFakeMDHistoWorkspace(4.56, 2, 5, 10.0, 2.0);
    a->setUsingMask(*mask, *c);
    checkWorkspace(a, 1.23, 3.0);

    mask->setTo(1.0, 0.0, 0.0);
    a->setUsingMask(*mask, *c);
    checkWorkspace(a, 4.56, 2.0);

    a->setUsingMask(*mask, 7.89, 11);
    checkWorkspace(a, 7.89, 11 * 11);

    mask->setTo(0.0, 0.0, 0.0);
    a->setUsingMask(*mask, 6.66, 7.77);
    checkWorkspace(a, 7.89, 11 * 11);

    // Now a partial mask
    mask->setSignalAt(0, 1.0);
    mask->setSignalAt(2, 1.0);
    a->setTo(1.23, 4.56, 0.0);
    a->setUsingMask(*mask, 6.78, 7.89);
    TS_ASSERT_DELTA(a->getSignalAt(0), 6.78, 1e-5);
    TS_ASSERT_DELTA(a->getSignalAt(1), 1.23, 1e-5);
    TS_ASSERT_DELTA(a->getSignalAt(2), 6.78, 1e-5);
  }

  void doTestMasking(MDImplicitFunction *function,
                     size_t expectedNumberMasked) {
    // 10x10x10 eventWorkspace
    MDHistoWorkspace_sptr ws =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 3, 10, 10.0);

    ws->setMDMasking(function);

    size_t numberMasked = getNumberMasked(ws);
    TSM_ASSERT_EQUALS("Didn't perform the masking as expected",
                      expectedNumberMasked, numberMasked);
  }

  void test_maskNULL() {
    doTestMasking(NULL, 0); // 1000 out of 1000 bins masked
  }

  void test_mask_everything() {
    std::vector<coord_t> min;
    std::vector<coord_t> max;

    // Make the box that covers half the bins in the workspace.
    min.push_back(0);
    min.push_back(0);
    min.push_back(0);
    max.push_back(10);
    max.push_back(10);
    max.push_back(10);

    // Create an function that encompases ALL of the total bins.
    MDImplicitFunction *function = new MDBoxImplicitFunction(min, max);
    doTestMasking(function, 1000); // 1000 out of 1000 bins masked
  }

  void test_maskHalf() {
    std::vector<coord_t> min;
    std::vector<coord_t> max;

    // Make the box that covers half the bins in the workspace.
    min.push_back(0);
    min.push_back(0);
    min.push_back(0);
    max.push_back(10);
    max.push_back(10);
    max.push_back(4.99f);

    // Create an function that encompases 1/2 of the total bins.
    MDImplicitFunction *function = new MDBoxImplicitFunction(min, max);
    doTestMasking(function, 500); // 500 out of 1000 bins masked
  }

  void test_clearMasking() {
    // Create a function that masks everything.
    std::vector<coord_t> min;
    std::vector<coord_t> max;
    min.push_back(0);
    min.push_back(0);
    min.push_back(0);
    max.push_back(10);
    max.push_back(10);
    max.push_back(10);
    MDImplicitFunction *function = new MDBoxImplicitFunction(min, max);

    MDEventWorkspace3Lean::sptr ws =
        MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 1 /*event per box*/);
    ws->setMDMasking(function);

    TSM_ASSERT_EQUALS("Everything should be masked.", 1000,
                      getNumberMasked(ws));
    TS_ASSERT_THROWS_NOTHING(ws->clearMDMasking());
    TSM_ASSERT_EQUALS("Nothing should be masked.", 0, getNumberMasked(ws));
  }

  void test_getSpecialCoordinateSystem_default() {
    MDHistoWorkspace_sptr ws =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1);
    TSM_ASSERT_EQUALS("Should default to no special coordinate system.",
                      Mantid::Kernel::None, ws->getSpecialCoordinateSystem());
  }

  void test_getSpecialCoordinateSystem_when_MDFrames_are_set() {
    // Arrange
    Mantid::Geometry::QSample frame1;
    Mantid::Geometry::QSample frame2;
    Mantid::coord_t min = 0;
    Mantid::coord_t max = 10;
    size_t bins = 2;
    auto dimension1 = boost::make_shared<MDHistoDimension>(
        "QSampleX", "QSampleX", frame1, min, max, bins);
    auto dimension2 = boost::make_shared<MDHistoDimension>(
        "QSampleY", "QSampleY", frame2, min, max, bins);
    auto ws = boost::make_shared<Mantid::DataObjects::MDHistoWorkspace>(
        dimension1, dimension2);

    // Act
    auto specialCoordinates = ws->getSpecialCoordinateSystem();

    // Assert
    TSM_ASSERT_EQUALS("Should detect QSample as the SpecialCoordinate",
                      specialCoordinates,
                      Mantid::Kernel::SpecialCoordinateSystem::QSample);
  }

  void test_displayNormalizationDefault() {
    Mantid::Geometry::GeneralFrame frame("m", "m");
    MDHistoDimension_sptr dimX(
        new MDHistoDimension("X", "x", frame, -10, 10, 5));
    // Constructor variant 1.
    MDHistoWorkspace ws1(dimX);
    TS_ASSERT_EQUALS(Mantid::API::NoNormalization, ws1.displayNormalization());

    auto geometry2 = std::vector<IMDDimension_sptr>(1, dimX);
    // Constructor variant 2.
    MDHistoWorkspace ws2(geometry2);
    TS_ASSERT_EQUALS(Mantid::API::NoNormalization, ws2.displayNormalization());

    auto geometry3 = std::vector<MDHistoDimension_sptr>(1, dimX);
    // Constructor variant 3.
    MDHistoWorkspace ws3(geometry3);
    TS_ASSERT_EQUALS(Mantid::API::NoNormalization, ws3.displayNormalization());
  }

  void test_setDisplayNormalization() {
    auto targetDisplayNormalization = Mantid::API::VolumeNormalization;
    Mantid::Geometry::GeneralFrame frame("m", "m");
    MDHistoDimension_sptr dimX(
        new MDHistoDimension("X", "x", frame, -10, 10, 5));
    // Constructor variant 1.
    MDHistoWorkspace ws1(dimX, dimX, dimX, dimX, targetDisplayNormalization);
    TS_ASSERT_EQUALS(targetDisplayNormalization, ws1.displayNormalization());

    auto geometry2 = std::vector<IMDDimension_sptr>(1, dimX);
    // Constructor variant 2.
    MDHistoWorkspace ws2(geometry2, targetDisplayNormalization);
    TS_ASSERT_EQUALS(targetDisplayNormalization, ws2.displayNormalization());

    auto geometry3 = std::vector<MDHistoDimension_sptr>(1, dimX);
    // Constructor variant 3.
    MDHistoWorkspace ws3(geometry3, targetDisplayNormalization);
    TS_ASSERT_EQUALS(targetDisplayNormalization, ws3.displayNormalization());

    // Quick check of clone
    auto clone = ws3.clone();
    TS_ASSERT_EQUALS(targetDisplayNormalization, clone->displayNormalization());
  }

  /**
  * Test declaring an input IMDHistoWorkspace and retrieving as const_sptr or
  * sptr
  */
  void testGetProperty_const_sptr() {
    const std::string wsName = "InputWorkspace";
    Mantid::Geometry::GeneralFrame frame("m", "m");
    MDHistoDimension_sptr dimX(
        new MDHistoDimension("X", "x", frame, -10, 10, 5));
    IMDHistoWorkspace_sptr wsInput(new MDHistoWorkspace(
        dimX, dimX, dimX, dimX, Mantid::API::VolumeNormalization));
    PropertyManagerHelper manager;
    manager.declareProperty(wsName, wsInput, Direction::Input);

    // Check property can be obtained as const_sptr or sptr
    IMDHistoWorkspace_const_sptr wsConst;
    IMDHistoWorkspace_sptr wsNonConst;
    TS_ASSERT_THROWS_NOTHING(
        wsConst = manager.getValue<IMDHistoWorkspace_const_sptr>(wsName));
    TS_ASSERT(wsConst != NULL);
    TS_ASSERT_THROWS_NOTHING(
        wsNonConst = manager.getValue<IMDHistoWorkspace_sptr>(wsName));
    TS_ASSERT(wsNonConst != NULL);
    TS_ASSERT_EQUALS(wsConst, wsNonConst);

    // Check TypedValue can be cast to const_sptr or to sptr
    PropertyManagerHelper::TypedValue val(manager, wsName);
    IMDHistoWorkspace_const_sptr wsCastConst;
    IMDHistoWorkspace_sptr wsCastNonConst;
    TS_ASSERT_THROWS_NOTHING(wsCastConst = (IMDHistoWorkspace_const_sptr)val);
    TS_ASSERT(wsCastConst != NULL);
    TS_ASSERT_THROWS_NOTHING(wsCastNonConst = (IMDHistoWorkspace_sptr)val);
    TS_ASSERT(wsCastNonConst != NULL);
    TS_ASSERT_EQUALS(wsCastConst, wsCastNonConst);
  }
};

#endif /* MANTID_DATAOBJECTS_MDHISTOWORKSPACETEST_H_ */
