#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTDIFFRACTIONREDUCTION_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTDIFFRACTIONREDUCTION_H_

//----------------------
// Includes
//----------------------
#include "ui_IndirectDiffractionReduction.h"
#include "MantidQtAPI/UserSubWindow.h"

namespace MantidQt
{
namespace CustomInterfaces
{
class IndirectDiffractionReduction : public MantidQt::API::UserSubWindow
{
  Q_OBJECT

public:
  /// The name of the interface as registered into the factory
  static std::string name() { return "Diffraction"; }
  // This interface's categories.
  static QString categoryInfo() { return "Indirect"; }

public:
  /// Default Constructor
  IndirectDiffractionReduction(QWidget *parent = 0);
  ~IndirectDiffractionReduction();

public slots:
  void demonRun();  
  void instrumentSelected(int);
  void reflectionSelected(int);
  void openDirectoryDialog();
  void help();

private:
  /// Initialize the layout
  virtual void initLayout();
  void initLocalPython();
  void loadSettings();
  void saveSettings();

  bool validateDemon();

private:
  /// The form generated using Qt Designer
  Ui::IndirectDiffractionReduction m_uiForm;
  QIntValidator * m_valInt;
  QDoubleValidator * m_valDbl;
  /// The settings group
  QString m_settingsGroup;

};

}
}

#endif //MANTIDQTCUSTOMINTERFACES_INDIRECTDIFFRACTIONREDUCTION_H_
