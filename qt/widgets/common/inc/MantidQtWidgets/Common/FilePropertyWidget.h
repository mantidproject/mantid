// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
*/
class DLLExport FilePropertyWidget : public TextPropertyWidget {
  Q_OBJECT

public:
  FilePropertyWidget(Mantid::Kernel::Property *prop, QWidget *parent = nullptr, QGridLayout *layout = nullptr,
                     int row = -1);
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
