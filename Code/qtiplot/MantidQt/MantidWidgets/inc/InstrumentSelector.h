#ifndef MANTIDQTMANTIDWIDGETS_INSTRUMENTSELECTOR_H_
#define MANTIDQTMANTIDWIDGETS_INSTRUMENTSELECTOR_H_

#include "WidgetDllOption.h"
#include <QComboBox>

namespace MantidQt
{
  namespace MantidWidgets
  {
    /** 
    This class defines a widget for selecting an instrument known to Mantid

    @author Martyn Gigg, Tessella Support Services plc
    @date 10/08/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
    */
    class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS InstrumentSelector : public QComboBox
    {
      Q_OBJECT

    public:
      /// Default Constructor
      InstrumentSelector(QWidget *parent = NULL, bool init = true);

    public slots:
      /// Update list for a new facility
      void fillFromFacility(const QString & name = QString());

    signals:
      /// Indicate that the instrument selection has changed. The parameter will contain the new name
      void instrumentSelectionChanged(const QString &);

    private slots:
      /// Update Mantid's default instrument
      void updateDefaultInstrument(const QString & name) const;
    };

  }
}

#endif //MANTIDQTMANTIDWIDGETS_INSTRUMENTSELECTOR_H_