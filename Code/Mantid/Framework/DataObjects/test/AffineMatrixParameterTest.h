#ifndef AFFINE_MATRIX_PARAMETER_TEST_H
#define AFFINE_MATRIX_PARAMETER_TEST_H

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/AffineMatrixParameter.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"

using namespace Mantid::MDEvents;
using namespace Mantid;

class AffineMatrixParameterTest :    public CxxTest::TestSuite
{
public:

 void testConstruction()
 {
   AffineMatrixParameter param(2, 3);
   AffineMatrixType affineMatrix = param.getAffineMatrix();
   std::pair<size_t, size_t> size = affineMatrix.size();

   TS_ASSERT_EQUALS(size.first, 3);
   TS_ASSERT_EQUALS(size.second, 4);
   TS_ASSERT(!param.isValid()); //No call to setMatrix.
 }

 void testCopy()
 {
   AffineMatrixParameter A(2, 3);
   AffineMatrixParameter B(A);

   AffineMatrixType expectedProduct = A.getAffineMatrix();
   AffineMatrixType copyProduct = B.getAffineMatrix();

   TS_ASSERT_EQUALS(expectedProduct, copyProduct);
   TS_ASSERT_EQUALS(A.isValid(), B.isValid());
 }

 void testSetMatrix()
 {
   AffineMatrixParameter param(3, 3);
   
   AffineMatrixType transform(4,4);
   param.setMatrix(transform);
   TS_ASSERT(param.isValid()); //Matrix is now set.
   TS_ASSERT_EQUALS(transform, param.getAffineMatrix());
 }

 void testSetMatrixThrowsIfOutDimsNotEqual()
 {
   AffineMatrixParameter param(1, 3);
   AffineMatrixType transform(4, 4);

   TS_ASSERT_THROWS(param.setMatrix(transform) ,std::runtime_error);
 }

 void testSetMatrixThrowsIfInDimsNotEqual()
 {
   AffineMatrixParameter param(3, 1);
   AffineMatrixType transform(4, 4);

   TS_ASSERT_THROWS(param.setMatrix(transform) ,std::runtime_error);
 }

 void testAssign()
 {
   AffineMatrixParameter A(4, 4);
   AffineMatrixParameter B(4, 4);

   B = A;

   AffineMatrixType AProduct = A.getAffineMatrix();
   AffineMatrixType BProduct = B.getAffineMatrix();

   TS_ASSERT_EQUALS(AProduct, BProduct);
   TS_ASSERT_EQUALS(A.isValid(), B.isValid());
 }

 void testClone()
 {
   AffineMatrixParameter A(2, 3);

   AffineMatrixParameter* B = A.clone();

   AffineMatrixType AProduct = A.getAffineMatrix();
   AffineMatrixType BProduct = B->getAffineMatrix();

   TS_ASSERT_EQUALS(AProduct, BProduct);
   TS_ASSERT_EQUALS(A.isValid(), B->isValid());
   delete B;
 }

 void testAssignmentThrowsIfOutDimsNotEqual()
 {
   AffineMatrixParameter A(2, 4);
   AffineMatrixParameter B(4, 4);

   TS_ASSERT_THROWS(B = A, std::runtime_error);
 }

 void testAssignementThrowsIfInDimsNotEqual()
 {
   AffineMatrixParameter A(4, 2);
   AffineMatrixParameter B(4, 4);

   TS_ASSERT_THROWS(B = A, std::runtime_error);
 }

 void testToXMLString()
 {
   AffineMatrixParameter param(3, 3);
   AffineMatrixType transform(4,4);
   int count = 0;
    for(int i = 0; i < 4; i++)
    {
      for(int j = 0; j < 4; j++)
      {
        transform[i][j] = static_cast<coord_t>(count);
        count++;
      }
    }

    param.setMatrix(transform);
    std::string result = param.toXMLString();
    TSM_ASSERT_EQUALS("Serialization of CoordTransform has not worked correctly.", 
      "<Parameter><Type>AffineMatrixParameter</Type><Value>0,1,2,3;4,5,6,7;8,9,10,11;12,13,14,15</Value></Parameter>", param.toXMLString());
 }

};

#endif
