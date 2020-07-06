// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/WarningSuppressions.h"

#include <QMap>
#include <QStringList>
#include <boost/optional.hpp>
#include <unordered_map>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
namespace ConvTypes {

using namespace Mantid::API;

enum class FitType {
  None,
  OneLorentzian,
  TwoLorentzians,
  TeixeiraWater,
  StretchedExpFT,
  ElasticDiffSphere,
  ElasticDiffRotDiscreteCircle,
  InelasticDiffSphere,
  InelasticDiffRotDiscreteCircle,
};

extern std::map<FitType, bool> FitTypeQDepends;
extern std::unordered_map<FitType, std::string> FitTypeEnumToString;
extern std::unordered_map<std::string, FitType> FitTypeStringToEnum;

enum class BackgroundType { None, Flat, Linear };

enum class TempCorrectionType { None, Exponential };

enum class ParamID {
  NONE,
  LOR1_AMPLITUDE,
  LOR1_PEAKCENTRE,
  LOR1_FWHM,
  LOR2_AMPLITUDE_1,
  LOR2_PEAKCENTRE_1,
  LOR2_FWHM_1,
  LOR2_AMPLITUDE_2,
  LOR2_PEAKCENTRE_2,
  LOR2_FWHM_2,
  TW_HEIGHT,
  TW_DIFFCOEFF,
  TW_TAU,
  TW_CENTRE,
  DELTA_HEIGHT,
  DELTA_CENTER,
  TEMPERATURE,
  SE_HEIGHT,
  SE_TAU,
  SE_BETA,
  SE_CENTRE,
  EDP_HEIGHT,
  EDP_CENTRE,
  EDP_RADIUS,
  IDP_INTENSITY,
  IDP_RADIUS,
  IDP_DIFFUSION,
  IDP_SHIFT,
  IDRDC_INTENSITY,
  IDRDC_RADIUS,
  IDRDC_DECAY,
  IDRDC_SHIFT,
  EDRDC_HEIGHT,
  EDRDC_CENTRE,
  EDRDC_RADIUS,
  FLAT_BG_A0,
  LINEAR_BG_A0,
  LINEAR_BG_A1,
};

QString paramName(ParamID id);

inline ParamID &operator++(ParamID &id) {
  id = ParamID(static_cast<std::underlying_type<ParamID>::type>(id) + 1);
  return id;
}

inline void applyToParamIDRange(ParamID from, ParamID to,
                                const std::function<void(ParamID)> &fun) {
  if (from == ParamID::NONE || to == ParamID::NONE)
    return;
  for (auto i = from; i <= to; ++i)
    fun(i);
}

enum SubTypeIndex {
  Fit = 0,
  Background = 1,
};

struct TemplateSubType {
  virtual QString name() const = 0;
  virtual QStringList getTypeNames() const = 0;
  virtual int getTypeIndex(const QString &typeName) const = 0;
  virtual int getNTypes() const = 0;
  virtual QList<ParamID> getParameterIDs(int typeIndex) const = 0;
  virtual QStringList getParameterNames(int typeIndex) const = 0;
  virtual QList<std::string> getParameterDescriptions(int typeIndex) const = 0;
  virtual ~TemplateSubType() {}
};

struct TemplateSubTypeDescriptor {
  QString name;
  std::string function;
  std::vector<ParamID> blocks;
};

// This warning is complaining that the static type g_typeMap does not have
// explicit instantiations in this translation unit. We have explcict
// instantions of all the template specialisations we require in the cpp file
// and if someone adds one later it will lead to a linker error so think we are
// ok to disable this warning.
GNU_DIAG_OFF("undefined-var-template")
template <class Type> struct TemplateSubTypeImpl : public TemplateSubType {
  QStringList getTypeNames() const override {
    QStringList out;
    for (auto &&it : g_typeMap) {
      out << it.second.name;
    }
    return out;
  }
  int getTypeIndex(const QString &typeName) const override {
    for (auto &&it : g_typeMap) {
      if (it.second.name == typeName)
        return static_cast<int>(it.first);
    }
    return static_cast<int>(FitType::None);
  }
  int getNTypes() const override { return static_cast<int>(g_typeMap.size()); }
  QList<ParamID> getParameterIDs(int typeIndex) const override {
    QList<ParamID> ids;
    auto fillIDs = [&ids](ParamID id) { ids << id; };
    applyToType(static_cast<Type>(typeIndex), fillIDs);
    return ids;
  }
  QStringList getParameterNames(int typeIndex) const override {
    QStringList names;
    auto fillNames = [&names](ParamID id) { names << paramName(id); };
    applyToType(static_cast<Type>(typeIndex), fillNames);
    return names;
  }
  QList<std::string> getParameterDescriptions(int typeIndex) const override {
    auto const type = static_cast<Type>(typeIndex);
    QList<std::string> descriptions;
    auto const function = g_typeMap[type].function;
    if (!function.empty()) {
      IFunction_sptr fun = FunctionFactory::Instance().createFunction(function);
      auto fillDescriptions = [&descriptions, &fun](ParamID id) {
        descriptions << fun->parameterDescription(
            fun->parameterIndex(paramName(id).toStdString()));
      };
      applyToParamIDRange(g_typeMap[type].blocks.front(),
                          g_typeMap[type].blocks.back(), fillDescriptions);
    }
    return descriptions;
  }

  std::string getFunctionName(Type type) const {
    return g_typeMap[type].function;
  }

  void applyToType(Type type, std::function<void(ParamID)> paramFun) const {
    applyToParamIDRange(g_typeMap[type].blocks.front(),
                        g_typeMap[type].blocks.back(), paramFun);
  }

  static std::map<Type, TemplateSubTypeDescriptor> g_typeMap;
};
GNU_DIAG_ON("undefined-var-template")

struct FitSubType : public TemplateSubTypeImpl<FitType> {
  QString name() const override { return "Fit Type"; }
};

struct BackgroundSubType : public TemplateSubTypeImpl<BackgroundType> {
  QString name() const override { return "Background"; }
};

struct DeltaSubType : public TemplateSubTypeImpl<bool> {
  QString name() const override { return "Delta"; }
};

struct TempSubType : public TemplateSubTypeImpl<TempCorrectionType> {
  QString name() const override { return "TempCorrection"; }
};

void applyToFitType(FitType fitType,
                    const std::function<void(ParamID)> &paramFun);
void applyToBackground(BackgroundType bgType,
                       const std::function<void(ParamID)> &paramFun);
void applyToDelta(bool deltaType, const std::function<void(ParamID)> &paramFun);

void applyToTemp(TempCorrectionType tempCorrectionType,
                 const std::function<void(ParamID)> &paramFun);

} // namespace ConvTypes
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
