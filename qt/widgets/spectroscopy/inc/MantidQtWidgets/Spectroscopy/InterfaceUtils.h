// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "DllConfig.h"
#include <MantidQtWidgets/Common/QtPropertyBrowser/qtpropertybrowser.h>
#include <MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h>
#include <MantidQtWidgets/Plotting/RangeSelector.h>
#include <QString>
#include <QStringList>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace InterfaceUtils {

/// The function to use to check whether input data should be restricted based on its name.
/// This is defined, rather than calling SettingsHelper::restrictInputDataByName() directly, to make it
/// possible to override it in tests in order to mock out the SettingsHelper.
MANTID_SPECTROSCOPY_DLL extern std::function<bool()> restrictInputDataByName;

MANTID_SPECTROSCOPY_DLL std::string getInterfaceProperty(std::string const &interfaceName,
                                                         std::string const &propertyName, std::string const &attribute);

MANTID_SPECTROSCOPY_DLL QStringList getExtensions(std::string const &interfaceName);
MANTID_SPECTROSCOPY_DLL QStringList getCalibrationExtensions(std::string const &interfaceName);

MANTID_SPECTROSCOPY_DLL QStringList getSampleFBSuffixes(std::string const &interfaceName);
MANTID_SPECTROSCOPY_DLL QStringList getSampleWSSuffixes(std::string const &interfaceName);

MANTID_SPECTROSCOPY_DLL QStringList getVanadiumFBSuffixes(std::string const &interfaceName);
MANTID_SPECTROSCOPY_DLL QStringList getVanadiumWSSuffixes(std::string const &interfaceName);

MANTID_SPECTROSCOPY_DLL QStringList getResolutionFBSuffixes(std::string const &interfaceName);
MANTID_SPECTROSCOPY_DLL QStringList getResolutionWSSuffixes(std::string const &interfaceName);

MANTID_SPECTROSCOPY_DLL QStringList getCalibrationFBSuffixes(std::string const &interfaceName);
MANTID_SPECTROSCOPY_DLL QStringList getCalibrationWSSuffixes(std::string const &interfaceName);

MANTID_SPECTROSCOPY_DLL QStringList getContainerFBSuffixes(std::string const &interfaceName);
MANTID_SPECTROSCOPY_DLL QStringList getContainerWSSuffixes(std::string const &interfaceName);

MANTID_SPECTROSCOPY_DLL QStringList getCorrectionsFBSuffixes(std::string const &interfaceName);
MANTID_SPECTROSCOPY_DLL QStringList getCorrectionsWSSuffixes(std::string const &interfaceName);

MANTID_SPECTROSCOPY_DLL std::pair<double, double> convertTupleToPair(std::tuple<double, double> const &doubleTuple);
MANTID_SPECTROSCOPY_DLL QString makeQStringNumber(double value, int precision);

/// Function to set the range limits of the plot
MANTID_SPECTROSCOPY_DLL void setPlotPropertyRange(QtDoublePropertyManager *dblPropertyManager,
                                                  MantidWidgets::RangeSelector *rs, QtProperty *min, QtProperty *max,
                                                  const std::pair<double, double> &bounds);
/// Function to set the range selector on the mini plot
MANTID_SPECTROSCOPY_DLL void setRangeSelector(QtDoublePropertyManager *dblPropertyManager,
                                              MantidWidgets::RangeSelector *rs, QtProperty *lower, QtProperty *upper,
                                              const std::pair<double, double> &range,
                                              const std::optional<std::pair<double, double>> &bounds = std::nullopt);
/// Sets the min of the range selector if it is less than the max
MANTID_SPECTROSCOPY_DLL void setRangeSelectorMin(QtDoublePropertyManager *dblPropertyManager, QtProperty *minProperty,
                                                 QtProperty *maxProperty, MantidWidgets::RangeSelector *rangeSelector,
                                                 double newValue);
/// Sets the max of the range selector if it is more than the min
MANTID_SPECTROSCOPY_DLL void setRangeSelectorMax(QtDoublePropertyManager *dblPropertyManager, QtProperty *minProperty,
                                                 QtProperty *maxProperty, MantidWidgets::RangeSelector *rangeSelector,
                                                 double newValue);

} // namespace InterfaceUtils
} // namespace CustomInterfaces
} // namespace MantidQt
