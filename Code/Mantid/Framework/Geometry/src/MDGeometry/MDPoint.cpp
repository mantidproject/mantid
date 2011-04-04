#include "MantidGeometry/MDGeometry/MDPoint.h"

namespace Mantid
{
  namespace Geometry
  {
    MDPoint::MDPoint(double signal, double error,const std::vector<coordinate>& vertexes, IDetector_sptr detector, IInstrument_sptr instrument,
        const int runId)
      : m_signal(signal),
        m_error(error),
        m_instrument(instrument),
        m_detector(detector),
        m_vertexes(vertexes),
        m_runId(runId)
    {
    }

      std::vector<coordinate> MDPoint::getVertexes() const
      {
        return this->m_vertexes;
      }

      double MDPoint::getSignal() const
      {
        return this->m_signal;
      }

      double MDPoint::getError() const
      {
        return this->m_error;
      }

      IDetector_sptr MDPoint::getDetector() const
      {
        return this->m_detector;
      }

      IInstrument_sptr MDPoint::getInstrument() const
      {
        return this->m_instrument;
      }

      int MDPoint::getRunId() const
      {
        return this->m_runId;
      }

      std::vector<boost::shared_ptr<MDPoint> > MDPoint::getContributingPoints() const
      {
        throw std::logic_error("A Point is indivisible, cannot have contributing Points to a Point.");
      }

      MDPoint::~MDPoint()
      {
      }

  }
}
