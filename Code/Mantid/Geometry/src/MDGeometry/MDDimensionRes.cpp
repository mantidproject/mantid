#include "MantidGeometry/MDGeometry/MDDimensionRes.h"
namespace Mantid{
    namespace Geometry{


MDDimensionRes::MDDimensionRes(const DimensionID &ID):
MDDimension(ID)
{
 
  
   int id= MDDimension::getDimensionID();
   if(id<0||id>=3){ 
       g_log.error()<<"MDDimensionRes: dimension ID exceeds the acceptable range; Are you trying to initiate an resiprocal dimension using ortogonal dimension using constructor?";
       throw(std::out_of_range("dimension ID exceeds the acceptable range; Are you trying to initiate an resiprocal dimension using ortogonal dimension using constructor?"));
   }

    this->coord.assign(3,0);
    this->coord[id]=1;
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
}
}