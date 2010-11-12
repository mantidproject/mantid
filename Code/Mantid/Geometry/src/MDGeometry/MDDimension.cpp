#include "MantidGeometry/MDGeometry/MDDimension.h"

namespace Mantid
{
    namespace Geometry
    {
    // get reference to logger for MD workspaces
    Kernel::Logger& MDDimension::g_log=Kernel::Logger::get("MDWorkspaces");
//
void
MDDimension::getAxisPoints(std::vector<double>  &rez)const
{
    rez.resize(this->nBins);
    for(unsigned int i=0;i<nBins;i++){
        rez[i]=0.5*(this->Axis[i]+this->Axis[i+1]);
    }
}

// default dimension is always integrated (it has one point and limits) 
MDDimension::MDDimension(const std::string &ID):
dimTag(ID),
latticeParam(1),
isIntegrated(true),
coord(1,1),
nStride(0),
nBins(1)
{
    // default name coinside with the tag but can be overwritten later
   this->setRange();
   this->setName(dimTag);

}
/// this function sets Linear range. 
void  MDDimension::setRange(double rxMin,double rxMax,unsigned int nxBins)
{
    if(rxMin>rxMax){
        g_log.error()<< "Attempting to set minimal integration limit higer then maximal for Dimension tag: "<<this->dimTag<<std::endl;
        g_log.error()<< "setting MinVal: "<<rxMin<<" MaxVal: "<<rxMax<<std::endl;

        throw(std::invalid_argument("setRange: wrong argument"));
    }
    this->rMin = rxMin;
    this->rMax = rxMax;

    this->setExpanded(nxBins);

}
/// set dimension expanded;
void
MDDimension::setExpanded(double rxMin, double rxMax,unsigned int nBins)
{
    this->check_ranges(rxMin,rxMax);
    this->rMin=rxMin;
    this->rMax=rxMax;

    this->setExpanded(nBins);
}
//
void
MDDimension::check_ranges(double rxMin,double rxMax)
{
   if(rxMin>rxMax){
        g_log.error()<< "Attempting to set minimal integration limit higer then maximal for Dimension tag: "<<this->dimTag<<std::endl;
        g_log.error()<< "setting MinVal: "<<rxMin<<" MaxVal: "<<rxMax<<std::endl;

        throw(std::invalid_argument("checkRanges: rMin>rMax"));
    }
    if(rxMin>this->rMax||rxMax<this->rMin){
        g_log.error()<< "Attempting to set integration limits outside the data range in Dimension ID N: "<<this->dimTag<<std::endl;
        g_log.error()<< "existing MinVal: "<<this->rMin<<" MaxVal: "<<this->rMax<<" Setting: minVal: "<<rxMin<<" maxVal: "<<rxMax<<std::endl;
        throw(std::invalid_argument("checkRanges: wrong rMin or rMax"));

    }

}
void
MDDimension::setExpanded(unsigned int nxBins)
{
    if(nxBins<1||nxBins>MAX_REASONABLE_BIN_NUMBER){
        g_log.error()<< "Setting number of bins="<<nxBins<<" our of range  for Dimension tag: "<<this->dimTag<<std::endl;
        throw(std::invalid_argument("setExpanded: wrong number of bin"));
    }
    if(nxBins> 1){
        this->isIntegrated=false;
    }else{
        this->isIntegrated=true;
    }
    this->nBins= nxBins;
    double Delta=this->getRange()/(nBins);

    double r;
    this->Axis.clear();
    this->Axis.reserve(nBins+1);
    for(unsigned int i=0;i<nBins+1;i++){
        r=this->rMin+i*Delta;
        this->Axis.push_back(r);
    }
// but all this is not enough -- > stride has to be set extrenaly, on the basis of other dimensions which were or were not integrated;
// stide is undefined here
}
/// clear dimension and sets integrated sign;
void
MDDimension::setIntegrated(void)
{
  this->isIntegrated=true;
  this->nBins  =1;
  this->nStride=0;  // the stride of neighboring dimensions has to change accordingly
  this->Axis.clear();
  this->Axis.assign(2,0);
  this->Axis[0] = this->rMin;
  this->Axis[1] = this->rMax;
}
void
MDDimension::setIntegrated(double rxMin){
    if(rxMin>this->rMax){
        g_log.error()<< "Attempting to set minimal integration limit higer then maximal for Dimension tag: "<<this->dimTag<<std::endl;
        g_log.error()<< "existing MaxVal: "<<this->rMax<<" setting minVal: "<<rxMin<<std::endl;
        throw(std::invalid_argument("setIntegrated: new min integration limit is higer than existing max integration limit"));
    }
    this->rMin=rxMin; 
    this->setIntegrated();
}
void
MDDimension::setIntegrated(double rxMin, double rxMax)
{
    this->check_ranges(rxMin,rxMax);

    this->rMin=rxMin; 
    this->rMax=rxMax; 
    this->setIntegrated();
}

MDDimension::~MDDimension()
{
}
}
}