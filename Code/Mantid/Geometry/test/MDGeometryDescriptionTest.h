#include <cxxtest/TestSuite.h>
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"

using namespace Mantid;
using namespace Geometry;
class testMDGeometryDescription : public CxxTest::TestSuite
{
    MDGeometryDescription *pSlice;
public:
   void testMDGDconstructor(){
     TS_ASSERT_THROWS_NOTHING(pSlice = new MDGeometryDescription());
   }
   // these functions has not been written yet
   void testMDGDInput(){
     std::string input("");
     TS_ASSERT_EQUALS(pSlice->fromXMLstring(input),true);
   }
   void testMDGDOutput(){
     std::string output("");
     TS_ASSERT_THROWS_NOTHING(output=pSlice->toXMLstring());
     TS_ASSERT_EQUALS(output,"TEST PROPERTY");
   }


   testMDGeometryDescription():pSlice(NULL){}
   ~testMDGeometryDescription(){
     if(pSlice)delete pSlice;
   }
};