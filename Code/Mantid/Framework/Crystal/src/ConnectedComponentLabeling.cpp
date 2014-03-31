#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/V3D.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidCrystal/ConnectedComponentLabeling.h"
#include "MantidCrystal/BackgroundStrategy.h"
#include "MantidCrystal/DisjointElement.h"
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/scoped_ptr.hpp>
#include <stdexcept>
#include <set>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Crystal::ConnectedComponentMappingTypes;

namespace Mantid
{
  namespace Crystal
  {
    namespace
    {
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
    }

    //----------------------------------------------------------------------------------------------
    /** Constructor
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

    int ConnectedComponentLabeling::getNThreads() const
    {
      return m_runMultiThreaded ? API::FrameworkManager::Instance().getNumOMPThreads() : 1;
    }

    void ConnectedComponentLabeling::calculateDisjointTree(IMDHistoWorkspace_sptr ws, 
      BackgroundStrategy * const strategy, VecElements& neighbourElements,
      LabelIdIntensityMap& labelMap,
      PositionToLabelIdMap& positionLabelMap
      ) const
    {

      VecIndexes allNonBackgroundIndexes;
      allNonBackgroundIndexes.reserve(ws->getNPoints());

      if(m_runMultiThreaded)
      {
    
        std::vector<API::IMDIterator*> iterators = ws->createIterators(getNThreads());
        const int nthreads = static_cast<int>(iterators.size());
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
              }
            }
            while(iterator->next());
          }
          // Consolidate work from individual threads.
          for(size_t i = 0; i < nthreads; ++i)
          {
            VecIndexes& source = manyNonBackgroundIndexes[i];
            allNonBackgroundIndexes.insert(allNonBackgroundIndexes.end(), source.begin(), source.end());
          }
      }
      else
      {
        API::IMDIterator *iterator = ws->createIterator(NULL);
        do
        {
          if(!strategy->isBackground(iterator))
          {
            allNonBackgroundIndexes.push_back( iterator->getLinearIndex() );
          }
        }
        while(iterator->next());
      }

      // -------- Perform labeling -----------
      const size_t maxNeighbours = calculateMaxNeighbours(ws.get());
      IMDIterator* iterator = ws->createIterator(NULL);
      size_t currentLabelCount = m_startId;
      for(size_t ii = 0; ii < allNonBackgroundIndexes.size(); ++ii)
      {
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
          // Make this element a copy of the parent
          neighbourElements[currentIndex] = parentElement;
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

    boost::shared_ptr<Mantid::API::IMDHistoWorkspace> ConnectedComponentLabeling::execute(
      IMDHistoWorkspace_sptr ws, BackgroundStrategy * const strategy) const
    {
      VecElements neighbourElements(ws->getNPoints());

      // Perform the bulk of the connected component analysis, but don't collapse the elements yet.
      LabelIdIntensityMap labelMap; // This will not get used.
      PositionToLabelIdMap positionLabelMap; // This will not get used.
      calculateDisjointTree(ws, strategy, neighbourElements, labelMap, positionLabelMap);

      // Create the output workspace from the input workspace
      IMDHistoWorkspace_sptr outWS = cloneInputWorkspace(ws);

      // Set each pixel to the root of each disjointed element.
      PARALLEL_FOR_NO_WSP_CHECK()
      for (int i = 0; i < static_cast<int>(neighbourElements.size()); ++i)
      {
        //std::cout << "Element\t" << i << " Id: \t" << neighbourElements[i].getId() << " This location:\t"<< &neighbourElements[i] << " Root location:\t" << neighbourElements[i].getParent() << " Root Id:\t" <<  neighbourElements[i].getRoot() << std::endl;
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

    boost::shared_ptr<Mantid::API::IMDHistoWorkspace> ConnectedComponentLabeling::executeAndIntegrate(
      IMDHistoWorkspace_sptr ws, BackgroundStrategy * const strategy, LabelIdIntensityMap& labelMap,
      PositionToLabelIdMap& positionLabelMap) const
    {
      VecElements neighbourElements(ws->getNPoints());

      // Perform the bulk of the connected component analysis, but don't collapse the elements yet.
      calculateDisjointTree(ws, strategy, neighbourElements, labelMap, positionLabelMap);

      // Create the output workspace from the input workspace
      IMDHistoWorkspace_sptr outWS = cloneInputWorkspace(ws);

      // Set each pixel to the root of each disjointed element.
      for (size_t i = 0; i < neighbourElements.size(); ++i)
      {
        if(!neighbourElements[i].isEmpty())
        {
          const double& signal = ws->getSignalAt(i); // Intensity value at index
          double errorSQ = ws->getErrorAt(i);
          errorSQ *=errorSQ; // Error squared at index
          const size_t& labelId = neighbourElements[i].getRoot();
          // Set the output cluster workspace signal value
          outWS->setSignalAt(i, labelId);

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
      }

      return outWS;
    }

  } // namespace Crystal
} // namespace Mantid
