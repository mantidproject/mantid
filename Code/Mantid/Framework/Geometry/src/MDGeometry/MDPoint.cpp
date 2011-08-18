#include "MantidGeometry/MDGeometry/MDPoint.h"

namespace Mantid
{
  namespace Geometry
  {
    MDPoint::MDPoint(signal_t signal, signal_t error,
        const std::vector<Coordinate>& vertexes, IDetector_sptr detector, Instrument_sptr instrument,
        const int runId)
      : m_signal(signal),
        m_error(error),
        m_instrument(instrument),
        m_detector(detector),
        m_vertexes(vertexes),
        m_runId(runId)
    {
    }

      std::vector<Coordinate> MDPoint::getVertexes() const
      {
        return this->m_vertexes;
      }

      signal_t MDPoint::getSignal() const
      {
        return this->m_signal;
      }

      signal_t MDPoint::getError() const
      {
        return this->m_error;
      }

      IDetector_sptr MDPoint::getDetector() const
      {
        return this->m_detector;
      }

      Instrument_sptr MDPoint::getInstrument() const
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
