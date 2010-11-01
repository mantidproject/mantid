#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"


namespace Mantid{
    namespace Geometry{

 // get reference to logger for MD workspaces
    Kernel::Logger& MDGeometryDescription::g_log=Kernel::Logger::get("MDWorkspaces");

        
/// the function returns the rotation vector which allows to transform vector inumber i into the basis;
std::vector<double> 
MDGeometryDescription::rotations(unsigned int i,const std::vector<double> basis[3])const
{
// STUB  !!! 
    this->check_index(i,"rotations");
    if(i>2){
        return std::vector<double>(1,1);
    }

    std::vector<double> tmp;
    tmp.assign(3,0);
    tmp[i]=1;
    return tmp;
}
/// this extracts the size and shape of the current DND object
MDGeometryDescription::MDGeometryDescription(const MDGeometry &origin)
{
    MDDimension *pDim;

    std::vector<std::string> tag  = origin.getBasisTags();
    this->nDimensions             = origin.getNumDims();
    this->nReciprocalDimensions   = origin.getNumReciprocalDims();

    for(unsigned int i=0;i<nReciprocalDimensions;i++){
            this->coordinates[i].assign(3,0);
            pDim = origin.getDimension(tag[i]);
           if(pDim){
                this->coordinates[i] = pDim->getCoord();
            }
    }
    
    SlicingData any;
    this->data.assign(nDimensions,any);    
  
    unsigned int i;
    TagIndex curTag;
    for(i=0;i<nDimensions;i++){
        pDim=origin.getDimension(i);
        this->data[i].trans_bott_left=0;
        this->data[i].cut_min  = pDim->getMinimum();
        this->data[i].cut_max  = pDim->getMaximum();
        this->data[i].nBins    = pDim->getNBins();
        this->data[i].AxisName = pDim->getName();

        curTag.index=i;
        curTag.Tag  =pDim->getDimensionTag();
        this->DimTags.push_back(curTag);
    }

}
int 
MDGeometryDescription::getTagNum(const std::string &Tag,bool do_throw)const{
    int iTagNum=-1;
    std::list<TagIndex>::const_iterator it;
    for(it=DimTags.begin();it!=DimTags.end();it++){
        if(it->Tag.compare(Tag)==0){
            iTagNum=it->index;
            break;
        }
    }
    if(iTagNum<0 && do_throw){
        g_log.error()<<" Tag "<<Tag<<" does not exist\n";
        throw(std::invalid_argument(" The requested tag does not exist"));
    }
    return iTagNum;
}

//****** SET *******************************************************************************

void 
MDGeometryDescription::setPAxis(unsigned int i, const std::string &Tag)
{
   this->check_index(i,"setPAxis");

   unsigned int ic(0);
   
   std::list<TagIndex>::const_iterator it,old_place,new_place;
   old_place = DimTags.end();
   for(it=DimTags.begin();it!=old_place;it++){
       if(it->Tag.compare(Tag)==0){
            old_place=it;
            if(ic >i)break;
       }
       if(ic == i){
           new_place=it;
           if(old_place!=DimTags.end())break;
       }
       ic++;
    }
    if(old_place==DimTags.end()){
        g_log.error()<<" Tag "<<Tag<<" does not exist\n";
        throw(std::invalid_argument(" The requested tag does not exist"));
    }
    if(new_place!=old_place){
        DimTags.insert(new_place,*old_place);
        DimTags.erase(old_place);
        this->sortAxisTags();
    }

   
 
}
//
void
MDGeometryDescription::setShift(unsigned int i,double Val)
{
   this->check_index(i,"setShift");
   this->data[i].trans_bott_left=Val;
}
void
MDGeometryDescription::setCutMin(unsigned int i,double Val)
{
    this->check_index(i,"setCutMin");
    this->data[i].cut_min=Val;
}
void
MDGeometryDescription::setCutMax(unsigned int i,double Val)
{
    this->check_index(i,"setCutMax");
    this->data[i].cut_max=Val;
}
void
MDGeometryDescription::setNumBins(unsigned int i,unsigned int Val)
{
    this->check_index(i,"setNumBins");
    if(Val>MAX_REASONABLE_BIN_NUMBER){
        throw(std::invalid_argument("SlicingProperty::setNumBins value bin requested is larger than MAX_REASONABLE_BIN_NUMBER"));
    }
    this->data[i].nBins=Val;
}
void
MDGeometryDescription::setAxisName(unsigned int i,const std::string &Name)
{
    this->check_index(i,"setAxisName");
    this->data[i].AxisName=Name;
}
//*************************************************************************************

double 
MDGeometryDescription::cutMin(unsigned int i)const
{
    this->check_index(i,"cutMin");
    return this->data[i].cut_min;
}
double
MDGeometryDescription::cutMax(unsigned int i)const
{
    this->check_index(i,"cutMax");
    return this->data[i].cut_max;

}
unsigned int 
MDGeometryDescription::numBins(unsigned int i)const
{
    this->check_index(i,"numBins");
    return this->data[i].nBins;

}
bool
MDGeometryDescription::isAxisNamePresent(unsigned int i)const
{
    this->check_index(i,"isAxisNamePresent");

    if(this->data[i].AxisName.empty()){
       return false;
    }else{
        return true;
    }
}
std::string 
MDGeometryDescription::getAxisName(unsigned int i)const
{
    this->check_index(i,"getAxisName");
    return (this->data[i].AxisName);
}

void
MDGeometryDescription::sortAxisTags(void)
{
  
    std::list<TagIndex>::iterator it;
    std::vector<SlicingData> sorted_data(nDimensions,SlicingData());
    unsigned int ic(0);
    SlicingData Current;
    for(it=this->DimTags.begin();it!=DimTags.end();it++){
        sorted_data[ic]=data[it->index];
        it->index      = ic;
        ic++;
    }
    this->data=sorted_data;
}


std::vector<std::string> 
MDGeometryDescription::getAxisTags(void)const
{
    std::vector<std::string> tags(this->nDimensions,"");

    std::list<TagIndex>::const_iterator it;
    unsigned int ic(0);
    for(it=this->DimTags.begin();it!=DimTags.end();it++){
         tags[ic] = it->Tag;
         ic++;
    }
    return tags;
}

/*
DimensionsID 
SlicingProperty::getPAxis(unsigned int i)const
{
    this->check_index(i,"getPAxis");
    return this->pAxis[i];
}
*/

MDGeometryDescription::MDGeometryDescription(unsigned int numDims,unsigned int numRecDims):
nDimensions(numDims),
nReciprocalDimensions(numRecDims)
{
    this->intit_default_slicing(nDimensions,nReciprocalDimensions);

}
/*
SlicingProperty::SlicingProperty(std::vector<DimensionsID> &IDs)
{
    unsigned int i;
    unsigned int nDims=(unsigned int)IDs.size();
    this->intit_default_slicing(nDims);

    // calculate the reciprocal dimensions which are not present in the ID list and clear their coordinates;
    unsigned int nReciprocalDims(0);
    std::vector<unsigned int> rec_dim_to_clear(3,1);
    for(i=0;i<nDims;i++){
        if(IDs[i]<3){
            nReciprocalDims++;
            rec_dim_to_clear[i]=0;
        }
    }

    if(nReciprocalDims<3){
        for(i=0;i<nDims;i++){
            if(rec_dim_to_clear[i]){
                this->coordinates[i].assign(3,0);
            }
        }
    }
}
*/
void
MDGeometryDescription::intit_default_slicing(unsigned int nDims,unsigned int nRecDims)
{
    if(nDims>MAX_MD_DIMS_POSSIBLE){
        throw(std::invalid_argument("SlicingProperty::intit_default_slicing: attemting to init more dimension that it is actually possible "));
    }
    nDimensions          = nDims;
    nReciprocalDimensions= nRecDims;


    nDimensions=nDims;
    MDGeometry DefBasis(nDimensions,nReciprocalDimensions);
    std::vector<std::string> def_tags=DefBasis.getBasisTags();

    unsigned int i;
    SlicingData defaults;
    defaults.trans_bott_left=0;
    defaults.cut_min =-1;
    defaults.cut_max = 1;
    defaults.nBins   = 1;
    defaults.AxisName.assign("");

    this->data.assign(nDimensions,defaults);
 
    for(i=0;i<nReciprocalDimensions;i++){
        this->coordinates[i].assign(3,0);
        this->coordinates[i].at(i)= 1;
    }
    DimTags.clear();
    TagIndex cur;
    for(i=0;i<nDimensions;i++){
        cur.index=i;
        cur.Tag  = def_tags[i];
        this->DimTags.push_back(cur);
        this->data[i].AxisName = def_tags[i];

   }

}
void 
MDGeometryDescription::setCoord(unsigned int i,const std::vector<double> &coord)
{
    this->check_index(i,"setCoord");
    if(i<3){
        if(coord.size()!=3){
            throw(std::invalid_argument("SlicingProperty::setCoord wrong parameter, index<3 and attempting to set a non-3 point coordinate"));
        }
        this->coordinates[i]=coord;
    }else{
        if(coord.size()!=1){
            throw(std::invalid_argument("SlicingProperty::setCoord wrong parameter, index>=3 and attempting to set a coordinate of orthogonal dimension"));
        }
    }
}

/*
std::vector<double> 
SlicingProperty::getCoord(DimensionsID id)const
{
    if(id>2){
        throw(std::invalid_argument("SlicingProperty::getCoord attemt to get coordinate of non-reciprocal dimension"));
    }
    return this->coordinates[id];
}

*/
MDGeometryDescription::~MDGeometryDescription(void)
{
}
void 
MDGeometryDescription::check_index(unsigned int i,const char *fName)const
{
    if(i>=this->nDimensions){
        g_log.error()<<" index out of range for function: "<<fName<<std::endl;
        g_log.error()<<" Allowed nDims: "<<this->nDimensions<<" and requested is: "<<i<<std::endl;
        throw(std::invalid_argument("MDGeometryDescription: index out of range"));
    }
}

} // Geometry
} // Mantid
