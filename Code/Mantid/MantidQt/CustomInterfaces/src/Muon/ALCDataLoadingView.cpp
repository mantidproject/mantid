#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingView.h"

#include <QMessageBox>

namespace MantidQt
{
namespace CustomInterfaces
{
  std::string ALCDataLoadingView::firstRun()
  {
    return m_ui.firstRun->text().toStdString();
  }

  std::string ALCDataLoadingView::lastRun()
  {
    return m_ui.lastRun->text().toStdString();
  }

  std::string ALCDataLoadingView::log()
  {
    return m_ui.log->text().toStdString();
  }

  void ALCDataLoadingView::displayData(MatrixWorkspace_const_sptr data)
  {
    std::ostringstream wsView;

    for ( size_t i = 0; i < data->blocksize(); ++i )
    {
      wsView << data->readY(0)[i] << std::endl;
    }

    m_ui.result->setText(QString::fromStdString(wsView.str()));
  }

  void ALCDataLoadingView::displayError(const std::string& error)
  {
    QMessageBox::critical(m_widget, "Loading error", QString::fromStdString(error));
  }

  ALCDataLoadingView::ALCDataLoadingView(QWidget* widget)
    : m_dataLoading(this), m_widget(widget)
  {
    m_dataLoading.initialize();

    m_ui.setupUi(m_widget);

    connect(m_ui.load, SIGNAL(pressed()), this, SIGNAL(loadData()));
  }

} // namespace CustomInterfaces
} // namespace MantidQt
