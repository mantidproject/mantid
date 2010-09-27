#ifndef H_GEOMETRY
#define H_GEOMETRY
#include "WorkspaceGeometry.h"
#include "Dimension.h"
#include "DimensionRes.h"
/// class provides interface and convenient container to sizes and shapes of DND object
class SlicingData;

/** The class describes the geometry of the N-D visualisation workspace
*
*   It is specific workspace geometry, which is used for visualisation and analysis. 
*   It describes current size and shape of the data and its dimensions, including the dimensions which are integrated. 
*   It changes as the result of operations as user tries to look at the reciprocal space from different points of view and selects for 
*   analysis different dimensions and different parts of the space.
**/
class Geometry :   public WorkspaceGeometry
{
    // here we test the private methods for Geometry
    friend class testGeometry;
public:
    // the functions return the particular dimensions 
    Dimension & getXDimension(void)const{return *(theDimension[0]);}
    Dimension & getYDimension(void)const;
    Dimension & getZDimension(void)const;
    Dimension & getTDimension(void)const;
    std::vector<Dimension *> getIntegratedDimensions(void);

    /// functions return the pointer to the dimension requested as the dimension num. Throws if dimension is out of range. 
    Dimension * getDimension(unsigned int i)const;
    /// functions return the pointer to the dimension requested as the dimension ID. Returns NULL if no such dimension present in the Geometry or throws is ID is not casted properly;
    Dimension * getDimension(DimensionsID ID)const;

    /// function returns an axis vector of the dimension, specified by ID; it is 1 for orthogonal dimensions and triplet for the reciprocal 
    /// (but in a form of <1,0,0> if reciprocals are orthogonal to each other;
    std::vector<double> getOrt(DimensionsID id)const;
 
    ~Geometry(void);
protected: 

    Geometry(unsigned int nDimensions=4);
    /// the parameter describes the dimensions, which are not integrated. These dimensions are always at the beginning of the dimensions vector. 
    unsigned int n_expanded_dim;
    /// the array of Dimensions. Some are collapsed (integrated over)
     std::vector<Dimension *>  theDimension;
     /// array specifying the location of the dimension as the function of the specified ID
     std::vector<int>          theDimensionIDNum;

     /** function sets ranges of the data as in transformation request; Useless without real change of the ranges */
     void setRanges(const SlicingData &trf);

   /** function used to reset dimensions according to the requested transformaton and to arrange them properly, e.g. according to the order of the 
       dimensions in IDs   plus all non-collapsped dimensions first */
     void arrangeDimensionsProperly(const std::vector<DimensionsID> &IDS);
   /* function returns tne location of the dimension specified by the ID, in the array theDimension (in the Geomerty)
       negative value specifies that the requested dimension is not present in the array. */
    int getDimNum(DimensionsID ID)const;
    /// function resets WorkspaceGeometry and Geometry as new 
    /// if any ID in the list is different from existing or just resets the structure into new ID shape if new ID-s list includes all from the old one;
    /// when the structure is indeed 
    void reinit_Geometry(const SlicingData &trf);
private:
    void init_empty_dimensions(const std::vector<DimensionsID> &IDS);
};
#endif
