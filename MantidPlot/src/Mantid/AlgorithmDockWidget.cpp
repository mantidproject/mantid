#include "AlgorithmDockWidget.h"
#include "MantidUI.h"
#include <iomanip>

//-------------------- AlgorithmDockWidget ----------------------//
/** Create a QDockWidget containing:
* The AlgorithmSelectorWidget
* The progress bar and Details button
*/
AlgorithmDockWidget::AlgorithmDockWidget(MantidUI *mui, ApplicationWindow *w)
    : QDockWidget(w), m_progressBar(NULL), m_algID(), m_mantidUI(mui) {
  setWindowTitle(tr("Algorithms"));
  setObjectName(
      "exploreAlgorithms"); // this is needed for QMainWindow::restoreState()
  setMinimumHeight(150);
  setMinimumWidth(200);
  w->addDockWidget(Qt::RightDockWidgetArea, this); //*/

  // Add the AlgorithmSelectorWidget
  m_selector = new MantidQt::MantidWidgets::AlgorithmSelectorWidget(this);
  connect(m_selector, SIGNAL(executeAlgorithm(const QString &, const int)),
          m_mantidUI, SLOT(showAlgorithmDialog(const QString &, const int)));

  m_runningLayout = new QHBoxLayout();
  m_runningLayout->setObjectName("testA");

  m_runningButton = new QPushButton("Details");
  m_runningLayout->addStretch();
  m_runningLayout->addWidget(m_runningButton);
  connect(m_runningButton, SIGNAL(clicked()), m_mantidUI,
          SLOT(showAlgMonitor()));

  QFrame *f = new QFrame(this);
  QVBoxLayout *layout = new QVBoxLayout(f);
  layout->setSpacing(4);
  layout->setMargin(4);
  f->setLayout(layout);
  layout->setMargin(0);
  layout->addWidget(m_selector);
  layout->addLayout(m_runningLayout);

  setWidget(f);
}

/** Update the list of algorithms in the dock */
void AlgorithmDockWidget::update() { m_selector->update(); }

void AlgorithmDockWidget::updateProgress(void *alg, const double p,
                                         const QString &msg,
                                         double estimatedTime,
                                         int progressPrecision) {
  if (m_algID.empty())
    return;
  if (alg == m_algID.first() && p >= 0 && p <= 100 && m_progressBar) {
    m_progressBar->setValue(static_cast<int>(p));
    // Make the progress string
    std::ostringstream mess;
    mess << msg.toStdString();
    mess.precision(progressPrecision);
    mess << " " << std::fixed << p << "%";
    if (estimatedTime > 0.5) {
      mess.precision(0);
      mess << " (~";
      if (estimatedTime < 60)
        mess << static_cast<int>(estimatedTime) << "s";
      else if (estimatedTime < 60 * 60) {
        int min = static_cast<int>(estimatedTime / 60);
        int sec = static_cast<int>(estimatedTime - min * 60);
        mess << min << "m" << std::setfill('0') << std::setw(2) << sec << "s";
      } else {
        int hours = static_cast<int>(estimatedTime / 3600);
        int min = static_cast<int>((estimatedTime - hours * 3600) / 60);
        mess << hours << "h" << std::setfill('0') << std::setw(2) << min << "h";
      }
      mess << ")";
    }
    QString formatStr = QString::fromStdString(mess.str());
    m_progressBar->setFormat(formatStr);
  }
}

void AlgorithmDockWidget::algorithmStarted(void *alg) {
  m_algID.push_front(alg);
  hideProgressBar();
  showProgressBar();
}

void AlgorithmDockWidget::algorithmFinished(void *alg) {
  if (m_algID.empty())
    return;
  m_algID.removeAll(alg);
  hideProgressBar();
}

void AlgorithmDockWidget::showProgressBar() {
  if (m_progressBar == NULL) {
    // insert progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setAlignment(Qt::AlignHCenter);
    m_runningLayout->insertWidget(1, m_progressBar);
    // remove the stretch item
    m_runningLayout->removeItem(m_runningLayout->takeAt(0));
  }
}

void AlgorithmDockWidget::hideProgressBar() {
  if (m_progressBar && m_algID.empty()) {
    m_runningLayout->insertStretch(0);
    m_runningLayout->removeWidget(m_progressBar);
    m_progressBar->close();
    delete m_progressBar;
    m_progressBar = NULL;
  }
}
