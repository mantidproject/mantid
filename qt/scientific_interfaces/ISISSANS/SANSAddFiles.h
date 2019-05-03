// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_SANSADDFILES_H_
#define MANTIDQTCUSTOMINTERFACES_SANSADDFILES_H_

#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "SANSConstants.h"
#include "ui_SANSRunWindow.h"
#include <Poco/NObserver.h>
#include <QString>

namespace MantidQt {
namespace CustomInterfaces {

class SANSAddFiles : public MantidQt::API::UserSubWindow {
  Q_OBJECT
public:
  /// Default Constructor
  SANSAddFiles(QWidget *parent, Ui::SANSRunWindow *ParWidgets);
  /// Destructor
  ~SANSAddFiles() override;

private:
  /// set to point to the object that has the Add Files controls
  Ui::SANSRunWindow *m_SANSForm;
  // set to a pointer to the parent form
  QWidget *parForm;
  // set to true when execution of the python scripts starts and false on
  // completion
  bool m_pythonRunning;
  // this is set to the extensions supported by the Load algorithm
  std::vector<std::string> m_exts;
  // this is set to the extensions supported by LoadRaw
  std::vector<std::string> m_rawExts;
  /// the directory to which files will be saved
  QString m_outDir;
  /// The text that goes into the beginning of the output directory message
  static const QString OUT_MSG;

  Poco::NObserver<SANSAddFiles, Mantid::Kernel::ConfigValChangeNotification>
      m_newOutDir;

  /// Cache for custom binning string
  QString m_customBinning;
  /// Text for label for custom binning
  QString m_customBinningText;
  /// Text for tooltip for custom binning
  QString m_customBinningToolTip;
  /// Text for label for save event data
  QString m_saveEventDataText;
  /// Text for tooltip for save event data
  QString m_saveEventDataToolTip;
  /// Set the bin field
  void setHistogramUiLogic(QString label, QString toolTip, QString lineEditText,
                           bool enabled);
  /// Set the histo gram input enabled or disabled
  void setInputEnabled(bool enabled);
  /// Create Python string list
  QString createPythonStringList(QString inputString);
  /// SANS constants
  SANSConstants m_constants;

  void initLayout() override;
  void setToolTips();
  QListWidgetItem *insertListFront(const QString &text);
  void
  changeOutputDir(Mantid::Kernel::ConfigValChangeNotification_ptr pDirInfo);
  void setOutDir(std::string dir);
  bool checkValidityTimeShiftsForAddedEventFiles();

private slots:
  /// insert another row into the files to sum table (tbRunsToAdd), in response
  /// to a click on the pbNewRow button
  void add2Runs2Add();
  /// run the Python that sums the files together in response to a pbSum button
  /// click
  void runPythonAddFiles();
  void outPathSel();
  /// this slot opens a browser to select a new file to add
  void new2AddBrowse();
  /// sets data associated with the cell
  void setCellData(QListWidgetItem * /*unused*/);
  /// clears the table that contains the names of the files to add
  void clearClicked();
  /// clears the contents of the selected row
  void removeSelected();
  /// Enables/disables the "Sum" button based on whether there are files to sum.
  void enableSumming();
  /// reacts to changges of the combo box selection for the histogram options
  /// for event data
  void onCurrentIndexChangedForHistogramChoice(int index);
  /// reacts to changes of the overlay check box
  void onStateChangedForOverlayCheckBox(int /*state*/);
  /// checks if a file corresponds to a histogram worksapce
  bool isEventWorkspace(QString file_name);
  /// checks if the files which are to be added are all based on event
  /// workspaces
  bool existNonEventFiles();
  /// sets the binning options
  void setBinningOptions(bool enable);
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_SANSADDFILES_H_
