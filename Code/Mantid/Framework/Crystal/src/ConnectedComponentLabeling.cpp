#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/V3D.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/Progress.h"
#include "MantidCrystal/ConnectedComponentLabeling.h"
#include "MantidCrystal/BackgroundStrategy.h"
#include "MantidCrystal/DisjointElement.h"
#include "MantidCrystal/Cluster.h"
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/scoped_ptr.hpp>
#include <stdexcept>
#include <set>
#include <algorithm>
#include <iterator>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Crystal::ConnectedComponentMappingTypes;

namespace Mantid
{
  namespace Crystal
  {
    namespace
    {
      /**
      * Perform integer power to determine the maximum number of face and edge connected
      * neighbours for a given dimensionality
      * @param ws : Workspace with dimensionality
      * @return : Maximum number of possible neighbours
      */
      size_t calculateMaxNeighbours(IMDHistoWorkspace const * const ws)
      {
        const size_t ndims = ws->getNumDims();
        size_t maxNeighbours = 3;
        for(size_t i = 1; i < ndims; ++i)
        {
          maxNeighbours *= 3;
        }
        maxNeighbours -= 1;
        return maxNeighbours;
      }

      /**
      * Helper non-member to clone the input workspace
      * @param inWS: To clone
      * @return : Cloned MDHistoWorkspace
      */
      boost::shared_ptr<Mantid::API::IMDHistoWorkspace> cloneInputWorkspace(IMDHistoWorkspace_sptr& inWS)
      {
        auto alg = AlgorithmManager::Instance().createUnmanaged("CloneWorkspace");
        alg->initialize();
        alg->setChild(true);
        alg->setProperty("InputWorkspace", inWS);
        alg->setPropertyValue("OutputWorkspace", "out_ws");
        alg->execute();
        Mantid::API::IMDHistoWorkspace_sptr outWS;
        {
          Mantid::API::Workspace_sptr temp = alg->getProperty("OutputWorkspace");
          outWS = boost::dynamic_pointer_cast<IMDHistoWorkspace>(temp);
        }
        // Initialize to zero.
        PARALLEL_FOR_NO_WSP_CHECK()
        for(int i = 0; i < static_cast<int>(outWS->getNPoints()); ++i)
        {
          outWS->setSignalAt(i, 0);
          outWS->setErrorSquaredAt(i, 0);
        }
        return outWS;
      }

      /**
      * Helper function to calculate report frequecny
      * @param maxReports : Maximum number of reports wanted
      * @param maxIterations : Maximum number of possible iterations
      * @return
      */
      template<typename T>
      T reportEvery(const T& maxReports, const T& maxIterations)
      {
        T frequency = maxReports;
        if (maxIterations >= maxReports)
        {
          frequency = maxIterations/maxReports;
        }
        return frequency;
      }

      // Helper function for determining if a set contains a specific value.
      template<typename Container>
      bool does_contain_key(const Container& container, const typename Container::key_type& value)
      {
        return container.find(value) != container.end();
      }


      typedef boost::tuple<size_t, size_t> EdgeIndexPair;

      struct DisjointPair
      {
        const DisjointElement m_currentLabel;
        const DisjointElement m_neighbourLabel;
        DisjointPair(const DisjointElement& currentLabel, const DisjointElement& neighbourLabel) 
          : m_currentLabel(currentLabel), m_neighbourLabel(neighbourLabel)
        {}
      };

      typedef std::set<DisjointPair> SetDisjointPair;
      typedef std::vector<EdgeIndexPair> VecEdgeIndexPair;

