#include "MantidQtAPI/MdSettings.h"
#include "MantidQtAPI/MdConstants.h"
#include "boost/scoped_ptr.hpp"
#include <QSettings>
#include <QString>

using namespace MantidQt::API;

MdSettings::MdSettings() : m_vsiGroup("Mantid/MdPlotting/Vsi"),
                           m_generalMdGroup("Mantid/MdPlotting/General"),
                           m_sliceViewerGroup("Mantid/SliceViewer"),// This is the same as in Slice Viewer !!
                           m_lblUserSettingColorMap("usersettingcolormap"),
                           m_lblGeneralMdColorMap("generalcolormap"),
                           m_lblGeneralMdColorMapName("generalcolormapname"),
                           m_lblUseGeneralMdColorMap("usegeneralcolormap"),
                           m_lblLastSessionColorMap("lastsessioncolormap"),
                           m_lblUseLastSessionColorMap("uselastsessioncolormap"),
                           m_lblUserSettingBackgroundColor("usersettingbackgroundcolor"),
                           m_lblLastSessionBackgroundColor("lastsessionbackgroundcolor"),
                           m_lblSliceViewerColorMap("ColormapFile"), // This is the same as in Slice Viewer !!,
                           m_lblUserSettingInitialView("initialview")
                           m_lblLastSessionLogScale("lastsessionlogscale")
{
  m_mdConstants.initializeSettingsConstants();
};

MdSettings::~MdSettings(){};

QString MdSettings::getUserSettingColorMap()
{
    QSettings settings;

    settings.beginGroup(m_vsiGroup);
    QString userSettingColorMap = settings.value(m_lblUserSettingColorMap, QString("")).toString();
    settings.endGroup();
        
    return userSettingColorMap;
}

void MdSettings::setUserSettingColorMap(QString colorMap)
{
  QSettings settings;

  settings.beginGroup(m_vsiGroup);
  settings.setValue(m_lblUserSettingColorMap, colorMap);
  settings.endGroup();
}

QString MdSettings::getLastSessionColorMap()
{
  QSettings settings;

  settings.beginGroup(m_vsiGroup);
  QString colormap = settings.value(m_lblLastSessionColorMap, QString("")).toString();
  settings.endGroup();

  return colormap;
}

void MdSettings::setLastSessionColorMap(QString colorMap)
{
  QSettings settings;

  settings.beginGroup(m_vsiGroup);
  settings.setValue(m_lblLastSessionColorMap, colorMap);
  settings.endGroup();
}

QColor MdSettings::getUserSettingBackgroundColor()
{
  QSettings settings;

  settings.beginGroup(m_vsiGroup);
  QColor backgroundColor= settings.value(m_lblUserSettingBackgroundColor,
                                         m_mdConstants.getDefaultBackgroundColor()).value<QColor>();
  settings.endGroup();

  return backgroundColor;
}

void MdSettings::setUserSettingBackgroundColor(QColor backgroundColor)
{
  QSettings settings;

  settings.beginGroup(m_vsiGroup);
  settings.setValue(m_lblUserSettingBackgroundColor, backgroundColor);
  settings.endGroup();
}

QColor MdSettings::getLastSessionBackgroundColor()
{
  QSettings settings;

  settings.beginGroup(m_vsiGroup);
  QColor backgroundColor= settings.value(m_lblLastSessionBackgroundColor,
                                         m_mdConstants.getDefaultBackgroundColor()).value<QColor>();
  settings.endGroup();

  return backgroundColor;
}

QColor MdSettings::getDefaultBackgroundColor()
{
  return m_mdConstants.getDefaultBackgroundColor();
}

void MdSettings::setLastSessionBackgroundColor(QColor backgroundColor)
{
  QSettings settings;

  settings.beginGroup(m_vsiGroup);
  settings.setValue(m_lblLastSessionBackgroundColor, backgroundColor);
  settings.endGroup();
}

void MdSettings::setGeneralMdColorMap(QString colorMapName, QString colorMapFile)
{
  QSettings settings;

  settings.beginGroup(m_generalMdGroup);
  settings.setValue(m_lblGeneralMdColorMapName, colorMapName);
  settings.setValue(m_lblGeneralMdColorMap, colorMapFile);
  bool generalMdPlotting = settings.value(m_lblUseGeneralMdColorMap, false).asBool();
  settings.endGroup();
}

QString  MdSettings::getGeneralMdColorMapFile()
{
  QSettings settings;

  settings.beginGroup(m_generalMdGroup);
  QString colorMap = settings.value(m_lblGeneralMdColorMap, QString("")).toString();
  settings.endGroup();

  return colorMap;
}


QString MdSettings::getGeneralMdColorMapName()
{
  QSettings settings;

  settings.beginGroup(m_generalMdGroup);
  QString  colorMap = settings.value(m_lblGeneralMdColorMapName, m_mdConstants.getGeneralMdColorMap()).toString();
  settings.endGroup();

  return colorMap;
}


void MdSettings::setUsageGeneralMdColorMap(bool flag)
{
  QSettings settings;

  settings.beginGroup(m_generalMdGroup);
  settings.setValue(m_lblUseGeneralMdColorMap, flag);
  settings.endGroup();
}

bool MdSettings::getUsageGeneralMdColorMap()
{
  QSettings settings;

  settings.beginGroup(m_generalMdGroup);
  bool flag = settings.value(m_lblUseGeneralMdColorMap, false).asBool();
  settings.endGroup();

  return flag;
}

void MdSettings::setUsageLastSession(bool flag)
{
  QSettings settings;

  settings.beginGroup(m_vsiGroup);
  settings.setValue(m_lblUseLastSessionColorMap, flag);
  settings.endGroup();
}

bool MdSettings::getUsageLastSession()
{
  QSettings settings;

  settings.beginGroup(m_vsiGroup);
  bool flag = settings.value(m_lblUseLastSessionColorMap, false).asBool();
  settings.endGroup();

  return flag;
}

void MdSettings::setLastSessionLogScale(bool logScale)
{
  QSettings settings;

  settings.beginGroup(m_vsiGroup);
  settings.setValue(m_lblLastSessionLogScale, logScale);
  settings.endGroup();
}

QString MdSettings::getUserSettingInitialView()
{
  QSettings settings;

  settings.beginGroup(m_vsiGroup);
  QString initialView = settings.value(m_lblUserSettingInitialView, m_mdConstants.getTechniqueDependence()).asString();
  settings.endGroup();

  return initialView;
}

bool MdSettings::getLastSessionLogScale()
{
  QSettings settings;

  settings.beginGroup(m_vsiGroup);
  bool logScale = settings.value(m_lblLastSessionLogScale, false).asBool();
  settings.endGroup();

  return logScale;
}


void MdSettings::setUserSettingIntialView(QString initialView)
{
  QSettings settings;

  settings.beginGroup(m_vsiGroup);
  settings.setValue(m_lblUserSettingInitialView, initialView);
  settings.endGroup();
}