#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOGSAVU_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOGSAVU_H_

#include "ui_TomoToolConfigSavu.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"
#include <MantidAPI/ITableWorkspace.h>

namespace Mantid {
namespace API {
class TableRow;
}
}

namespace MantidQt {
namespace CustomInterfaces {
class TomoToolConfigDialogSavu : public QMainWindow,
                                 public TomoToolConfigDialogBase {
  Q_OBJECT
public:
  TomoToolConfigDialogSavu(QWidget *parent = 0);

private:
  void setupMethodSelected() override;
  void setupToolSettingsFromPaths() override;
  void setupDialogUi() override;
  void initialiseDialog() override;
  int executeQt() override;

  void initSavuWindow();
  void loadAvailablePlugins();
  void refreshAvailablePluginListUI();
  void refreshCurrentPluginListUI();
  void availablePluginSelected();

  void createPluginTreeEntry(Mantid::API::TableRow &row);
  void createPluginTreeEntries(Mantid::API::ITableWorkspace_sptr table);
  void paramValModified(QTreeWidgetItem *item, int /*column*/);
  void currentPluginSelected();
  std::string paramValStringFromArray(const Json::Value &jsonVal,
                                      const std::string &name);

  std::string pluginParamValString(const Json::Value &jsonVal,
                                   const std::string &name);

  void loadSavuTomoConfig(std::string &filePath,
                          Mantid::API::ITableWorkspace_sptr &currentPlugins);
  void moveUpClicked();
  void moveDownClicked();
  void removeClicked();
  void menuOpenClicked();
  void menuSaveClicked();
  void menuSaveAsClicked();
  QString tableWSRowToString(Mantid::API::ITableWorkspace_sptr table, size_t i);
  void expandedItem(QTreeWidgetItem *item);
  void transferClicked();
  std::string createUniqueNameHidden();

  void userWarning(const std::string &err, const std::string &description);
  void userError(const std::string &err, const std::string &description);

  static size_t g_nameSeqNo;

  Ui::TomoToolConfigSavu m_savuUi;
  Mantid::API::ITableWorkspace_sptr m_availPlugins;
  Mantid::API::ITableWorkspace_sptr m_currPlugins;
  std::string m_currentParamPath;
};

} // CustomInterfaces
} // MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOGSAVU_H_
