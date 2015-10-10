#ifndef MANTID_VATES_MD_LOADING_VIEW_ADAPTER_H
#define MANTID_VATES_MD_LOADING_VIEW_ADAPTER_H

#include "MantidVatesAPI/MDLoadingView.h"

namespace Mantid
{
  namespace VATES
  {

     /** 
    @class MDLoadingViewAdapter
    Templated type for wrapping non-MDLoadingView types. Adapter pattern.
    @author Owen Arnold, Tessella plc
    @date 07/09/2011

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
    template<typename ViewType>
    class DLLExport MDLoadingViewAdapter : public MDLoadingView
    {
    private:
      ViewType* m_adaptee;
    public:

      MDLoadingViewAdapter(ViewType* adaptee) : m_adaptee(adaptee)
      {
      }

      virtual double getTime() const
      {
        return m_adaptee->getTime();
      }

      virtual size_t getRecursionDepth() const
      {
        return m_adaptee->getRecursionDepth();
      }

      virtual bool getLoadInMemory() const
      {
        return m_adaptee->getLoadInMemory();
      }

      virtual ~MDLoadingViewAdapter()
      {
        //Do not delete adaptee.
      }
    };
  }
}

#endif
