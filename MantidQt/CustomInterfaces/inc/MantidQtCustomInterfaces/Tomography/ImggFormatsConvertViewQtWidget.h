#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMGGFORMATSCONVERTVIEWQTWIDGET_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMGGFORMATSCONVERTVIEWQTWIDGET_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Tomography/IImggFormatsConvertPresenter.h"
#include "MantidQtCustomInterfaces/Tomography/IImggFormatsConvertView.h"

#include "ui_ImggFormatsConvert.h"

#include <QWidget>

// forward declarations for Qt
class QComboBox;
class QImage;

namespace MantidQt {
namespace CustomInterfaces {

/**
Qt view of a widget to convert images and stacks of images between
different formats.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

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
class MANTIDQT_CUSTOMINTERFACES_DLL ImggFormatsConvertViewQtWidget
    : public QWidget,
      public IImggFormatsConvertView {
  Q_OBJECT

public:
  ImggFormatsConvertViewQtWidget(QWidget *parent = 0);

  ~ImggFormatsConvertViewQtWidget() override;

  void userWarning(const std::string &err,
                   const std::string &description) override;

  void userError(const std::string &err,
                 const std::string &description) override;

  void setFormats(const std::vector<std::string> &fmts,
                  const std::vector<bool> &enableLoad,
                  const std::vector<bool> &enableSave) override;

  std::string inputPath() const override;
  std::string inputFormatName() const override;

  std::string outputPath() const override;
  std::string outputFormatName() const override;

  bool compressHint() const override;

  size_t maxSearchDepth() const override;

  void convert(const std::string &inputName, const std::string &inputFormat,
               const std::string &outputName,
               const std::string &outputFormat) const override;

  void writeImg(Mantid::API::MatrixWorkspace_sptr inWks,
                const std::string &outputName,
                const std::string &outFormat) const override;

  Mantid::API::MatrixWorkspace_sptr
  loadImg(const std::string &inputName,
          const std::string &inFormat) const override;

protected:
  void initLayout();

  void saveSettings() const override;
  void readSettings();
  void setup();

private slots:
  void browseImgInputConvertClicked();
  void browseImgOutputConvertClicked();
  void convertClicked();

private:
  void writeImgFile(const QImage &img, const std::string &outputName,
                    const std::string &outFormat) const;

  QImage loadImgFile(const std::string &inputName,
                     const std::string inFormat) const;

  void setFormatsCombo(QComboBox *cbox, const std::vector<std::string> &fmts,
                       const std::vector<bool> &enable);

  std::string
  grabUserBrowseDir(QLineEdit *le,
                    const std::string &userMsg = "Open directory/folder",
                    bool remember = true);
  std::string askImgOrStackPath();

  void closeEvent(QCloseEvent *event) override;

  Ui::ImggFormatsConvert m_ui;

  // path name for persistent settings
  const static std::string m_settingsGroup;

  // presenter as in the model-view-presenter
  std::unique_ptr<IImggFormatsConvertPresenter> m_presenter;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMGGFORMATSCONVERTVIEWQTWIDGET_H_
