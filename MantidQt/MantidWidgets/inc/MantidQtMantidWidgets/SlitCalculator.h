#ifndef MANTID_MANTIDWIDGETS_SLITCALCULATOR_H
#define MANTID_MANTIDWIDGETS_SLITCALCULATOR_H

//----------------------------------
// Includes
//----------------------------------
#include "WidgetDllOption.h"
#include "ui_SlitCalculator.h"

#include <QDialog>

namespace MantidQt
{
  namespace MantidWidgets
  {
    /** SlitCalculator : A calculator for Reflectometry instrument slits

    Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS SlitCalculator : public QDialog
    {
      Q_OBJECT
    public:
        SlitCalculator(QWidget* parent);
        virtual ~SlitCalculator();
    protected:
        Ui::SlitCalculator ui;
    private slots:
        void on_recalculate_triggered();
    };
  }
}

#endif /* MANTID_MANTIDWIDGETS_SLITCALCULATOR_H */
