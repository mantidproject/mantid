// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/SequentialFitDialog.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidQtWidgets/Common/FitPropertyBrowser.h"
#include "MantidQtWidgets/Common/MantidDesktopServices.h"
#include "MantidQtWidgets/Common/SelectWorkspacesDialog.h"

#include <Poco/ActiveResult.h>

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QUrl>

namespace MantidQt {
using API::MantidDesktopServices;

namespace {
Mantid::Kernel::Logger g_log("SequentialFitDialog");
}

namespace MantidWidgets {

/// Constructor
/// @param fitBrowser
/// @param mantidui Its purpose is to provide the slot showSequentialPlot
SequentialFitDialog::SequentialFitDialog(FitPropertyBrowser *fitBrowser,
                                         QObject *mantidui)
    : QDialog(fitBrowser), m_fitBrowser(fitBrowser) {
  ui.setupUi(this);

  connect(ui.btnAddFile, SIGNAL(clicked()), this, SLOT(addFile()));
  connect(ui.btnAddWorkspace, SIGNAL(clicked()), this, SLOT(addWorkspace()));
  connect(ui.btnDelete, SIGNAL(clicked()), this, SLOT(removeItem()));

  connect(ui.btnFit, SIGNAL(clicked()), this, SLOT(accept()));
  connect(ui.btnCancel, SIGNAL(clicked()), this, SLOT(reject()));
  connect(ui.btnHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
  connect(ui.ckbLogPlot, SIGNAL(toggled(bool)), this,
          SLOT(plotAgainstLog(bool)));
  connect(ui.ckCreateOutput, SIGNAL(toggled(bool)), ui.ckOutputCompMembers,
          SLOT(setEnabled(bool)));
  connect(ui.ckCreateOutput, SIGNAL(toggled(bool)), ui.ckConvolveMembers,
          SLOT(setEnabled(bool)));

  ui.cbLogValue->setEditable(true);
  ui.ckbLogPlot->setChecked(true);
  ui.sbPeriod->setValue(1);

  populateParameters();

  connect(fitBrowser, SIGNAL(functionChanged()), this, SLOT(functionChanged()));

  // When a fit is completed finishHandle is called which emits needShowPlot
  bool connected = connect(
      this,
      SIGNAL(needShowPlot(Ui::SequentialFitDialog *,
                          MantidQt::MantidWidgets::FitPropertyBrowser *)),
      mantidui,
      SLOT(showSequentialPlot(Ui::SequentialFitDialog *,
                              MantidQt::MantidWidgets::FitPropertyBrowser *)));

  // This has been added to warn users of a known bug #26333
  // It should be removed once the bug has been fixed
  if (!connected) {
    g_log.warning("Unable to update plot and parameters in the fit function "
                  "browser but the sequential fit has completed and is in the "
                  "Workspace Toolbox. This will be fixed in r4.2");
  }
  connect(ui.tWorkspaces, SIGNAL(cellChanged(int, int)), this,
          SLOT(spectraChanged(int, int)));
  connect(ui.tWorkspaces, SIGNAL(itemSelectionChanged()), this,
          SLOT(selectionChanged()));

  selectionChanged();
}

void SequentialFitDialog::addWorkspace() {
  SelectWorkspacesDialog *dlg =
      new SelectWorkspacesDialog(this, "MatrixWorkspace");
  if (dlg->exec() == QDialog::Accepted) {
    addWorkspaces(dlg->getSelectedNames());
  }
}

bool SequentialFitDialog::addWorkspaces(const QStringList wsNames) {
  if (wsNames.isEmpty())
    return false;
  int row = ui.tWorkspaces->rowCount();
  ui.tWorkspaces->model()->insertRows(row, wsNames.size());
  int wi = m_fitBrowser->workspaceIndex();
  QAbstractItemModel *model = ui.tWorkspaces->model();
  foreach (QString name, wsNames) {
    model->setData(model->index(row, 0, QModelIndex()), name);

    if (row == 0) {
      ui.ckbLogPlot->setChecked(validateLogs(name));
    }

    // disable the period cell
    model->setData(model->index(row, 1, QModelIndex()), "");
    QTableWidgetItem *item = ui.tWorkspaces->item(row, 1);
    if (item) {
      item->setBackgroundColor(QColor(Qt::lightGray));
      item->setFlags(Qt::NoItemFlags);
    }

    if (ui.ckbLogPlot->isChecked()) {
      // set spectrum number corresponding to the workspace index
      Mantid::API::MatrixWorkspace_sptr ws =
          boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
              Mantid::API::AnalysisDataService::Instance().retrieve(
                  name.toStdString()));
      int spec = -1;
      if (ws) {
        Mantid::API::Axis *y = ws->getAxis(1);
        if (y->isSpectra()) {
          spec = y->spectraNo(wi);
        }
      }
      // model->setData(model->index(row,2,QModelIndex()),spec);
      setSpectrum(row, spec);
      if (row == 0) {
        ui.sbSpectrum->setValue(spec);
      }

      // set workspace index
      // model->setData(model->index(row,3,QModelIndex()),wi);
      setWSIndex(row, wi);
    }
    ++row;
  }
  ui.tWorkspaces->resizeRowsToContents();
  ui.tWorkspaces->resizeColumnsToContents();
  return true;
}

void SequentialFitDialog::addFile() {
  QFileDialog dlg(this);
  dlg.setFileMode(QFileDialog::ExistingFiles);
  const std::vector<std::string> &searchDirs =
      Mantid::Kernel::ConfigService::Instance().getDataSearchDirs();
  QString dir;
  if (searchDirs.empty()) {
    dir = "";
  } else {
    dir = QString::fromStdString(searchDirs.front());
  }
  dlg.setDirectory(dir);
  if (dlg.exec()) {
    QStringList fileNames;
    fileNames = dlg.selectedFiles();
    if (fileNames.isEmpty())
      return;
    fileNames.sort();

    int row = ui.tWorkspaces->rowCount();
    ui.tWorkspaces->model()->insertRows(row, fileNames.size());
    // int wi = m_fitBrowser->workspaceIndex();
    QAbstractItemModel *model = ui.tWorkspaces->model();
    foreach (QString name, fileNames) {
      model->setData(model->index(row, 0, QModelIndex()), name); // file name
      model->setData(model->index(row, 1, QModelIndex()),
                     ui.sbPeriod->value()); // period
      model->setData(model->index(row, 2, QModelIndex()),
                     ui.sbSpectrum->value());                  // spectrum
      model->setData(model->index(row, 3, QModelIndex()), ""); // ws index
      QTableWidgetItem *item = ui.tWorkspaces->item(row, 3);
      if (item) {
        item->setBackgroundColor(QColor(Qt::lightGray));
        item->setFlags(Qt::NoItemFlags);
      }
      ++row;
    }
    ui.tWorkspaces->resizeRowsToContents();
    ui.tWorkspaces->resizeColumnsToContents();
  }
}

void SequentialFitDialog::removeItem() {
  QList<QTableWidgetSelectionRange> ranges = ui.tWorkspaces->selectedRanges();
  while (!ranges.empty()) {
    ui.tWorkspaces->model()->removeRows(ranges[0].topRow(),
                                        ranges[0].rowCount());
    ranges = ui.tWorkspaces->selectedRanges();
  }
}

bool SequentialFitDialog::validateLogs(const QString wsName) {
  Mantid::API::MatrixWorkspace_sptr ws =
      boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          Mantid::API::AnalysisDataService::Instance().retrieve(
              wsName.toStdString()));
  if (ws) {
    const std::vector<Mantid::Kernel::Property *> logs = ws->run().getLogData();
    QStringList logNames;
    for (auto log : logs) {
      Mantid::Kernel::TimeSeriesProperty<double> *p =
          dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(log);
      if (!p)
        continue;
      logNames << QString::fromStdString(log->name());
    }
    int n = ui.cbLogValue->count();
    // if the ws has no logs - do not include it
    if (logNames.empty()) {
      return false;
    }
    // if the log value combo box is empty fill it in with the log names from ws
    if (n == 0) {
      ui.cbLogValue->insertItems(0, logNames);
    } else { // keep only those logs which are included in both ui.cbLogValue
             // and logNames
      QStringList namesToRemove;
      for (int i = 0; i < n; ++i) {
        QString name = ui.cbLogValue->itemText(i);
        if (!logNames.contains(name)) {
          namesToRemove << name;
        }
      }
      foreach (QString name, namesToRemove) {
        int i = ui.cbLogValue->findText(name);
        if (i >= 0) {
          ui.cbLogValue->removeItem(i);
        }
      }
      if (ui.cbLogValue->count() == 0) {
        QMessageBox::warning(m_fitBrowser, "MantidPlot - Warning",
                             "The list of the log names is empty:\n"
                             "The selected workspaces do not have common logs");
        return false;
      }
    }
  }
  return true;
}

bool SequentialFitDialog::isFile(int row) const {
  QTableWidgetItem *item = ui.tWorkspaces->item(row, 3);
  return !item || item->flags().testFlag(Qt::ItemIsEnabled) == false;
}

/**
 * Returns index for the data source in row row to be used in "Input" property
 * of PlotPeakByLogValue.
 * Index includes the prefix "sp", "i", or "v"
 */
QString SequentialFitDialog::getIndex(int row) const {
  QString index;
  QString spectrum = ui.tWorkspaces->model()
                         ->data(ui.tWorkspaces->model()->index(row, 2))
                         .toString();
  QString wsIndex = ui.tWorkspaces->model()
                        ->data(ui.tWorkspaces->model()->index(row, 3))
                        .toString();
  QString range = ui.tWorkspaces->model()
                      ->data(ui.tWorkspaces->model()->index(row, 4))
                      .toString();

  if (isFile(row)) {
    // if it is a file the ndex can be either spectrum or a range of values
    // range takes priority over spectrum
    if (!range.isEmpty()) {
      index = "v" + range;
    } else {
      index = "sp" + spectrum;
    }
  } else {
    Mantid::API::MatrixWorkspace_sptr ws =
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                name(row).toStdString()));
    Mantid::API::Axis *y = ws->getAxis(1);

