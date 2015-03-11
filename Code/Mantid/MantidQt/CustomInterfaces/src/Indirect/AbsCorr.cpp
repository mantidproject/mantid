#include "MantidQtCustomInterfaces/Indirect/AbsCorr.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

namespace
{
  Mantid::Kernel::Logger g_log("AbsCorr");
}

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  AbsCorr::AbsCorr(QWidget * parent) :
    IDATab(parent), m_dblVal(NULL), m_posDblVal(NULL)
  {
    m_uiForm.setupUi(parent);
  }

  void AbsCorr::setup()
  {
    //TODO
  }

  void AbsCorr::run()
  {
    //TODO

    // Set the result workspace for Python script export
    m_pythonExportWsName = "";
  }

  bool AbsCorr::validate()
  {
    UserInputValidator uiv;

    //TODO

    if(!uiv.isAllInputValid())
    {
      QString error = uiv.generateErrorMessage();
      showMessageBox(error);
    }

    return error.isAllInputValid();
  }

  void AbsCorr::loadSettings(const QSettings & settings)
  {
    //TODO
  }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
