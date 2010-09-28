#include "stdafx.h"
#include "Dimension.h"
namespace Mantid
{
    namespace MDDataObjects
    {
//
void
Dimension::getAxisPoints(std::vector<double>  &rez)const
{
    rez.resize(this->nBins);
    for(unsigned int i=0;i<nBins;i++){
        rez[i]=0.5*(this->Axis[i]+this->Axis[i+1]);
    }
}

// default dimension is always integrated (it has one point and limits) 
Dimension::Dimension(DimensionsID ID):
DimensionID(ID),
latticeParam(1),
isIntegrated(true),
coord(1,1)
{
    const char *default_axis_names[MAX_NDIMS_POSSIBLE]={
          "","","","En","u1","u2","u3","u4","u5","u6","u7"
    };

    
   this->setRange();
   if(ID<0||ID>=MAX_NDIMS_POSSIBLE){ 
       throw(std::out_of_range("dimension ID exceeds the acceptable range; Are you trying to initiate an ortogonal dimension using resiprocal dimensions constructor?"));
   }

   this->setName(default_axis_names[(int)(ID)]);

}
/// this function sets Linear range. 
void  Dimension::setRange(double rxMin,double rxMax,unsigned int nxBins)
{
    if(rxMin>rxMax){
        std::stringstream err;
        err<< "Attempting to set minimal integration limit higer then maximal in Dimension, ID N: "<<this->DimensionID<<std::endl;
        err<< "setting MinVal: "<<rxMin<<" MaxVal: "<<rxMax<<std::endl;

        throw(std::invalid_argument(err.str()));
    }
    this->rMin = rxMin;
    this->rMax = rxMax;

    this->setExpanded(nxBins);

}
/// set dimension expanded;
void
Dimension::setExpanded(double rxMin, double rxMax,unsigned int nBins)
{
    this->check_ranges(rxMin,rxMax);
    this->rMin=rxMin;
    this->rMax=rxMax;

    this->setExpanded(nBins);
}
//
void
Dimension::check_ranges(double rxMin,double rxMax)
{
   if(rxMin>rxMax){
        std::stringstream err;
        err<< "Attempting to set minimal integration limit higer then maximal in Dimension, ID N: "<<this->DimensionID<<std::endl;
        err<< "setting MinVal: "<<rxMin<<" MaxVal: "<<rxMax<<std::endl;

        throw(std::invalid_argument(err.str()));
    }
    if(rxMin>this->rMax||rxMax<this->rMin){
        std::stringstream err;
        err<< "Attempting to set integration limits outside the data range in Dimension ID N: "<<this->DimensionID<<std::endl;
        err<< "existing MinVal: "<<this->rMin<<" MaxVal: "<<this->rMax<<" Setting: minVal: "<<rxMin<<" maxVal: "<<rxMax<<std::endl;
        throw(std::invalid_argument(err.str()));

    }

}
void
Dimension::setExpanded(unsigned int nxBins)
{
    if(nxBins<1||nxBins>MAX_REASONABLE_BIN_NUMBER){
        std::stringstream err;
        err<< "Setting number of bins="<<nxBins<<" our of range in dimension, ID N: "<<this->DimensionID<<std::endl;
        throw(std::invalid_argument(err.str()));
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

}
/// clear dimension and sets integrated sign;
void
Dimension::setIntegrated(void)
{
  this->isIntegrated=true;
  this->nBins=1;
  this->Axis.clear();
  this->Axis.assign(2,0);
  this->Axis.front()= this->rMin;
  this->Axis.back() = this->rMax;
}
void
Dimension::setIntegrated(double rxMin){
    if(rxMin>this->rMax){
        std::stringstream err;
        err<< "Attempting to set minimal integration limit higer then maximal in dimension, ID N: "<<this->DimensionID<<std::endl;
        err<< "existing MaxVal: "<<this->rMax<<" setting minVal: "<<rxMin<<std::endl;
        throw(std::invalid_argument(err.str()));
    }
    this->rMin=rxMin; 
    this->setIntegrated();
}
void
Dimension::setIntegrated(double rxMin, double rxMax)
{
    this->check_ranges(rxMin,rxMax);

    this->rMin=rxMin; 
    this->rMax=rxMax; 
    this->setIntegrated();
}

Dimension::~Dimension()
{
}
}
}