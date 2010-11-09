#ifndef H_MD_WORKSPACE
#define H_MD_WORKSPACE
#include "MDDataObjects/MDPixels.h"
#include "MantidAPI/WorkspaceFactory.h"


/** Class is empty to provide calling name for all MD-workspace components and provide interfaces to all workspace  public set functions 
*   and to the one not included in separate components (like get_detectors)    (in a future) 
   
    
    @author Alex Buts, RAL ISIS
    @date 08/10/2010

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

namespace Mantid
{
    namespace MDDataObjects
{

class DLLExport MDWorkspace :  public MDPixels
{
    public:
     
     virtual long getMemorySize(void)const{return MDData::getMemorySize();} // + mdPixels memory size
     MDWorkspace(unsigned int nDimensions=4):MDPixels(nDimensions){};
     virtual ~MDWorkspace(void){};

     void alloc_mdd_arrays(const MDGeometryDescription &transf){MDData::alloc_mdd_arrays(transf);}
     void finalise_rebinning(void){MDPixels::finalise_rebinning();}
  
};


///shared pointer to the MD workspace base class
typedef boost::shared_ptr<MDWorkspace> MDWorkspace_sptr;
///shared pointer to the MD workspace base class (const version)
typedef boost::shared_ptr<const MDWorkspace> MDWorkspace_const_sptr;


   
}}



#endif