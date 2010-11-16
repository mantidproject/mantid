#ifndef H_DIMENSIONRES
#define H_DIMENSIONRES
#include "MantidGeometry/MDGeometry/MDDimension.h"
namespace Mantid{
    namespace Geometry{
/** This is the dimension which corresponds to reciprocal dimension
*
* It differs from usual dimension because can have some  nonortogonal direction in the WorkspaceGeometry system of coordinates
*
    @author Alex Buts, RAL ISIS
    @date 28/09/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
      /** enum describes reciprocal dimension numbers and reflects the request to up to three reciprocal dimensions */
      enum rec_dim{
        q1,
        q2,
        q3
      }; 

class DLLExport MDDimensionRes :   public MDDimension
{
   // this is to initiate and set the Dimensions from the Geometry
    friend class MDGeometry;
public:
    virtual ~MDDimensionRes(void);

  /// it is reciprocal dimension -> convenience function
    virtual bool isReciprocal(void)const{return true;}
 /// 
    virtual std::vector<double> const & getCoord(void)const{return this->coord;}
protected:
    MDDimensionRes(const std::string &ID,const rec_dim nDim);
private:
    // function sets the coordinates of the dimension;
    virtual void setCoord(const std::vector<double> &theCoord);
    /// number of this reciprocal dimension (one of 3)
    rec_dim nRecDim; 
};
} // MDDataObjects
} // Mantid
#endif
