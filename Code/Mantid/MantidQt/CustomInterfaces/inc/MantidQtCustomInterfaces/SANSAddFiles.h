#ifndef MANTIDQTCUSTOMINTERFACES_SANSADDFILES_H_
#define MANTIDQTCUSTOMINTERFACES_SANSADDFILES_H_

#include "ui_SANSRunWindow.h"
#include "MantidQtAPI/UserSubWindow.h"
#include <QString>

#include <set>

namespace MantidQt
{
namespace CustomInterfaces
{

class SANSAddFiles : public MantidQt::API::UserSubWindow
{
  Q_OBJECT
public:
  /// Default Constructor
  SANSAddFiles(QWidget *parent, Ui::SANSRunWindow *ParWidgets);
  /// Destructor
  virtual ~SANSAddFiles();

private:
  ///set to point to the object that has the Add Files controls
  Ui::SANSRunWindow *m_SANSForm;
  //set to a pointer to the parent form
  QWidget *parForm;
  //set to true when execution of the python scripts starts and false on completion
  bool m_pythonRunning;
  //this is set to the extensions supported by the Load algorithm
  std::set<std::string> m_exts;
  //this is set to the extensions supported by LoadRaw
  std::set<std::string> m_rawExts;
  //A reference to a logger
  static Mantid::Kernel::Logger & g_log;

  void initLayout();
  void readSettings();
  void saveSettings();
  QListWidgetItem* insertListFront(const QString &text);

private slots:
  ///insert another row into the files to sum table (tbRunsToAdd), in response to a click on the pbNewRow button
  void add2Runs2Add();
  ///run the Python that sums the files together in response to a pbSum button click
  void runPythonAddFiles();
  ///this slot opens the output file path browser
  void summedPathBrowse();
  ///this slot opens a browser to select a new file to add
  void new2AddBrowse();
  ///sets data associated with the cell
  void setCellData(QListWidgetItem *);
  ///clears the table that contains the names of the files to add
  void clearClicked();
  ///clears the contents of the selected row
  void removeSelected();
};

}
}

#endif  //MANTIDQTCUSTOMINTERFACES_SANSADDFILES_H_
