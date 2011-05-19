#ifndef MDGEOMETRYXMLBUILDER_H_
#define MDGEOMETRYXMLBUILDER_H_

#include <vector>
#include <string>
#include <algorithm>
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace Geometry
{

/**
 @class  MDGeometryBuliderXML
 @brief Computes Boolean algebra for simplification
 @author Owen Arnold
 @date May 2011
 @version 1.0

 Handles the generation of well formed description of a geometry based on input IMDDimensions.
 Outputs xml.

 Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 */
template <typename CheckDimensionPolicy>
class DLLExport MDGeometryBuilderXML
{

public:

  /// Constructor
  MDGeometryBuilderXML();

  /// Destructor
  ~MDGeometryBuilderXML();

  /// Add a dimension that is neither considered x, y, z or t.
  bool addOrdinaryDimension(IMDDimension_const_sptr dimension) const;

  /// Add x dimension.
  bool addXDimension(IMDDimension_const_sptr dimension) const;

  /// Add y dimension.
  bool addYDimension(IMDDimension_const_sptr dimension) const;

  /// Add z dimension.
  bool addZDimension(IMDDimension_const_sptr dimension) const;

  /// Add t dimension.
  bool addTDimension(IMDDimension_const_sptr dimension) const;


  /// Create the xml.
  const std::string& create() const;

private:

  typedef std::vector<IMDDimension_const_sptr> DimensionContainerType;

  mutable DimensionContainerType m_vecDimensions;

  mutable IMDDimension_const_sptr m_spXDimension;

  mutable IMDDimension_const_sptr m_spYDimension;

  mutable IMDDimension_const_sptr m_spZDimension;

  mutable IMDDimension_const_sptr m_spTDimension;

  MDGeometryBuilderXML(const MDGeometryBuilderXML&);

  MDGeometryBuilderXML& operator=(const MDGeometryBuilderXML&);

  /// Determine whetether a valid x dimension has been provided.
  bool hasXDimension() const;

  /// Determine whetether a valid y dimension has been provided.
  bool hasYDimension() const;

  /// Determine whetether a valid z dimension has been provided.
  bool hasZDimension() const;

  /// Determine whetether a valid t dimension has been provided.
  bool hasTDimension() const;

  /// Instantiate and apply the checking policy.
  void applyPolicyChecking(IMDDimension_const_sptr dimensionToAdd) const;

  /// Flag indicating that some change in the inputs has occured. Triggers full recreation.
  mutable bool m_changed;

  /// Variable suports lazy calculation.
  mutable std::string m_lastResult;

};

/*
 @class StrictDimensionPolicy
 @brief Unary operator that throws if the dimension provided is integrated.
 @author Owen Arnold
 @date May 2011
 @version 1.0
*/
struct StrictDimensionPolicy: public std::unary_function<IMDDimension_const_sptr, void>
{
private:
  Kernel::Logger& g_log;
public:
  StrictDimensionPolicy() : g_log(Kernel::Logger::get("StrictDimensionPolicy")){}
  void operator()(IMDDimension_const_sptr item)
  {
    if(true == item->getIsIntegrated())
    {
      std::string message = "StrictDimensionPolicy bans the use of integrated IMDDimensions mapped to x, y, z or t in a IMDWorkspace.";
      message += "Attempted to do so with IMDDimension: " + item->getDimensionId();
      g_log.error(message);
      throw std::invalid_argument(message);
    }
  }
};

/*
 @class NoDimensionPolicy
 @brief Unary operator that has no effect.
 @author Owen Arnold
 @date May 2011
*/
struct NoDimensionPolicy: public std::unary_function<IMDDimension_const_sptr, void>
{
  void operator()(IMDDimension_const_sptr)
  {
    //Do nothing.
  }
};

}
}

#endif /* GEOMETRYXMLBUILDER_H_ */