    // if it is a single workspace index can only be a range since we are doing
    // a multiple fitting
    if (rowCount() == 1 && range.isEmpty()) {
      index = QString("v%1:%2").arg((*y)(0)).arg((*y)(y->length() - 1));
    } else {
      if (!range.isEmpty()) {
        index = "v" + range;
      } else {
        index = "i" + wsIndex;
      }
    }
  }

  return index;
}

void SequentialFitDialog::accept() {
  QStringList inputStr;
  for (int i = 0; i < ui.tWorkspaces->rowCount(); ++i) {
    QString name = ui.tWorkspaces->model()
                       ->data(ui.tWorkspaces->model()->index(i, 0))
                       .toString();
    QString parStr = name + "," + getIndex(i);
    if (isFile(i)) { // add the period
      parStr += QString(",") + ui.tWorkspaces->model()
                                   ->data(ui.tWorkspaces->model()->index(i, 1))
                                   .toString();
    }
    inputStr << parStr;
  }
  std::string funStr;
  if (m_fitBrowser->m_compositeFunction->nFunctions() > 1) {
    funStr = m_fitBrowser->m_compositeFunction->asString();
  } else {
    funStr = (m_fitBrowser->m_compositeFunction->getFunction(0))->asString();
  }

  Mantid::API::IAlgorithm_sptr alg =
      Mantid::API::AlgorithmManager::Instance().create("PlotPeakByLogValue");
  alg->initialize();
  alg->setPropertyValue("Input", inputStr.join(";").toStdString());
  alg->setProperty("WorkspaceIndex", m_fitBrowser->workspaceIndex());
  alg->setProperty("StartX", m_fitBrowser->startX());
  alg->setProperty("EndX", m_fitBrowser->endX());
  alg->setPropertyValue("OutputWorkspace", m_fitBrowser->outputName());
  alg->setPropertyValue("Function", funStr);
  alg->setProperty("CreateOutput", ui.ckCreateOutput->isChecked());
  alg->setProperty("OutputCompositeMembers",
                   ui.ckOutputCompMembers->isChecked());
  alg->setProperty("ConvolveMembers", ui.ckConvolveMembers->isChecked());
  if (ui.ckbLogPlot->isChecked()) {
    std::string logName = ui.cbLogValue->currentText().toStdString();
    alg->setPropertyValue("LogValue", logName);
    observeFinish(alg);
  } else if (rowCount() > 1) {
    alg->setPropertyValue("LogValue", "SourceName");
  } else {
    observeFinish(alg);
  }
  alg->setPropertyValue("Minimizer", m_fitBrowser->minimizer());
  alg->setPropertyValue("CostFunction", m_fitBrowser->costFunction());
  alg->setProperty("MaxIterations", m_fitBrowser->maxIterations());
  if (ui.rbIndividual->isChecked()) {
    alg->setPropertyValue("FitType", "Individual");
  }
  if (m_fitBrowser->isHistogramFit()) {
    alg->setProperty("EvaluationType", "Histogram");
  }

  bool passWSIndexToFunction = ui.ckbPassWS->isChecked();
  alg->setProperty("PassWSIndexToFunction", passWSIndexToFunction);

  alg->executeAsync();
  QDialog::accept();
}

