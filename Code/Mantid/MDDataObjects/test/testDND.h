#ifndef H_TEST_DND
#define H_TEST_DND


#include <cxxtest/TestSuite.h>
#include "DND.h"

class testDND :    public CxxTest::TestSuite
{
public:
    void testDNDRead(void){
      try{
        DND dnd_obj(5);
        TS_ASSERT_THROWS_NOTHING(dnd_obj.read_dnd("../../partial_dataset_access/sqw_test/fe_demo.sqw"));

        std::vector<unsigned int> selection(2,1);
        std::vector<point3D> img;
        // returns 2D image
        TS_ASSERT_THROWS_NOTHING(img=dnd_obj.getPointData(selection) );
        TS_ASSERT_EQUALS(img.size(),2500);

        // fails as we select 5 dimensions but the dataset is actually 4-D
        selection.assign(5,20);
        TS_ASSERT_THROWS_ANYTHING(img=dnd_obj.getPointData(selection) );


        // returns pointer to 3D image with 4-th dimension selected at 20;
        selection.assign(1,20);
        std::vector<point3D> *pimg;
        TS_ASSERT_THROWS_NOTHING(pimg=&dnd_obj.getPointData(selection) );
        TS_ASSERT_EQUALS(pimg->size(),50*50*50);
       // this should work fine and reduce memory usage but invalidate pimg like all other subsequent calls to getPointData
        TS_ASSERT_THROWS_NOTHING(dnd_obj.clearPointsMemory());

        // this should return single point at (20,20,20,20)
        selection.assign(4,20);
        TS_ASSERT_THROWS_NOTHING(img=dnd_obj.getPointData(selection) );
        TS_ASSERT_EQUALS(img.size(),1);
        // this should return line of size 50 
        selection.assign(3,10);
        TS_ASSERT_THROWS_NOTHING(img=dnd_obj.getPointData(selection) );
        TS_ASSERT_EQUALS(img.size(),50);
        
      }catch(errorMantid &err){
          std::cout<<" error of the Geomerty constructor "<<err.what()<<std::endl;
          TS_ASSERT_THROWS_NOTHING(throw(err));
      }
      system("pause");
    }

};
#endif