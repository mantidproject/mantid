#ifndef H_TEST_MAIN_SQW
#define H_TEST_MAIN_SQW
#include "find_mantid.h"

#include <cxxtest/TestSuite.h>
class tmain: public CxxTest::TestSuite
{
public:
      void testTMain(void){
           TS_WARN( "Main suite for sqw tests invoked" );
      }
};


std::string findTestFileLocation(void){

       std::string path=FileFinder::Instance().getFullPath("Mantid.properties");
        
        char pps[2];
        pps[0]=Poco::Path::separator();
        pps[1]=0; 
        std::string sps(pps);

        std::string root_path;
        size_t   nPos = path.find("Mantid"+sps+"Code");
        if(nPos==std::string::npos){
            root_path="../../../../Test/VATES/fe_demo.sqw";
        }else{
            root_path=path.substr(0,nPos)+"Mantid/Test/VATES/fe_demo.sqw";
        }
        std::cout << " test file location: "<< root_path<< std::endl;
        return root_path;
}
#endif