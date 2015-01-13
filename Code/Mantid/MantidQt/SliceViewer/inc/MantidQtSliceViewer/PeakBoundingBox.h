#ifndef MANTID_SLICEVIEWER_PEAK_BOUNDING_BOX_H_
#define MANTID_SLICEVIEWER_PEAK_BOUNDING_BOX_H_

#include "MantidKernel/System.h"
#include <string>
#include <vector>
#include "MantidAPI/PeakTransform.h"

namespace MantidQt
{
  namespace SliceViewer
  {
    /**
    DoubleParam
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
    typedef DoubleParam<5> Front;
    typedef DoubleParam<6> Back;

    /** A bounding box for a peak. Allows the SliceViewer to zoom to that region.
    
    @date 2013-01-09

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
      
      /// Left edge
      Left m_left;
      /// Right edge
      Right m_right;
      /// Top edge
      Top m_top;
      /// Bottom edge.
      Bottom m_bottom;
      /// Slice parallel to projection (z) position
      SlicePoint m_slicePoint;
      /// Front edge
      Front m_front;
      /// Back edge
      Back m_back;
      /// Check boundaries
      void validateBoundaries();

    public:
      /// Default constructor
      PeakBoundingBox();
      /// Constructor
      PeakBoundingBox(const Left& left, const Right& right, const Top& top, const Bottom& bottom, const SlicePoint& slicePoint);
      /// Constructor
      PeakBoundingBox(const Left& left, const Right& right, const Top& top, const Bottom& bottom, const SlicePoint& slicePoint, const Front& front, const Back& back);
      /// Destructor
      ~PeakBoundingBox();
      /// Copy constructor
      PeakBoundingBox(const PeakBoundingBox& other);
      /// Assignment
      PeakBoundingBox& operator=(const PeakBoundingBox& other);
      /// Equals
      bool operator==(const PeakBoundingBox &other) const;
      /// Not equals
      bool operator!=(const PeakBoundingBox &other) const;
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
      /// Get the back edge
      double front() const;
      /// Get the front edge
      double back() const;
      /// Serialize as a vector of extents.
      std::vector<double> toExtents() const;
      /// Serialize as set of comma separated values
      std::string toExtentsString() const;
      /// Transform the box.
      void transformBox(Mantid::API::PeakTransform_sptr transform);
      /// Make a new box based on the slice
      PeakBoundingBox makeSliceBox(const double& sliceDelta) const;
    };
  }
}

#endif /* MANTID_SLICEVIEWER_PEAK_BOUNDING_BOX_H_ */
