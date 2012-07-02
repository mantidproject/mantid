#include "MantidMDEvents/MDTransfNoQ.h"
//
namespace Mantid
{
namespace MDEvents
{

// register the class, whith conversion factory under NoQ name
DECLARE_MD_TRANSFID(MDTransfNoQ,CopyToMD);

/** Method fills-in all additional properties requested by user and not defined by matrix workspace itselt. 
 *  it fills in [nd - (1 or 2 -- depending on input ws)] values into the Coord vector;
 *
 *@param Coord -- input-output vector of MD-coordinates
 *@param nd    -- number of current dimensions
 *
 *@returns     -- Coord vector with nd-(1 or 2, depending on input ws) values of MD coordinates
 */
bool MDTransfNoQ::calcGenericVariables(std::vector<coord_t> &Coord, size_t nd)
{      
    // sanity check. If fails, something went fundamentally wrong
    if(nMatrixDim+addDimCoordinates.size()!=nd)
    {
        std::string ERR="Number of matrix dimensions: "+boost::lexical_cast<std::string>(nMatrixDim)+
                        " plus number of additional dimensions: "+boost::lexical_cast<std::string>(addDimCoordinates.size())+
                        " not equal to number of workspace dimensions: "+boost::lexical_cast<std::string>(nd);
        throw(std::invalid_argument(ERR));
    }
   // if one axis is numeric, 1  coordinate  came from workspace
   // if two axis are numeric, two values come from the ws. All other are defined by properties. 
    unsigned int ic(0);
    for(unsigned int i=nMatrixDim;i<nd;i++){
        if(addDimCoordinates[ic]<dim_min[i] || addDimCoordinates[ic]>=dim_max[i])return false;
        Coord[i]= addDimCoordinates[ic];
        ic++;
    }
    return true;
}
void MDTransfNoQ::initialize(const MDWSDescription &ConvParams)
{ 

        // get pointer to the positions of the detectors
        std::vector<Kernel::V3D> const & DetDir = ConvParams.getDetectors()->getDetDir();
        pDet = &DetDir[0];     //

        // get min and max values defined by the algorithm. 
        ConvParams.getMinMax(dim_min,dim_max);

        nMatrixDim = getNMatrixDimensions(ConvertToMD::Undef,ConvParams.getInWS());
        this->addDimCoordinates = ConvParams.getAddCoord();
        API::NumericAxis *pXAx;
        this->getAxes(ConvParams.getInWS(),pXAx,pYAxis);
        
}
/** Method updates the value of preprocessed detector coordinates in Q-space, used by other functions 
 *@param i -- index of the detector, which corresponds to the spectra to process. 
 * 
*/
bool MDTransfNoQ::calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i)
{
    if(pYAxis){   
       if(Coord[1]<dim_min[1]||Coord[1]>=dim_max[1])return false;
        Coord[1] = (coord_t)(pYAxis->operator()(i));
    }
    return true;
}
bool MDTransfNoQ::calcMatrixCoord(const double& X,std::vector<coord_t> &Coord)const
{
       if(X<dim_min[0]||X>=dim_max[0])return false;
          
       Coord[0]=(coord_t)X;
       return true;

}

 /** return the number of dimensions, calculated by the transformation from the workspace.
    Depending on ws axis units, the numebr here is either 1 or 2* and is independent on emode*/
unsigned int MDTransfNoQ::getNMatrixDimensions(ConvertToMD::EModes mode, API::MatrixWorkspace_const_sptr inWS)const
{
    UNUSED_ARG(mode);

    API::NumericAxis *pXAx,*pYAx;
    this->getAxes(inWS,pXAx,pYAx);

    unsigned int nMatrDim = 1;
    if(pYAx)nMatrDim =2;
    return nMatrDim ;

}
 // internal helper function which extract one or two axis from input matrix workspace;
void  MDTransfNoQ::getAxes(API::MatrixWorkspace_const_sptr inWS,API::NumericAxis *&pXAxis,API::NumericAxis *&pYAxis)
{
   // get the X axis of input workspace, it has to be there; if not axis throws invalid index
    pXAxis = dynamic_cast<API::NumericAxis *>(inWS->getAxis(0));
    if(!pXAxis ){
        std::string ERR="Can not retrieve X axis from the source workspace: "+inWS->getName();
        throw(std::invalid_argument(ERR));
    }
    // get optional Y axis which can be used in NoQ-kind of algorithms 
    pYAxis = dynamic_cast<API::NumericAxis *>(inWS->getAxis(1));

}

/**function returns units ID-s which this transformation prodiuces its ouptut.
   here it is usually input ws units, which are independent on emode 
 * @param  mode -- current energy analysis mode (not used in NoQ mode)
 * @param  inWS -- input matrix workspace shared pointer
 *
 */
std::vector<std::string> MDTransfNoQ::outputUnitID(ConvertToMD::EModes mode, API::MatrixWorkspace_const_sptr inWS)const
{
    UNUSED_ARG(mode);

    std::vector<std::string> rez;
    API::NumericAxis *pXAxis,*pYAx;
    this->getAxes(inWS,pXAxis,pYAx);

    if(pYAx){
        rez.resize(2);
        rez[1] = pYAx->unit()->unitID();
    }else{
        rez.resize(1);
    }
    rez[0] = pXAxis->unit()->unitID();

    return rez;
}
/**the default dimID-s in noQ mode equal to input WS dim-id-s */ 
std::vector<std::string> MDTransfNoQ::getDefaultDimID(ConvertToMD::EModes mode, API::MatrixWorkspace_const_sptr inWS)const
{
    return this->outputUnitID(mode,inWS);
}
/**  returns the units, the input ws is actually in as they coinside with input units for this class */
const std::string MDTransfNoQ::inputUnitID(ConvertToMD::EModes mode, API::MatrixWorkspace_const_sptr inWS)const
{
    UNUSED_ARG(mode);
    API::NumericAxis *pXAxis;
   // get the X axis of input workspace, it has to be there; if not axis throws invalid index
    pXAxis = dynamic_cast<API::NumericAxis *>(inWS->getAxis(0));
    if(!pXAxis ){
        std::string ERR="Can not retrieve X axis from the source workspace: "+inWS->getName();
        throw(std::invalid_argument(ERR));
    }
    return pXAxis->unit()->unitID();
}

MDTransfNoQ::MDTransfNoQ():
nMatrixDim(0),
pYAxis(NULL),
pDet(NULL)
{};


} // End MDAlgorighms namespace
} // End Mantid namespace

