#include "stdafx.h"
#include "CenterpieceRebinning.h"

void
CenterpieceRebinning::build_transformation_matrix()
{

    this->trans_bott_left.resize(this->nDims);
    this->cut_min.resize(this->nDims);
    this->cut_max.resize(this->nDims);
/*
    int i,j,ic;
    for(i=0;i<this->nDims;i++){
        ///if(
        //transform data limits into integer length along each axis selected; 
        if(this->dim_sizes[i]==0){ // if axis is a reduction axis, we do not rescale in this direction ->
            // its limits equal to cut-off limits but axis_step should be 1 ;
            Axis[i]->push_back(trf.cut_min[i]);
            Axis[i]->push_back(trf.cut_max[i]);
            axis_step[i]= 1;
        }else{                    // all limits are rescaled into ints along the axis otherwise;
            axis_step[i] =(trf.cut_max[i]-trf.cut_min[i])/this->dim_sizes[i];
            for(j=0;j<=this->dim_sizes[i];j++){
                Axis[i]->push_back(trf.cut_min[i]+i*axis_step[i]);
            }
        }
        shifts[i]    =trf.trans_bott_left[i];
        min_limit[i] =trf.cut_min[i]/axis_step[i];   
        max_limit[i] =trf.cut_max[i]/axis_step[i];
    }
    for(i=0;i<3;i++){
        ic = i*3;
        for(j=0;j<3;j++){
            rotations[i+j*3]=trf.rotations[i+j*3]/axis_step[i];
        }
    }
 
    unsigned int i;
    SlicingData tt;
    for(i=0;i<this->n_total_dim;i++){
        this->theDimension[i]->setRange(trf.cutMin(i),trf.cut_max(i),trf.numBins(i));
        if(trf.isAxisNamePresent(i)){
            this->theDimension[i]->setName(trf.AxisName(i));
        }
    }
*/
}
CenterpieceRebinning::CenterpieceRebinning(const SQW &orig,const SlicingData &tansformation):
origin(&orig),
trf(&tansformation),
nDims(trf->getNumDims())
{
    if(origin->getNumDims()!=trf->getNumDims()){
        throw(errorMantid("CenterpieceRebinning::CenterpieceRebinning: dimensions of the rebinning request do not equal to the dimensions of the original dataset"));
    }
    this->build_transformation_matrix();
}

CenterpieceRebinning::~CenterpieceRebinning(void)
{
}
