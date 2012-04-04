#include "MantidQtMantidWidgets/ProcessingAlgoWidget.h"
#include <Qsci/qscilexerpython.h>
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"

using Mantid::API::Algorithm_sptr;
using Mantid::API::AlgorithmManager;

namespace MantidQt
{
namespace MantidWidgets
{

//----------------------
// Public member functions
//----------------------
///Constructor
ProcessingAlgoWidget::ProcessingAlgoWidget(QWidget *parent)
: QWidget(parent)
{
  ui.setupUi(this);

  // Load all available algorithms
  ui.algoSelector->update();

  // Enable syntax highlighting
  ui.editor->setLexer(new QsciLexerPython);

  // Layout tweak
  QList<int> sizes; sizes.push_back(300); sizes.push_back(1000);
  ui.splitter->setSizes(sizes);
  ui.splitter->setStretchFactor(0, 0);
  ui.splitter->setStretchFactor(1, 0);

  //=========== SLOTS =============
  connect(ui.algoSelector, SIGNAL(algorithmSelectionChanged(const QString &, int)),
      this, SLOT(changeAlgorithm()));
}

/// Destructor
ProcessingAlgoWidget::~ProcessingAlgoWidget()
{
}

/** Save the inputs to algorithm history */
void ProcessingAlgoWidget::saveInput()
{
  ui.algoProperties->saveInput();
}



//------------------------------------------------------------------------------
/** Slot called when the algorithm selected changes
 */
void ProcessingAlgoWidget::changeAlgorithm()
{
  QString algName;
  int version;
  ui.algoSelector->getSelectedAlgorithm(algName,version);
  try
  {
    m_alg = AlgorithmManager::Instance().createUnmanaged(algName.toStdString(), version);
    m_alg->initialize();
  }
  catch (std::runtime_error &)
  {
    // Ignore when the m_algorithm is not found
    m_alg = Algorithm_sptr();

    // Signal that the algorithm just changed
    emit changedAlgorithm();
    return;
  }
  QStringList disabled;
  disabled.push_back("OutputWorkspace");
  disabled.push_back("InputWorkspace");
  ui.algoProperties->addEnabledAndDisableLists(QStringList(), disabled);
  // Sets the m_algorithm and also the properties from the InputHistory
  ui.algoProperties->setAlgorithm(m_alg.get());
  ui.algoProperties->hideOrDisableProperties();

  // Signal that the algorithm just changed
  emit changedAlgorithm();
}



}//namespace
}//namespace
