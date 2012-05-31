#include "MantidMDEvents/MDTransfNoQ.h"
//
namespace Mantid
{
namespace MDEvents
{

// register the class, whith conversion factory under NoQ name
DECLARE_MD_TRANSFID(MDTransfNoQ,NoQ);

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
        // obtain Y axis if availible
        pYAxis = ConvParams.getInWS()->getPAxis(1);

        nMatrixDim = getNMatrixDimensions(ConvertToMD::Undef,ConvParams.getInWS());
        this->addDimCoordinates = ConvParams.getAddCoord();
        
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

}
/**function returns units ID-s which this transformation prodiuces its ouptut.
   here it is usually input ws units, which are independent on emode */
std::vector<std::string> outputUnitID(ConvertToMD::EModes mode, API::MatrixWorkspace_const_sptr inWS)const
{
}
/**the default dimID-s in noQ mode equal to input WS dim-id-s */ 
std::vector<std::string> getDefaultDimID(ConvertToMD::EModes mode, API::MatrixWorkspace_const_sptr inWS)const
{
}
/**  returns the units, the input ws is actually in as they coinside with input units for this class */
const std::string inputUnitID(ConvertToMD::EModes mode, API::MatrixWorkspace_const_sptr inWS)const
{
}

MDTransfNoQ::MDTransfNoQ():
pYAxis(NULL),
nMatrixDim(0),
pDet(NULL)
{};

//// SPECIALIZATIONS:
////----------------------------------------------------------------------------------------------------------------------
//// ---->    NoQ
//// NoQ,ANY_Mode -- no units conversion. This templates just copies the data into MD events and not doing any momentum transformations
////
//#ifndef EXCLUDE_Q_TRANSFORMATION_NOQ
//template<ConvertToMD::AnalMode MODE,ConvertToMD::CnvrtUnits CONV,ConvertToMD::XCoordType TYPE,ConvertToMD::SampleType SAMPLE> 
//struct CoordTransformer<ConvertToMD::NoQ,MODE,CONV,TYPE,SAMPLE>
//{
//    inline bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd)    
//    {
//       // get optional Y axis which can be used in NoQ-kind of algorithms
//       pYAxis = pHost->getPAxis(1);
//       if(pYAxis){  // two inital properties came from workspace. All are independant; All other dimensions are obtained from properties
//           if(!pHost->fillAddProperties(Coord,nd,2))return false;
//       }else{        // only one workspace property availible;
//           if(!pHost->fillAddProperties(Coord,nd,1))return false;
//       }
//       //
//       pHost->getMinMax(dim_min,dim_max);
//        // set up units conversion defined by the host algorithm.  
//       ConvToMDPreprocDetectors Dummy;
//       CONV_UNITS_FROM.setUpConversion(Dummy,"",""); 
//       return true;
//    }
//

//
//


} // End MDAlgorighms namespace
} // End Mantid namespace

