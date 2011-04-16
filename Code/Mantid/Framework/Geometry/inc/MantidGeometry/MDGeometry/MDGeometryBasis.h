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

/** The class is the part of the VisualisationWorkspace and describes the basic multidimentional geometry of the object, 
*   e.g. the dimensions of the reciprocal space and other possible dimenions  
*   the reference reciprocal lattice, the size and shape of a primary crystall cell
*   and number of additional ortogonal dimensions, e.g. temperature, pressure etc. 
*
*   Class provides the reference framework for visualisation and analysis operations alowing to compare different workspaces and to modify 
*   the visualisation geometry as requested by user
* Obsolete: may be temporary
*   It also keeps the crystall information necessary to transform a Mantid workspace into MD workspace
*Current meaning:
*   It is the collections of the basis vectors of extended reciprocal lattice so, up to 3 vectors in k-space and 

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
	/// temporary measure; need to include real class 
    class OrientedCrystal;
    //****************************************************************************************************************************************
    class EXPORT_OPT_MANTID_GEOMETRY MDGeometryBasis
    {
    public:
      /// empty constructor used mainly to provide a dummy object for reading data into it
      MDGeometryBasis(unsigned int nDimensions=1,unsigned int nReciprocalDimensions=1);  
	  /// fully functional constructor; 
      MDGeometryBasis(const std::set<MDBasisDimension>& mdBasisDimensions,boost::shared_ptr<UnitCell> spSample);

      std::set<MDBasisDimension> getNonReciprocalDimensions() const;
      std::set<MDBasisDimension> getReciprocalDimensions() const;
      std::set<MDBasisDimension> getBasisDimensions() const;
     
      ~MDGeometryBasis(void);
      /// return the numbers of dimensions in current geometry; 
      size_t getNumDims() const {return this->m_mdBasisDimensions.size();}
      /// returns the number of reciprocal dimensions
      size_t getNumReciprocalDims() const {return this->n_reciprocal_dimensions;};
	  

      /** function checks if the ids supplied  coinside with the tags for current basis e.g all 
	   *  existing tags have to be there (the order of tags may be different) */
      bool checkIdCompartibility(const std::vector<std::string> &newTags)const;

	  void init(const std::set<MDBasisDimension>& mdBasisDimensions,boost::shared_ptr<UnitCell> spSample);
	  // copy constructor is used and can be default
    private:
	 /// shared pointer to a class, describing reciporcal lattice of the sample and its orientation if crystal;
	  boost::shared_ptr<UnitCell> spSample;
      /// logger -> to provide logging, for MD workspaces
      static Kernel::Logger& g_log;
      /// number of total dimensions in dataset;
      size_t n_total_dim;
      /// number of reciprocal dimensions (which are non-orthogonal to each other n_rsprcl_dim<=n_total_dim)
      size_t n_reciprocal_dimensions;
      /// the directions in reciprocal lattice plus all addtional orthogonal dimensions
      std::set<MDBasisDimension> m_mdBasisDimensions;
 

      /// checks if nDimensions consistent with n_reciprocal_dimensions; throws if not
      void check_nDims(unsigned int nDimensions,unsigned int nReciprocalDimensions);

      void checkInputBasisDimensions(const MDBasisDimension& dimension);

      /// it is unclear what is the meaning of =
      MDGeometryBasis& operator=(const MDGeometryBasis&);
    };
  } // namespace Geometry
}  // namespace MANTID
#endif
