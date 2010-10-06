#ifndef H_MD_WORKSPACE_CONSTANTS
#define H_MD_WORKSPACE_CONSTANTS
/*! The is the collection of the constants and enums, which are used in Multidimensional Workspace, usually in more then one file
*    
*  Collected here to simplify references and modifications

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
//***********************************************************************************************************************************
namespace Mantid{
    namespace MDDataObjects{

/** WorkspaceGeometry: 
*   The enum lists various dimensions ID, which can be found in the dataset
*   list is not comprehansive and created to simplify the dimensions identification.
*   in the program. 
*   The agreement is that the first three ID-s are the special dimensions which can be non-orthogonal 
*   and all higher dimensions are orthogonal to  that and to each other 
*   (polarisation needs further clarification)
*/
    enum DimensionsID
    {
        eh,  //! ! Three special dimensions h,k,l
        ek,  //! ! named after Miller indexes
        el,  //! !
        /// The orded of this indexes is important as number of algorithms deal with it trying to (UGLY! Redesighn!!!)
        en,  //* Energy
        u1,  //* something else e.g. Sample temperature
        u2,  //* something else e.g. sample pressure
        u3,  //* something else e.g. longnitudional component of polarisation
        u4,  //* something else e.g. transversal component of polarisation
        u5,   /// 
        u6,   /// something else
        u7,   /// something else
        MAX_NDIMS_POSSIBLE
    };

//* Dimension
/// we are not going to rebin data on more than some number of bins. Lets set this constant as the limit for checks 
/// (in case of wrong word padding or -int casted to unsigned int) 
/// 
#define MAX_REASONABLE_BIN_NUMBER 1000000
//* MDPixels
/// the size of the buffer to read pixels while reading parts of datasets --should be optimized for performance;
#define PIX_BUFFER_SIZE 1000000

    }
}
#endif