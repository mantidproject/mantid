#ifndef REBINDIALOG_H_
#define REBINDIALOG_H_

#include "ui_RebinDialog.h"
#include <QDialog>
#include "MantidVatesSimpleGuiQtWidgets/WidgetDllOption.h"
#include <QLabel>
#include <QSpinBox>

namespace Mantid
{
  namespace Vates
  {
    namespace SimpleGui
    {
      /**
       *
        This class provides a dialog to perform rebinning.

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

      class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_QTWIDGETS RebinDialog :  public QDialog
      {
        Q_OBJECT

        public:
          RebinDialog(QWidget *parent = 0);
          
          ~RebinDialog();


       public slots:
         void onUpdateDialog(QStringList algorithms, std::vector<QString> binNames, std::vector<int> bins);

        signals:
          void performRebinning(QString algorithm, std::vector<QString> binNames, std::vector<int> bins);

        private slots:
          void onAccept();

        private:
          Ui::RebinDialog ui;

          void setBins(std::vector<QString> binNames, std::vector<int> bins);
          void setAlgorithms(QStringList algorithms);

          bool m_validBins;

          QLabel *lblBin1, *lblBin2, *lblBin3;
          QSpinBox *boxBin1, *boxBin2, *boxBin3;
          
      };
    }
  }
}

#endif // MULTISLICEVIEW_H_