      size_t doConnectedComponentLabeling(
        IMDIterator* iterator, 
        BackgroundStrategy * const strategy, 
        VecElements& neighbourElements,
        Progress& progress, size_t maxNeighbours, 
        size_t startLabelId,
        VecEdgeIndexPair& edgeIndexVec
        )
      {
        size_t currentLabelCount = startLabelId;
        iterator->setNormalization(NoNormalization); // TODO: check that this is a valid assumption.
        do
        {
          if(!strategy->isBackground(iterator))
          {

            size_t currentIndex = iterator->getLinearIndex();

            // Linear indexes of neighbours
            VecIndexes neighbourIndexes = iterator->findNeighbourIndexes();
            VecIndexes nonEmptyNeighbourIndexes;
            nonEmptyNeighbourIndexes.reserve(maxNeighbours);
            SetIds neighbourIds;
            // Discover non-empty neighbours
            for (size_t i = 0; i < neighbourIndexes.size(); ++i)
            {
              size_t neighIndex = neighbourIndexes[i];
              if( !iterator->isWithinBounds(neighIndex) )
              {
                /* Record labels which appear to belong to the same cluster, but cannot be combined in this
                pass and will later need to be conjoined and resolved. As Labels cannot be guarnteed to be
                correcly provided for all neighbours until the end. We must store indexes instead.
                */
                edgeIndexVec.push_back(EdgeIndexPair(currentIndex, neighIndex));
                continue;
              }

              const DisjointElement& neighbourElement = neighbourElements[neighIndex];

              if (!neighbourElement.isEmpty())
              {
                nonEmptyNeighbourIndexes.push_back(neighIndex);
                neighbourIds.insert(neighbourElement.getId());
              }
            }

            if (nonEmptyNeighbourIndexes.empty())
            {
              neighbourElements[currentIndex] = DisjointElement(static_cast<int>(currentLabelCount)); // New leaf
              ++currentLabelCount;
            }
            else if (neighbourIds.size() == 1) // Do we have a single unique id amongst all neighbours.
            {
              neighbourElements[currentIndex] = neighbourElements[nonEmptyNeighbourIndexes.front()]; // Copy non-empty neighbour
            }
            else
            {
              // Choose the lowest neighbour index as the parent.
              size_t parentIndex = nonEmptyNeighbourIndexes[0];
              for (size_t i = 1; i < nonEmptyNeighbourIndexes.size(); ++i)
              {
                size_t neighIndex = nonEmptyNeighbourIndexes[i];
                if (neighbourElements[neighIndex].getId() < neighbourElements[parentIndex].getId())
                {
                  parentIndex = i;
                }
              }
              // Get the chosen parent
              DisjointElement& parentElement = neighbourElements[parentIndex];
              // Union remainder parents with the chosen parent
              for (size_t i = 0; i < nonEmptyNeighbourIndexes.size(); ++i)
              {
                size_t neighIndex = nonEmptyNeighbourIndexes[i];
                if (neighIndex != parentIndex)
                {
                  neighbourElements[neighIndex].unionWith(&parentElement);
                }
              }
            }
          }
        }
        while(iterator->next());
        return currentLabelCount;
      }
    }

    /**
    * Constructor
    * @param startId : Start Id to use for labeling
    * @param nThreads : Optional argument of number of threads to use.
    */
    ConnectedComponentLabeling::ConnectedComponentLabeling(const size_t& startId, const boost::optional<int> nThreads) 
      : m_startId(startId), m_nThreads(nThreads)
    {
      if(m_nThreads.is_initialized() && m_nThreads.get() < 0)
      {
        throw std::invalid_argument("Cannot request that CCL runs with less than one thread!");
      }
    }

    /**
    * Set a custom start id. This has no bearing on the output of the process other than
    * the initial id used.
    * @param id: Id to start with
    */
    void ConnectedComponentLabeling::startLabelingId(const size_t& id)
    {
      m_startId = id;
    }

