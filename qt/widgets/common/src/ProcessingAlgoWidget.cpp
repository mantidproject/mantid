#include "MantidQtMantidWidgets/ProcessingAlgoWidget.h"
#include <Qsci/qscilexerpython.h>
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include <iosfwd>
#include <fstream>
#include <QFileInfo>

using Mantid::API::Algorithm_sptr;
using Mantid::API::AlgorithmManager;

namespace MantidQt {
namespace MantidWidgets {

//----------------------
// Public member functions
//----------------------
/// Constructor
ProcessingAlgoWidget::ProcessingAlgoWidget(QWidget *parent) : QWidget(parent) {
  ui.setupUi(this);

  // Load all available algorithms
  ui.algoSelector->update();

  // Enable syntax highlighting
  ui.editor->setLexer(new QsciLexerPython);

  // Layout tweak
  QList<int> sizes;
  sizes.push_back(300);
  sizes.push_back(1000);
  ui.splitter->setSizes(sizes);
  ui.splitter->setStretchFactor(0, 0);
  ui.splitter->setStretchFactor(1, 0);

  //=========== SLOTS =============
  connect(ui.algoSelector,
          SIGNAL(algorithmSelectionChanged(const QString &, int)), this,
          SLOT(changeAlgorithm()));

  connect(ui.btnSave, SIGNAL(clicked()), this, SLOT(btnSaveClicked()));
  connect(ui.btnLoad, SIGNAL(clicked()), this, SLOT(btnLoadClicked()));

  loadSettings();
}

//------------------------------------------------------------------------------
/// Destructor
ProcessingAlgoWidget::~ProcessingAlgoWidget() { saveSettings(); }

//------------------------------------------------------------------------------
/** Save the inputs to algorithm history */
void ProcessingAlgoWidget::saveInput() { ui.algoProperties->saveInput(); }

//------------------------------------------------------------------------------------
/** Load QSettings from .ini-type files */
void ProcessingAlgoWidget::loadSettings() {
  QSettings settings;
  settings.beginGroup("Mantid/ProcessingAlgoWidget");
  m_lastFile = settings.value("LastFile", QString()).toString();
  settings.endGroup();
}

//------------------------------------------------------------------------------------
/** Save settings for next time. */
void ProcessingAlgoWidget::saveSettings() {
  QSettings settings;
  settings.beginGroup("Mantid/ProcessingAlgoWidget");
  settings.setValue("LastFile", m_lastFile);
  settings.endGroup();
}

//------------------------------------------------------------------------------
/** Slot called when the save button is clicked */
void ProcessingAlgoWidget::btnSaveClicked() {
  // Save to a .py file
  QString fileselection = QFileDialog::getSaveFileName(
      this, "Save a Python Script", QFileInfo(m_lastFile).absoluteFilePath(),
      "Python scripts (*.py);;All files (*)");
  if (!fileselection.isEmpty()) {
    m_lastFile = fileselection;
    std::ofstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
      file.open(fileselection.toStdString().c_str());
      file << ui.editor->text().toStdString();
      file.close();
    } catch (std::ifstream::failure &e) {
      QMessageBox::critical(
          this, "Exception saving file " + m_lastFile,
          QString(
              "The file could not be saved due to the following exception:\n") +
              QString(e.what()));
    }
  }
}

//------------------------------------------------------------------------------
/** Slot called when the Load button is clicked */
void ProcessingAlgoWidget::btnLoadClicked() {
  // Load a .py file
  QString fileselection = QFileDialog::getOpenFileName(
      this, "Load a Python Script", QFileInfo(m_lastFile).absoluteFilePath(),
      "Python scripts (*.py);;All files (*)");
  if (!fileselection.isEmpty()) {
    m_lastFile = fileselection;
    std::ifstream file(fileselection.toStdString().c_str());
    std::stringstream buffer;
    buffer << file.rdbuf();
    ui.editor->setText(QString::fromStdString(buffer.str()));
    file.close();
  }
}

//------------------------------------------------------------------------------
/** Slot called when the algorithm selected changes
 */
void ProcessingAlgoWidget::changeAlgorithm() {
  auto alg = ui.algoSelector->getSelectedAlgorithm();
  try {
    m_alg = AlgorithmManager::Instance().createUnmanaged(alg.name.toStdString(),
                                                         alg.version);
    m_alg->initialize();
  } catch (std::runtime_error &) {
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
  ui.algoProperties->setAlgorithm(m_alg);
  ui.algoProperties->hideOrDisableProperties();

  // Signal that the algorithm just changed
  emit changedAlgorithm();
}

//------------------------------------------------------------------------------
/** Set the name of the selected algorithm.
 * Updates the GUI.
 *
 * @param algo :: name of the algorithm
 */
void ProcessingAlgoWidget::setSelectedAlgorithm(QString algo) {
  ui.algoSelector->setSelectedAlgorithm(algo);
  this->changeAlgorithm();
}

/// @return the text in the script editor
QString ProcessingAlgoWidget::getScriptText() { return ui.editor->text(); }
/// Set the script editor text
void ProcessingAlgoWidget::setScriptText(QString text) {
  ui.editor->setText(text);
}

} // namespace
} // namespace
