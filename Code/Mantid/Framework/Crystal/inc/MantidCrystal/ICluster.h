#ifndef MANTID_CRYSTAL_ICLUSTER_H_
#define MANTID_CRYSTAL_ICLUSTER_H_

#include "MantidKernel/System.h"
#include "MantidCrystal/DisjointElement.h"
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

namespace Mantid
{
  namespace API
  {
    class IMDHistoWorkspace;
  }
  namespace Crystal
  {

    /** ICluster : Abstract cluster. Identifies neighbour elements in an image that are connected.

     Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

     File change history is stored at: <https://github.com/mantidproject/mantid>
     Code Documentation is available at: <http://doxygen.mantidproject.org>
     */
    class DLLExport ICluster
    {
    public:

      typedef boost::tuple<double, double> ClusterIntegratedValues;

      /// integrate the cluster
      virtual ClusterIntegratedValues integrate(
          boost::shared_ptr<const Mantid::API::IMDHistoWorkspace> ws) const = 0;

      /// Apply labels to the workspace
      virtual void writeTo(boost::shared_ptr<Mantid::API::IMDHistoWorkspace> ws) const = 0;

      /// Get the originally set label
      virtual size_t getOriginalLabel() const = 0;

      /// Get the cluster label
      virtual size_t getLabel() const = 0;

      /// Number of indexes tracked
      virtual size_t size() const = 0;

      /// Track a linear IMDHistoWorkspace index that belongs to the cluster.
      virtual void addIndex(const size_t& index) = 0;

      /// Resolve the proper label for this cluster.
      virtual void toUniformMinimum(std::vector<DisjointElement>& disjointSet) = 0;

      /// Virtual destructor
      virtual ~ICluster(){};

      /// Set the root cluster
      virtual void setRootCluster(ICluster const*  root) = 0;

      /// Get a represetiative index of the cluster
      virtual size_t getRepresentitiveIndex() const = 0;

      /// Is the label contained in the cluster
      virtual bool containsLabel(const size_t& label) const = 0;
    };

  } // namespace Crystal
} // namespace Mantid

#endif  /* MANTID_CRYSTAL_ICLUSTER_H_ */
