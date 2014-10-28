#include "MantidQtCustomInterfaces/QtReflOptionsDialog.h"
#include "MantidQtCustomInterfaces/QtReflMainView.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /** Constructor */
    QtReflOptionsDialog::QtReflOptionsDialog(ReflMainView* view, boost::shared_ptr<IReflPresenter> presenter) :
      QDialog(dynamic_cast<QtReflMainView*>(view)),
      m_presenter(presenter)
    {
      initLayout();
      loadOptions();
    }

    /** Destructor */
    QtReflOptionsDialog::~QtReflOptionsDialog()
    {
    }

    /** Initialise the ui */
    void QtReflOptionsDialog::initLayout()
    {
      ui.setupUi(this);
      connect(ui.buttonBox->button(QDialogButtonBox::Ok),    SIGNAL(clicked()), this, SLOT(saveOptions()));
      connect(ui.buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(saveOptions()));
    }

    /** This slot saves the currently configured options to the presenter */
    void QtReflOptionsDialog::saveOptions()
    {
      std::map<std::string,std::string> options = m_presenter->options();

      //Set the options map to match the UI
      options["WarnProcessAll"] = ui.checkWarnProcessAll->isChecked() ? "true" : "false";

      //Update the presenter's options
      m_presenter->setOptions(options);
    }

    /** This slot sets the ui to match the presenter's options */
    void QtReflOptionsDialog::loadOptions()
    {
      std::map<std::string,std::string> options = m_presenter->options();

      //Set the values from the options
      ui.checkWarnProcessAll->setChecked(options["WarnProcessAll"] == "true");
    }

  } //CustomInterfaces
} //MantidQt