void SequentialFitDialog::populateParameters() {
  QStringList names;
  for (size_t i = 0; i < m_fitBrowser->m_compositeFunction->nParams(); ++i) {
    names << QString::fromStdString(
        m_fitBrowser->m_compositeFunction->parameterName(i));
  }
  ui.cbParameter->clear();
  ui.cbParameter->insertItems(0, names);
}

void SequentialFitDialog::functionChanged() { populateParameters(); }

void SequentialFitDialog::finishHandle(
    const Mantid::API::IAlgorithm * /*alg*/) {
  emit needShowPlot(&ui, m_fitBrowser);
}

void SequentialFitDialog::helpClicked() {
  MantidDesktopServices::openUrl(
      QUrl("http://www.mantidproject.org/PlotPeakByLogValue"));
}

/**
 * Slot. Called in response to QTableWidget's cellChanged signal.
 * If the cell contains spectra or workspace index of a workspace
 * make them consistent.
 * @param row :: Row index of the modified cell
 * @param col :: Column index of the modified cell
 */
void SequentialFitDialog::spectraChanged(int row, int col) {
  if (!ui.ckbLogPlot->isChecked())
    return;
  QTableWidgetItem *item = ui.tWorkspaces->item(row, 3);
  if (!item)
    return;
  if ((col == 2 || col == 3) &&
      item->flags().testFlag(Qt::ItemIsEnabled) == true) { // it's a workspace
    QString name = ui.tWorkspaces->model()
                       ->data(ui.tWorkspaces->model()->index(row, 0))
                       .toString();
    Mantid::API::MatrixWorkspace_sptr ws;
    try {
      ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          Mantid::API::AnalysisDataService::Instance().retrieve(
              name.toStdString()));
    } catch (...) { //
      return;
    }
    if (!ws)
      return;
    int wi = ui.tWorkspaces->model()
                 ->data(ui.tWorkspaces->model()->index(row, 3))
                 .toInt();
    int spec = ui.tWorkspaces->model()
                   ->data(ui.tWorkspaces->model()->index(row, 2))
                   .toInt();
    Mantid::API::Axis *y = ws->getAxis(1);
    if (wi >= 0 && wi < static_cast<int>(ws->getNumberHistograms())) {
      // this prevents infinite loops
      if (!y->isSpectra() || y->spectraNo(wi) == spec)
        return;
    } else {
      // return to the previous state
      col = 2;
    }
    if (col == 3) // changed workspace index
    {
      try {
        spec = y->spectraNo(wi);
        // ui.tWorkspaces->model()->setData(ui.tWorkspaces->model()->index(row,2),spec);
        setSpectrum(row, spec);
      } catch (...) {
        // return to the previous state
        col = 2;
      }
    }
    if (col == 2) // changed spectrum number
    {
      for (int i = 0; i < static_cast<int>(y->length()); ++i) {
        if ((*y)(i) == spec) {
          // ui.tWorkspaces->model()->setData(ui.tWorkspaces->model()->index(row,3),i);
          setWSIndex(row, i);
          return;
        }
      }
      try {
        // ui.tWorkspaces->model()->setData(ui.tWorkspaces->model()->index(row,2),(*y)(0));
        setSpectrum(row, int((*y)(0)));
      } catch (...) {
      }
    }
  }
}

