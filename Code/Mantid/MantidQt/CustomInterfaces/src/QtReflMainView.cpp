#include "MantidQtCustomInterfaces/QtReflMainView.h"
#include "MantidQtCustomInterfaces/QReflTableModel.h"
#include "MantidQtCustomInterfaces/ReflNullMainViewPresenter.h"
#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"
#include "MantidAPI/ITableWorkspace.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    using namespace Mantid::API;

    DECLARE_SUBWINDOW(QtReflMainView);

    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    QtReflMainView::QtReflMainView(QWidget *parent) : UserSubWindow(parent), m_presenter(new ReflNullMainViewPresenter())
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    QtReflMainView::~QtReflMainView()
    {
    }

    void QtReflMainView::initLayout()
    {
      ui.setupUi(this);
      ui.workspaceSelector->refresh();
      connect(ui.workspaceSelector,SIGNAL(activated(QString)),this,SLOT(setModel(QString)));
    }

    void QtReflMainView::setModel(QString name)
    {
      m_presenter.swap(boost::scoped_ptr<IReflPresenter>(new ReflMainViewPresenter(name.toStdString(),this)));
      m_presenter->notify();
    }

    void QtReflMainView::showTable(ITableWorkspace_sptr model)
    {
      ui.viewTable->setModel(new QReflTableModel(model));
      ui.viewTable->resizeColumnsToContents();
    }
  } // namespace CustomInterfaces
} // namespace Mantid
