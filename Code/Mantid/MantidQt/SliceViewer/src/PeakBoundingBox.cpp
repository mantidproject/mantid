#include "MantidQtSliceViewer/PeakBoundingBox.h"
#include <stdexcept>

namespace MantidQt
{
  namespace SliceViewer
  {
      /**
       * Default Constructor
       */
      PeakBoundingBox::PeakBoundingBox() : m_left(0), m_right(0), m_top(0), m_bottom(0), m_slicePoint(0)
      {
      }

      /**
       * Constructor
       * @param left: Box left
       * @param right : Box right
       * @param top : Box top
       * @param bottom : Box bottom
       * @param slicePoint : Slicing point.
       */
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

      /// Destructor
      PeakBoundingBox::~PeakBoundingBox()
      {
      }

      /**
       * Copy constructor
       * @param other
       */
      PeakBoundingBox::PeakBoundingBox(const PeakBoundingBox& other) : m_left(other.m_left), m_right(other.m_right), m_top(other.m_top), m_bottom(other.m_bottom), m_slicePoint(other.m_slicePoint)
      {
      }

      /**
       * Assignment operator
       * @param other : Other box to assign from
       * @return : This object after assignment
       */
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

      /**
       * Getter for left edge.
       * @return Left edge
       */
      double PeakBoundingBox::left() const
      {
        return m_left();
      }

      /**
       * Getter for right edge.
       * @return Right edge
       */
      double PeakBoundingBox::right() const
      {
        return m_right();
      }

      /**
       * Getter for top edge
       * @return Top edge
       */
      double PeakBoundingBox::top() const
      {
        return m_top();
      }

      /**
       * Getter for bottom edge
       * @return bottom edge
       */
      double PeakBoundingBox::bottom() const
      {
        return m_bottom();
      }

      /**
       * Getter for the slice point
       * @return The slice point
       */
      double PeakBoundingBox::slicePoint() const
      {
        return m_slicePoint();
      }
  }
}
