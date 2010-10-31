#ifndef H_MD_WORKSPACE_CONSTANTS
#define H_MD_WORKSPACE_CONSTANTS
//
#include <string>

/*! The is the collection of the constants and enums, which are used in Multidimensional Workspace, usually in more then one file
*    
*  Collected here to simplify references and modifications, the dimensionsID will be replaced by proper class

    @author Alex Buts, RAL ISIS
    @date 01/10/2010

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


/** We define maximal number of dimensionsMAX_MD_DIMS_POSSIBLE  which a MD dataset can have because 
   \li a) every additional dimension will be expensive to process 
   \li b) minimal size of the visualisation dataset is 2^MAX_NDIMS_POSSIBLE, so this number has to be reasonable and any bigger number is probably because of insigned int  error; 
   \li c) if we need to increase the number of dimensions, it can be easy done here
*/
#define MAX_MD_DIMS_POSSIBLE 11

/// we are not going to rebin data on more than some number of bins. Lets set this constant as the limit for checks 
/// (in case of wrong word padding or -int casted to unsigned int) 
#define MAX_REASONABLE_BIN_NUMBER 1000000
//* MDPixels
/// the size of the buffer to read pixels while reading parts of datasets --should be optimized for performance and moved out of here;
#define PIX_BUFFER_SIZE 1000000
//***********************************************************************************************************************************


#endif