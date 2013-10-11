#include "RotationSurface.h"

#include <QCursor>
#include <QMessageBox>
#include <QApplication>

using namespace Mantid::Geometry;

RotationSurface::RotationSurface(const InstrumentActor* rootActor,const Mantid::Kernel::V3D& origin,const Mantid::Kernel::V3D& axis):
    UnwrappedSurface(rootActor),
    m_pos(origin),
    m_zaxis(axis),
    m_u_correction(0)
{
}

/**
 * Initialize the surface.
 */
void RotationSurface::init()
{
    // the actor calls this->callback for each detector
    m_unwrappedDetectors.clear();
    m_assemblies.clear();

    size_t ndet = m_instrActor->ndetectors();
    m_unwrappedDetectors.resize(ndet);
    if (ndet == 0) return;

    // Pre-calculate all the detector positions (serial because
    // I suspect the IComponent->getPos() method to not be properly thread safe)
    m_instrActor->cacheDetPos();

    Instrument_const_sptr inst = m_instrActor->getInstrument();

    // First detector defines the surface's x axis
    if (m_xaxis.nullVector())
    {
      Mantid::Kernel::V3D pos = m_instrActor->getDetPos(0) - m_pos;
      double z = pos.scalar_prod(m_zaxis);
      if (z == 0.0 || fabs(z) == pos.norm())
      {
        // find the shortest projection of m_zaxis and direct m_xaxis along it
        bool isY = false;
        bool isZ = false;
        if (fabs(m_zaxis.Y()) < fabs(m_zaxis.X())) isY = true;
        if (fabs(m_zaxis.Z()) < fabs(m_zaxis.Y())) isZ = true;
        if (isZ)
        {
          m_xaxis = Mantid::Kernel::V3D(0,0,1);
        }
        else if (isY)
        {
          m_xaxis = Mantid::Kernel::V3D(0,1,0);
        }
        else
        {
          m_xaxis = Mantid::Kernel::V3D(1,0,0);
        }
      }
      else
      {
        m_xaxis = pos - m_zaxis * z;
        m_xaxis.normalize();
      }
      m_yaxis = m_zaxis.cross_prod(m_xaxis);
    }

    // give some valid values to u bounds in case some code checks
    // on u to be within them
    m_u_min = -DBL_MAX;
    m_u_max =  DBL_MAX;

    // For each detector in the order of actors
    // cppcheck-suppress syntaxError
    PRAGMA_OMP( parallel for )
    for(int ii = 0; ii < int(ndet); ++ii)
    {
      size_t i=size_t(ii);

      unsigned char color[3];
      Mantid::detid_t id = m_instrActor->getDetID(i);

      boost::shared_ptr<const Mantid::Geometry::IDetector> det;
      try
      {
        det = inst->getDetector(id);
      }
      catch (Mantid::Kernel::Exception::NotFoundError & )
      {
      }

      if (!det || det->isMonitor() || (id < 0))
      {
        // Not a detector or a monitor
        // Make some blank, empty thing that won't draw
        m_unwrappedDetectors[i] = UnwrappedDetector();
      }
      else
      {
        // A real detector.
        m_instrActor->getColor(id).getUB3(&color[0]);

        // Position, relative to origin
        //Mantid::Kernel::V3D pos = det->getPos() - m_pos;
        Mantid::Kernel::V3D pos = m_instrActor->getDetPos(i) - m_pos;

        // Create the unwrapped shape
        UnwrappedDetector udet(&color[0],det);
        // Calculate its position/size in UV coordinates
        this->calcUV(udet, pos);

        m_unwrappedDetectors[i] = udet;
      } // is a real detectord
    } // for each detector in pick order

    // Now find the overall edges in U and V coords.
    m_u_min =  DBL_MAX;
    m_u_max = -DBL_MAX;
    m_v_min =  DBL_MAX;
    m_v_max = -DBL_MAX;
    for(size_t i=0;i<m_unwrappedDetectors.size();++i)
    {
      const UnwrappedDetector& udet = m_unwrappedDetectors[i];
      if (! udet.detector ) continue;
      if (udet.u < m_u_min) m_u_min = udet.u;
      if (udet.u > m_u_max) m_u_max = udet.u;
      if (udet.v < m_v_min) m_v_min = udet.v;
      if (udet.v > m_v_max) m_v_max = udet.v;
    }

    findAndCorrectUGap();

    double dU = fabs(m_u_max - m_u_min);
    double dV = fabs(m_v_max - m_v_min);
    double du = dU * 0.05;
    double dv = dV * 0.05;
    if (m_width_max > du && m_width_max != std::numeric_limits<double>::infinity())
    {
      if (du > 0 && !(dU >= m_width_max))
      {
        m_width_max = dU;
      }
      du = m_width_max;
    }
    if (m_height_max > dv && m_height_max != std::numeric_limits<double>::infinity())
    {
      if (dv > 0 && !(dV >= m_height_max))
      {
        m_height_max = dV;
      }
      dv = m_height_max;
    }
    m_u_min -= du;
    m_u_max += du;
    m_v_min -= dv;
    m_v_max += dv;
    m_viewRect = RectF( QPointF(m_u_min,m_v_min), QPointF(m_u_max,m_v_max) );

}

