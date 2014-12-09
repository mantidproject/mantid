#ifndef MANTID_VATES_MD_REBINNING_VIEW
#define MANTID_VATES_MD_REBINNING_VIEW

#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"

class vtkImplicitFunction;
namespace Mantid
{
  namespace VATES
  {

     /** 
    @class MDRebinningView
    Abstract view for controlling multi-dimensional rebinning.
    @author Owen Arnold, Tessella plc
    @date 03/06/2011

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport MDRebinningView
    {
    public:
      virtual double getMaxThreshold() const = 0;
      virtual double getMinThreshold() const = 0;
      virtual bool getApplyClip() const = 0;
      virtual double getTimeStep() const = 0;
      virtual Mantid::Kernel::V3D getOrigin() const = 0;
      virtual Mantid::Kernel::V3D getB1() const = 0;
      virtual Mantid::Kernel::V3D getB2() const = 0;
      virtual double getLengthB1() const = 0;
      virtual double getLengthB2() const = 0;
      virtual double getLengthB3() const = 0;
      virtual bool getForceOrthogonal() const = 0;
      virtual bool getOutputHistogramWS() const = 0;
      virtual const char* getAppliedGeometryXML() const = 0;
      virtual void updateAlgorithmProgress(double, const std::string&) = 0;
      virtual ~MDRebinningView(){}
    };
  }
}

#endif
