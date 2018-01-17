#include "MantidQtWidgets/SliceViewer/PeaksTableColumnsDialog.h"
#include "MantidQtWidgets/SliceViewer/QPeaksTableModel.h"
#include "ui_PeaksTableColumnsDialog.h"

// REMOVE

namespace MantidQt {
namespace SliceViewer {

PeaksTableColumnsDialog::PeaksTableColumnsDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::PeaksTableColumnsDialog) {
  ui->setupUi(this);
}

PeaksTableColumnsDialog::~PeaksTableColumnsDialog() { delete ui; }

void PeaksTableColumnsDialog::setVisibleColumns(const std::set<QString> &cols) {
  // copy in the original values
  m_origVisible.clear();
  m_origVisible.insert(cols.begin(), cols.end());

  // set the state of all the checkboxes
  bool isChecked;

  isChecked = bool(m_origVisible.find(QPeaksTableModel::RUNNUMBER) !=
                   m_origVisible.end());
  ui->cb_runnumber->setChecked(isChecked);

  isChecked =
      bool(m_origVisible.find(QPeaksTableModel::DETID) != m_origVisible.end());
  ui->cb_detID->setChecked(isChecked);

  isChecked = bool(m_origVisible.find(QPeaksTableModel::WAVELENGTH) !=
                   m_origVisible.end());
  ui->cb_wavelength->setChecked(isChecked);

  isChecked = bool(m_origVisible.find(QPeaksTableModel::ENERGY_TRANSFER) !=
                   m_origVisible.end());
  ui->cb_deltaE->setChecked(isChecked);

  isChecked = bool(m_origVisible.find(QPeaksTableModel::INITIAL_ENERGY) !=
                   m_origVisible.end());
  ui->cb_ei->setChecked(isChecked);

  isChecked = bool(m_origVisible.find(QPeaksTableModel::FINAL_ENERGY) !=
                   m_origVisible.end());
  ui->cb_ef->setChecked(isChecked);

  isChecked =
      bool(m_origVisible.find(QPeaksTableModel::TOF) != m_origVisible.end());
  ui->cb_tof->setChecked(isChecked);

  isChecked = bool(m_origVisible.find(QPeaksTableModel::DSPACING) !=
                   m_origVisible.end());
  ui->cb_dspacing->setChecked(isChecked);

  isChecked = bool(m_origVisible.find(QPeaksTableModel::BINCOUNT) !=
                   m_origVisible.end());
  ui->cb_bincount->setChecked(isChecked);

  isChecked = bool(m_origVisible.find(QPeaksTableModel::BANKNAME) !=
                   m_origVisible.end());
  ui->cb_bankname->setChecked(isChecked);

  isChecked =
      bool(m_origVisible.find(QPeaksTableModel::ROW) != m_origVisible.end());
  ui->cb_row->setChecked(isChecked);

  isChecked =
      bool(m_origVisible.find(QPeaksTableModel::COL) != m_origVisible.end());
  ui->cb_column->setChecked(isChecked);

  isChecked =
      bool(m_origVisible.find(QPeaksTableModel::QLAB) != m_origVisible.end());
  ui->cb_qLab->setChecked(isChecked);

  isChecked = bool(m_origVisible.find(QPeaksTableModel::QSAMPLE) !=
                   m_origVisible.end());
  ui->cb_qSample->setChecked(isChecked);
}

/**
 * Add or remove an item from the "checked" set.
 * @param checked The set to modify
 * @param item The item to add or remove
 * @param addOrRemove true=add, false=remove
 */
void updateChecked(std::set<QString> &checked, const QString &item,
                   const bool addOrRemove) {
  if (addOrRemove) // add it to the set
  {
    checked.insert(item);
  } else // remove it from the set
  {
    auto iter = checked.find(item);
    if (iter != checked.end()) {
      checked.erase(iter);
    }
  }
}

std::set<QString> PeaksTableColumnsDialog::getVisibleColumns() {
  // copy what was originally visible
  std::set<QString> result(m_origVisible.begin(), m_origVisible.end());

  // delete what is no longer visible
  //      - it is done this way so things that aren't in the dialog stay visible
  updateChecked(result, QPeaksTableModel::RUNNUMBER,
                ui->cb_runnumber->isChecked());
  updateChecked(result, QPeaksTableModel::DETID, ui->cb_detID->isChecked());
  updateChecked(result, QPeaksTableModel::WAVELENGTH,
                ui->cb_wavelength->isChecked());
  updateChecked(result, QPeaksTableModel::ENERGY_TRANSFER,
                ui->cb_deltaE->isChecked());
  updateChecked(result, QPeaksTableModel::INITIAL_ENERGY,
                ui->cb_ei->isChecked());
  updateChecked(result, QPeaksTableModel::FINAL_ENERGY, ui->cb_ef->isChecked());
  updateChecked(result, QPeaksTableModel::TOF, ui->cb_tof->isChecked());
  updateChecked(result, QPeaksTableModel::DSPACING,
                ui->cb_dspacing->isChecked());
  updateChecked(result, QPeaksTableModel::BINCOUNT,
                ui->cb_bincount->isChecked());
  updateChecked(result, QPeaksTableModel::BANKNAME,
                ui->cb_bankname->isChecked());
  updateChecked(result, QPeaksTableModel::ROW, ui->cb_row->isChecked());
  updateChecked(result, QPeaksTableModel::COL, ui->cb_column->isChecked());
  updateChecked(result, QPeaksTableModel::QLAB, ui->cb_qLab->isChecked());
  updateChecked(result, QPeaksTableModel::QSAMPLE, ui->cb_qSample->isChecked());

  return result;
}

} // namespace SliceViewer
} // namespace MantidQt
