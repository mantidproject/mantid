#ifndef H_SLICICNG_DATA
#define H_SLICICNG_DATA
#include "MDDataObjects/MDGeometry.h"
#include "CenterpieceRebinning.h"

//* class describes slicing and rebinning matrix;
namespace Mantid{
    namespace MDDataObjects{
class DLLExport SlicingProperty
{
public:
    SlicingProperty(std::vector<DimensionsID> &IDs);
    SlicingProperty(unsigned int numDims=4);
    SlicingProperty(const MDGeometry &origin);
   ~SlicingProperty(void);
   unsigned int getNumDims(void)const{return nDimensions;}
   /// the function returns the rotation matrix which allows to transform vector inumber i into the basis;
   // TO DO : it is currently a stub returning argument independant unit matrix; has to be written propely
   std::vector<double> rotations(unsigned int i,const std::vector<double> basis[3])const;

   std::vector<double> getCoord(DimensionsID id)const;

   double shift(unsigned int i)const; 
   double cutMin(unsigned int i)const;
   double cutMax(unsigned int i)const;
   unsigned int numBins(unsigned int i)const;
   bool isAxisNamePresent(unsigned int i)const;
   std::string getAxisName(unsigned int i)const;
   DimensionsID getPAxis(unsigned int i)const;  
   std::vector<DimensionsID> getPAxis(void)const{return std::vector<DimensionsID>(this->pAxis);}



//*** SET
   void setCoord(DimensionsID i,const std::vector<double> &coord){
       int ind = this->AxisID[i];
       if(ind>=0){  setCoord(unsigned int(ind),coord);
       }
   }

   void setShift(DimensionsID i,double Val){
       int ind = this->AxisID[i];
       if(ind>=0){ setShift(unsigned int(ind),Val);
       }

   }
   void setCutMin(DimensionsID i,double Val){
       int ind = this->AxisID[i];
       if(ind>=0){   setCutMin(unsigned int(ind),Val);
       }
   }

   void setCutMax(DimensionsID i,double Val){
       int ind = this->AxisID[i];
       if(ind>=0){  setCutMax(unsigned int(ind),Val);
       }
   }
   void setNumBins(DimensionsID i,unsigned int Val){
       int ind = this->AxisID[i];
       if(ind>=0){   setNumBins(unsigned int(ind),Val);
       }
   }
   void setAxisName(DimensionsID i,const std::string &Name){
       int ind = this->AxisID[i];
       if(ind>=0){   setAxisName(unsigned int(ind),Name);
       }
   }

   void setCoord(unsigned int i,const std::vector<double> &coord);

   void setShift(unsigned int i,double Val); 
   void setCutMin(unsigned int i,double Val);
   void setCutMax(unsigned int i,double Val);
   void setNumBins(unsigned int i,unsigned int Val);
   void setAxisName(unsigned int i,const std::string &Name);

/**  function sets the ID requested into the position, defined by index i;
 *
 * If the index has been present in the array before, it moves into new place, swapping places with the one, which was in this location before. 
 * If it has not been present in the array, it replaces the index, which was on this place before.  */
   void setPAxis(unsigned int i, DimensionsID ID);

private:

    unsigned int nDimensions;               /**< real number of dimensions in the target dataset. 
                                                If source dataset has more dimensions than specified here, all other dimensions will be collapsed */
    std::vector<double> coordinates[3];     //< The coordinates of the target dataset in the WorkspaceGeometry system of coordinates (define the rotation matrix for qx,qy,qz coordinates;) 
    std::vector<double> trans_bott_left;    //< shift in all directions (tans_elo is 4th element of transf_bott_left. Shift expressed in the physical units
    std::vector<double> cut_min;            //< min limits to extract data;
    std::vector<double> cut_max;            //< max limits to extract data;
    std::vector<unsigned int> nBins;        //< number of bins in each direction, bins of size 1 are integrated (collased);
    std::vector<std::string>  AxisName;     //< new names for axis; 
    std::vector<int> AxisID;                //< the vector of size MAX_NDIMS_POSSIBLE, which describes the order of the dimensions in the current object;
    std::vector<DimensionsID> pAxis;        //< the vector of size nDimensions, which describes the order of the dimensions in the final object;


    /** auxiliary function which check if the index requested is allowed. ErrinFuncName allows to add to the error message the name of the colling function*/
    void check_index(unsigned int i,const char *ErrinFuncName)const;
    /** the function which provides default constructor functionality */
    void intit_default_slicing(unsigned int nDims=4);
};

} // MDDataObjects
} // Mantid
#endif
