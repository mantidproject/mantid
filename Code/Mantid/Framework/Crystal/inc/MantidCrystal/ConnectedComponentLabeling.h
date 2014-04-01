#ifndef MANTID_CRYSTAL_CONNECTEDCOMPONENTLABELING_H_
#define MANTID_CRYSTAL_CONNECTEDCOMPONENTLABELING_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidCrystal/DisjointElement.h"
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <map>

namespace Mantid
{
namespace Crystal
{
  namespace ConnectedComponentMappingTypes
  {
    typedef boost::tuple<double, double> SignalErrorSQPair;
    typedef std::map<size_t, SignalErrorSQPair > LabelIdIntensityMap;
    typedef std::map<Mantid::Kernel::V3D, size_t> PositionToLabelIdMap;
    typedef std::vector<size_t> VecIndexes;
    typedef std::vector<DisjointElement> VecElements;
    typedef std::set<size_t> SetIds;
  }

  class BackgroundStrategy;

  /** ConnectedComponentLabelling : Implements connected component labeling on MDHistoWorkspaces.
    
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
  class DLLExport ConnectedComponentLabeling
  {

  public:
    /// Constructor
    ConnectedComponentLabeling(const size_t&id = 1, const bool runMultiThreaded=true);
    /// Getter for the start label id
    size_t getStartLabelId() const;
    /// Setter for the label id
    void startLabelingId(const size_t& id);

    /// Execute and return clusters
    boost::shared_ptr<Mantid::API::IMDHistoWorkspace> execute(Mantid::API::IMDHistoWorkspace_sptr ws, BackgroundStrategy * const strategy) const;
    
    /// Execute and return clusters, as well as maps to integrated label values
    boost::shared_ptr<Mantid::API::IMDHistoWorkspace> executeAndIntegrate(
      Mantid::API::IMDHistoWorkspace_sptr ws, BackgroundStrategy * const strategy, ConnectedComponentMappingTypes::LabelIdIntensityMap& labelMap,
      ConnectedComponentMappingTypes::PositionToLabelIdMap& positionLabelMap) const;

    /// Destructor
    virtual ~ConnectedComponentLabeling();
  private:
    /// Get the number of threads to use.
    int getNThreads() const;
    /// Calculate the disjoint element tree across the image.
    void calculateDisjointTree(Mantid::API::IMDHistoWorkspace_sptr ws, 
      BackgroundStrategy * const strategy, std::vector<DisjointElement>& neighbourElements,
      ConnectedComponentMappingTypes::LabelIdIntensityMap& labelMap,
      ConnectedComponentMappingTypes::PositionToLabelIdMap& positionLabelMap) const;

    /// Start labeling index
    size_t m_startId;
    /// Run multithreaded
    const bool m_runMultiThreaded;
    
  };


} // namespace Crystal
} // namespace Mantid

#endif  /* MANTID_CRYSTAL_CONNECTEDCOMPONENTLABELING_H_ */
