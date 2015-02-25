#include "MantidQtAPI/MdSettings.h"
#include <QSettings>
#include <QString>

using namespace MantidQt::API;

MdSettings::MdSettings() : m_vsiGroup("Mantid/MdPlotting/Vsi"),
                           m_lblLastSessionLogScale("lastsessionlogscale")
{

};

MdSettings::~MdSettings(){};

void MdSettings::setLastSessionLogScale(bool logScale)
{
  QSettings settings;

  settings.beginGroup(m_vsiGroup);
  settings.setValue(m_lblLastSessionLogScale, logScale);
  settings.endGroup();
}

bool MdSettings::getLastSessionLogScale()
{
  QSettings settings;

  settings.beginGroup(m_vsiGroup);
  bool logScale = settings.value(m_lblLastSessionLogScale, false).asBool();
  settings.endGroup();

  return logScale;
}

