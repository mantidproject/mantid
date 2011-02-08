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
  static std::string name() { return "Indirect Diffraction"; }

public:
  /// Default Constructor
  IndirectDiffractionReduction(QWidget *parent = 0);

public slots:
  void demonRun();
  
  void instrumentSelected(int);
  void reflectionSelected(int);

  void correctionSelected(int);
  void groupingSelected(const QString & selected);

  void openDirectoryDialog();
  void help();

private:
  /// Initialize the layout
  virtual void initLayout();
  void initLocalPython();
  void loadSettings();

  bool validateDemon();
  QString grouping();

private:
  /// The form generated using Qt Designer
  Ui::IndirectDiffractionReduction m_uiForm;
  /// Whether to do "real" diffraction stuff (AlignDetectors, etc)
  bool m_realDiffraction;

};

}
}

#endif //MANTIDQTCUSTOMINTERFACES_INDIRECTDIFFRACTIONREDUCTION_H_
