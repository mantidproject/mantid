// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "../DllConfig.h"
#include "QString"
#include "QStringList"
#include <qstyleditemdelegate.h>
#include <string>

namespace Regexes {
const QString EMPTY = "^$";
const QString SPACE = "(\\s)*";
const QString COMMA = SPACE + "," + SPACE;
const QString NATURAL_NUMBER = "(0|[1-9][0-9]*)";
const QString REAL_NUMBER = "(-?" + NATURAL_NUMBER + "(\\.[0-9]*)?)";
const QString REAL_RANGE = "(" + REAL_NUMBER + COMMA + REAL_NUMBER + ")";
const QString MASK_LIST = "(" + REAL_RANGE + "(" + COMMA + REAL_RANGE + ")*" + ")|" + EMPTY;
} // namespace Regexes
namespace MantidQt {
namespace CustomInterfaces {
namespace InterfaceUtils {

MANTIDQT_INELASTIC_DLL std::string getInterfaceProperty(std::string const &interfaceName,
                                                        std::string const &propertyName, std::string const &attribute);

MANTIDQT_INELASTIC_DLL QStringList getExtensions(std::string const &interfaceName);
MANTIDQT_INELASTIC_DLL QStringList getCalibrationExtensions(std::string const &interfaceName);

MANTIDQT_INELASTIC_DLL QStringList getSampleFBSuffixes(std::string const &interfaceName);
MANTIDQT_INELASTIC_DLL QStringList getSampleWSSuffixes(std::string const &interfaceName);

MANTIDQT_INELASTIC_DLL QStringList getVanadiumFBSuffixes(std::string const &interfaceName);
MANTIDQT_INELASTIC_DLL QStringList getVanadiumWSSuffixes(std::string const &interfaceName);

MANTIDQT_INELASTIC_DLL QStringList getResolutionFBSuffixes(std::string const &interfaceName);
MANTIDQT_INELASTIC_DLL QStringList getResolutionWSSuffixes(std::string const &interfaceName);

MANTIDQT_INELASTIC_DLL QStringList getCalibrationFBSuffixes(std::string const &interfaceName);
MANTIDQT_INELASTIC_DLL QStringList getCalibrationWSSuffixes(std::string const &interfaceName);

MANTIDQT_INELASTIC_DLL QStringList getContainerFBSuffixes(std::string const &interfaceName);
MANTIDQT_INELASTIC_DLL QStringList getContainerWSSuffixes(std::string const &interfaceName);

MANTIDQT_INELASTIC_DLL QStringList getCorrectionsFBSuffixes(std::string const &interfaceName);
MANTIDQT_INELASTIC_DLL QStringList getCorrectionsWSSuffixes(std::string const &interfaceName);

MANTIDQT_INELASTIC_DLL QPair<double, double> convertTupleToQPair(std::tuple<double, double> const &doubleTuple);
MANTIDQT_INELASTIC_DLL std::pair<double, double> convertTupleToPair(std::tuple<double, double> const &doubleTuple);
MANTIDQT_INELASTIC_DLL QString makeQStringNumber(double value, int precision);

class MANTIDQT_INELASTIC_DLL ExcludeRegionDelegate : public QStyledItemDelegate {
public:
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override;
};

class MANTIDQT_INELASTIC_DLL NumericInputDelegate : public QStyledItemDelegate {
public:
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override;
  void setEditorData(QWidget *editor, const QModelIndex &index) const override;
};
} // namespace InterfaceUtils
} // namespace CustomInterfaces
} // namespace MantidQt