void RotationSurface::findAndCorrectUGap()
{
  double period = uPeriod();
  if (period == 0.0) return;
  const int nbins = 1000;
  std::vector<bool> ubins(nbins);
  double bin_width = fabs(m_u_max - m_u_min) / (nbins - 1);
  if (bin_width == 0.0)
  {
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
    QMessageBox::warning(NULL, tr("MantidPLot - Instrument view error"), tr("Failed to build unwrapped surface"));
    QApplication::restoreOverrideCursor();
    m_u_min = 0.0;
    m_u_max = 1.0;
    return;
  }

  std::vector<UnwrappedDetector>::const_iterator ud = m_unwrappedDetectors.begin();
  for(;ud != m_unwrappedDetectors.end(); ++ud)
  {
    if (! ud->detector ) continue;
    double u = ud->u;
    int i = int((u - m_u_min) / bin_width);
    ubins[i] = true;
  }

  int iFrom = 0; // marks gap start
  int iTo   = 0; // marks gap end
  int i0 = 0;
  bool inGap = false;
  for(int i = 0;i < int(ubins.size())-1;++i)
  {
    if (!ubins[i])
    {
      if (!inGap)
      {
        i0 = i;
      }
      inGap = true;
    }
    else
    {
      if (inGap && iTo - iFrom < i - i0)
      {
        iFrom = i0; // first bin in the gap
        iTo   = i;  // first bin after the gap
      }
      inGap = false;
    }
  }

  double uFrom = m_u_min + iFrom * bin_width;
  double uTo   = m_u_min + iTo   * bin_width;
  if (uTo - uFrom > period - (m_u_max - m_u_min))
  {
    double du = m_u_max - uTo;
    m_u_max = uFrom + du;
    std::vector<UnwrappedDetector>::iterator ud = m_unwrappedDetectors.begin();
    for(;ud != m_unwrappedDetectors.end(); ++ud)
    {
      if (! ud->detector ) continue;
      double& u = ud->u;
      u += du;
      if (u > m_u_max)
      {
        u -= period;
      }
    }
    m_u_correction += du;
    if (m_u_correction > m_u_max)
    {
      m_u_correction -= period;
    }
  }
}

/**
 * Apply a correction to u value of a projected point due to
 * change of u-scale by findAndCorrectUGap()
 * @param u :: u-coordinate to be corrected
 * @return :: Corrected u-coordinate.
 */
double RotationSurface::applyUCorrection(double u)const
{
  double period = uPeriod();
  if (period == 0.0) return u;
  u += m_u_correction;
  if (u < m_u_min)
  {
    u += period;
  }
  if (u > m_u_max)
  {
    u -= period;
  }
  return u;
}

