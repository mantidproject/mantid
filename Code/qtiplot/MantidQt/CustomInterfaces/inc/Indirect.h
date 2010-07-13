#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECT_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECT_H_

#include "MantidQtCustomInterfaces/ConvertToEnergy.h"
#include "MantidQtAPI/UserSubWindow.h"


//-----------------------------------------------------
// Forward declarations
//-----------------------------------------------------

namespace MantidQt
{
namespace CustomInterfaces
{

class Indirect : public ConvertToEnergy
{
  Q_OBJECT

public:
  /// explicit constructor, not to allow any overloading
  explicit Indirect(QWidget *parent, Ui::ConvertToEnergy & uiForm);

  /// Initialize the layout
  virtual void initLayout();
  /// run Python-based initialisation commands
  virtual void initLocalPython();
  /// open the wiki page for this interface in a web browser
  virtual void helpClicked();
  /// perform whatever operations needed for analysis
  virtual void runClicked();

public slots:
  /// gather necessary information from Instument Definition Files
  virtual void setIDFValues(const QString & prefix);


private:
  QWidget * const m_mantidplot;

};
}
}

#endif // MANTIDQTCUSTOMINTERFACES_INDIRECT_H_
