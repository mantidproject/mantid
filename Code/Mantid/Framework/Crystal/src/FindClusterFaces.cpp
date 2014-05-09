/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidCrystal/FindClusterFaces.h"

#include "MantidKernel/Utils.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/TableRow.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

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
  const std::string FindClusterFaces::name() const { return "FindClusterFaces";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int FindClusterFaces::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string FindClusterFaces::category() const { return "Crystal";}

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
    declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace","",Direction::Input), "An input image workspace consisting of cluster ids.");
    declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace","",Direction::Output), "An output table workspace containing cluster face information.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void FindClusterFaces::exec()
  {
    IMDHistoWorkspace_sptr clusterImage = getProperty("InputWorkspace");
    const int emptyLabelId = 0;

    auto out = WorkspaceFactory::Instance().createTable("TableWorkspace");
    out->addColumn("int", "ClusterId");
    out->addColumn("double", "MDWorkspaceIndex");
    out->addColumn("int", "FaceNormalDimension");
    out->addColumn("bool", "MaxEdge");

    std::vector<size_t> imageShape;
    for(size_t i = 0; i < clusterImage->getNumDims(); ++i)
    {
      imageShape.push_back( clusterImage->getDimension(i)->getNBins() );
    }

    auto mdIterator = clusterImage->createIterator(NULL);
    do
    {
      int id = static_cast<int>(mdIterator->getSignal());
      if(id > emptyLabelId)
      {
        // Add index to cluster id map.
        const size_t linearIndex = mdIterator->getLinearIndex();
        std::vector<size_t> indexes;
        Kernel::Utils::getIndicesFromLinearIndex(linearIndex, imageShape, indexes);

        const auto neighbours = mdIterator->findNeighbourIndexesFaceTouching();
        for(size_t i = 0; i < neighbours.size(); ++i)
        {
          size_t neighbourLinearIndex = neighbours[i];
          const int neighbourId = clusterImage->getSignalAt(neighbourLinearIndex);
          if(neighbourId <= emptyLabelId)
          {
            // We have an edge!

            // In which dimension is the edge?
            std::vector<size_t> neighbourIndexes;
            Kernel::Utils::getIndicesFromLinearIndex(neighbourLinearIndex, imageShape, neighbourIndexes);
            for(size_t j = 0; j < imageShape.size(); ++j)
            {
              if(indexes[j] != neighbourIndexes[j])
              {
                bool maxEdge = neighbourLinearIndex > linearIndex;

                TableRow row = out->appendRow();
                row << int(id) << double(linearIndex) << int(j) << maxEdge;
              }
            }
          }
        }
      }

    }
    while(mdIterator->next());


    setProperty("OutputWorkspace", out);
  }



} // namespace Crystal
} // namespace Mantid
