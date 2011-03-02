#ifndef MANTID_ALGORITHMS_MASKDETECTORSIF_H_
#define MANTID_ALGORITHMS_MASKDETECTORSIF_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace1D.h"
#include <boost/function.hpp>

// To be compatible with VSC Express edition that does not have tr1
#ifndef HAS_UNORDERED_MAP_H
#include <map>
#else
#include <tr1/unordered_map>
#endif

namespace Mantid
{
namespace Algorithms
{
/**
 *
This algorithm is used to select/deselect detectors in a *.cal file.


 @author Laurent Chapon, Pascal Manuel ISIS Facility, Rutherford Appleton Laboratory
 @date 06/07/2009

 Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport MaskDetectorsIf: public API::Algorithm
{
public:
  /// Default constructor
  MaskDetectorsIf();
  /// Destructor
  virtual ~MaskDetectorsIf();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "MaskDetectorsIf"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 2; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Diffraction"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
#ifndef HAS_UNORDERED_MAP_H
  /// Typedef for detector to value map
	typedef std::map<int,bool> udet2valuem;
#else
  // Typedef for det to value map
	typedef std::tr1::unordered_map<int,bool> udet2valuem;
#endif
	/// A map of detector numbers to mask boolean
	udet2valuem umap;
	/// Get the properties
	void retrieveProperties();
	/// Create a new cal file
	void createNewCalFile(const std::string& oldfile,const std::string& newfile);
	/// The input workspace
	API::MatrixWorkspace_const_sptr inputW;
	/// The Value parameter
	double value;
	/// A comparator function
	boost::function<bool (double,double)> compar_f;
	/// Whether select is on or off
	bool select_on;
	/// Overidden init
	void init();
	/// Overidden exec
	void exec();

};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_MASKDETECTORSIF_H_*/
