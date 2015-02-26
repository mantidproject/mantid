#include "MantidQtCustomInterfaces/Indirect/ILLEnergyTransfer.h"

#include "MantidQtCustomInterfaces/Background.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <QFileInfo>
#include <QInputDialog>

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ILLEnergyTransfer::ILLEnergyTransfer(IndirectDataReduction * idrUI, QWidget * parent) :
      IndirectDataReductionTab(idrUI, parent)
  {
    m_uiForm.setupUi(parent);

    // SIGNAL/SLOT CONNECTIONS
    // Update instrument information when a new instrument config is selected
    connect(this, SIGNAL(newInstrumentConfiguration()), this, SLOT(setInstrumentDefault()));

    // Validate to remove invalid markers
    validateTab();
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ILLEnergyTransfer::~ILLEnergyTransfer()
  {
  }


  void ILLEnergyTransfer::setup()
  {
  }


  bool ILLEnergyTransfer::validate()
  {
    //TODO
    return false;
  }


  void ILLEnergyTransfer::run()
  {
    //TODO
  }


  /**
   * Handles completion of the algorithm.
   *
   * Sets result workspace for Python export and ungroups result WorkspaceGroup.
   *
   * @param error True if the algorithm was stopped due to error, false otherwise
   */
  void ILLEnergyTransfer::algorithmComplete(bool error)
  {
    //TODO
  }


  /**
   * Called when the instrument has changed, used to update default values.
   */
  void ILLEnergyTransfer::setInstrumentDefault()
  {
    //TODO
  }


} // namespace CustomInterfaces
} // namespace Mantid
