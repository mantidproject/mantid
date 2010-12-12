#include "MDDataObjects/MDDataPointDescription.h"
#include <sstream>


namespace Mantid
{
namespace MDDataObjects
{

MDPointDescription::MDPointDescription():
PixDescriptor()
{
  buildDefaultTags(this->PixDescriptor);
}

MDPointDescription::MDPointDescription(const MDPointStructure &pixInfo,const std::vector<std::string> &IndataTags):
dataTags(IndataTags),PixDescriptor(pixInfo)
{
  unsigned int nFields = PixDescriptor.NumDimensions*PixDescriptor.DimFieldsPresent+PixDescriptor.NumDataFields*PixDescriptor.DataFieldsPresent+PixDescriptor.NumDimIDs;
  if(dataTags.size()!=nFields){
    throw(std::invalid_argument("number of dimension names has to be equal to the number of data fields;"));
  }
}
//
void 
MDPointDescription::buildDefaultTags(const MDPointStructure &pixInfo)
{

  unsigned int nFields = pixInfo.NumDimensions+pixInfo.NumDataFields+pixInfo.NumDimIDs;
  std::stringstream buf;
  std::vector<std::string> tags(nFields,"");
  unsigned int i0(0),i1(pixInfo.NumRecDimensions),i;
  for(i=0;i<i1;i++){
    buf<<i;
    tags[i]="q"+buf.str();
    buf.clear();
  }
  i0=i1;
  i1=pixInfo.NumDataFields;
  for(i=i0;i<i1;i++){
    buf<<i;
    tags[i]="u"+buf.str();
    buf.clear();
  }
  i0=i1;
  i1=pixInfo.NumDataFields;
  for(i=0;i<i1;i++){
    buf<<i;
    tags[i0+i]="S"+buf.str();
    buf.clear();
  }
  i0+=i1;
  i1=pixInfo.NumDimIDs;
  for(i=0;i<i1;i++){
    buf<<i;
    tags[i0+i]="Ind"+buf.str();
    buf.clear();
  }
  this->dataTags = tags;
}

MDPointDescription::MDPointDescription(const MDPointStructure &pixInfo):
PixDescriptor(pixInfo)
{
 
  this->buildDefaultTags(pixInfo);

}
//
} // namespaces
}