#include "MantidQtSliceViewer/PeakBoundingBox.h"
#include <stdexcept>

namespace MantidQt
{
  namespace SliceViewer
  {
      PeakBoundingBox::PeakBoundingBox() : m_left(0), m_right(0), m_top(0), m_bottom(0), m_slicePoint(0)
      {
      }

      PeakBoundingBox::PeakBoundingBox(const Left& left, const Right& right, const Top& top, const Bottom& bottom, const SlicePoint& slicePoint) : m_left(left), m_right(right), m_top(top), m_bottom(bottom), m_slicePoint(slicePoint)
      {
        if(right() < left())
        {
          throw std::invalid_argument("Right < Left");
        }
        if(top() < bottom())
        {
          throw std::invalid_argument("Top < Bottom");
        }
      }

      PeakBoundingBox::~PeakBoundingBox()
      {
      }

      PeakBoundingBox::PeakBoundingBox(const PeakBoundingBox& other) : m_left(other.m_left), m_right(other.m_right), m_top(other.m_top), m_bottom(other.m_bottom), m_slicePoint(other.m_slicePoint)
      {
      }

      PeakBoundingBox& PeakBoundingBox::operator=(const PeakBoundingBox& other)
      {
        if(&other != this)
        {
          m_top = other.m_top;
          m_bottom = other.m_bottom;
          m_left = other.m_left;
          m_right = other.m_right;
          m_slicePoint = other.m_slicePoint;
        }
        return *this;
      }

      double PeakBoundingBox::left() const
      {
        return m_left();
      }

      double PeakBoundingBox::right() const
      {
        return m_right();
      }

      double PeakBoundingBox::top() const
      {
        return m_top();
      }

      double PeakBoundingBox::bottom() const
      {
        return m_bottom();
      }

      double PeakBoundingBox::slicePoint() const
      {
        return m_slicePoint();
      }
  }
}