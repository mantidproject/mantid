//----------------------------------
// Includes
//----------------------------------
#include "MantidSampleMaterialDialog.h"

#include "MantidUI.h"

#include "MantidAPI/ExperimentInfo.h"

#include <QTreeWidgetItem>
#include <QTreeWidget>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

/**
 * Constructs a sample material dialog.
 *
 * @param wsName Name of workspace to show material for
 * @param mtdUI Pointer to the MantidUI object
 * @param flags Window flags
 */
MantidSampleMaterialDialog::MantidSampleMaterialDialog(const QString & wsName, MantidUI* mtdUI, Qt::WFlags flags):
  QDialog(mtdUI->appWindow(), flags),
  m_wsName(wsName),
  m_mantidUI(mtdUI)
{
  m_uiForm.setupUi(this);

  connect(m_uiForm.pbClose, SIGNAL(clicked()), this, SLOT(close()));

  connect(m_uiForm.pbSetMaterial, SIGNAL(clicked()), this, SLOT(handleSetMaterial()));
  connect(m_uiForm.pbCopyMaterial, SIGNAL(clicked()), this, SLOT(handleCopyMaterial()));
}

/**
 * Gets the sample material for a workspace and displays its properties in the tree.
 */
void MantidSampleMaterialDialog::updateMaterial()
{
  MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_wsName.toStdString());
  const Material material = ws->sample().getMaterial();

  m_uiForm.treeMaterialProperties->clear();

  QTreeWidgetItem *item;
  QTreeWidgetItem *subItem;
  QTreeWidgetItem *subSubItem;

  item = new QTreeWidgetItem();
  item->setText(0, "Formula");
  item->setText(1, QString::fromStdString(material.name()));
  m_uiForm.treeMaterialProperties->addTopLevelItem(item);

  item = new QTreeWidgetItem();
  item->setText(0, "Number Density");
  item->setText(1, QString::number(material.numberDensity()));
  m_uiForm.treeMaterialProperties->addTopLevelItem(item);

  item = new QTreeWidgetItem();
  item->setText(0, "Temperature");
  item->setText(1, QString::number(material.temperature()));
  m_uiForm.treeMaterialProperties->addTopLevelItem(item);

  item = new QTreeWidgetItem();
  item->setText(0, "Pressure");
  item->setText(1, QString::number(material.pressure()));
  m_uiForm.treeMaterialProperties->addTopLevelItem(item);

  item = new QTreeWidgetItem();
  item->setText(0, "Cross Sections");
  m_uiForm.treeMaterialProperties->addTopLevelItem(item);

  subItem = new QTreeWidgetItem();
  subItem->setText(0, "Absorption");
  subItem->setText(1, QString::number(material.absorbXSection()));
  item->addChild(subItem);

  subItem = new QTreeWidgetItem();
  subItem->setText(0, "Scattering");
  item->addChild(subItem);

  // Expand the Cross Sections section
  item->setExpanded(true);

  subSubItem = new QTreeWidgetItem();
  subSubItem->setText(0, "Total");
  subSubItem->setText(1, QString::number(material.totalScatterXSection()));
  subItem->addChild(subSubItem);

  subSubItem = new QTreeWidgetItem();
  subSubItem->setText(0, "Coherent");
  subSubItem->setText(1, QString::number(material.cohScatterXSection()));
  subItem->addChild(subSubItem);

  subSubItem = new QTreeWidgetItem();
  subSubItem->setText(0, "Incoherent");
  subSubItem->setText(1, QString::number(material.incohScatterXSection()));
  subItem->addChild(subSubItem);

  // Expand the Scattering section
  subItem->setExpanded(true);
}

/**
 * Creates a SetSampleMaterial dialog to set the sample material of the workspace.
 */
void MantidSampleMaterialDialog::handleSetMaterial()
{
  QHash<QString, QString> presets;
  presets["InputWorkspace"] = m_wsName;

  m_mantidUI->showAlgorithmDialog("SetSampleMaterial", presets, this);
}

/**
 * Creates a CopySample dialog with pre filled input to copy the sample material.
 */
void MantidSampleMaterialDialog::handleCopyMaterial()
{
  QHash<QString, QString> presets;
  presets["InputWorkspace"] = m_wsName;
  presets["CopyName"] = "0";
  presets["CopyMaterial"] = "1";
  presets["CopyEnvironment"] = "0";
  presets["CopyShape"] = "0";
  presets["CopyLattice"] = "0";
  presets["CopyOrientationOnly"] = "0";

  m_mantidUI->showAlgorithmDialog("CopySample", presets, this);
}

/**
 * Reloads the material information when an algorithm started from the dialog finishes.
 *
 * @param alg Completed algorithm (unused)
 */
void MantidSampleMaterialDialog::finishHandle(const IAlgorithm *alg)
{
  UNUSED_ARG(alg);
  updateMaterial();
}
