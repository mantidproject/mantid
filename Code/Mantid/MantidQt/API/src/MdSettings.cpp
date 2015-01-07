#include "MantidQtAPI/MdSettings.h"
#include "MantidQtAPI/MdConstants.h"
#include "boost/scoped_ptr.hpp"
#include <QSettings>

using namespace MantidQt::API;

MdSettings::MdSettings() : mdConstants(new MdConstants()),
                           vsiGroup("Mantid/MdPlotting/Vsi"),
                           generalMdGroup("Mantid/MdPlotting/General"),
                           lblUserSettingColorMap("usersettingcolormap"),
                           lblGeneralMdColorMap("generalcolormap"),
                           lblUseGeneralMdColorMap("usegeneralcolormap"),
                           lblLastSessionColorMap("lastsessioncolormap"),
                           lblUseLastSessionColorMap("uselastsessioncolormap"),
                           lblUserSettingBackgroundColor("usersettingbackgroundcolor"),
                           lblLastSessionBackgroundColor("lastsessionbackgroundcolor")
{
  mdConstants->initializeSettingsConstants();
};

MdSettings::~MdSettings(){};

std::string MdSettings::getUserSettingColorMap()
{
    QSettings settings;

    settings.beginGroup(vsiGroup);
    std::string UserSettingColorMap = settings.value(lblUserSettingColorMap, QString("")).toString().toStdString();
    settings.endGroup();
        
    return UserSettingColorMap;
}

void MdSettings::setUserSettingColorMap(std::string colorMap)
{
  QSettings settings;

  settings.beginGroup(vsiGroup);
  settings.setValue(lblUserSettingColorMap, QString::fromStdString(colorMap));
  settings.endGroup();
}

std::string MdSettings::getLastSessionColorMap()
{
  QSettings settings;

  settings.beginGroup(vsiGroup);
  std::string colormap = settings.value(lblLastSessionColorMap, QString("")).toString().toStdString();
  settings.endGroup();

  return colormap;
}

void MdSettings::setLastSessionColorMap(std::string colorMap)
{
  QSettings settings;

  settings.beginGroup(vsiGroup);
  settings.setValue(lblLastSessionColorMap, QString::fromStdString(colorMap));
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

void MdSettings::setGeneralMdColorMap(std::string colorMap)
{
  QSettings settings;
        
  settings.beginGroup(generalMdGroup);
  settings.setValue(lblGeneralMdColorMap, QString(colorMap.c_str()));
  settings.endGroup();
}

std::string MdSettings::getGeneralMdColorMap()
{
  QSettings settings;

  settings.beginGroup(generalMdGroup);
  std::string colorMap = settings.value(lblGeneralMdColorMap,
                                        mdConstants->getGeneralMdColorMap()).toString().toStdString();
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

QStringList MdSettings::getVsiColorMaps()
{
  return mdConstants->getVsiColorMaps();
}

