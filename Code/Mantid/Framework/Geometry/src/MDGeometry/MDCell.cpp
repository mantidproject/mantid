#include "MantidGeometry/MDGeometry/MDCell.h"
#include "MantidGeometry/MDGeometry/MDPoint.h"

namespace Mantid
{
  namespace Geometry
  {

    MDCell::MDCell(std::vector<boost::shared_ptr<Mantid::Geometry::MDPoint> > pContributingPoints, std::vector<coordinate> vertexes) 
      : m_cachedSignal(0),
        m_cachedError(0),
        m_vertexes(vertexes),
        m_contributingPoints(pContributingPoints)
    {
      //TODO, Accelerate. if no contributing points then can immediately exit and.
      //TODO, depending on client-code usage, may be more optimal to place method call on the getters.
      calculateCachedValues();
    }

    MDCell::MDCell(const double& signal,const double& error, const std::vector<coordinate>& vertexes)
    : m_cachedSignal(signal),
      m_cachedError(error),
      m_vertexes(vertexes)
    {

    }



    std::vector<coordinate> MDCell::getVertexes() const
    {
      return this->m_vertexes;
    }

    double MDCell::getSignal() const
    {
      return this->m_cachedSignal;
    }
    double MDCell::getError() const
    {
      return this->m_cachedError;
    }
    std::vector<boost::shared_ptr<MDPoint> > MDCell::getContributingPoints() const
    {
      return this->m_contributingPoints;
    }

    void MDCell::calculateCachedValues()
    {
      for(unsigned int i = 0; i < m_contributingPoints.size(); i++)
      {
        boost::shared_ptr<SignalAggregate> point = m_contributingPoints.at(i);

        this->m_cachedSignal += point->getSignal();
        this->m_cachedError += point->getError();
      }
    }

    MDCell::~MDCell()
    {
    }
  }
}
