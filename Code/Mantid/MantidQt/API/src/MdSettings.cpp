#include "MantidQtAPI/MdSettings.h"
#include "MantidQtAPI/MdConstants.h"
#include "boost/scoped_ptr.hpp"
#include <QSettings>

using namespace MantidQt::API;

MdSettings::MdSettings() : mdConstants(new MdConstants()),
                           vsiGroup("Mantid/MdPlotting/Vsi"),
                           generalMdGroup("Mantid/MdPlotting/General"),
                           sliceViewerGroup("Mantid/SliceViewer"),// This is the same as in Slice Viewer !!
                           lblUserSettingColorMap("usersettingcolormap"),
                           lblGeneralMdColorMap("generalcolormap"),
                           lblGeneralMdColorMapName("generalcolormapname"),
                           lblUseGeneralMdColorMap("usegeneralcolormap"),
                           lblLastSessionColorMap("lastsessioncolormap"),
                           lblUseLastSessionColorMap("uselastsessioncolormap"),
                           lblUserSettingBackgroundColor("usersettingbackgroundcolor"),
                           lblLastSessionBackgroundColor("lastsessionbackgroundcolor"),
                           lblSliceViewerColorMap("ColormapFile"), // This is the same as in Slice Viewer !!
                           lblUserSettingInitialView("initialview")
{
  mdConstants->initializeSettingsConstants();
};

MdSettings::~MdSettings(){};

QString MdSettings::getUserSettingColorMap()
{
    QSettings settings;

    settings.beginGroup(vsiGroup);
    QString userSettingColorMap = settings.value(lblUserSettingColorMap, QString("")).toString();
    settings.endGroup();
        
    return userSettingColorMap;
}

void MdSettings::setUserSettingColorMap(QString colorMap)
{
  QSettings settings;

  settings.beginGroup(vsiGroup);
  settings.setValue(lblUserSettingColorMap, colorMap);
  settings.endGroup();
}

QString MdSettings::getLastSessionColorMap()
{
  QSettings settings;

  settings.beginGroup(vsiGroup);
  QString colormap = settings.value(lblLastSessionColorMap, QString("")).toString();
  settings.endGroup();

  return colormap;
}

void MdSettings::setLastSessionColorMap(QString colorMap)
{
  QSettings settings;

  settings.beginGroup(vsiGroup);
  settings.setValue(lblLastSessionColorMap, colorMap);
  settings.endGroup();
}

QColor MdSettings::getUserSettingBackgroundColor()
{
  QSettings settings;

  settings.beginGroup(vsiGroup);
  QColor backgroundColor= settings.value(lblUserSettingBackgroundColor,
                                         mdConstants->getDefaultBackgroundColor()).value<QColor>();
  settings.endGroup();

  return backgroundColor;
}

void MdSettings::setUserSettingBackgroundColor(QColor backgroundColor)
{
  QSettings settings;

  settings.beginGroup(vsiGroup);
  settings.setValue(lblUserSettingBackgroundColor, backgroundColor);
  settings.endGroup();
}

QColor MdSettings::getLastSessionBackgroundColor()
{
  QSettings settings;

  settings.beginGroup(vsiGroup);
  QColor backgroundColor= settings.value(lblLastSessionBackgroundColor,
                                         mdConstants->getDefaultBackgroundColor()).value<QColor>();
  settings.endGroup();

  return backgroundColor;
}

QColor MdSettings::getDefaultBackgroundColor()
{
  return mdConstants->getDefaultBackgroundColor();
}

void MdSettings::setLastSessionBackgroundColor(QColor backgroundColor)
{
  QSettings settings;

  settings.beginGroup(vsiGroup);
  settings.setValue(lblLastSessionBackgroundColor, backgroundColor);
  settings.endGroup();
}

void MdSettings::setGeneralMdColorMap(QString colorMapName, QString colorMapFile)
{
  QSettings settings;

  settings.beginGroup(generalMdGroup);
  settings.setValue(lblGeneralMdColorMapName, colorMapName);
  settings.setValue(lblGeneralMdColorMap, colorMapFile);
  bool generalMdPlotting = settings.value(lblUseGeneralMdColorMap, false).asBool();
  settings.endGroup();
}

QString  MdSettings::getGeneralMdColorMapFile()
{
  QSettings settings;

  settings.beginGroup(generalMdGroup);
  QString colorMap = settings.value(lblGeneralMdColorMap, QString("")).toString();
  settings.endGroup();

  return colorMap;
}


QString MdSettings::getGeneralMdColorMapName()
{
  QSettings settings;

  settings.beginGroup(generalMdGroup);
  QString  colorMap = settings.value(lblGeneralMdColorMapName, mdConstants->getGeneralMdColorMap()).toString();
  settings.endGroup();

  return colorMap;
}


void MdSettings::setUsageGeneralMdColorMap(bool flag)
{
  QSettings settings;

  settings.beginGroup(generalMdGroup);
  settings.setValue(lblUseGeneralMdColorMap, flag);
  settings.endGroup();
}

bool MdSettings::getUsageGeneralMdColorMap()
{
  QSettings settings;

  settings.beginGroup(generalMdGroup);
  bool flag = settings.value(lblUseGeneralMdColorMap, false).asBool();
  settings.endGroup();

  return flag;
}

void MdSettings::setUsageLastSession(bool flag)
{
  QSettings settings;

  settings.beginGroup(vsiGroup);
  settings.setValue(lblUseLastSessionColorMap, flag);
  settings.endGroup();
}

bool MdSettings::getUsageLastSession()
{
  QSettings settings;

  settings.beginGroup(vsiGroup);
  bool flag = settings.value(lblUseLastSessionColorMap, false).asBool();
  settings.endGroup();

  return flag;
}

QString MdSettings::getUserSettingInitialView()
{
  QSettings settings;

  settings.beginGroup(vsiGroup);
  QString initialView = settings.value(lblUserSettingInitialView, mdConstants->getTechniqueDependence()).asString();
  settings.endGroup();

  return initialView;
}

void MdSettings::setUserSettingIntialView(QString initialView)
{
  QSettings settings;

  settings.beginGroup(vsiGroup);
  settings.setValue(lblUserSettingInitialView, initialView);
  settings.endGroup();
}