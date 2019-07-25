// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECT_CONVTYPES_H_
#define MANTIDQT_INDIRECT_CONVTYPES_H_

#include "DllConfig.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FunctionModel.h"
#include "ParameterEstimation.h"

#include <QMap>
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
namespace ConvTypes {

enum class FitType { None, OneLorentzian, TwoLorentzians };
enum class BackgroundType { None, Flat, Linear };

enum class ParamID {
  NONE,
  LOR1_AMPLITUDE,
  LOR1_PEAKCENTRE,
  LOR1_FWHM,
  LOR2_AMPLITUDE,
  LOR2_PEAKCENTRE,
  LOR2_FWHM,
  BG_A0,
  BG_A1
};

QString paramName(ParamID id);

struct TemplateSubType {
  virtual QString name() const = 0;
  virtual QStringList getTypeNames() const = 0;
  virtual int getTypeIndex(const QString &typeName) const = 0;
  virtual int getNTypes() const = 0;
  virtual QList<ParamID> getParameterIDs(int typeIndex) const = 0;
  virtual QStringList getParameterNames(int typeIndex) const = 0;
  virtual QList<std::string> getParameterDescriptions(int typeIndex) const = 0;
};

struct FitSubType : public TemplateSubType {
  QString name() const override { return "Fit Type"; }
  QStringList getTypeNames() const override;
  int getTypeIndex(const QString &typeName) const override;
  int getNTypes() const override;
  QList<ParamID> getParameterIDs(int typeIndex) const override;
  QStringList getParameterNames(int typeIndex) const override;
  QList<std::string> getParameterDescriptions(int typeIndex) const override;
};

struct BackgroundSubType : public TemplateSubType {
  QString name() const override { return "Background"; }
  QStringList getTypeNames() const override;
  int getTypeIndex(const QString &typeName) const override;
  int getNTypes() const override;
  QList<ParamID> getParameterIDs(int typeIndex) const override;
  QStringList getParameterNames(int typeIndex) const override;
  QList<std::string> getParameterDescriptions(int typeIndex) const override;
  std::string getFunctionName(BackgroundType bgType) const;
};

void applyToFitType(
    FitType fitType, std::function<void(ParamID)> paramFun);
void applyToBackground(
    BackgroundType bgType, std::function<void(ParamID)> paramFun);

} // namespace ConvTypes
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_INDIRECT_CONVTYPES_H_ */
