#include "MantidQtCustomInterfaces/Muon/ALCDataLoading.h"

#include "MantidAPI/AlgorithmManager.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  DECLARE_SUBWINDOW(ALCInterface);

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ALCDataLoadingPresenter::ALCDataLoadingPresenter(IALCDataLoadingView* view)
    : m_view(view)
  {}
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ALCDataLoadingPresenter::~ALCDataLoadingPresenter()
  {}

  void ALCDataLoadingPresenter::initialize()
  {
    connectView();
  }

  void ALCDataLoadingPresenter::connectView()
  {
    connect(m_view, SIGNAL(loadData()), SLOT(loadData()));
  }

  void ALCDataLoadingPresenter::loadData()
  {
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("PlotAsymmetryByLogValue");
    alg->setChild(true); // Don't want workspaces in the ADS
    alg->setProperty("FirstRun", m_view->firstRun());
    alg->setProperty("LastRun", m_view->lastRun());
    alg->setProperty("LogValue", m_view->log());
    alg->setPropertyValue("OutputWorkspace", "__NotUsed__");
    alg->execute();

    MatrixWorkspace_const_sptr result = alg->getProperty("OutputWorkspace");
    m_view->displayData(result);
  }

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

  ALCDataLoadingView::ALCDataLoadingView(QWidget* widget)
    : m_dataLoading(this)
  {
    m_dataLoading.initialize();

    m_ui.setupUi(widget);

    connect(m_ui.load, SIGNAL(pressed()), this, SIGNAL(loadData()));
  }

  void ALCInterface::initLayout()
  {
    new ALCDataLoadingView(this);
  }

} // namespace CustomInterfaces
} // namespace Mantid
