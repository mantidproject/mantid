// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_MDHISTODIMENSION_BUILDER_TEST_H_
#define MANTID_GEOMETRY_MDHISTODIMENSION_BUILDER_TEST_H_

#include "MantidGeometry/MDGeometry/MDHistoDimensionBuilder.h"
#include "MantidGeometry/MDGeometry/QLab.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include <cxxtest/TestSuite.h>

using Mantid::Geometry::IMDDimension_sptr;
using Mantid::Geometry::MDHistoDimension;
using Mantid::Geometry::MDHistoDimensionBuilder;

class MDHistoDimensionBuilderTest : public CxxTest::TestSuite {
public:
  void test_resizeToFit_Min_Positive_Max_Positive() {
    const Mantid::coord_t min0(0.1f), max0(0.5f);
    Mantid::coord_t min1(min0), max1(max0);
    MDHistoDimensionBuilder::resizeToFitMDBox(min1, max1);
    TS_ASSERT(min1 < min0);
    TS_ASSERT(max1 > max0);
  }

  void test_resizeToFit_Min_Negative_Max_Positive() {
    const Mantid::coord_t min0(-0.1f), max0(0.5f);
    Mantid::coord_t min1(min0), max1(max0);
    MDHistoDimensionBuilder::resizeToFitMDBox(min1, max1);
    TS_ASSERT(min1 < min0);
    TS_ASSERT(max1 > max0);
  }

  void test_resizeToFit_Min_Negative_Max_Negative() {
    const Mantid::coord_t min0(-0.5f), max0(-0.1f);
    Mantid::coord_t min1(min0), max1(max0);
    MDHistoDimensionBuilder::resizeToFitMDBox(min1, max1);
    TS_ASSERT(min1 < min0);
    TS_ASSERT(max1 > max0);
  }

  void testConstructRaw() {
    MDHistoDimensionBuilder builder;
    builder.setName("testDimName");
    builder.setId("testDimId");
    builder.setUnits("A^-1");
    builder.setMin(0);
    builder.setMax(2);
    builder.setNumBins(1);
    builder.setFrameName("QLab");

    MDHistoDimension *product = builder.createRaw();

    TS_ASSERT_EQUALS("testDimName", product->getName());
    TS_ASSERT_EQUALS("testDimId", product->getDimensionId());
    Mantid::Kernel::InverseAngstromsUnit expectedUnit;
    TS_ASSERT_EQUALS(expectedUnit.getUnitLabel(), product->getUnits().ascii());
    TS_ASSERT_EQUALS(0, product->getMinimum());
    TS_ASSERT_EQUALS(2, product->getMaximum());
    TS_ASSERT_EQUALS(1, product->getNBins());
    TSM_ASSERT_EQUALS("Should have selected QLab as the frame",
                      Mantid::Geometry::QLab::QLabName,
                      product->getMDFrame().name());

    delete product;
  }

  void testConstruct() {
    MDHistoDimensionBuilder builder;
    builder.setName("testDimName");
    builder.setId("testDimId");
    builder.setUnits("A^-1");
    builder.setMin(0);
    builder.setMax(2);
    builder.setNumBins(1);
    builder.setFrameName("QSample");

    IMDDimension_sptr product;
    TS_ASSERT_THROWS_NOTHING(product = builder.create());
    TS_ASSERT_EQUALS("testDimName", product->getName());
    TS_ASSERT_EQUALS("testDimId", product->getDimensionId());
    Mantid::Kernel::InverseAngstromsUnit expectedUnit;
    TS_ASSERT_EQUALS(expectedUnit.getUnitLabel(), product->getUnits().ascii());
    TS_ASSERT_EQUALS(0, product->getMinimum());
    TS_ASSERT_EQUALS(2, product->getMaximum());
    TS_ASSERT_EQUALS(1, product->getNBins());
    TSM_ASSERT_EQUALS("Should have selected QSample as the frame",
                      Mantid::Geometry::QSample::QSampleName,
                      product->getMDFrame().name());
  }

  void testConstruct_without_frame_name() {
    MDHistoDimensionBuilder builder;
    builder.setName("testDimName");
    builder.setId("testDimId");
    builder.setUnits("A^-1");
    builder.setMin(0);
    builder.setMax(2);
    builder.setNumBins(1);

    IMDDimension_sptr product;
    TS_ASSERT_THROWS_NOTHING(product = builder.create());
    TS_ASSERT_EQUALS("testDimName", product->getName());
    TS_ASSERT_EQUALS("testDimId", product->getDimensionId());
    Mantid::Kernel::InverseAngstromsUnit expectedUnit;
    TS_ASSERT_EQUALS(expectedUnit.getUnitLabel(), product->getUnits().ascii());
    TS_ASSERT_EQUALS(0, product->getMinimum());
    TS_ASSERT_EQUALS(2, product->getMaximum());
    TS_ASSERT_EQUALS(1, product->getNBins());
    TSM_ASSERT_EQUALS("Should have selected GeneralFrame as the frame",
                      "testDimName", product->getMDFrame().name());
  }

