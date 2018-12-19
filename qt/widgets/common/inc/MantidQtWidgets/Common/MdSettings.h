// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MDSETTINGS_H_
#define MDSETTINGS_H_

#include "DllOption.h"
#include "MantidQtWidgets/Common/MdConstants.h"
#include <QColor>
#include <QString>
#include <QStringList>

namespace MantidQt {
namespace API {
/**
  *
  This class is for reading and persisting MD properties.

  @date 19/12/2014
  */

class EXPORT_OPT_MANTIDQT_COMMON MdSettings {
public:
  MdSettings();

  ~MdSettings();

  /**
   * Set the UserSetting color map for the vsi.
   *@param colorMap UserSetting colormap for the vsi
   */
  void setUserSettingColorMap(QString colorMap);

  /**
   * Get the UserSetting color map for the vsi.
   * @returns The UserSetting color map for the vsi.
   */
  QString getUserSettingColorMap();

  /**
   * Get the LastSession color map
   */
  QString getLastSessionColorMap();

  /**
   * Set the LastSession color map
   * @param colorMap The colormap for the VSI.
   */
  void setLastSessionColorMap(QString colorMap);

  /**
   * Get the background color for the user setting.
   * @returns The background color.
   */
  QColor getUserSettingBackgroundColor();

  /**
   * Get the default background color.
   * @returns The default background color.
   */
  QColor getDefaultBackgroundColor();

  /**
   * Set the background color for the user setting.
   * @param backgroundColor The background color.
   */
  void setUserSettingBackgroundColor(QColor backgroundColor);

  /**
   * Get the background color for the last session.
   * @returns The background color.
   */
  QColor getLastSessionBackgroundColor();

  /**
   * Set the background color for the user setting.
   * @param backgroundColor The background color.
   */
  void setLastSessionBackgroundColor(QColor backgroundColor);

  /**
   * Set the general MD color map
   * @param colorMapName The name of the general color map.
   * @param colorMapFile The file name of the general color map.
   */
  void setGeneralMdColorMap(QString colorMapName, QString colorMapFile);

  /**
   * Get the general MD color map file
   * @returns The file path to the general md color map .map file.
   */
  QString getGeneralMdColorMapFile();

  /**
   * Get the general MD color map name
   * @returns The name of the general Md color map.
   */
  QString getGeneralMdColorMapName();

  /**
   *  Set the flag if general color map is desired or not.
   * @param flag If a general color map is desired or not.
   */
  void setUsageGeneralMdColorMap(bool flag);

  /**
   * Get the flag if the general color map is desired or not.
   * @returns Is a general color map desired?
   */
  bool getUsageGeneralMdColorMap();

  /**
   * Set the flag which indicates if the last active color map is supposed to
   * be used.
   * @param flag If the last active color map is supposed to be used or not.
   */
  void setUsageLastSession(bool flag);

  /**
   * Get the flag which indicates if the last active color map is supposed to
   * be used.
   * @returns Is the last active color map to be used?
   */
  bool getUsageLastSession();

  /**
   * Get user setting for the initial view.
   * @returns The initial view
   */
  QString getUserSettingInitialView();

  /**
   * Sets if the color scale was log in the last session
   * @param logScale The state of the log scale.
   */
  void setLastSessionLogScale(bool logScale);

  /**
   * Set the user setting for the initial view.
   * @param initialView The selected initial view.
   */
  void setUserSettingIntialView(QString initialView);

  /**
   * Retrieves the state of the last session's log scale.
   * @returns Was a log scale state?
   */
  bool getLastSessionLogScale();

  bool getUserSettingAutoColorAxes();
  void setUserSettingAutoColorAxes(bool autoScale);

private:
  MdConstants m_mdConstants;

  QString m_vsiGroup;
  QString m_generalMdGroup;
  QString m_sliceViewerGroup;

  QString m_lblUserSettingColorMap;
  QString m_lblLastSessionColorMap;
  QString m_lblGeneralMdColorMap;
  QString m_lblGeneralMdColorMapName;
  QString m_lblUseGeneralMdColorMap;
  QString m_lblUseLastSessionColorMap;

  QString m_lblUserSettingBackgroundColor;
  QString m_lblLastSessionBackgroundColor;

  QString m_lblSliceViewerColorMap;

  QString m_lblUserSettingInitialView;
  QString m_lblLastSessionLogScale;
  QString m_lblUserSettingAutoColorAxes;
};
} // namespace API
} // namespace MantidQt

#endif
