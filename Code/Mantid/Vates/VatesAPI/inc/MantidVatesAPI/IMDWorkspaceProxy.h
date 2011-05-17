#ifndef MANTID_VATES_IMDWorkspaceProxy_H_
#define MANTID_VATES_IMDWorkspaceProxy_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IMDWorkspace.h"
#include <boost/scoped_ptr.hpp>
#include <map>

#include "MantidVatesAPI/DimensionComparitor.h"

#include <boost/function.hpp>
#include <boost/bind.hpp>

namespace Mantid
{
namespace VATES
{

/** 
 * Acts as a proxy for IMDWorkspaces. Remaps dimensions for the IMDWorkspace instance that it wraps. Acts like a decorator, but does not add any new outward behaviour over IMDWorkspace.
 *

 @author Owen Arnold, Tessella plc
 @date 30/03/2011

 Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

class DLLExport IMDWorkspaceProxy: public Mantid::API::IMDWorkspace
{

public:
  /// type definitions for member function returning Dimension_sptr
  typedef Mantid::Geometry::IMDDimension_const_sptr (Mantid::API::IMDWorkspace::*MemFuncGetter)() const;

  /// Constructional method.
  static Mantid::API::IMDWorkspace_sptr New(Mantid::API::IMDWorkspace_sptr workspace,
      Dimension_const_sptr xDim, Dimension_const_sptr yDim, Dimension_const_sptr zDim, Dimension_const_sptr tDim);

  /// Destructor.
  virtual ~IMDWorkspaceProxy();

  /// Get X Dimension via internal memberfunction-to-dimension id map.
  virtual Mantid::Geometry::IMDDimension_const_sptr getXDimension(void) const;
  /// Get Y Dimension via internal memberfunction-to-dimension id map.
  virtual Mantid::Geometry::IMDDimension_const_sptr getYDimension(void) const;
  /// Get Z Dimension via internal memberfunction-to-dimension id map.
  virtual Mantid::Geometry::IMDDimension_const_sptr getZDimension(void) const;
  /// Get t Dimension
  virtual Mantid::Geometry::IMDDimension_const_sptr getTDimension(void) const;

  boost::function<double(size_t, size_t, size_t, size_t)> getMappedSignalAt();
  //-----------------------------------------------------------------------------------------------
  /** find
   * @param key: key to find.
   */
  MemFuncGetter find(const std::string& key) const;

  virtual uint64_t getNPoints() const;

  virtual size_t getNumDims() const;

  virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getDimension(std::string id) const;

  virtual const std::vector<std::string> getDimensionIDs() const;

  virtual const Mantid::Geometry::SignalAggregate& getPoint(size_t index) const;

  virtual const Mantid::Geometry::SignalAggregate& getCell(size_t dim1Increment) const;

  virtual const Mantid::Geometry::SignalAggregate& getCell(size_t dim1Increment,
      size_t dim2Increment) const;

  virtual const Mantid::Geometry::SignalAggregate& getCell(size_t dim1Increment,
      size_t dim2Increment, size_t dim3Increment) const;

  virtual const Mantid::Geometry::SignalAggregate& getCell(size_t dim1Increment,
      size_t dim2Increment, size_t dim3Increment, size_t dim4Increment) const;

  virtual const Mantid::Geometry::SignalAggregate& getCell(...) const;

  virtual std::string getWSLocation() const;

  virtual std::string getGeometryXML() const;

  virtual const std::string id() const;

  virtual size_t getMemorySize() const;

  /// Get the signal at the specified index.
  virtual double getSignalAt(size_t index1, size_t index2, size_t index3, size_t index4) const;

  /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t), normalized by cell volume
  virtual double getSignalNormalizedAt(size_t index1, size_t index2, size_t index3, size_t index4) const;

  virtual Mantid::Geometry::VecIMDDimension_const_sptr getNonIntegratedDimensions() const;

private:

  /// Constructor.
  IMDWorkspaceProxy(Mantid::API::IMDWorkspace_sptr workspace, Dimension_const_sptr xDim, Dimension_const_sptr yDim,
      Dimension_const_sptr zDim, Dimension_const_sptr tDim);

  /// Separate initalizer as may throw.
  void initalize();

  /// workspace shared ptr.
  Mantid::API::IMDWorkspace_sptr m_workspace;
  /// Actual x dimension
  Dimension_const_sptr m_xDimension;
  /// Actual y dimension
  Dimension_const_sptr m_yDimension;
  /// Actual z dimension
  Dimension_const_sptr m_zDimension;
  /// Actual t dimension
  Dimension_const_sptr m_tDimension;
  /// map of id to member function.
  std::map<std::string, MemFuncGetter> m_fmap;


  boost::function<double(size_t, size_t, size_t, size_t)> m_function;

  IMDWorkspaceProxy(const IMDWorkspaceProxy&); // Not implemented.
  void operator=(const IMDWorkspaceProxy&); // Not implemented.

};

}
}

#endif
