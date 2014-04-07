#ifndef MANTID_CRYSTAL_CLUSTER_H_
#define MANTID_CRYSTAL_CLUSTER_H_

#include "MantidKernel/System.h"
#include "MantidCrystal/DisjointElement.h"
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>


namespace Mantid
{
  namespace API
  {
    class IMDHistoWorkspace;
  }
  namespace Crystal
  {

    /** Cluster : Image cluster used by connected component labeling

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport Cluster 
    {
     
      public:

        /// Constructor
        Cluster(const size_t& label);

        /// integrate the cluster
        void integrate(boost::shared_ptr<const Mantid::API::IMDHistoWorkspace> ws);

        /// Apply labels to the workspace
        void writeTo(boost::shared_ptr<Mantid::API::IMDHistoWorkspace> ws) const;

        /// Get the cluster label
        size_t getLabel() const;

        /// Get the signal integrated value
        double getSignalInt() const;

        /// Get the error squared integrated value
        double getErrorSQInt() const;

        /// Number of indexes tracked
        size_t size();

        /// Track a linear IMDHistoWorkspace index that belongs to the cluster.
        void addIndex(const size_t& index);

        /// Resolve the proper label for this cluster.
        void toUniformMinimum(std::vector<DisjointElement>& disjointSet);

        /// Overloaded equals.
        bool operator==(const Cluster& other) const;

        /// Merge and own 
        void consumeCluster(Cluster& other);

       private:

        /// Disabled copy construction
        Cluster(const Cluster&);
        /// Disabled assignement
        Cluster& operator=(const Cluster&);

        /// Label used by cluster
        size_t m_label;
        /// indexes belonging to cluster. This is how we track cluster objects.
        std::vector<size_t> m_indexes;
        /// Error sq integrated value of cluster
        boost::optional<double> m_errorSQInt;
        /// Signal integrated value of cluster
        boost::optional<double> m_signalInt;

    };


  } // namespace Crystal
} // namespace Mantid

#endif  /* MANTID_CRYSTAL_CLUSTER_H_ */