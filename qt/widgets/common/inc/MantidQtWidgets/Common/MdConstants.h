// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MDCONSTANTS_H_
#define MDCONSTANTS_H_

#include "DllOption.h"
#include <QColor>
#include <QString>
#include <QStringList>

namespace MantidQt {
namespace API {
/**
  *
  This class is a collection of constants and keys used for the VSI.

  @date 6/1/2015
  */

class EXPORT_OPT_MANTIDQT_COMMON MdConstants {
public:
  // MD ParaView plugin names
  static const QString MantidParaViewSplatterPlot;
  static const QString MantidParaViewSpecialCoordinates;
  static const QString MDPeaksFilter;
  static const QString MantidParaViewPeaksFilter;
  static const QString PeakDimensions;
  static const QString PeaksWorkspace;
  static const QString Delimiter;
  static const QString WorkspaceName;
  static const QString ProbePoint;
  static const QString Threshold;

  MdConstants();

  ~MdConstants();

  /**
   * Initialize constants which are required to store and persist MD settings.
   */
  void initializeSettingsConstants();

  /**
   * Initialize constants which are required for the view
   */
  void initializeViewConstants();

  QString getGeneralMdColorMap() const;

  QColor getDefaultBackgroundColor() const;

  QStringList getVsiColorMaps() const;

  QString getStandardView() const;

  QString getMultiSliceView() const;

  QString getThreeSliceView() const;

  QString getSplatterPlotView() const;

  QString getTechniqueDependence() const;

  double getColorScaleStandardMax();

  QStringList getAllInitialViews() const;

  double getLogScaleDefaultValue();

private:
  QString m_generalMdColorMap;
  QColor m_defaultBackgroundColor;
  QStringList m_vsiColorMaps;
  QString m_standardView;
  QString m_multiSliceView;
  QString m_threeSliceView;
  QString m_splatterPlotView;
  QString m_techniqueDependence;

  const double m_colorScaleStandardMax;
  const double m_logScaleDefaultValue;
};
} // namespace API
} // namespace MantidQt

#endif
