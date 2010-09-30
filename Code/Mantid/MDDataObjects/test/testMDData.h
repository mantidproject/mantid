#ifndef H_TEST_DND
#define H_TEST_DND


#include <cxxtest/TestSuite.h>
#include "MDData.h"

using namespace Mantid;
using namespace MDDataObjects;

class testDND :    public CxxTest::TestSuite
{
public:
    void testDNDRead(void){
      try{
      // define an object 
        MDData dnd_obj(5);
        TS_ASSERT_THROWS_NOTHING(dnd_obj.read_mdd("c:/mantid/Test/VATES/fe_demo.sqw"));

        std::vector<unsigned int> selection(2,1);
        std::vector<point3D> img;
        // returns 2D image
        TS_ASSERT_THROWS_NOTHING(dnd_obj.getPointData(selection,img) );
        TS_ASSERT_EQUALS(img.size(),2500);

        // fails as we select 5 dimensions but the dataset is actually 4-D
        selection.assign(5,20);
        TS_ASSERT_THROWS_ANYTHING(dnd_obj.getPointData(selection,img) );


        // returns 3D image with 4-th dimension selected at 20;
        selection.assign(1,20);
        TS_ASSERT_THROWS_NOTHING(dnd_obj.getPointData(selection,img) );
        TS_ASSERT_EQUALS(img.size(),50*50*50);
 

        // this should return single point at (20,20,20,20)
        selection.assign(4,20);
        TS_ASSERT_THROWS_NOTHING(dnd_obj.getPointData(selection,img) );
        TS_ASSERT_EQUALS(img.size(),1);
        // this should return line of size 50 
        selection.assign(3,10);
        TS_ASSERT_THROWS_NOTHING(dnd_obj.getPointData(selection,img) );
        TS_ASSERT_EQUALS(img.size(),50);
        
      }catch(std::exception &err){
          std::cout<<" error of the Geomerty constructor "<<err.what()<<std::endl;
          TS_ASSERT_THROWS_NOTHING(throw(err));
      }
    //  system("pause");
    }

};
#endif