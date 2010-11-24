#ifndef H_FIND_MANTID
#define H_FIND_MANTID

#include "MantidAPI/FileFinder.h"
#include "Poco/Path.h"

std::string findTestFileLocation(void){

       std::string path = Mantid::Kernel::getDirectoryOfExecutable();
        
        char pps[2];
        pps[0]=Poco::Path::separator();
        pps[1]=0; 
        std::string sps(pps);

        std::string root_path;
        size_t   nPos = path.find("Mantid"+sps+"Code");
        if(nPos==std::string::npos){
            std::cout <<" can not identify application location\n";
            root_path="../../../../Test/VATES/fe_demo.sqw";
        }else{
            root_path=path.substr(0,nPos)+"Mantid/Test/VATES/fe_demo.sqw";
        }
        std::cout << " test file location: "<< root_path<< std::endl;
        return root_path;
}

#endif