void SequentialFitDialog::setSpectrum(int row, int spec) {
  ui.tWorkspaces->model()->setData(ui.tWorkspaces->model()->index(row, 2),
                                   spec);
}

void SequentialFitDialog::setWSIndex(int row, int wi) {
  ui.tWorkspaces->model()->setData(ui.tWorkspaces->model()->index(row, 3), wi);
}

int SequentialFitDialog::rowCount() const { return ui.tWorkspaces->rowCount(); }

int SequentialFitDialog::defaultSpectrum() const {
  return ui.sbSpectrum->value();
}

QString SequentialFitDialog::name(int row) const {
  return ui.tWorkspaces->model()
      ->data(ui.tWorkspaces->model()->index(row, 0))
      .toString();
}

void SequentialFitDialog::setRange(int row, double from, double to) {
  QString range = QString::number(from) + ":" + QString::number(to);
  ui.tWorkspaces->model()->setData(ui.tWorkspaces->model()->index(row, 3),
                                   range);
}

void SequentialFitDialog::plotAgainstLog(bool yes) {
  // ui.btnAddFile->setEnabled(yes);
  // ui.btnAddWorkspace->setEnabled(yes);
  // ui.btnDelete->setEnabled(yes);
  ui.lblLogValue->setVisible(yes);
  ui.cbLogValue->setVisible(yes);
  ui.lblPeriod->setVisible(yes);
  ui.sbPeriod->setVisible(yes);
  ui.lblSpectrum->setVisible(yes);
  ui.sbSpectrum->setVisible(yes);
  // if (yes)
  //{// plot agains log value
  //  ui.tWorkspaces->showColumn(3);
  //  ui.tWorkspaces->horizontalHeaderItem(2)->setData(Qt::DisplayRole,"Spectrum");
  //  int spec = defaultSpectrum();
  //  for(int row = 0; row < rowCount(); ++row)
  //  {
  //    setSpectrum(row,spec);
  //  }
  //}
  // else
  //{// plot against "spectra" axis values
  //  if (rowCount() == 1)
  //  {
  //    ui.tWorkspaces->hideColumn(3);
  //    ui.tWorkspaces->horizontalHeaderItem(2)->setData(Qt::DisplayRole,"Range");
  //    Mantid::API::MatrixWorkspace_sptr ws =
  //    boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
  //      Mantid::API::AnalysisDataService::Instance().retrieve(name(0).toStdString())
  //      );
  //    Mantid::API::Axis* y = ws->getAxis(1);
  //    setRange(0,(*y)(0),(*y)(y->length()-1));
  //  }
  //}
}

/**
 * Update the dialog's controls apropriately.
 */
void SequentialFitDialog::selectionChanged() {
  ui.btnDelete->setEnabled(ui.tWorkspaces->selectionModel()->hasSelection());
}
} // namespace MantidWidgets
} // namespace MantidQt
