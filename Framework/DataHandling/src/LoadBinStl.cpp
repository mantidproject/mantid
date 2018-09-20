#include "MantidDataHandling/LoadBinStl.h"
#include "MantidKernel/BinaryStreamReader.h"


#include<iostream>
#include <fstream>
namespace Mantid {
namespace DataHandling {

Kernel::V3D LoadBinStl::makeV3(char *facet) {
  char xChar[4] = {facet[0], facet[1], facet[2], facet[3]};

  char yChar[4] = {facet[4], facet[5], facet[6], facet[7]};

  char zChar[4] = {facet[8], facet[9], facet[10], facet[11]};

  float xx = *((float *)xChar);
  float yy = *((float *)yChar);
  float zz = *((float *)zChar);
  Kernel::V3D vec = Kernel::V3D(double(xx),double(yy),double(zz));
  return vec;
}

void LoadBinStl::readStl(std::string filename) {
  std::ifstream myFile(filename.c_str(), std::ios::in | std::ios::binary);

  char header_info[80] = "";
  char nTri[4];
  unsigned long numberTrianglesLong;
  Kernel::BinaryStreamReader streamReader = Kernel::BinaryStreamReader(myFile);
  //skip header
  streamReader.moveStreamToPosition(80);

  // change this after implementing uint32 into the reader
  if (myFile) {
    myFile.read(nTri, 4);
    numberTrianglesLong = *((unsigned long *)nTri);
  }
  int next = 80;
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
  myFile.close()
  return;
}
}//namespace datahandling
}//namespace mantid