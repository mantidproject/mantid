#include "MantidKernel/MultiThreaded.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidCrystal/ConnectedComponentLabeling.h"
#include "MantidCrystal/BackgroundStrategy.h"
#include "MantidCrystal/DisjointElement.h"
#include <boost/shared_ptr.hpp>
#include <stdexcept>
#include <set>

using namespace Mantid::API;

namespace Mantid
{
  namespace Crystal
  {
    namespace
    {
      typedef std::vector<size_t> VecIndexes;
      typedef std::vector<DisjointElement> VecElements;
      typedef std::set<size_t> SetIds;

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

    boost::shared_ptr<Mantid::API::IMDHistoWorkspace> ConnectedComponentLabeling::execute(
      IMDHistoWorkspace_sptr ws, BackgroundStrategy * const strategy) const
    {

      auto alg = AlgorithmManager::Instance().createUnmanaged("CloneWorkspace");
      alg->initialize();
      alg->setChild(true);
      alg->setProperty("InputWorkspace", ws);
      alg->setPropertyValue("OutputWorkspace", "out_ws");
      alg->execute();

      Mantid::API::IMDHistoWorkspace_sptr out_ws;
      {
        Mantid::API::Workspace_sptr temp = alg->getProperty("OutputWorkspace");
        out_ws = boost::dynamic_pointer_cast<IMDHistoWorkspace>(temp);
      }

      VecElements neighbourElements(ws->getNPoints());

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


      // Set each pixel to the root of each disjointed element.
      PARALLEL_FOR_NO_WSP_CHECK()
      for (int i = 0; i < static_cast<int>(neighbourElements.size()); ++i)
      {
        //std::cout << "Element\t" << i << " Id: \t" << neighbourElements[i].getId() << " This location:\t"<< &neighbourElements[i] << " Root location:\t" << neighbourElements[i].getParent() << " Root Id:\t" <<  neighbourElements[i].getRoot() << std::endl;
        if(!neighbourElements[i].isEmpty())
        {
          out_ws->setSignalAt(i, neighbourElements[i].getRoot());
        }
        else
        {
          out_ws->setSignalAt(i, 0);
        }
        out_ws->setErrorSquaredAt(i, 0);
      }

      return out_ws;
    }

  } // namespace Crystal
} // namespace Mantid
