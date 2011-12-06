#ifdef H_CONVERT_TO_MDEVENTS_COORD
#define H_CONVERT_TO_MDEVENTS_COORD
#include "MantidMDAlgorithms/ConvertToMDEvents.h"
#include "MantidMDAlgorithms/ConvertToMDUnitsConv.h"

namespace Mantid
{
namespace MDAlgorithms
{

///
template<Q_state Q,AnalMode MODE,CnvrtUnits CONV>
struct COORD_TRANSFORMER
{
      COORD_TRANSFORMER(ConvertToMDEvents *){}
    /**Template defines common interface to common part of the algorithm, where all variables
     * needed within the loop calculated outside of the loop. 
     * In addition it caluclates the property-dependant coordinates 
     *
     * @param n_ws_variabes -- subalgorithm specific number of variables, calculated from the workspace data
     *
     * @return Coord        -- subalgorithm specific number of variables, calculated from properties and placed into specific place of the Coord vector;
     * @return true         -- if all Coord are within the range requested by algorithm. false otherwise
     *
     * has to be specialized
    */
    inline bool calcGenericVariables(std::vector<coord_t> &Coord, size_t n_ws_variabes){
        UNUSED_ARG(Coord); UNUSED_ARG(n_ws_variabes);throw(Kernel::Exception::NotImplementedError(""));
        return false;}

   
    /** template generalizes the code to calculate Y-variables within the external loop of processQND workspace
     * @param X    -- vector of X workspace values
     * @param i    -- index of external loop, identifying current y-coordinate
     * 
     * @return Coord -- current Y coordinate, placed in the position of the Coordinate vector, specific for particular subalgorithm.    
     * @return true         -- if all Coord are within the range requested by algorithm. false otherwise   
     * 
     *  some default implementations possible (e.g mode Q3D,ragged  Any_Mode( Direct, indirect,elastic), 
     */
    inline bool calcYDepCoordinatese(std::vector<coord_t> &Coord,uint64_t i){
        UNUSED_ARG(Coord); UNUSED_ARG(i);  return true;}

    /** template generalizes the code to calculate all remaining coordinates, defined within the inner loop
     * @param X    -- vector of X workspace values
     * @param i    -- index of external loop, identifying generic y-coordinate
     * @param j    -- index of internal loop, identifying generic x-coordinate
     * 
     * @return Coord --Subalgorithm specific number of coordinates, placed in the proper position of the Coordinate vector   
     * @return true  -- if all Coord are within the range requested by algorithm. false otherwise   
     *
     * has to be specialized
     */
    inline bool calculate_ND_coordinatese(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord){
        UNUSED_ARG(X); UNUSED_ARG(i); UNUSED_ARG(j); UNUSED_ARG(Coord);throw(Kernel::Exception::NotImplementedError(""));
        return false;}
}; // end COORD_TRANSFORMER structure:


//----------------------------------------------------------------------------------------------------------------------
// SPECIALIZATIONS:
//----------------------------------------------------------------------------------------------------------------------
// NoQ,ANY_Mode -- no units conversion 
template<AnalMode MODE,CnvrtUnits CONV> 
struct COORD_TRANSFORMER<NoQ,MODE,CONV>
{
    inline bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd)    
    {
    // workspace defines 2 properties
         pHost->fillAddProperties(Coord,nd,2);
         for(size_t i=2;i<nd;i++){
            if(Coord[i]<pHost->dim_min[i]||Coord[i]>=pHost->dim_max[i])return false;
         }
      // get the Y axis; 
         pYAxis = dynamic_cast<API::NumericAxis *>(pHost->inWS2D->getAxis(1));
         if(!pYAxis ){ // the cast should be verified earlier; just in case here:
            throw(std::invalid_argument("Input workspace has to have Y-axis"));
          }
         return true;
    }

    inline bool calcYDepCoordinates(std::vector<coord_t> &Coord,uint64_t i)
    {
        Coord[1]                  = pYAxis->operator()(i);
        if(Coord[1]<pHost->dim_min[1]||Coord[1]>=pHost->dim_max[1])return false;
        return true;
    }

    inline bool calculate_ND_coordinates(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord){
        UNUSED_ARG(i);
        Coord[0]    = 0.5*( X[j]+ X[j+1]);
        if(Coord[0]<pHost->dim_min[0]||Coord[0]>=pHost->dim_max[0])return false;
        return true;
    }
    COORD_TRANSFORMER(ConvertToMDEvents *pConv):pHost(pConv)
    {}
private:
   // the variables used for exchange data between different specific parts of the generic ND algorithm:
    // pointer to Y axis of MD workspace
     API::NumericAxis *pYAxis;
     ConvertToMDEvents *pHost;

};
//
////----------------------------------------------------------------------------------------------------------------------
//
// modQ,ANY_Mode 
template<AnalMode MODE,CnvrtUnits CONV> 
struct COORD_TRANSFORMER<modQ,MODE,CONV>
{
    inline bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd){
        UNUSED_ARG(Coord);
        UNUSED_ARG(nd);
        throw(Kernel::Exception::NotImplementedError("not yet implemented"));
    }
    inline bool calcYDepCoordinates(std::vector<coord_t> &Coord,uint64_t i)
     {UNUSED_ARG(Coord); UNUSED_ARG(i); return false;}

