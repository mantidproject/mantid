#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/DimensionRes.h"
namespace Mantid{
    namespace MDDataObjects{
DimensionRes::DimensionRes(DimensionsID ID):
Dimension(ID)
{
    const char *default_axis_names[3]={
          "H","K","L"
    };

    
   this->setRange();
   if(ID<0||ID>=3){ 
       throw(std::out_of_range("dimension ID exceeds the acceptable range; Are you trying to initiate an resiprocal dimension using ortogonal dimension using constructor?"));
   }

   this->setName(default_axis_names[(int)(ID)]);
   this->coord.assign(3,0);
   this->coord[int(ID)]=1;
}
void 
DimensionRes::setCoord(const std::vector<double> &theCoord)
{
    if(theCoord.size()!=3){
        throw(std::invalid_argument("DimensionRes::setCoord: Attempt to set the dimension which is not a 3-vector"));
    }
    coord=theCoord;
}
DimensionRes::~DimensionRes(void)
{
}
}
}