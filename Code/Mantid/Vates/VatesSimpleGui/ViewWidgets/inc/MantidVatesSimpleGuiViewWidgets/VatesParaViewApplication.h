#ifndef VATESPARAVIEWAPPLICATION_H_
#define VATESPARAVIEWAPPLICATION_H_

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "MantidKernel/Logger.h"

#include <QObject>
#include <QPointer>

namespace Mantid
{
  namespace Vates
  {
    namespace SimpleGui
    {
      /**
       *
       This class creates four views of the given dataset. There are three 2D views
       for the three orthogonal Cartesian planes and one 3D view of the dataset
       showing the planes.

       @author
       @date

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
      class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS VatesParaViewApplication : public QObject
      {
        Q_OBJECT

      public:
        static VatesParaViewApplication* instance();
        void setupParaViewBehaviors();
      protected:
        VatesParaViewApplication();
        ~VatesParaViewApplication();
      private:
        Q_DISABLE_COPY(VatesParaViewApplication)
        Mantid::Kernel::Logger m_logger;
        bool m_behaviorsSetup;

      };
    } //SimpleGui
  } //Vates
} //Mantid
#endif