    /**
    @return: The start label id.
    */
    size_t ConnectedComponentLabeling::getStartLabelId() const
    {
      return m_startId;
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    ConnectedComponentLabeling::~ConnectedComponentLabeling()
    {
    }

    /**
    * Get the number of threads available
    * @return : Number of available threads
    */
    int ConnectedComponentLabeling::getNThreads() const
    {
      if(m_nThreads.is_initialized())
      {
        return m_nThreads.get(); // Follow explicit instructions if provided.
      }
      else
      {
        return API::FrameworkManager::Instance().getNumOMPThreads(); // Figure it out.
      }
    }


    /**
    * Perform the work of the CCL algorithm
    * - Pre filtering of background
    * - Labeling using DisjointElements
    *
    * @param ws : MDHistoWorkspace to run CCL algorithm on
    * @param baseStrategy : Background strategy
    * @param progress : Progress object
    * @return : Map of label ids to clusters.
    */
    ClusterMap ConnectedComponentLabeling::calculateDisjointTree(IMDHistoWorkspace_sptr ws, 
      BackgroundStrategy * const baseStrategy,
      Progress& progress
      ) const
    {
      std::map<size_t, boost::shared_ptr<Cluster> > clusterMap;
      VecElements neighbourElements(ws->getNPoints());

      const size_t maxNeighbours = calculateMaxNeighbours(ws.get());

      // For each process maintains pair of index from within process bounds to index outside process bounds
      const int nThreadsToUse = getNThreads();
      if(nThreadsToUse > 1)
      {
        std::vector<API::IMDIterator*> iterators = ws->createIterators(nThreadsToUse);
        const int nthreads = getNThreads(); 
        std::vector<VecEdgeIndexPair> parallelEdgeVec(nthreads);
 
        std::vector<std::map<size_t, boost::shared_ptr<Cluster> > > parallelClusterMapVec(nthreads);

        // ------------- Stage One. Local CCL in parallel.
        PARALLEL_FOR_NO_WSP_CHECK()
          for(int i = 0; i < nthreads; ++i)
          {
            API::IMDIterator* iterator = iterators[i];
            boost::scoped_ptr<BackgroundStrategy> strategy(baseStrategy->clone()); // local strategy
            VecEdgeIndexPair& edgeVec = parallelEdgeVec[i]; // local edge indexes

            const size_t startLabel = m_startId + (i * ws->getNPoints()); // Ensure that label ids are totally unique within each parallel unit.
            const size_t endLabel = doConnectedComponentLabeling(iterator, strategy.get(), neighbourElements, progress, maxNeighbours, startLabel, edgeVec);

            // Create clusters from labels.
            std::map<size_t, boost::shared_ptr<Cluster> >& localClusterMap= parallelClusterMapVec[i]; // local cluster map.
            for(size_t labelId=startLabel; labelId != endLabel; ++labelId)
            {
              auto cluster = boost::make_shared<Cluster>(labelId); // Create a cluster for the label and key it by the label.
              localClusterMap[labelId] = cluster;
            }

            // Associate the member DisjointElements with a cluster. Involves looping back over iterator.
            iterator->jumpTo(0); // Reset
            iterator->setNormalization(NoNormalization); // TODO: check that this is a valid assumption.
            std::set<size_t> labelIds;
            do
            {
              if(!baseStrategy->isBackground(iterator))
              {
                const size_t& index = iterator->getLinearIndex();
                const size_t& labelAtIndex = neighbourElements[index].getRoot();
                localClusterMap[labelAtIndex]->addIndex(index);  
              }
            }
            while(iterator->next());
          }

          // -------------------- Stage 2 --- Preparation stage for combining equivalent clusters. Must be done in sequence.
          // Combine cluster maps processed by each thread.
          for(auto it = parallelClusterMapVec.begin(); it != parallelClusterMapVec.end(); ++it)
          {
            clusterMap.insert(it->begin(), it->end());
          }

          // Percolate minimum label across boundaries for indexes where there is ambiguity.
          std::vector<boost::shared_ptr<Cluster> > incompleteClusterVec;
          for(auto it = parallelEdgeVec.begin(); it != parallelEdgeVec.end(); ++it) 
          {
            VecEdgeIndexPair& indexPairVec = *it;
            for(auto iit = indexPairVec.begin(); iit != indexPairVec.end(); ++iit)
            {
              DisjointElement& a = neighbourElements[iit->get<0>()];
              DisjointElement& b = neighbourElements[iit->get<1>()];
              if(!a.isEmpty() && !b.isEmpty())
              {
                incompleteClusterVec.push_back( clusterMap[a.getId()] );
                incompleteClusterVec.push_back( clusterMap[b.getId()] );

                if(a.getId() < b.getId())
                {
                  b.unionWith(&a);
                }
                else
                {
                  a.unionWith(&b);
                }
              }
            }
          }

          // ------------- Stage 3 In parallel, process each incomplete cluster.
          PARALLEL_FOR_NO_WSP_CHECK()
            for(int i = 0; i < static_cast<int>(incompleteClusterVec.size()); ++i)
            {
              auto cluster = incompleteClusterVec[i];
              cluster->toUniformMinimum(neighbourElements);
            }

            // Now combine clusters and remove old ones.
            std::map<size_t, boost::shared_ptr<Cluster> > consumptionMap; // Track incomplete clusters to avoid duplicates.
            for(size_t i = 0; i < incompleteClusterVec.size(); ++i)
            {
              const size_t label = incompleteClusterVec[i]->getLabel();
              if(!does_contain_key(consumptionMap, label))
              {
                consumptionMap.insert(std::make_pair(label, incompleteClusterVec[i]));
              }
              else
              {
                consumptionMap[label]->consumeCluster(*incompleteClusterVec[i].get());
                clusterMap.erase(label); // We no longer need this label or cluster as it has been consumed.
              }
            }

      }
      else
      {
        API::IMDIterator* iterator = ws->createIterator(NULL);
        VecEdgeIndexPair edgeIndexPair; // This should never get filled in a single threaded situation.
        size_t endLabelId = doConnectedComponentLabeling(iterator, baseStrategy, neighbourElements, progress, maxNeighbours, m_startId, edgeIndexPair);

        // Create clusters from labels.
        for(size_t labelId=m_startId; labelId != endLabelId; ++labelId)
            {
              auto cluster = boost::make_shared<Cluster>(labelId); // Create a cluster for the label and key it by the label.
              clusterMap[labelId] = cluster;
            }

        // Associate the member DisjointElements with a cluster. Involves looping back over iterator.
        iterator->jumpTo(0); // Reset
        iterator->setNormalization(NoNormalization); // TODO: check that this is a valid assumption.
        std::set<size_t> labelIds;
        do
        {
          if(!baseStrategy->isBackground(iterator))
          {
            const size_t& index = iterator->getLinearIndex();
            const size_t& labelAtIndex = neighbourElements[index].getRoot();
            clusterMap[labelAtIndex]->addIndex(index);  
          }
        }
        while(iterator->next());
      }
      return clusterMap;
    }


    /**
    * Execute CCL to produce a cluster output workspace containing labels
    * @param ws : Workspace to perform CCL on
    * @param strategy : Background strategy
    * @param progress : Progress object
    * @return Cluster output workspace of results
    */
    boost::shared_ptr<Mantid::API::IMDHistoWorkspace> ConnectedComponentLabeling::execute(
      IMDHistoWorkspace_sptr ws, BackgroundStrategy * const strategy, Progress& progress) const
    {
      // Perform the bulk of the connected component analysis, but don't collapse the elements yet.
      ClusterMap clusters = calculateDisjointTree(ws, strategy, progress);

      // Create the output workspace from the input workspace
      IMDHistoWorkspace_sptr outWS = cloneInputWorkspace(ws);

      for (auto it = clusters.begin(); it != clusters.end(); ++it) 
      {
        it->second->writeTo(outWS); // Apply cluster onto output workspace.
      }

      return outWS;
    }

    /**
    * Execute and integrate
    * @param ws : Image workspace to integrate
    * @param strategy : Background strategy
    * @param labelMap : Label map to fill. Label ids to integrated signal and errorsq for that label
    * @param positionLabelMap : Label ids to position in workspace coordinates. This is filled as part of the work.
    * @param progress : Progress object
    * @return Image Workspace containing clusters.
    */
    boost::shared_ptr<Mantid::API::IMDHistoWorkspace> ConnectedComponentLabeling::executeAndIntegrate(
      IMDHistoWorkspace_sptr ws, BackgroundStrategy * const strategy, LabelIdIntensityMap& labelMap,
      PositionToLabelIdMap& positionLabelMap, Progress& progress) const
    {

      // Perform the bulk of the connected component analysis, but don't collapse the elements yet.
      ClusterMap clusters = calculateDisjointTree(ws, strategy,progress);

      // Create the output workspace from the input workspace
      IMDHistoWorkspace_sptr outWS = cloneInputWorkspace(ws);

      progress.doReport("Integrating clusters and generating cluster image");
      const size_t nIterations = ws->getNPoints();
      const size_t maxReports = 1000;
      const size_t frequency = reportEvery(maxReports, nIterations);
      progress.resetNumSteps(maxReports, 0.5, 0.75);
      // Set each pixel to the root of each disjointed element.
      int i = 0;
      for (auto it = clusters.begin(); it != clusters.end(); ++it, ++i) 
      {
        it->second->writeTo(outWS); // TODO. could be done in parallel.
        it->second->integrate(ws);

        if(i % frequency == 0)
        {
          progress.doReport();
        }
      }

      return outWS;
    }

  } // namespace Crystal
} // namespace Mantid
