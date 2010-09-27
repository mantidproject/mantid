#ifndef H_DIMENSIONRES
#define H_DIMENSIONRES
#include "Dimension.h"
/** This is the dimension which corresponds to reciprocal dimension
*
* It differs from usual dimension because can have some  nonortogonal direction in the WorkspaceGeometry system of coordinates
*
*/

class DimensionRes :   public Dimension
{
   // this is to initiate and set the Dimensions from the Geometry
    friend class Geometry;
   // this is to test this class separately
    friend class testDimension;
public:
    virtual ~DimensionRes(void);

    virtual std::vector<double> getCoord(void)const{return std::vector<double>(coord);}
private:
    DimensionRes(DimensionsID ID);
/// the coordinate of a reciprocal dimension in an WorkspaceGeometry system of coordinates. 
    std::vector<double> coord;
    // function sets the coordinates of the dimension;
    virtual void setCoord(const std::vector<double> &theCoord);
};
#endif