  void testCopy() {
    MDHistoDimensionBuilder builderA;
    builderA.setName("testDimName");
    builderA.setId("testDimId");
    builderA.setUnits("A^-1");
    builderA.setMin(0);
    builderA.setMax(2);
    builderA.setNumBins(1);

    // Make copy
    MDHistoDimensionBuilder builderB(builderA);

    // Test copy constancy via products
    IMDDimension_sptr productA = builderA.create();
    IMDDimension_sptr productB = builderB.create();

    TS_ASSERT_EQUALS(productA->getName(), productB->getName());
    TS_ASSERT_EQUALS(productA->getDimensionId(), productB->getDimensionId());
    TS_ASSERT_EQUALS(productA->getUnits(), productB->getUnits());
    TS_ASSERT_EQUALS(productA->getMinimum(), productB->getMinimum());
    TS_ASSERT_EQUALS(productA->getMaximum(), productB->getMaximum());
    TS_ASSERT_EQUALS(productA->getNBins(), productB->getNBins());
  }

  void testAssignment() {
    MDHistoDimensionBuilder builderA;
    builderA.setName("testDimName");
    builderA.setId("testDimId");
    builderA.setUnits("A^-1");
    builderA.setMin(0);
    builderA.setMax(2);
    builderA.setNumBins(1);

    // Make another
    MDHistoDimensionBuilder builderB;
    // Do assignement
    builderB = builderA;

    // Test assignment constancy via products
    IMDDimension_sptr productA = builderA.create();
    IMDDimension_sptr productB = builderB.create();

    TS_ASSERT_EQUALS(productA->getName(), productB->getName());
    TS_ASSERT_EQUALS(productA->getDimensionId(), productB->getDimensionId());
    TS_ASSERT_EQUALS(productA->getUnits(), productB->getUnits());
    TS_ASSERT_EQUALS(productA->getMinimum(), productB->getMinimum());
    TS_ASSERT_EQUALS(productA->getMaximum(), productB->getMaximum());
    TS_ASSERT_EQUALS(productA->getNBins(), productB->getNBins());
  }

  void testNoNameThrows() {
    MDHistoDimensionBuilder builder;
    // builder.setName("testDimName");
    builder.setId("testDimId");
    builder.setUnits("A^-1");
    builder.setMin(0);
    builder.setMax(2);
    builder.setNumBins(1);

    TS_ASSERT_THROWS(builder.create(), const std::invalid_argument &);
  }

  void testNoIdThrows() {
    MDHistoDimensionBuilder builder;
    builder.setName("testDimName");
    // builder.setId("testDimId");
    builder.setUnits("A^-1");
    builder.setMin(0);
    builder.setMax(2);
    builder.setNumBins(1);

    TS_ASSERT_THROWS(builder.create(), const std::invalid_argument &);
  }

  void testNoUnitThrows() {
    MDHistoDimensionBuilder builder;
    builder.setName("testDimName");
    builder.setId("testDimId");
    // builder.setUnits("A^-1");
    builder.setMin(0);
    builder.setMax(2);
    builder.setNumBins(1);

    TS_ASSERT_THROWS(builder.create(), const std::invalid_argument &);
  }

  void testNoMaxThrows() {
    MDHistoDimensionBuilder builder;
    builder.setName("testDimName");
    builder.setId("testDimId");
    builder.setUnits("A^-1");
    builder.setMin(0);
    // builder.setMax(2);
    builder.setNumBins(1);

    TS_ASSERT_THROWS(builder.create(), const std::invalid_argument &);
  }

  void testNoMinThrows() {
    MDHistoDimensionBuilder builder;
    builder.setName("testDimName");
    builder.setId("testDimId");
    builder.setUnits("A^-1");
    // builder.setMin(0);
    builder.setMax(2);
    builder.setNumBins(1);

    TS_ASSERT_THROWS(builder.create(), const std::invalid_argument &);
  }

  void testMinLessThanMaxThrows() {
    MDHistoDimensionBuilder builder;
    builder.setName("testDimName");
    builder.setId("testDimId");
    builder.setUnits("A^-1");
    builder.setMin(1); // min > max
    builder.setMax(0);
    builder.setNumBins(1);

    TS_ASSERT_THROWS(builder.create(), const std::invalid_argument &);
  }

  void testMinEqualToMaxThrows() {
    MDHistoDimensionBuilder builder;
    builder.setName("testDimName");
    builder.setId("testDimId");
    builder.setUnits("A^-1");
    builder.setMin(1); // Max and min set to same
    builder.setMax(1);
    builder.setNumBins(1);

    TS_ASSERT_THROWS(builder.create(), const std::invalid_argument &);
  }

  void testNBinsLessThanOneThrows() {
    MDHistoDimensionBuilder builder;
    builder.setName("testDimName");
    builder.setId("testDimId");
    builder.setUnits("A^-1");
    builder.setMin(0);
    builder.setMax(2);
    builder.setNumBins(0); // No bins!

    TS_ASSERT_THROWS(builder.create(), const std::invalid_argument &);
  }
};

#endif
