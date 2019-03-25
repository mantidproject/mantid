// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plugins/AlgorithmDialogs/PlotAsymmetryByLogValueDialog.h"
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/Property.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalMapper>
#include <QVBoxLayout>

// Add this class to the list of specialised dialogs in this namespace
namespace MantidQt {
namespace CustomDialogs {
DECLARE_DIALOG(PlotAsymmetryByLogValueDialog)
}
} // namespace MantidQt

using namespace MantidQt::CustomDialogs;
using namespace MantidQt::API;

/**
 * Constructor
 */
PlotAsymmetryByLogValueDialog::PlotAsymmetryByLogValueDialog(QWidget *parent)
    : AlgorithmDialog(parent) {
  browseButtonMapper = new QSignalMapper();

  connect(browseButtonMapper, SIGNAL(mapped(const QString &)), this,
          SLOT(openFileDialog(const QString &)));
}

/**
 *Destructor
 */
PlotAsymmetryByLogValueDialog::~PlotAsymmetryByLogValueDialog() {
  delete browseButtonMapper;
}

/**
 * Reimplemented virtual function to set up the dialog
 */
void PlotAsymmetryByLogValueDialog::initLayout() {
  m_uiForm.setupUi(this);

  // Tie all the properties
  tie(m_uiForm.firstRunBox, "FirstRun", m_uiForm.FirstRunLayout);
  tie(m_uiForm.lastRunBox, "LastRun", m_uiForm.LastRunLayout);
  tie(m_uiForm.logBox, "LogValue");
  tie(m_uiForm.typeBoxLog, "Function");
  tie(m_uiForm.outWSBox, "OutputWorkspace", m_uiForm.OutputWSLayout);
  tie(m_uiForm.typeBox, "Type");
  tie(m_uiForm.redBox, "Red");
  tie(m_uiForm.greenBox, "Green");
  tie(m_uiForm.forwardBox, "ForwardSpectra");
  tie(m_uiForm.backwardBox, "BackwardSpectra");
  tie(m_uiForm.timeMinBox, "TimeMin");
  tie(m_uiForm.timeMaxBox, "TimeMax");
  tie(m_uiForm.dtcType, "DeadTimeCorrType");
  tie(m_uiForm.dtcFile, "DeadTimeCorrFile");

  // Set-up browse button mapping
  browseButtonMapper->setMapping(m_uiForm.browseFirstButton, "FirstRun");
  browseButtonMapper->setMapping(m_uiForm.browseLastButton, "LastRun");
  browseButtonMapper->setMapping(m_uiForm.dtcFileBrowseButton,
                                 "DeadTimeCorrFile");

  // Connect Browse buttons to the mapper
  connect(m_uiForm.browseFirstButton, SIGNAL(clicked()), browseButtonMapper,
          SLOT(map()));
  connect(m_uiForm.browseLastButton, SIGNAL(clicked()), browseButtonMapper,
          SLOT(map()));
  connect(m_uiForm.dtcFileBrowseButton, SIGNAL(clicked()), browseButtonMapper,
          SLOT(map()));

  connect(m_uiForm.firstRunBox, SIGNAL(textChanged(const QString &)), this,
          SLOT(fillLogBox(const QString &)));

  connect(m_uiForm.dtcType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(showHideDeadTimeFileWidget(int)));

  // Fill ComboBoxes with allowed values
  fillAndSetComboBox("Type", m_uiForm.typeBox);
  fillAndSetComboBox("Function", m_uiForm.typeBoxLog);
  fillAndSetComboBox("DeadTimeCorrType", m_uiForm.dtcType);

  // Fill log values from the file
  if (!m_uiForm.firstRunBox->text().isEmpty())
    fillLogBox(m_uiForm.firstRunBox->text());

  // So user can enter a custom value
  m_uiForm.logBox->setEditable(true);

  // Create and add the OK/Cancel/Help. buttons
  m_uiForm.verticalLayout->addLayout(this->createDefaultButtonLayout());
}

/**
 * Opens a file dialog. Updates the QLineEdit provided when the dialog is
 * closed.
 */
void PlotAsymmetryByLogValueDialog::openFileDialog(
    const QString &filePropName) {
  QString selectedPath = AlgorithmDialog::openFileDialog(filePropName);

  if (!selectedPath.isEmpty()) {
    // Save used directory for the next time
    AlgorithmInputHistory::Instance().setPreviousDirectory(
        QFileInfo(selectedPath).absoluteDir().path());

    // Get the widget for the file property
    QLineEdit *lineEdit =
        dynamic_cast<QLineEdit *>(m_tied_properties[filePropName]);

    if (!lineEdit)
      throw std::runtime_error("Widget of the file property was not found");

    lineEdit->setText(selectedPath.trimmed());
  }
}

/**
 * Fill m_uiForm.logBox with names of the log values read from one of the input
 * files
 */
void PlotAsymmetryByLogValueDialog::fillLogBox(const QString &) {
  QString nexusFileName = m_uiForm.firstRunBox->text();
  QFileInfo file(nexusFileName);
  if (!file.exists()) {
    return;
  }

  m_uiForm.logBox->clear();

  Mantid::API::IAlgorithm_sptr alg =
      Mantid::API::AlgorithmFactory::Instance().create("LoadMuonNexus", -1);
  alg->initialize();
  try {
    alg->setPropertyValue("Filename", nexusFileName.toStdString());
    alg->setPropertyValue("OutputWorkspace",
                          "PlotAsymmetryByLogValueDialog_tmp");
    alg->setPropertyValue("SpectrumList",
                          "1"); // Need to load at least one spectrum
    alg->execute();
    if (alg->isExecuted()) {
      std::string wsName = alg->getPropertyValue("OutputWorkspace");
      Mantid::API::Workspace_sptr ws =
          Mantid::API::AnalysisDataService::Instance().retrieve(wsName);
      if (!ws) {
        return;
      }
      Mantid::API::MatrixWorkspace_sptr mws =
          boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);
      Mantid::API::WorkspaceGroup_sptr gws =
          boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(ws);
      if (gws) {
        if (gws->getNumberOfEntries() < 2)
          return;
        mws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                gws->getNames()[1]));
      }
      const std::vector<Mantid::Kernel::Property *> &props =
          mws->run().getLogData();
      if (gws) {
        std::vector<std::string> wsNames = gws->getNames();
        for (auto &wsName : wsNames) {
          Mantid::API::AnalysisDataService::Instance().remove(wsName);
        }
      } else {
        Mantid::API::AnalysisDataService::Instance().remove(
            "PlotAsymmetryByLogValueDialog_tmp");
      }
      for (auto prop : props) {
        m_uiForm.logBox->addItem(QString::fromStdString(prop->name()));
      }
      // Display the appropriate value
      QString displayed("");
      if (!isForScript()) {
        displayed =
            MantidQt::API::AlgorithmInputHistory::Instance().previousInput(
                "PlotAsymmetryByLogValue", "LogValue");
      }
      if (!displayed.isEmpty()) {
        int index = m_uiForm.logBox->findText(displayed);
        if (index >= 0) {
          m_uiForm.logBox->setCurrentIndex(index);
        }
      }
    }

  } catch (std::exception &) {
  }
}

/**
 * Show or hide Dead Time file widget depending on which Dead Time type is
 * selected.
 * @param deadTimeTypeIndex Selected Dead Time Correction type index
 */
void PlotAsymmetryByLogValueDialog::showHideDeadTimeFileWidget(
    int deadTimeTypeIndex) {
  // Show only if "Using specified file" selected
  m_uiForm.dtcFileContainer->setVisible(deadTimeTypeIndex == 2);
}
