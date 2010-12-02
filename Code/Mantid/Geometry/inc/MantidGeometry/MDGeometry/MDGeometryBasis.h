#ifndef MD_GEOMETRY_BASIS_H
#define MD_GEOMETRY_BASIS_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include <string>
#include <map>
#include <boost/unordered_set.hpp>
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidGeometry/MDGeometry/MDWorkspaceConstants.h"
#include "MantidGeometry/MDGeometry/MDBasisDimension.h"

/** The class is the part of the VisualisationWorkspace and describes the basic multidimentional geometry of the object, 
*   e.g. the dimensions of the reciprocal space and other possible dimenions  
*   the reference reciprocal lattice, the size and shape of a primary crystall cell
*   and number of additional ortogonal dimensions, e.g. temperature, pressure etc. 
*
*   Class provides the reference framework for visualisation and analysis operations alowing to compare different workspaces and to modify 
*   the visualisation geometry as requested by user
*   It also keeps the crystall information necessary to transform a Mantid workspace into MD workspace

@author Alex Buts, RAL ISIS
@date 27/09/2010

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
  namespace Geometry
  {


    class DLLExport UnitCell //HACK: UnitCell type will be introduced by L. Chapon in near future. This Type is a temporary measure.
    {
	public: 
		UnitCell(void){};
    };

    //****************************************************************************************************************************************
    class DLLExport MDGeometryBasis
    {
    public:

      MDGeometryBasis(unsigned int nDimensions=1,unsigned int nReciprocalDimensions=1);  

      MDGeometryBasis(const std::set<MDBasisDimension>& mdBasisDimensions, const UnitCell &cell);

      std::set<MDBasisDimension> getNonReciprocalDimensions() const;
      std::set<MDBasisDimension> getReciprocalDimensions() const;
      std::set<MDBasisDimension> getBasisDimensions() const;
     
      ~MDGeometryBasis(void);
      /// return the numbers of dimensions in current geometry; 
      unsigned int getNumDims(void)const{return this->m_mdBasisDimensions.size();}
      /// returns the number of reciprocal dimensions
      unsigned int getNumReciprocalDims(void)const{return this->n_reciprocal_dimensions;};

      /// function checks if the ids supplied  coinside with the tags for current basis e.g all existing tags have to be here (the order of tags may be different)
      bool checkIdCompartibility(const std::vector<std::string> &newTags)const;

	  void init(const std::set<MDBasisDimension>& mdBasisDimensions, const UnitCell &cell);
    private:
      /// logger -> to provide logging, for MD workspaces
      static Kernel::Logger& g_log;
      /// number of total dimensions in dataset;
      unsigned int n_total_dim;
      /// number of reciprocal dimensions (which are non-orthogonal to each other n_rsprcl_dim<=n_total_dim)
      unsigned int n_reciprocal_dimensions;

      std::set<MDBasisDimension> m_mdBasisDimensions;
      UnitCell m_cell;

      /// checks if nDimensions consistent with n_reciprocal_dimensions; throws if not
      void check_nDims(unsigned int nDimensions,unsigned int nReciprocalDimensions);

      void checkInputBasisDimensions(const MDBasisDimension& dimension);

      /// it is unclear what is the meaning of =
      MDGeometryBasis& operator=(const MDGeometryBasis&);
    };
  } // namespace Geometry
}  // namespace MANTID
#endif
