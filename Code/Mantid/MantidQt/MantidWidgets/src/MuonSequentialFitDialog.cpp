#include "MantidQtMantidWidgets/MuonSequentialFitDialog.h"
#include "MantidAPI/AnalysisDataService.h" 

namespace MantidQt
{
namespace MantidWidgets
{
  using namespace Mantid::Kernel;
  using namespace Mantid::API;

  Logger& MuonSequentialFitDialog::g_log(Logger::get("MuonSequentialFitDialog"));

  /** 
   * Constructor
   */
  MuonSequentialFitDialog::MuonSequentialFitDialog(QWidget* parent) :
    QDialog(parent) 
  {
    ui.setupUi(this);

    connect( ui.labelInput, SIGNAL( textChanged(const QString&) ), 
      this, SLOT( validateLabel(const QString&) ) );
  }

  /**
   * Checks if specified name is valid as a name for label. 
   * @param label :: The name to check
   * @return Empty string if valid, otherwise a string describing why is invalid
   */
  std::string MuonSequentialFitDialog::isValidLabel(const std::string& label)
  {
    if ( label.empty() )
      return "Can not be empty";
    else
      return AnalysisDataService::Instance().isValid(label);
  }

  /**
   * Updates visibility/tooltip of label error asterisk.
   * @param label :: New label as specified by user 
   */
  void MuonSequentialFitDialog::validateLabel(const QString& label)
  {
    std::string error = isValidLabel( label.toStdString() );

    ui.labelError->setVisible( ! error.empty() ); 
    ui.labelError->setToolTip( QString::fromStdString(error) );
  }

  /**
   * Destructor
   */
  MuonSequentialFitDialog::~MuonSequentialFitDialog()
  {}

} // namespace MantidWidgets
} // namespace Mantid
