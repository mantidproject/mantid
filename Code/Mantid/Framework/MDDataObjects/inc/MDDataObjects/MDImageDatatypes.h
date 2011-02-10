#ifndef MD_IMAGE_DATATYPES_H
#define MD_IMAGE_DATATYPES_H
#include <vector>
// to get uint64_t datatype
#include <MantidKernel/System.h>
/** The header contains the description of the datatypes, used by MDImage
 * There are currently three structures:
 <ul>
 * <li><b> MD_image_point</b> -- the structure describiung single point(cell) in multidimensional image array </li>
 * <li><b> MD_img_data   </b> -- the structure holding MD dimensional array above plus some additional information 
                                  about this array, like min, max and extend in each dimensions </li>
 * <li><b> point3D       </b>   -- Point3D short class used for visualisation purposes, through casting MD-point into 3D space
 *                                 effective casting involves dealing with sequence of points so, nothing big or essential in the class.  </li>
 </ul>
 * 
 * 
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

namespace Mantid{
       namespace MDDataObjects{
/** the structure describes an cell of MD_image array
     the connectivity of this point with other ppoints of this array can be established using MDGeometry
  */
struct MD_image_point
{
        double      s;    // signal 
        double     err;   // error
        uint64_t   npix;  // numer of data points (pixels) contributed into this point;
		MD_image_point():s(0),err(0),npix(0){}
};

/// structure of the multidimension data array, which is the basis of MDData class and should be exposed to modyfying algorighms
struct MD_img_data{
    size_t data_size;              ///< size of the data points array expressed as 1D array (number of data cells);
	size_t data_array_size;        ///< size of allocated part of MD_Image_point *data block (may be bigger then data_size)
    MD_image_point *data;           ///< multidimensional array of image cells, represented as a single dimensional array;
	uint64_t npixSum;              ///< sum of all npix fields in the data array; Used to check if the image is consistent with MDDPoints;
    // descriptors for dimensions;
    std::vector<size_t>dimStride;
    std::vector<size_t>dimSize;     ///< number of bin in this dimension
	// Min-max values for real data should sit with MDData points
    std::vector<double> min_value;  /**< min value of cut upplied in the selected dimension   -- should be the range set in geometry*/ 
    std::vector<double> max_value;  /**< max value of data extend in the selected dimension  */
    MD_img_data():data_size(0),data_array_size(0),data(NULL),npixSum(0){}

};

//*  the class descrines a point in 3D space and how you can cast an N-d point into it
class DLLExport point3D
{

public:
    point3D(void):x(0),y(0),z(0){};
    point3D(double x0, double y0, double z0):x(x0),y(y0),z(z0),s(0){};

    ~point3D(){};

    inline double GetX() const{return x;}
    inline double GetY() const{return y;}
    inline double GetZ() const{return z;}
    double GetS()const{return s;}
 //   double GetErr()const{return err;}
 //   unsigned int GetNpix()const{return npix;}

    double &X(){return x;}
    double &Y(){return y;}
    double &Z(){return z;}
    double &S(){return s;}
 //   double &Err(){return err;}
 //   unsigned long &Npix(){return npix;}
	point3D &operator=(const  MD_image_point &data){
		this->s  = data.s;
		return *this;
	}
 private:
    double x,y,z;
    double s;   // signal field;
 
};

}
}
#endif
