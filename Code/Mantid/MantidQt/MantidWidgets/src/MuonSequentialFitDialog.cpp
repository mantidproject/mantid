#include "MantidQtMantidWidgets/MuonSequentialFitDialog.h"

namespace MantidQt
{
namespace MantidWidgets
{

  /** 
   * Constructor
   */
  MuonSequentialFitDialog::MuonSequentialFitDialog(QWidget* parent) :
    QDialog(parent)
  {
    ui.setupUi(this);
  }
    
  /**
   * Destructor
   */
  MuonSequentialFitDialog::~MuonSequentialFitDialog()
  {}

} // namespace MantidWidgets
} // namespace Mantid
