#ifndef MD_GEOMETRY_BASIS_H
#define MD_GEOMETRY_BASIS_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidGeometry/DllExport.h"
#include "MantidGeometry/MDGeometry/MDWorkspaceConstants.h"
#include "MantidGeometry/MDGeometry/MDBasisDimension.h"
#include "MantidGeometry/Crystal/UnitCell.h" // has to be replaced by oriented cell later
#include <vector>
#include <string>
#include <map>
#include <boost/unordered_set.hpp>
#include <MantidGeometry/Crystal/OrientedLattice.h>

/** The class is the part of the VisualisationWorkspace and describes the basic multidimentional geometry of the object, 
*   e.g. the dimensions of the reciprocal space and other possible dimenions  
*   the reference reciprocal lattice, the size and shape of a primary crystall cell
*   plus number of additional ortogonal dimensions, e.g. temperature, pressure etc. 
*
*   Class provides the reference framework for visualisation and analysis operations alowing to compare different workspaces and to modify 
*   the visualisation geometry as requested by user
*
*   It is the collections of the basis vectors of extended reciprocal lattice namely, from 1 to 3 bais vectors of k-space 
*   adjacent to reciprocal lattice cell (orts of projection axes defined by u1 || a*, u2 in plane of a* and b* i.e. crystal Cartesian axes)
*   and all additional basis  vectors for orthogonal dimensions (energy transfer, temperature, etc.) 
*   In addition to that the class keeps pointer to the the unit crystal cell of the reciprocal lattice 
*   If the temperature is present among dimensions, the unit cell should depend on it?

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
    //****************************************************************************************************************************************
    class EXPORT_OPT_MANTID_GEOMETRY MDGeometryBasis
    {
    public:
      /// empty constructor used mainly to provide a dummy object for reading data into it
      MDGeometryBasis(size_t nDimensions=1,size_t nReciprocalDimensions=1);  
	  /// fully functional constructor; 
      MDGeometryBasis(const std::set<MDBasisDimension>& mdBasisDimensions,boost::shared_ptr<OrientedLattice> spSample);

      std::set<MDBasisDimension> getNonReciprocalDimensions() const;
      std::set<MDBasisDimension> getReciprocalDimensions() const;
      std::set<MDBasisDimension> getBasisDimensions() const;
     
      ~MDGeometryBasis(void);
      /// return the numbers of dimensions in current geometry; 
      size_t getNumDims() const {return this->m_mdBasisDimensions.size();}
      /// returns the number of reciprocal dimensions
      size_t getNumReciprocalDims() const {return this->n_reciprocal_dimensions;};
	  /** function returns symbolioc names(IDs) of the dimensions. 
	   *  These names have to coinside with  id-s found in MDDataPoints as each 
	   *  dimID describes one column of MDDPoints table*/
	  std::vector<std::string> getBasisIDs(void)const;
	  /** function returns the existing Reciprocal geometry basis, which describes MDDPoints basis too; 
	    *  It is an orthogonal basis which differs from the basis attached to the reciprocal cell as some axis 
		*  can be in different position in relation to the cell (cell has x-axis placed along a* direction of unit cell 
		   when recBasis can have it everywhere

		   TODO: what to return in 1D mode (powder) (rather how to interpret this mode)
		*/
	  std::vector<V3D>  get_constRecBasis(void)const;

    /// Returns reference to the unit cell, which used in basis; Will throw through dereference of sp if unit cell is not defined;
    OrientedLattice const & get_constOrientedLattice()const{return *spSample;}

    /// Returns reference to the unit cell, which used in basis; Will throw through dereference of sp if unit cell is not defined;
    OrientedLattice & get_OrientedLattice() {return *spSample;}

      /** function checks if the ids supplied  coinside with the tags for current basis e.g all 
	   *  existing tags have to be there (the order of tags may be different) */
      bool checkIdCompartibility(const std::vector<std::string> &newTags)const;

	  void init(const std::set<MDBasisDimension>& mdBasisDimensions,boost::shared_ptr<OrientedLattice> spSample);
	  // copy constructor is used and can be default

    private:
	 /// shared pointer to a class, describing reciporcal lattice of the sample and its orientation if crystal;
	  boost::shared_ptr<OrientedLattice> spSample;
      /// logger -> to provide logging, for MD workspaces
      static Kernel::Logger& g_log;
      /// number of total dimensions in dataset;
      size_t n_total_dim;
      /// number of reciprocal dimensions (which are non-orthogonal to each other n_rsprcl_dim<=n_total_dim)
      size_t n_reciprocal_dimensions;
      /// the directions in reciprocal lattice plus all addtional orthogonal dimensions
      std::set<MDBasisDimension> m_mdBasisDimensions;
 

      /// checks if nDimensions consistent with n_reciprocal_dimensions; throws if not
      void check_nDims(size_t nDimensions, size_t nReciprocalDimensions);

      void checkInputBasisDimensions(const MDBasisDimension& dimension);

      /// it is unclear what would be the meaning of =
      MDGeometryBasis& operator=(const MDGeometryBasis&);
    };
  } // namespace Geometry
}  // namespace MANTID
#endif
