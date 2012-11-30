#ifndef MANTID_SLICEVIEWER_PEAKTRANSFORM_H_
#define MANTID_SLICEVIEWER_PEAKTRANSFORM_H_

#include "MantidKernel/V3D.h"
#include <boost/regex.hpp>

namespace MantidQt
{
  namespace SliceViewer
  {
    /**
    @class PeakTransformHKL
    Used to remap coordinates into a form consistent with an axis reordering.
    */
    class DLLExport PeakTransformHKL 
    {
    public:
      PeakTransformHKL(const std::string& xPlotLabel, const std::string& yPlotLabel);
      virtual ~PeakTransformHKL();
      PeakTransformHKL(const PeakTransformHKL& other);
      PeakTransformHKL & operator=(const PeakTransformHKL & other);
      Mantid::Kernel::V3D transform(const Mantid::Kernel::V3D& original) const;
      boost::regex getFreePeakAxisRegex() const;
    private:
      std::string m_xPlotLabel;
      std::string m_yPlotLabel;
      int m_indexOfPlotX;
      int m_indexOfPlotY;
      int m_indexOfPlotZ;
      boost::regex m_HRegex;
      boost::regex m_KRegex;
      boost::regex m_LRegex;
    };
    
    /**
    @class PeakTransformException
    Exceptions occuring when PeakTransformations cannot be formed.
    */
    class PeakTransformException : public std::exception
    {
    public:
      PeakTransformException() : std::exception()
      {
      }
    };

  }
}

#endif /* MANTID_SLICEVIEWER_CONCRETEPEAKSPRESENTER_H_ */