#include "MantidQtCustomInterfaces/Indirect/ILLCalibration.h"

using namespace Mantid::API;
using MantidQt::API::BatchAlgorithmRunner;

namespace MantidQt
{
namespace CustomInterfaces
{
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ILLCalibration::ILLCalibration(IndirectDataReduction * idrUI, QWidget * parent) :
    IndirectDataReductionTab(idrUI, parent)
  {
    m_uiForm.setupUi(parent);

    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ILLCalibration::~ILLCalibration()
  {
  }

  void ILLCalibration::setup()
  {
  }

  void ILLCalibration::run()
  {
    //TODO

    m_batchAlgoRunner->executeBatchAsync();
  }

  void ILLCalibration::algorithmComplete(bool error)
  {
    if(error)
      return;

    //TODO
  }

  bool ILLCalibration::validate()
  {
    MantidQt::CustomInterfaces::UserInputValidator uiv;

    //TODO
    return false;

    /* return uiv.isAllInputValid(); */
  }

} // namespace CustomInterfaces
} // namespace Mantid