      inline bool calculate_ND_coordinates(const MantidVec& X,uint64_t i,size_t j,std::vector<coord_t> &Coord){
          UNUSED_ARG(i);UNUSED_ARG(j);UNUSED_ARG(X);UNUSED_ARG(Coord);
          return false;
     }

    COORD_TRANSFORMER(ConvertToMDEvents *) {}
};
//------------------------------------------------------------------------------------------------------------------------------
// Q3D any mode 
template<AnalMode MODE>
inline double k_trans(double Ei, double E_tr){
    UNUSED_ARG(Ei);UNUSED_ARG(E_tr);
    throw(Kernel::Exception::NotImplementedError("Generic K_tr not implemented"));
}
template<>
inline double k_trans<Direct>(double Ei, double E_tr){
    return sqrt((Ei-E_tr)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
}
template<>
inline double k_trans<Indir>(double Ei, double E_tr){
    return sqrt((Ei+E_tr)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
}



//==========================================================================
template<AnalMode MODE,CnvrtUnits CONV> 
struct COORD_TRANSFORMER<Q3D,MODE,CONV>
{
    inline bool calcGenericVariables(std::vector<coord_t> &Coord, size_t nd)
    {
        // four inital properties came from workspace and all are interconnnected all additional defined by  properties:
        pHost->fillAddProperties(Coord,nd,4);
        for(size_t i=4;i<nd;i++){
            if(Coord[i]<pHost->dim_min[i]||Coord[i]>=pHost->dim_max[i])return false;
         }
        // energy 
         Ei  =  boost::lexical_cast<double>(pHost->inWS2D->run().getProperty("Ei")->value());
         // the wave vector of incident neutrons;
         ki=sqrt(Ei/PhysicalConstants::E_mev_toNeutronWavenumberSq); 
         // 
        rotMat = pHost->get_transf_matrix();
        UnitsConvertor.setUpConversion(this); 
      
        return true;
    }
    //
    inline bool calcYDepCoordinates(std::vector<coord_t> &Coord,uint64_t i){
              UNUSED_ARG(Coord); 
              UnitsConvertor.updateConversion(i);
              return true;}

    inline bool calculate_ND_coordinates(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord){

          coord_t E_tr = UnitsConvertor.get_x_converted(X,j);
          Coord[3]    = E_tr;
          if(Coord[3]<pHost->dim_min[3]||Coord[3]>=pHost->dim_max[3])return false;


          double k_tr = k_trans<MODE>(Ei,E_tr);
   
          double  ex = pHost->det_loc.det_dir[i].X();
          double  ey = pHost->det_loc.det_dir[i].Y();
          double  ez = pHost->det_loc.det_dir[i].Z();
          double  qx  =  -ex*k_tr;                
          double  qy  =  -ey*k_tr;
          double  qz  = ki - ez*k_tr;

         Coord[0]  = (coord_t)(rotMat[0]*qx+rotMat[3]*qy+rotMat[6]*qz);  if(Coord[0]<pHost->dim_min[0]||Coord[0]>=pHost->dim_max[0])return false;
         Coord[1]  = (coord_t)(rotMat[1]*qx+rotMat[4]*qy+rotMat[7]*qz);  if(Coord[1]<pHost->dim_min[1]||Coord[1]>=pHost->dim_max[1])return false;
         Coord[2]  = (coord_t)(rotMat[2]*qx+rotMat[5]*qy+rotMat[8]*qz);  if(Coord[2]<pHost->dim_min[2]||Coord[2]>=pHost->dim_max[2])return false;

         return true;
    }
    // constructor;
    COORD_TRANSFORMER(ConvertToMDEvents *pConv):pHost(pConv){}
    // helper functions: To assist with units conversion
    std::string              getNativeUnitsID()const{ return pHost->natural_units;}
    Kernel::Unit_sptr        getAxisUnits()const{return pHost->inWS2D->getAxis(0)->unit();}
    preprocessed_detectors & getPrepDetectors()const{return pHost->det_loc;}
    double             getEi()const{return (boost::lexical_cast<double>(pHost->inWS2D->run().getProperty("Ei")->value())); }
private:

    // the energy of the incident neutrons
    double Ei;
    // the wavevector of incident neutrons
    double ki;
    // the matrix which transforms the neutron momentums from lablratory to crystall coordinate system. 
    std::vector<double> rotMat;
    //
    ConvertToMDEvents *pHost;
    // class that performs untis conversion;
    UnitConversion<CONV,Q3D,MODE> UnitsConvertor ;
};

} // End MDAlgorighms namespace
} // End Mantid namespace

#endif