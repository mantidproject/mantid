#include "MDDataObjects/MDDataPointDescription.h"
#include <sstream>


namespace Mantid
{
namespace MDDataObjects
{

MDPointDescription::MDPointDescription():
PixDescriptor()
{
  buildDefaultIDs(this->PixDescriptor);
}
//
MDPointDescription::MDPointDescription(const MDPointStructure &pixInfo,const std::vector<std::string> &IndataTags):
dataIDs(IndataTags),PixDescriptor(pixInfo)
{
    if(pixInfo.NumRecDimensions>pixInfo.NumDimensions){
            throw(std::invalid_argument("number of dimensions is lower then the number of reciprocal dimensions"));
    }

    unsigned int nFields = PixDescriptor.NumDimensions*PixDescriptor.DimFieldsPresent+PixDescriptor.NumDataFields*PixDescriptor.DataFieldsPresent+PixDescriptor.NumDimIDs;

    if(dataIDs.size()!=nFields){
      throw(std::invalid_argument("number of dimension names has to be equal to the number of data fields;"));
    }
}
//
unsigned int 
MDPointDescription::sizeofMDDPoint(void)const
{
    unsigned int length(0);
    if(this->PixDescriptor.DimFieldsPresent){
     length= PixDescriptor.NumDimensions*PixDescriptor.DimLength;
    }
   if(this->PixDescriptor.DataFieldsPresent){
     length += PixDescriptor.NumDataFields*PixDescriptor.SignalLength;
   }

   // calculate length of all dataID-s
   // there could be 2 compressed fields-> more are not currently supported;
   if(this->PixDescriptor.NumPixCompressionBits>0){
     // account for compressed fields;
         int num_dimID = this->PixDescriptor.NumDimIDs;
         if(num_dimID>=2){
              num_dimID -=2;
            // two pixels ID are compressed into 4 bytes;
              length += 4 + num_dimID*PixDescriptor.DimIDlength;
         }else{
              length +=  num_dimID*PixDescriptor.DimIDlength;
         }

   }else{ // all ID fields have equal length
        length += PixDescriptor.NumDimIDs*PixDescriptor.DimIDlength;
   }

    return length;
}
//
std::vector<std::string> 
MDPointDescription::getDimensionsID(void)const
{
    std::vector<std::string> tags(dataIDs.begin(),dataIDs.begin()+PixDescriptor.NumDimensions);
    return tags;
}
//
void 
MDPointDescription::buildDefaultIDs(const MDPointStructure &pixInfo)
{

  unsigned int nFields = pixInfo.NumDimensions+pixInfo.NumDataFields+pixInfo.NumDimIDs;
  std::stringstream buf;
  std::vector<std::string> tags(nFields,"");
  unsigned int i0(0),i1(pixInfo.NumRecDimensions),i;
  for(i=0;i<i1;i++){
    buf<<i;
    tags[i]="q"+buf.str();
    buf.seekp(std::ios::beg);
  }
  i0=i1;
  i1=pixInfo.NumDimensions;
  for(i=i0;i<i1;i++){
    buf<<i;
    tags[i]="u"+buf.str();
    buf.seekp(std::ios::beg);
  }
  i0=i1;
  i1=pixInfo.NumDataFields;
  for(i=0;i<i1;i++){
    buf<<i;
    tags[i0+i]="S"+buf.str();
    buf.seekp(std::ios::beg);
  }
  i0+=i1;
  i1=pixInfo.NumDimIDs;
  for(i=0;i<i1;i++){
    buf<<i;
    tags[i0+i]="Ind"+buf.str();
    buf.seekp(std::ios::beg);
  }
  this->dataIDs = tags;
}

MDPointDescription::MDPointDescription(const MDPointStructure &pixInfo):
PixDescriptor(pixInfo)
{
    if(pixInfo.NumRecDimensions>pixInfo.NumDimensions){
            throw(std::invalid_argument("number of dimensions is lower then the number of reciprocal dimensions"));
    }


  this->buildDefaultIDs(pixInfo);

}
//
} // namespaces
}