#include "MantidDataHandling/LoadBinStl.h"
#include "MantidKernel/BinaryStreamReader.h"


#include<iostream>
#include <fstream>
namespace Mantid {
namespace DataHandling {



void LoadBinStl::readStl(std::string filename) {
  std::ifstream myFile(filename.c_str(), std::ios::in | std::ios::binary);

  char header_info[80] = "";
  char nTri[4];
  uint32_t numberTrianglesLong;
  Kernel::BinaryStreamReader streamReader = Kernel::BinaryStreamReader(myFile);
  //skip header
  streamReader.moveStreamToPosition(80);

  // change this after implementing uint32 into the reader

  streamReader >> numberTrianglesLong;
  int next = 96;
  // now read in all the triangles
  for (unsigned long i = 0; i < numberTrianglesLong; i++) {
    next = next + i*50;
  

    if (myFile) {


      // populate each point of the triangle
      for(i=0;i<3;i++){
        float xVal;
        float yVal;
        float zVal;
      
        streamReader >> xVal;
        streamReader >> yVal;
        streamReader >> zVal;
        Kernel::V3D vec = Kernel::V3D(double(xVal),double(yVal),double(zVal));
        verticies.push_back(vec);
      }
      

      
     

      // add index of new verticies to triangle
      int newIndex = triangle.size();
      triangle.push_back(newIndex);
      triangle.push_back(newIndex + 1);
      triangle.push_back(newIndex + 2);
    }
  }
  myFile.close();
  return;
}
}//namespace datahandling
}//namespace mantid