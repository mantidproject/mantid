#ifndef MANTID_SLICEVIEWER_PEAK_BOUNDING_BOX_H_
#define MANTID_SLICEVIEWER_PEAK_BOUNDING_BOX_H_

#include "MantidKernel/System.h"

namespace MantidQt
{
  namespace SliceViewer
  {
    /**
    @class DoubleParam
    IntToType Parameter Type. Simple mechanism for ensuring type 
    safety when working with so many arguments of the same core type in PeakBoundingBox.
    */
    template<int I>
    class DLLExport DoubleParam
    {
    public:
      explicit DoubleParam(const double& val) : value(val){}
      DoubleParam(const DoubleParam<I>& other) : value(other.value) {}
      DoubleParam<I>& operator=(const DoubleParam<I>& other){value = other.value; return *this;}
      double operator()() const {return value;}
    private:
      double value;
      enum{typeValue = I};
    };

    typedef DoubleParam<0> Left;
    typedef DoubleParam<1> Right;
    typedef DoubleParam<2> Top;
    typedef DoubleParam<3> Bottom;
    typedef DoubleParam<4> SlicePoint;

    /** A bounding box for a peak. Allows the SliceViewer to zoom to that region.
    
    @date 2013-01-09

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
    class DLLExport PeakBoundingBox
    {
    private:
      
      Left m_left;
      Right m_right;
      Top m_top;
      Bottom m_bottom;

      /// Slice parellel to projection (z) position
      SlicePoint m_slicePoint;

    public:
      /// Default constructor
      PeakBoundingBox();
      /// Constructor
      PeakBoundingBox(const Left& left, const Right& right, const Top& top, const Bottom& bottom, const SlicePoint& slicePoint);
      /// Destructor
      ~PeakBoundingBox();
      /// Copy constructor
      PeakBoundingBox(const PeakBoundingBox& other);
      /// Assignment
      PeakBoundingBox& operator=(const PeakBoundingBox& other);

      /// Get the box left edge
      double left() const;
      /// Get the box right edge
      double right() const;
      /// Get the box top edge
      double top() const;
      /// Get the box bottom edge
      double bottom() const;
      /// Get the slice point
      double slicePoint() const;

    };
  }
}

#endif /* MANTID_SLICEVIEWER_PEAK_BOUNDING_BOX_H_ */