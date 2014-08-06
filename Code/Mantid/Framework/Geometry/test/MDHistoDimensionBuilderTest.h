#ifndef MANTID_GEOMETRY_MDHISTODIMENSION_BUILDER_TEST_H_
#define MANTID_GEOMETRY_MDHISTODIMENSION_BUILDER_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/MDGeometry/MDHistoDimensionBuilder.h"

using Mantid::Geometry::MDHistoDimension;
using Mantid::Geometry::MDHistoDimensionBuilder;
using Mantid::Geometry::IMDDimension_sptr;

class MDHistoDimensionBuilderTest : public CxxTest::TestSuite
{
public:
 
 void testConstructRaw()
 {
   MDHistoDimensionBuilder builder;
   builder.setName("testDimName");
   builder.setId("testDimId");
   builder.setUnits("A^-1");
   builder.setMin(0);
   builder.setMax(2);
   builder.setNumBins(1);

   MDHistoDimension* product = builder.createRaw();

   TS_ASSERT_EQUALS("testDimName", product->getName());
   TS_ASSERT_EQUALS("testDimId", product->getDimensionId());
   TS_ASSERT_EQUALS("A^-1", product->getUnits().ascii());
   TS_ASSERT_EQUALS(0, product->getMinimum());
   TS_ASSERT_EQUALS(2, product->getMaximum());
   TS_ASSERT_EQUALS(1, product->getNBins());
   delete product;
 }

 void testConstruct()
 {
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
   TS_ASSERT_EQUALS("A^-1", product->getUnits().ascii());
   TS_ASSERT_EQUALS(0, product->getMinimum());
   TS_ASSERT_EQUALS(2, product->getMaximum());
   TS_ASSERT_EQUALS(1, product->getNBins());
 }

 void testCopy()
 {
   MDHistoDimensionBuilder builderA;
   builderA.setName("testDimName");
   builderA.setId("testDimId");
   builderA.setUnits("A^-1");
   builderA.setMin(0);
   builderA.setMax(2);
   builderA.setNumBins(1);

   //Make copy
   MDHistoDimensionBuilder builderB(builderA);

   //Test copy constancy via products
   IMDDimension_sptr productA = builderA.create();
   IMDDimension_sptr productB = builderB.create();

   TS_ASSERT_EQUALS(productA->getName(), productB->getName());
   TS_ASSERT_EQUALS(productA->getDimensionId(), productB->getDimensionId());
   TS_ASSERT_EQUALS(productA->getUnits(), productB->getUnits());
   TS_ASSERT_EQUALS(productA->getMinimum(), productB->getMinimum());
   TS_ASSERT_EQUALS(productA->getMaximum(), productB->getMaximum());
   TS_ASSERT_EQUALS(productA->getNBins(), productB->getNBins());
 }

 void testAssignment()
 {
   MDHistoDimensionBuilder builderA;
   builderA.setName("testDimName");
   builderA.setId("testDimId");
   builderA.setUnits("A^-1");
   builderA.setMin(0);
   builderA.setMax(2);
   builderA.setNumBins(1);

   //Make another
   MDHistoDimensionBuilder builderB;
   //Do assignement
   builderB = builderA;

   //Test assignment constancy via products
   IMDDimension_sptr productA = builderA.create();
   IMDDimension_sptr productB = builderB.create();

   TS_ASSERT_EQUALS(productA->getName(), productB->getName());
   TS_ASSERT_EQUALS(productA->getDimensionId(), productB->getDimensionId());
   TS_ASSERT_EQUALS(productA->getUnits(), productB->getUnits());
   TS_ASSERT_EQUALS(productA->getMinimum(), productB->getMinimum());
   TS_ASSERT_EQUALS(productA->getMaximum(), productB->getMaximum());
   TS_ASSERT_EQUALS(productA->getNBins(), productB->getNBins());
 }

 void testNoNameThrows()
 {
   MDHistoDimensionBuilder builder;
   //builder.setName("testDimName");
   builder.setId("testDimId");
   builder.setUnits("A^-1");
   builder.setMin(0);
   builder.setMax(2);
   builder.setNumBins(1);

   TS_ASSERT_THROWS(builder.create(), std::invalid_argument);
 }

 void testNoIdThrows()
 {
   MDHistoDimensionBuilder builder;
   builder.setName("testDimName");
   //builder.setId("testDimId");
   builder.setUnits("A^-1");
   builder.setMin(0);
   builder.setMax(2);
   builder.setNumBins(1);

   TS_ASSERT_THROWS(builder.create(), std::invalid_argument);
 }

 void testNoUnitThrows()
 {
   MDHistoDimensionBuilder builder;
   builder.setName("testDimName");
   builder.setId("testDimId");
   //builder.setUnits("A^-1");
   builder.setMin(0);
   builder.setMax(2);
   builder.setNumBins(1);

   TS_ASSERT_THROWS(builder.create(), std::invalid_argument);
 }

 void testNoMaxThrows()
 {
   MDHistoDimensionBuilder builder;
   builder.setName("testDimName");
   builder.setId("testDimId");
   builder.setUnits("A^-1");
   builder.setMin(0);
   //builder.setMax(2);
   builder.setNumBins(1);

   TS_ASSERT_THROWS(builder.create(), std::invalid_argument);
 }

 void testNoMinThrows()
 {
   MDHistoDimensionBuilder builder;
   builder.setName("testDimName");
   builder.setId("testDimId");
   builder.setUnits("A^-1");
   //builder.setMin(0);
   builder.setMax(2);
   builder.setNumBins(1);

   TS_ASSERT_THROWS(builder.create(), std::invalid_argument);
 }

 void testMinLessThanMaxThrows()
 {
   MDHistoDimensionBuilder builder;
   builder.setName("testDimName");
   builder.setId("testDimId");
   builder.setUnits("A^-1");
   builder.setMin(1); // min > max
   builder.setMax(0);
   builder.setNumBins(1);

   TS_ASSERT_THROWS(builder.create(), std::invalid_argument);
 }

 void testMinEqualToMaxThrows()
 {
   MDHistoDimensionBuilder builder;
   builder.setName("testDimName");
   builder.setId("testDimId");
   builder.setUnits("A^-1");
   builder.setMin(1); //Max and min set to same
   builder.setMax(1);
   builder.setNumBins(1);

   TS_ASSERT_THROWS(builder.create(), std::invalid_argument);
 }

 void testNBinsLessThanOneThrows()
 {
   MDHistoDimensionBuilder builder;
   builder.setName("testDimName");
   builder.setId("testDimId");
   builder.setUnits("A^-1");
   builder.setMin(0);
   builder.setMax(2);
   builder.setNumBins(0); //No bins!

   TS_ASSERT_THROWS(builder.create(), std::invalid_argument);
 }

};

#endif
