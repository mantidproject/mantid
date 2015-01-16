#ifndef REBINMANAGER_H_
#define REBINMANAGER_H_

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include <QObject>
#include <vector>
#include <QStringList>

namespace Mantid
{
  namespace Vates
  {
    namespace SimpleGui
    {
      class RebinDialog;

      /**
       *
       This class coordinates the rebinning of a workspace and updates the pipeline and view to make the changes of the 
       underlying workspace visible.

       @date 15/01/2015

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
      class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS RebinManager :public QObject
      {
        Q_OBJECT
        public:
          RebinManager(QObject* parent = 0);

          ~RebinManager();

          void sendUpdate();

          void connectDialog(RebinDialog* rebinDialog);

        signals:
          void udpateDialog(QStringList algorithms,std::vector<QString> binNames, std::vector<int> bins);

        public slots:
          void onPerformRebinning(QString algorithm,std::vector<QString> binNames, std::vector<int> bins);

        private:
          RebinDialog* m_rebinDialog;
      };

    } // SimpleGui
  } // Vates
} // Mantid

#endif 
