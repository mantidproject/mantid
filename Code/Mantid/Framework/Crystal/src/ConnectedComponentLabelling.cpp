#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidCrystal/ConnectedComponentLabelling.h"
#include "MantidCrystal/BackgroundStrategy.h"
#include "MantidCrystal/DisjointElement.h"
#include <boost/shared_ptr.hpp>
#include <stdexcept>

using namespace Mantid::API;

namespace Mantid
{
  namespace Crystal
  {
    namespace
    {
      typedef std::vector<size_t> VecIndexes;
      typedef std::vector<DisjointElement> VecElements;
      typedef std::vector<DisjointElement*> VecElementPtrs;
    }

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    ConnectedComponentLabelling::ConnectedComponentLabelling() : m_startId(0)
    {
    }

    /**
     * Set a custom start id. This has no bearing on the output of the process other than
     * the initial id used.
     * @param id: Id to start with
     */
    void ConnectedComponentLabelling::startLabellingId(const size_t& id)
    {
      m_startId = id;
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    ConnectedComponentLabelling::~ConnectedComponentLabelling()
    {
    }

    boost::shared_ptr<Mantid::API::IMDHistoWorkspace> ConnectedComponentLabelling::execute(
        IMDHistoWorkspace_sptr ws, BackgroundStrategy * const strategy) const
    {

      auto alg = AlgorithmManager::Instance().create("CloneWorkspace");
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

      IMDIterator* iterator = ws->createIterator(NULL);

      VecElements neighbourElements;
      for(size_t i = 0; i < ws->getNPoints(); ++i)
      {
        neighbourElements.push_back(DisjointElement());
      }

      size_t currentLabelCount = m_startId;
      size_t currentIndex = 0; // We assume current index in the image can be kept in sync with the iterator.
      do
      {
        if (!strategy->isBackground(iterator))
        {
          // Linear indexes of neighbours
          VecIndexes neighbourIndexes = iterator->findNeighbourIndexes();
          VecIndexes nonEmptyNeighbourIndexes;
          // Discover non-empty neighbours
          for (size_t i = 0; i < neighbourIndexes.size(); ++i)
          {
            size_t neighIndex = neighbourIndexes[i];
            DisjointElement& neighbourElement = neighbourElements[neighIndex];

            if (!neighbourElement.isEmpty())
            {
              nonEmptyNeighbourIndexes.push_back(neighIndex);
            }
          }

          if (nonEmptyNeighbourIndexes.empty())
          {
            neighbourElements[currentIndex] = DisjointElement(currentLabelCount); // New leaf
            ++currentLabelCount;
          }
          else if (nonEmptyNeighbourIndexes.size() == 1)
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
        ++currentIndex;
      } while (iterator->next());

      // Set each pixel to the root of each disjointed element.
      for (size_t i = 0; i < neighbourElements.size(); ++i)
      {
        //std::cout << "Element\t" << i << " Id: \t" << neighbourElements[i].getId() << " This location:\t"<< &neighbourElements[i] << " Root location:\t" << neighbourElements[i].getParent() << " Root Id:\t" <<  neighbourElements[i].getRoot() << std::endl;
        out_ws->setSignalAt(i, neighbourElements[i].getRoot());
      }

      return out_ws;
    }

  } // namespace Crystal
} // namespace Mantid
