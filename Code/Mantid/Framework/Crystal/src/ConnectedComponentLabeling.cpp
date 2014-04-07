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

      class Cluster
      {

      private:

        size_t m_label;
        VecIndexes m_indexes;
        double m_errorSQInt;
        double m_signalInt;

      public:

        Cluster(const size_t& label, const double& signalInt, const double& errorSQInt):
          m_label(label),
          m_signalInt(signalInt),
          m_errorSQInt(errorSQInt)
        {
        }

        size_t getLabel() const
        {
          return m_label;
        }

        double getSignalInt() const
        {
          return m_signalInt;
        }

        double getErrorSQInt() const
        {
          return m_errorSQInt;
        }

        size_t size()
        {
          return m_indexes.size();
        }

        void addIndex(const size_t& index)
        {
          m_indexes.push_back(index);
        }

        void toUniformMinimum(VecElements& disjointSet)
        {
          size_t minLabelIndex = m_indexes.front();
          size_t minLabel= disjointSet[m_indexes.front()].getRoot();
          
          for(size_t i = 1; i< m_indexes.size(); ++i)
          {
            const size_t& currentLabel = disjointSet[m_indexes[i]].getRoot();
            if(currentLabel < minLabel)
            {
              minLabel = currentLabel;
              minLabelIndex = i;
            }
          }
          m_label = minLabel;
          for(size_t i = 1; i< m_indexes.size(); ++i)
          {
            disjointSet[m_indexes[i]].unionWith(disjointSet[minLabelIndex].getParent());
          }
        }

        bool operator==(const Cluster& other) const
        {
          return getLabel() == other.getLabel();
        }

        void consumeCluster(Cluster& other)
        {
          if(other.getLabel() != this->getLabel())
          {
            throw std::runtime_error("Label Ids differ. Cannot combine Clusters.");
          }
          m_signalInt += other.getSignalInt();
          m_errorSQInt += other.getErrorSQInt();
          m_indexes.insert(m_indexes.end(), other.m_indexes.begin(), other.m_indexes.end());
        }
      };

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

          void doConnectedComponentLabeling(
      IMDIterator* iterator, 
      BackgroundStrategy * const strategy, 
      VecElements& neighbourElements,
      LabelIdIntensityMap& labelMap,
      PositionToLabelIdMap& positionLabelMap,
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
                labelMap[currentLabelCount] = 0; // Pre-fill the currentlabelcount.
                /* HACK REMOVE FOLLOWING! as is not thread safe. 
                const VMD& center = iterator->getCenter(); 
                positionLabelMap[V3D(center[0], center[1], center[2])] = currentLabelCount; // Get the position at this label.
              */
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
    * @param neighbourElements : Neighbour elements containing DisjointElements
    * @param labelMap : Map of label id to signal, error_sq pair for integration purposes to fill
    * @param positionLabelMap : Map of label ids to position in workspace coordinates to fill
    * @param progress : Progress object
    */
    void ConnectedComponentLabeling::calculateDisjointTree(IMDHistoWorkspace_sptr ws, 
      BackgroundStrategy * const baseStrategy, VecElements& neighbourElements,
      LabelIdIntensityMap& labelMap,
      PositionToLabelIdMap& positionLabelMap,
      Progress& progress
      ) const
    {

      const size_t maxNeighbours = calculateMaxNeighbours(ws.get());

      // For each process maintains pair of index from within process bounds to index outside process bounds
      const int nThreadsToUse = getNThreads();
      if(nThreadsToUse > 1)
      {
        std::vector<API::IMDIterator*> iterators = ws->createIterators(getNThreads());
        const int nthreads = getNThreads(); 
        std::vector<VecEdgeIndexPair> parallelEdgeVec(nthreads);
        std::vector<LabelIdIntensityMap> parallelLabelIntensityMapVec(nthreads);
        PARALLEL_FOR_NO_WSP_CHECK()
        for(int i = 0; i < nthreads; ++i)
        {
          API::IMDIterator* iterator = iterators[i];
          boost::scoped_ptr<BackgroundStrategy> strategy(baseStrategy->clone()); // local strategy
          VecEdgeIndexPair& edgeVec = parallelEdgeVec[i]; // local edge indexes
          LabelIdIntensityMap& intensityMap = parallelLabelIntensityMapVec[i]; // local intensity map
          const size_t startLabel = m_startId + (i * ws->getNPoints()); // Ensure that label ids are totally unique within each parallel unit.
          doConnectedComponentLabeling(iterator, strategy.get(), neighbourElements, intensityMap, positionLabelMap, progress, maxNeighbours, startLabel, edgeVec);
        }

        // Create clusters from labels.
        std::map<size_t, boost::shared_ptr<Cluster> > clusterMap;
        for(auto it = parallelLabelIntensityMapVec.begin(); it != parallelLabelIntensityMapVec.end(); ++it)
        {
          LabelIdIntensityMap& local = *it;
          for(auto iit = local.begin(); iit != local.end(); ++iit)
          {
            const size_t& labelId = iit->first;
            const double& signalInt = iit->second.get<0>();
            const double& errorSQInt = iit->second.get<1>();
            auto cluster = boost::make_shared<Cluster>(labelId, signalInt, errorSQInt);
            clusterMap[labelId] = cluster;
          }
        }

        // Assign indexes to clusters. Clusters are still disjointed at this stage.
        IMDIterator* iterator = ws->createIterator(NULL);
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

        // In parallel, process each incomplete cluster.
        PARALLEL_FOR_NO_WSP_CHECK()
          for(int i = 0; i < static_cast<int>(incompleteClusterVec.size()); ++i)
        {
          auto cluster = incompleteClusterVec[i];
          cluster->toUniformMinimum(neighbourElements);
        }

        // Now combine clusters and remove old ones.
        std::map<size_t, boost::shared_ptr<Cluster> > consumptionMap;
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
        // TODO. Return the clusters. These are useful in their own right!

      }
      else
      {
        API::IMDIterator* iterator = ws->createIterator(NULL);
        VecEdgeIndexPair edgeIndexPair; // This should never get filled in a single threaded situation.
        doConnectedComponentLabeling(iterator, baseStrategy, neighbourElements, labelMap, positionLabelMap, progress, maxNeighbours, m_startId, edgeIndexPair);
      }
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
            VecElements neighbourElements(ws->getNPoints());

            // Perform the bulk of the connected component analysis, but don't collapse the elements yet.
            LabelIdIntensityMap labelMap; // This will not get used.
            PositionToLabelIdMap positionLabelMap; // This will not get used.
            calculateDisjointTree(ws, strategy, neighbourElements, labelMap, positionLabelMap, progress);

            // Create the output workspace from the input workspace
            IMDHistoWorkspace_sptr outWS = cloneInputWorkspace(ws);
            const int nIndexesToProcess = static_cast<int>(neighbourElements.size());
            for (int i = 0; i < nIndexesToProcess; ++i)
            {
              if(!neighbourElements[i].isEmpty())
              {
                outWS->setSignalAt(i, neighbourElements[i].getRoot());
              }
              else
              {
                outWS->setSignalAt(i, 0);
              }
              outWS->setErrorSquaredAt(i, 0);
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
            VecElements neighbourElements(ws->getNPoints());

            // Perform the bulk of the connected component analysis, but don't collapse the elements yet.
            calculateDisjointTree(ws, strategy, neighbourElements, labelMap, positionLabelMap, progress);

            // Create the output workspace from the input workspace
            IMDHistoWorkspace_sptr outWS = cloneInputWorkspace(ws);

            progress.doReport("Integrating clusters and generating cluster image");
            const size_t nIterations = neighbourElements.size();
            const size_t maxReports = 1000;
            const size_t frequency = reportEvery(maxReports, nIterations);
            progress.resetNumSteps(maxReports, 0.5, 0.75);
            // Set each pixel to the root of each disjointed element.
            for (size_t i = 0; i < nIterations; ++i)
            {
              if(!neighbourElements[i].isEmpty())
              {
                const double& signal = ws->getSignalAt(i); // Intensity value at index
                double errorSQ = ws->getErrorAt(i);
                errorSQ *=errorSQ; // Error squared at index
                const size_t& labelId = neighbourElements[i].getRoot();
                // Set the output cluster workspace signal value
                outWS->setSignalAt(i, static_cast<Mantid::signal_t>(labelId));

                SignalErrorSQPair current = labelMap[labelId];
                // Sum labels. This is integration!
                labelMap[labelId] = SignalErrorSQPair(current.get<0>() + signal, current.get<1>() + errorSQ);

                outWS->setSignalAt(i, neighbourElements[i].getRoot());
              }
              else
              {
                outWS->setSignalAt(i, 0);
              }
              outWS->setErrorSquaredAt(i, 0);
              if(i % frequency == 0)
              {
                progress.doReport();
              }
            }

            return outWS;
          }

        } // namespace Crystal
    } // namespace Mantid
