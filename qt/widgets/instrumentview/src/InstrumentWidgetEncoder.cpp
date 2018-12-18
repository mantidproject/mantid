// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/InstrumentView/InstrumentWidgetEncoder.h"

QMap<QString, QVariant>
InstrumentWidgetEncoder::encode(const InstrumentWidget &obj) {
  QMap<QString, QVariant> map();

  map.insert(QString("workspaceName"), obj.getWorkspaceNameStdString());
  map.insert(QString("surfaceType"), obj.getSurfaceType());
  map.insert(QString("surface"), this->encodeSurface(obj.getSurface()));
  map.insert(QString("currentTab"), obj.getCurrentTab());

  QList energyTransferList();
  energyTransferList.append(obj.m_xIntegration->getMinimum());
  energyTransferList.append(obj.m_xIntegration->getMaximum());
  map.insert(QString("energyTransfer"), energyTransferList);

  map.insert(QString("actor"), this->encodeActor(obj.m_instrumentActor));
  map.insert(QString("tabs"), this->encodeTabs(obj));

  return map;
}

QList<QVariant>
InstrumentWidgetEncoder::encodeTabs(const InstrumentWidget &obj) {
  QList<QVariant> tabs();

  for (const auto &tab : obj.m_tabs) {
    tabs.append(this->encodeTab(tab))
  }

  return tabs;
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeRenderTab(const InstrumentWidgetRenderTab *tab) {

}

// This is the tab labelled draw
QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeMaskTab(const InstrumentWidgetMaskTab *tab) {
  QMap<QString, QVariant> map();
  QMap<QString, QVariant> activeTools();
  QMap<QString, QVariant> activeType();

  activeTools.insert(QString("moveButton"), QVariant(tab.m_move->isChecked()));
  activeTools.insert(QString("pointerButton"),
                     QVariant(tab.m_pointer->isChecked()));
  activeTools.insert(QString("ellipseButton"),
                     QVariant(tab.m_ellipse->isChecked()));
  activeTools.insert(QString("ringEllipseButton"),
                     QVariant(tab.m_ring_ellipse->isChecked()));
  activeTools.insert(QString("ringRectangleButton"),
                     QVariant(tab.m_ring_rectangle->isChecked()));
  activeTools.insert(QString("freeDrawButton"),
                     QVariant(tab.m_free_draw->isChecked()));
  map.insert(QString("activeTools"), QVariant(activeTools));

  activeType.insert(QString("maskingOn"),
                    QVariant(tab.m_masking_on->isChecked()));
  activeType.insert(QString("groupingOn"),
                    QVariant(tab.m_grouping_on->isChecked()));
  activeType.insert(QString("roiOn"), QVariant(tab.m_roi_on));
  map.insert(QString("activeType"), QVariant(activeType));

  // Save the masks applied to view but not saved to a workspace
  auto wsName =
      m_instrWidget->getWorkspaceName().toStdString() + "MaskView.xml";
  bool success = saveMaskViewToProject(wsName);
  map.insert(QString("maskWorkspaceSaved"), QVariant(success));
  if (success){
    map.insert(QString("maskWorkspaceName"), QVariant(wsName));
  }
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodePickTab(const InstrumentWidgetPickTab *tab) {}

QMap<QString, QVariant> InstrumentWidgetEncoder::encodeInstrumentTab(
    const InstrumentWidgetTreeTab *tab) {}

QMap<QString, QVariant> InstrumentWidgetEncoder::encodeActor(
    const std::unique_ptr<InstrumentActor> &obj) {
  QMap<QString, QVariant> map();

  const std::string currentColorMap = getCurrentColorMap().toStdString();

  map.insert(QString("fileName"), QString(currentColorMap));
  map.insert(QString("binMasks"), this->encodeMaskBinsData(m_maskBinsData));

  return map;
}

QList<QVariant>
InstrumentWidgetEncoder::encodeMaskBinsData(const MaskBinsData &obj) {
  QList<QVariant> list();

  for (const auto &binMask : obj.m_masks) {
    list.append(this->encodeBinMask(binMask))
  }

  return list;
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeBinMask(const BinMask &obj) {
  QMap<QString, QVariant> map();

  QList<double> range;
  range.append(obj.start);
  range.append(obj.end);
  map.insert(QString("range"), range);

  QList<size_t> spectra;
  for (const auto &spectrum : obj.spectra) {
    spectra.append(spectrum)
  }
  map.insert(QString("spectra"), spectra);
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeSurface(const ProjectionSurface_sptr &obj) {
  QMap<QString, QVariant> map();

  map.insert(QString("backgroundColor"), obj.m_backgroundColor);
  map.insert(QString("shapes"), this->encodeMaskShapes(obj.m_maskShapes));
  map.insert(QString("alignmentInfo"), this->encodeAlignmentInfo(obj));

  return map;
}

QList<QVariant>
InstrumentWidgetEncoder::encodeMaskShapes(const Shape2DCollection &obj) {
  QList<QVariant> list();

  for (const auto &shape : obj.m_shapes) {
    list.append(this->encodeShape(shape));
  }

  return list;
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeShape(const Shape2D &obj) {
  QMap<QString, QVariant> map();

  map.insert(QString("properties"), this->encodeShapeProperties(obj));
  map.insert(QString("color"), obj.getColor());
  map.insert(QString("fillColor"), obj.getFillColor());

  return map;
}

QMap<QString, QVariant>
InstrumentWidgetEncoder::encodeShapeProperties(const Shape2D &obj) {
  QMap<QString, QVariant> map();

  map.insert(QString("scalable"), QVariant(obj.m_scalable));
  map.insert(QString("editing"), QVariant(obj.m_editing));
  map.insert(QString("selected"), QVariant(obj.m_selected));
  map.insert(QString("visible"), QVariant(obj.m_visible));

  return map;
}

QMap<QString, QVariant> InstrumentWidgetEncoder::encodeAlignmentInfo(
    const ProjectionSurface_sptr &obj) {
  QMap<QString, QVariant> map();

  for (const auto &item : obj.m_selectedAlignmentPlane) {
    const auto qLab = item.first;
    QList qLabList();
    qLabList.append(qLab.x());
    qLabList.append(qLab.Y());
    qLabList.append(qLab.Z());
    map.insert(QString("qLab"), qLabList);
    map.insert(QString("marker"), item.second);
  }

  return map;
}