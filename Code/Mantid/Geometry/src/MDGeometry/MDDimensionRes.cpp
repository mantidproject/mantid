#include "MantidGeometry/MDGeometry/MDDimensionRes.h"
namespace Mantid{
    namespace Geometry{


MDDimensionRes::MDDimensionRes(const std::string &ID,const rec_dim nRecDim0):
MDDimension(ID),
nRecDim(nRecDim0)
{
    this->coord.assign(3,0);
    this->coord[nRecDim]=1;
}
void 
MDDimensionRes::setCoord(const std::vector<double> &theCoord)
{
    if(theCoord.size()!=3){
        g_log.error()<<"MDDimensionRes::setCoord: Attempt to set the dimension which is not a 3-vector";
        throw(std::invalid_argument("MDDimensionRes::setCoord: Attempt to set the dimension which is not a 3-vector"));
    }
    coord=theCoord;
}
MDDimensionRes::~MDDimensionRes(void)
{
}
//********************************************************************************************************************************************
MDDimDummy::MDDimDummy(unsigned int nRecDim):
MDDimensionRes("DUMMY REC_DIM",(rec_dim)nRecDim)
{ 
  // set 1 bin and the dimension integrated;
  this->setRange(0,1,1);
  this->setName("DUMMY AXIS NAME");
}
  //Get coordinate for index; Throws  if ind is out of range 
double 
MDDimDummy::getX(unsigned int ind)
{
      switch(ind){
      case(0): return 0;
      case(1): return 1;
      default: throw(std::out_of_range("Dummy dimension index is out of range (0,1) "));
      }
}



}
}