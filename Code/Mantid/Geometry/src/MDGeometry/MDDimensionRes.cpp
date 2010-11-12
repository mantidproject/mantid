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
}
}