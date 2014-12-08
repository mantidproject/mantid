#ifndef MANTID_API_FILEPROPERTYWIDGET_H_
#define MANTID_API_FILEPROPERTYWIDGET_H_

#include "MantidKernel/System.h"
#include "MantidQtAPI/TextPropertyWidget.h"
#include <QtCore/qstringlist.h>
#include <QtCore/qstring.h>
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/FileProperty.h"
#include <qpushbutton.h>


namespace MantidQt
{
namespace API
{

  /** Widget for FileProperty, which has a "Browse" button.
    
    @date 2012-02-17

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport FilePropertyWidget : public TextPropertyWidget
  {
    Q_OBJECT

  public:
    FilePropertyWidget(Mantid::Kernel::Property * prop, QWidget * parent = NULL, QGridLayout * layout = NULL, int row=-1);
    virtual ~FilePropertyWidget();
    
    static QString openFileDialog(Mantid::Kernel::Property * baseProp);
    static QStringList openMultipleFileDialog(Mantid::Kernel::Property * baseProp);

  public slots:
    void browseClicked();

  protected:
    /// "Browse" button
    QPushButton * m_browseButton;

    /// Is a file property
    Mantid::API::FileProperty* m_fileProp;

    /// Is a multiple file property
    Mantid::API::MultipleFileProperty* m_multipleFileProp;

  };


} // namespace API
} // namespace MantidQt

#endif  /* MANTID_API_FILEPROPERTYWIDGET_H_ */
