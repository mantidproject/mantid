// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FilePropertyWidget.h"
#include "MantidKernel/Property.h"

#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
#include "MantidQtWidgets/Common/FileDialogHandler.h"

using namespace Mantid::Kernel;

namespace MantidQt::API {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
FilePropertyWidget::FilePropertyWidget(Mantid::Kernel::Property *prop, QWidget *parent, QGridLayout *layout, int row)
    : TextPropertyWidget(prop, parent, layout, row) {
  m_fileProp = dynamic_cast<Mantid::API::FileProperty *>(prop);
  m_multipleFileProp = dynamic_cast<Mantid::API::MultipleFileProperty *>(prop);

  // Create a browse button
  m_browseButton = new QPushButton(tr("Browse"), m_parent);
  // Make current value visible
  this->setValue(QString::fromStdString(m_prop->value()));
  // Make sure the connection comes after updating any values
  connect(m_browseButton, SIGNAL(clicked()), this, SLOT(browseClicked()));
  m_widgets.push_back(m_browseButton);

  // Add to the 2nd column
  m_gridLayout->addWidget(m_browseButton, m_row, 2);
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
FilePropertyWidget::~FilePropertyWidget() = default;

//----------------------------------------------------------------------------------------------
/** Slot called when the browse button is clicked */
void FilePropertyWidget::browseClicked() {
  // Open dialog to get the filename
  QString filename;
  if (m_fileProp) {
    filename = openFileDialog(m_prop);
  } else if (m_multipleFileProp) {
    // Current filename text
    filename = m_textbox->text();

    // Adjust the starting directory from the current file
    if (!filename.isEmpty()) {
      QStringList files = filename.split(",");
      if (files.size() > 0) {
        QString firstFile = files[0];
        AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(firstFile).absoluteDir().path());
      }
    }

    // Open multiple files in the dialog
    QStringList files = FilePropertyWidget::openMultipleFileDialog(m_prop);

    // Make into comma-sep string
    filename.clear();
    QStringList list = files;
    QStringList::Iterator it = list.begin();
    while (it != list.end()) {
      if (it != list.begin())
        filename += ",";
      filename += *it;
      it++;
    }
  }

  // TODO: set the value.
  if (!filename.isEmpty()) {
    m_textbox->clear();
    m_textbox->setText(filename);
    userEditedProperty();
  }
}

//----------------------------------------------------------------------------------------------
/** Open the file dialog for a given property
 *
 * @param baseProp :: Property pointer
 * @return full path to the file(s) to load/save
 */
QString FilePropertyWidget::openFileDialog(Mantid::Kernel::Property *baseProp) {
  auto *prop = dynamic_cast<Mantid::API::FileProperty *>(baseProp);
  if (!prop)
    return "";

  /* MG 20/07/09: Static functions such as these that use native Windows and MAC
     dialogs
     in those environments are alot faster. This is unforunately at the expense
     of
     shell-like pattern matching, i.e. [0-9].
  */
  QString filename;
  if (prop->isLoadProperty()) {
    const auto filter = FileDialogHandler::getFilter(baseProp);
    const auto caption = FileDialogHandler::getCaption("Open file", baseProp);
    filename = QFileDialog::getOpenFileName(nullptr, caption, AlgorithmInputHistory::Instance().getPreviousDirectory(),
                                            filter);
  } else if (prop->isSaveProperty()) {
    filename = FileDialogHandler::getSaveFileName(nullptr, prop);
  } else if (prop->isDirectoryProperty()) {
    const auto caption = FileDialogHandler::getCaption("Choose a Directory", baseProp);
    filename =
        QFileDialog::getExistingDirectory(nullptr, caption, AlgorithmInputHistory::Instance().getPreviousDirectory());
  } else {
    throw std::runtime_error("Invalid type of file property! This should not happen.");
  }

  if (!filename.isEmpty()) {
    AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(filename).absoluteDir().path());
  }
  return filename;
}

//-------------------------------------------------------------------------------------------------
/** Open a file selection box to select Multiple files to load.
 *
 * @param baseProp:: pointer to an instance of MultipleFileProperty used to set
 * up the valid extensions for opening multiple file dialog.
 * @return list of full paths to files
 */
QStringList FilePropertyWidget::openMultipleFileDialog(Mantid::Kernel::Property *baseProp) {
  if (!baseProp)
    return QStringList();
  auto *prop = dynamic_cast<Mantid::API::MultipleFileProperty *>(baseProp);
  if (!prop)
    return QStringList();

  const auto filter = FileDialogHandler::getFilter(baseProp);
  QStringList files = QFileDialog::getOpenFileNames(nullptr, "Open Multiple Files",
                                                    AlgorithmInputHistory::Instance().getPreviousDirectory(), filter);

  return files;
}

} // namespace MantidQt::API
