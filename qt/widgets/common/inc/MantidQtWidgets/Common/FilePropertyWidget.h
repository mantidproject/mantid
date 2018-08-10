#ifndef MANTID_API_FILEPROPERTYWIDGET_H_
#define MANTID_API_FILEPROPERTYWIDGET_H_

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/TextPropertyWidget.h"
#include <QPushButton>
#include <QString>
#include <QStringList>

namespace MantidQt {
namespace API {

/** Widget for FileProperty, which has a "Browse" button.

  @date 2012-02-17

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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
class DLLExport FilePropertyWidget : public TextPropertyWidget {
  Q_OBJECT

public:
  FilePropertyWidget(Mantid::Kernel::Property *prop, QWidget *parent = nullptr,
                     QGridLayout *layout = nullptr, int row = -1);
  ~FilePropertyWidget() override;

  static QString openFileDialog(Mantid::Kernel::Property *baseProp);
  static QStringList openMultipleFileDialog(Mantid::Kernel::Property *baseProp);

public slots:
  void browseClicked();

protected:
  /// "Browse" button
  QPushButton *m_browseButton;

  /// Is a file property
  Mantid::API::FileProperty *m_fileProp;

  /// Is a multiple file property
  Mantid::API::MultipleFileProperty *m_multipleFileProp;
};

} // namespace API
} // namespace MantidQt

#endif /* MANTID_API_FILEPROPERTYWIDGET_H_ */
