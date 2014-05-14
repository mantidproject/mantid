/*WIKI*

Algorithm takes an image workspace (a.k.a [[IMDHistoWorkspace]]) and determines the faces of the clusters contained within the image.
The image is expected to be a labeled image workspace outputted from [[IntegratePeaksUsingClusters]]. The algorithm generates a 
[[TableWorkspace]] as output, which contains all the cluster edge faces required to draw the outer edge of all clusters within the workspace.

You may optionally provide a FilterWorkspace, which is a [[PeaksWorkspace]]. If provided, the Peak locations are projected onto the InputWorkspace
and the center locations are used to restrict the output to only include the clusters that are the union between the peak locations and the image clusters.
 
*WIKI*/

#include "MantidCrystal/FindClusterFaces.h"

#include "MantidKernel/Utils.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidAPI/TableRow.h"
#include "MantidCrystal/PeakClusterProjection.h"

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <set>
#include <deque>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace
{
  using namespace Mantid::Crystal;
  // Set of labels
  typedef std::set<int> LabelSet;
  // Optional set of labels
  typedef boost::optional<LabelSet> OptionalLabelSet;

  /**
   * Create an optional label set for filtering.
   * @param dimensionality : Dimensionality of the workspace.
   * @param emptyLabelId : Label id corresponding to empty.
   * @param filterWorkspace : Peaks workspace to act as filter.
   * @param clusterImage : Image constaining clusters for inspection.
   * @return Set of labels to inspect for.
   */
  OptionalLabelSet createOptionalLabelFilter(size_t dimensionality, int emptyLabelId,
      IPeaksWorkspace_sptr filterWorkspace, IMDHistoWorkspace_sptr& clusterImage)
  {
    OptionalLabelSet optionalAllowedLabels;

    if (filterWorkspace)
    {
      if (dimensionality < 3)
      {
        throw std::invalid_argument(
            "A FilterWorkspace has been given, but the dimensionality of the labeled workspace is < 3.");
      }
      LabelSet allowedLabels;
      PeakClusterProjection projection(clusterImage);
      for (int i = 0; i < filterWorkspace->getNumberPeaks(); ++i)
      {
        IPeak& peak = filterWorkspace->getPeak(i);
        const int labelIdAtPeakCenter = static_cast<int>(projection.signalAtPeakCenter(peak));
        if (labelIdAtPeakCenter > emptyLabelId)
        {
          allowedLabels.insert(labelIdAtPeakCenter);
        }
      }
      optionalAllowedLabels = allowedLabels;
    }
    return optionalAllowedLabels;
  }

  /**
  Type to represent cluster face (a.k.a a row in the output table)
  */
  struct ClusterFace
  {
    int clusterId;
    size_t workspaceIndex;
    int faceNormalDimension;
    bool maxEdge;
  };

  typedef std::deque<ClusterFace> ClusterFaces;

}

namespace Mantid
{
  namespace Crystal
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(FindClusterFaces)

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    FindClusterFaces::FindClusterFaces()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    FindClusterFaces::~FindClusterFaces()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string FindClusterFaces::name() const
    {
      return "FindClusterFaces";
    }
    ;

    /// Algorithm's version for identification. @see Algorithm::version
    int FindClusterFaces::version() const
    {
      return 1;
    }
    ;

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string FindClusterFaces::category() const
    {
      return "Crystal";
    }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void FindClusterFaces::initDocs()
    {
      this->setWikiSummary("Find faces for clusters in a cluster image.");
      this->setOptionalMessage(this->getWikiSummary());
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void FindClusterFaces::init()
    {
      declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace", "", Direction::Input),
          "An input image workspace consisting of cluster ids.");
      declareProperty(
          new WorkspaceProperty<IPeaksWorkspace>("FilterWorkspace", "", Direction::Input,
              PropertyMode::Optional),
          "Optional filtering peaks workspace. Used to restrict face finding to clusters in image which correspond to peaks in the workspace.");
      declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace", "", Direction::Output),
          "An output table workspace containing cluster face information.");
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void FindClusterFaces::exec()
    {
      IMDHistoWorkspace_sptr clusterImage = getProperty("InputWorkspace");
      const int emptyLabelId = 0;

      std::vector<size_t> imageShape;
      const size_t dimensionality = clusterImage->getNumDims();
      for (size_t i = 0; i < dimensionality; ++i)
      {
        imageShape.push_back(clusterImage->getDimension(i)->getNBins());
      }

      IPeaksWorkspace_sptr filterWorkspace = this->getProperty("FilterWorkspace");
      OptionalLabelSet optionalAllowedLabels = createOptionalLabelFilter(dimensionality, emptyLabelId,
          filterWorkspace, clusterImage);

      ClusterFaces clusterFaces;
      auto mdIterator = clusterImage->createIterator(NULL); // TODO. This could be done in parallel!
      do
      {
        const int id = static_cast<int>(mdIterator->getSignal());
        /*
         * Only if the label has been allowed by the filtering, should it be used.
         */
        if (!optionalAllowedLabels.is_initialized()
            || (optionalAllowedLabels->find(id) != optionalAllowedLabels->end()))
        {
          if (id > emptyLabelId)
          {
            // Add index to cluster id map.
            const size_t linearIndex = mdIterator->getLinearIndex();
            std::vector<size_t> indexes;
            Kernel::Utils::getIndicesFromLinearIndex(linearIndex, imageShape, indexes);

            const auto neighbours = mdIterator->findNeighbourIndexesFaceTouching();
            for (size_t i = 0; i < neighbours.size(); ++i)
            {
              size_t neighbourLinearIndex = neighbours[i];
              const int neighbourId = static_cast<int>(clusterImage->getSignalAt(neighbourLinearIndex));
              if (neighbourId <= emptyLabelId)
              {
                // We have an edge!

                // In which dimension is the edge?
                std::vector<size_t> neighbourIndexes;
                Kernel::Utils::getIndicesFromLinearIndex(neighbourLinearIndex, imageShape,
                    neighbourIndexes);
                for (size_t j = 0; j < imageShape.size(); ++j)
                {
                  if (indexes[j] != neighbourIndexes[j])
                  {
                    const bool maxEdge = neighbourLinearIndex > linearIndex;
                    
                    ClusterFace face;
                    face.clusterId = id;
                    face.workspaceIndex = linearIndex;
                    face.faceNormalDimension = j;
                    face.maxEdge = maxEdge;

                    clusterFaces.push_back(face);

                  }
                }
              }
            }
          }
        }

      } while (mdIterator->next());

      auto out = WorkspaceFactory::Instance().createTable("TableWorkspace");
      out->addColumn("int", "ClusterId");
      out->addColumn("double", "MDWorkspaceIndex");
      out->addColumn("int", "FaceNormalDimension");
      out->addColumn("bool", "MaxEdge");
      for(auto it = clusterFaces.begin(); it != clusterFaces.end(); ++it)
      {
        const ClusterFace& clusterFace = *it;
        TableRow row = out->appendRow();
        row << clusterFace.clusterId << double(clusterFace.workspaceIndex) << clusterFace.faceNormalDimension << clusterFace.maxEdge; 
      }

      setProperty("OutputWorkspace", out);
    }

  } // namespace Crystal
} // namespace Mantid
