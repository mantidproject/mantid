#include "MantidSampleMaterialDialog.h"

#include "MantidUI.h"

#include "MantidAPI/ExperimentInfo.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/ArrayProperty.h"

#include <QTreeWidgetItem>
#include <QTreeWidget>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

MantidSampleMaterialDialog::MantidSampleMaterialDialog(MantidUI* mtdUI, Qt::WFlags flags):
  QDialog(mtdUI->appWindow(), flags),
  m_mantidUI(mtdUI)
{
  m_uiForm.setupUi(this);

  QStringList titles;
  titles << "Property" << "Value";
  m_uiForm.treeMaterialProperties->setHeaderLabels(titles);

  connect(m_uiForm.pbClose, SIGNAL(clicked()), this, SLOT(close()));
}

void MantidSampleMaterialDialog::showWorkspace(const QString wsName)
{
  auto materialInfo = getMaterial(wsName);
  showPropsOnTree(materialInfo);
}

QMap<QString, QString> MantidSampleMaterialDialog::getMaterial(const QString workspaceName)
{
  MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName.toStdString());
  const Material material = ws->sample().getMaterial();

  QMap<QString, QString> materialInfo;

  materialInfo["Name"] = QString::fromStdString(material.name());
  materialInfo["Number Density"] = QString::number(material.numberDensity());
  materialInfo["Temperature"] = QString::number(material.temperature());
  materialInfo["Pressure"] = QString::number(material.pressure());
  materialInfo["Coh scatter cross section"] = QString::number(material.cohScatterXSection());
  materialInfo["Incoh scatter cross section"] = QString::number(material.incohScatterXSection());
  materialInfo["Total scatter cross section"] = QString::number(material.totalScatterXSection());
  materialInfo["Absorb cross section"] = QString::number(material.absorbXSection());

  return materialInfo;
}

void MantidSampleMaterialDialog::showPropsOnTree(QMap<QString, QString> materialProps)
{
  m_uiForm.treeMaterialProperties->clear();

  for(auto it = materialProps.begin(); it != materialProps.end(); ++it)
  {
    QTreeWidgetItem *treeItem = new QTreeWidgetItem();
    treeItem->setText(0, it.key());
    treeItem->setText(1, it.value());
    m_uiForm.treeMaterialProperties->addTopLevelItem(treeItem);
  }
}
