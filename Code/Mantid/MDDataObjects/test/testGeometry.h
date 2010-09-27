#include <cxxtest/TestSuite.h>
#include "Geometry.h"
#include "SlicingData.h"
class testGeometry : public CxxTest::TestSuite
{
public:
    void testGeometryC(void)
    {
      try{
          Geometry   dnd_geometry;

          TS_ASSERT_THROWS_NOTHING(dnd_geometry.getXDimension());
          TS_ASSERT_THROWS_NOTHING(dnd_geometry.getYDimension());
          TS_ASSERT_THROWS_NOTHING(dnd_geometry.getZDimension());
          TS_ASSERT_THROWS_NOTHING(dnd_geometry.getTDimension());

          std::vector<Dimension *> Dims;
          TS_ASSERT_THROWS_NOTHING(Dims=dnd_geometry.getIntegratedDimensions());
          // default size of the dimensions is equal 4
          TS_ASSERT_EQUALS(Dims.size(),4);

    /// functions return the pointer to the dimension requested as the dimension num. Throws if dimension is out of range. 
        Dimension *pDim;
        // get pointer to the dimension 0
        TS_ASSERT_THROWS_NOTHING(pDim=dnd_geometry.getDimension(0));
        TS_ASSERT_EQUALS(pDim->getDimensionID(),eh);
        Dimension *pDim0;
        // no such dimension
        TS_ASSERT_THROWS_ANYTHING(pDim0=dnd_geometry.getDimension(8));
        // no such dimension
        TS_ASSERT_THROWS_NOTHING(pDim0=dnd_geometry.getDimension(u7));
//        TS_ASSERT_EQUALS(pDim0,NULL);

        // the same dimension as above
        TS_ASSERT_THROWS_NOTHING(pDim0=dnd_geometry.getDimension(eh));
        TS_ASSERT_EQUALS(pDim0,pDim);

         SlicingData slice(dnd_geometry);

//       we want these data to be non-integrated;
         TS_ASSERT_THROWS_NOTHING(slice.setNumBins(en,100));
         TS_ASSERT_THROWS_NOTHING(slice.setNumBins(eh,200));

// we want first (0) axis to be energy 
         TS_ASSERT_THROWS_NOTHING(slice.setPAxis(0,en));
         TS_ASSERT_THROWS_NOTHING(slice.setPAxis(0,en));
// and the third (2) ->el (z-axis) 
         TS_ASSERT_THROWS_NOTHING(slice.setPAxis(3,el));
         TS_ASSERT_THROWS_NOTHING(slice.setPAxis(2,el));


         TS_ASSERT_THROWS_NOTHING(dnd_geometry.setRanges(slice));

         // arrange final dimensions according to pAxis, this will run through one branch of reinit_Geometry only
         TS_ASSERT_THROWS_NOTHING(dnd_geometry.reinit_Geometry(slice));

         TS_ASSERT_THROWS_NOTHING(pDim = dnd_geometry.getDimension(0));
         TS_ASSERT_EQUALS(pDim->getDimensionID(),slice.getPAxis(0))
         TS_ASSERT_THROWS_NOTHING(pDim = dnd_geometry.getDimension(1));
         TS_ASSERT_EQUALS(pDim->getDimensionID(),slice.getPAxis(1))
         TS_ASSERT_THROWS_NOTHING(pDim = dnd_geometry.getDimension(2));
         TS_ASSERT_EQUALS(pDim->getDimensionID(),slice.getPAxis(2))
         TS_ASSERT_THROWS_NOTHING(pDim = dnd_geometry.getDimension(3));
         TS_ASSERT_EQUALS(pDim->getDimensionID(),slice.getPAxis(3))

      }catch(errorMantid &err){
          std::cout<<" error of the Geomerty constructor "<<err.what()<<std::endl;
          TS_ASSERT_THROWS_NOTHING(throw(err));

      }
    }


};
