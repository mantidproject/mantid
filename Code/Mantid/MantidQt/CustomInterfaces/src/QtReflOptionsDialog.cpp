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
    }

    /** Destructor */
    QtReflOptionsDialog::~QtReflOptionsDialog()
    {
    }

    /** Initialise the ui */
    void QtReflOptionsDialog::initLayout()
    {
      ui.setupUi(this);
    }

  } //CustomInterfaces
} //MantidQt

