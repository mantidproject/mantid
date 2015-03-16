#include "MantidQtCustomInterfaces/Indirect/AbsorptionCorrections.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

namespace
{
  Mantid::Kernel::Logger g_log("AbsorptionCorrections");
}

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  AbsorptionCorrections::AbsorptionCorrections(QWidget * parent) :
    IDATab(parent)
  {
    m_uiForm.setupUi(parent);
  }

  void AbsorptionCorrections::setup()
  {
    //TODO
  }

  void AbsorptionCorrections::run()
  {
    //TODO

    // Set the result workspace for Python script export
    m_pythonExportWsName = "";
  }

  bool AbsorptionCorrections::validate()
  {
    UserInputValidator uiv;

    //TODO

    if(!uiv.isAllInputValid())
    {
      QString error = uiv.generateErrorMessage();
      showMessageBox(error);
    }

    return uiv.isAllInputValid();
  }

  void AbsorptionCorrections::loadSettings(const QSettings & settings)
  {
    UNUSED_ARG(settings);
    //TODO
  }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
