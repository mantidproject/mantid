#ifndef H_DIMENSIONRES
#define H_DIMENSIONRES
#include "Dimension.h"
namespace Mantid{
    namespace MDDataObjects{
/** This is the dimension which corresponds to reciprocal dimension
*
* It differs from usual dimension because can have some  nonortogonal direction in the WorkspaceGeometry system of coordinates
*
*/

class DLLExport DimensionRes :   public Dimension
{
   // this is to initiate and set the Dimensions from the Geometry
    friend class MDGeometry;
public:
    virtual ~DimensionRes(void);

    virtual std::vector<double> const & getCoord(void)const{return this->coord;}
protected:
    DimensionRes(DimensionsID ID);
private:
    // function sets the coordinates of the dimension;
    virtual void setCoord(const std::vector<double> &theCoord);
};
} // MDDataObjects
} // Mantid
#endif