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

        nMatrixDim = getNMatrixDimensions(ConvertToMD::Undef,ConvParams.getInWS());
        this->addDimCoordinates = ConvParams.getAddCoord();
        
}

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
//    inline bool calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i)
//    {
//        CONV_UNITS_FROM.updateConversion(i);
//        if(pYAxis){   
//            if(Coord[1]<dim_min[1]||Coord[1]>=dim_max[1])return false;
//            Coord[1] = (coord_t)(pYAxis->operator()(i));
//        }
//        return true;
//    }
//
//    inline bool calc1MatrixCoord(const double& X,std::vector<coord_t> &Coord)const
//    {
//       if(X<dim_min[0]||X>=dim_max[0])return false;
//          
//       Coord[0]=(coord_t)X;
//       return true;
//    }
//    // should be actually on ICoordTransformer but there is problem with template-overloaded functions
//    inline bool calcMatrixCoord(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord)const
//    {
//       UNUSED_ARG(i);
//       double X_ev = CONV_UNITS_FROM.getXConverted(X,j);
//
//       return calc1MatrixCoord(X_ev,Coord);
//    }
//    inline bool convertAndCalcMatrixCoord(const double & X,std::vector<coord_t> &Coord)const
//    {
//         double X_ev = CONV_UNITS_FROM.getXConverted(X);
//         return calc1MatrixCoord(X_ev,Coord);
//    }   
//
//    // constructor;
//    CoordTransformer():pYAxis(NULL),pHost(NULL){} 
//
//    inline void setUpTransf(IConvertToMDEventsWS *pConv){
//        pHost = pConv;
//    }
//private:
//// class which would convert units
//     UnitsConverter<CONV,TYPE> CONV_UNITS_FROM;
//};

//
} // End MDAlgorighms namespace
} // End Mantid namespace

