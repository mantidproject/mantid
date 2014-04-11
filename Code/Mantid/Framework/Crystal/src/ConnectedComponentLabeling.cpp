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
    }

    /**
     * Constructor
     * @param startId : Start Id to use for labeling
     * @param runMultiThreaded : Run multi threaded. Defaults to true.
     */
    ConnectedComponentLabeling::ConnectedComponentLabeling(const size_t& startId, const bool runMultiThreaded) 
      : m_startId(startId), m_runMultiThreaded(runMultiThreaded)
    {
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
      return m_runMultiThreaded ? API::FrameworkManager::Instance().getNumOMPThreads() : 1;
    }

    /**
     * Perform the work of the CCL algorithm
     * - Pre filtering of background
     * - Labeling using DisjointElements
     *
     * @param ws : MDHistoWorkspace to run CCL algorithm on
     * @param strategy : Background strategy
     * @param neighbourElements : Neighbour elements containing DisjointElements
     * @param labelMap : Map of label id to signal, error_sq pair for integration purposes to fill
     * @param positionLabelMap : Map of label ids to position in workspace coordinates to fill
     * @param progress : Progress object
     */
    void ConnectedComponentLabeling::calculateDisjointTree(IMDHistoWorkspace_sptr ws, 
      BackgroundStrategy * const strategy, VecElements& neighbourElements,
      LabelIdIntensityMap& labelMap,
      PositionToLabelIdMap& positionLabelMap,
      Progress& progress
      ) const
    {

      VecIndexes allNonBackgroundIndexes;
      allNonBackgroundIndexes.reserve(ws->getNPoints());

      progress.doReport("Pre-processing to filter background out");
      progress.resetNumSteps(100000, 0.0, 0.25);
      if(m_runMultiThreaded)
      {
        std::vector<API::IMDIterator*> iterators = ws->createIterators(getNThreads());
        const int nthreads = getNThreads();
        std::vector<VecIndexes> manyNonBackgroundIndexes(nthreads);

        PARALLEL_FOR_NO_WSP_CHECK()
          for(int i = 0; i < nthreads; ++i)
          {
            boost::scoped_ptr<BackgroundStrategy> strategyCopy(strategy->clone());
            API::IMDIterator *iterator = iterators[i];
            VecIndexes& nonBackgroundIndexes = manyNonBackgroundIndexes[i];
            do
            {
              if(!strategyCopy->isBackground(iterator))
              {
                nonBackgroundIndexes.push_back( iterator->getLinearIndex() );
                progress.report();
              }
            }
            while(iterator->next());
          }
          // Consolidate work from individual threads.
          for(int i = 0; i < nthreads; ++i)
          {
            VecIndexes& source = manyNonBackgroundIndexes[i];
            allNonBackgroundIndexes.insert(allNonBackgroundIndexes.end(), source.begin(), source.end());
          }
      }
      else
      {
        progress.resetNumSteps(1, 0.0, 0.5);
        API::IMDIterator *iterator = ws->createIterator(NULL);
        do
        {
          if(!strategy->isBackground(iterator))
          {
            allNonBackgroundIndexes.push_back( iterator->getLinearIndex() );
            progress.report();
          }
          
        }
        while(iterator->next());
      }

      // -------- Perform labeling -----------
      progress.doReport("Perform connected component labeling");
      
      const size_t maxNeighbours = calculateMaxNeighbours(ws.get());
      IMDIterator* iterator = ws->createIterator(NULL);
      size_t currentLabelCount = m_startId;
      const size_t nIndexesToProcess= allNonBackgroundIndexes.size();
      const size_t maxReports = 100;
      const size_t frequency = reportEvery(maxReports, nIndexesToProcess);
      progress.resetNumSteps(100, 0.25, 0.5);
      for(size_t ii = 0; ii < nIndexesToProcess ; ++ii)
      {
        if(ii % frequency == 0)
        {
          progress.doReport();
        }
        size_t& currentIndex = allNonBackgroundIndexes[ii];
        iterator->jumpTo(currentIndex);

        // Linear indexes of neighbours
        VecIndexes neighbourIndexes = iterator->findNeighbourIndexes();
        VecIndexes nonEmptyNeighbourIndexes;
        nonEmptyNeighbourIndexes.reserve(maxNeighbours);
        SetIds neighbourIds;
        // Discover non-empty neighbours
        for (size_t i = 0; i < neighbourIndexes.size(); ++i)
        {
          size_t neighIndex = neighbourIndexes[i];
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
          const VMD& center = iterator->getCenter(); 
          positionLabelMap[V3D(center[0], center[1], center[2])] = currentLabelCount; // Get the position at this label.
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

      progress.doReport("Generating cluster image");
      const int nIndexesToProcess = static_cast<int>(neighbourElements.size());
      progress.resetNumSteps(nIndexesToProcess, 0.5, 0.75);
      // Set each pixel to the root of each disjointed element.
      PARALLEL_FOR_NO_WSP_CHECK()
      for (int i = 0; i < nIndexesToProcess; ++i)
      {
        if(!neighbourElements[i].isEmpty())
        {
          outWS->setSignalAt(i, neighbourElements[i].getRoot());
          progress.doReport();
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
