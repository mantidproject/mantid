#ifndef MANTID_VATES_MD_REBINNING_VIEW_ADAPTER_H
#define MANTID_VATES_MD_REBINNING_VIEW_ADAPTER_H

#include "MantidVatesAPI/MDRebinningView.h"

class vtkImplicitFunction;
namespace Mantid
{
  namespace VATES
  {

    /** 
    @class MDRebinningViewAdapter
    Adapter for non-MDRebinningView types that need to be used as MDRebinningViews. See Adapter pattern.
    @author Owen Arnold, Tessella plc
    @date 07/09/2011

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
    template<typename ViewType>
    class DLLExport MDRebinningViewAdapter : public MDRebinningView
    {
    private:

      ViewType* m_adaptee;

    public:

      MDRebinningViewAdapter(ViewType* adaptee) : m_adaptee(adaptee)
      {
      }

      virtual bool getForceOrthogonal() const
      {
        return m_adaptee->getForceOrthogonal();
      }

      virtual Mantid::Kernel::V3D getOrigin() const
      {
        return m_adaptee->getOrigin();
      }

      virtual Mantid::Kernel::V3D getB1() const
      {
        return m_adaptee->getB1();
      }

      virtual Mantid::Kernel::V3D getB2() const
      {
        return m_adaptee->getB2();
      }

      virtual double getLengthB1() const
      {
        return m_adaptee->getLengthB1();
      }

      virtual double getLengthB2() const
      {
        return m_adaptee->getLengthB2();
      }

      virtual double getLengthB3() const
      {
        return m_adaptee->getLengthB3();
      }

      virtual double getMaxThreshold() const
      {
        return m_adaptee->getMaxThreshold();
      }

      virtual double getMinThreshold() const
      {
        return m_adaptee->getMinThreshold();
      }

      virtual bool getApplyClip() const
      {
        return m_adaptee->getApplyClip();
      }

      virtual double getTimeStep() const
      {
        return m_adaptee->getTimeStep();
      }

      virtual const char* getAppliedGeometryXML() const
      {
        return m_adaptee->getAppliedGeometryXML();
      }

      virtual void updateAlgorithmProgress(double progress, const std::string& message)
      {
        return m_adaptee->updateAlgorithmProgress(progress, message);
      }

      virtual bool getOutputHistogramWS() const
      {
        return m_adaptee->getOutputHistogramWS();
      }

      virtual ~MDRebinningViewAdapter()
      {
        //Do not delete adaptee.
      }
    };
  }
}

#endif
