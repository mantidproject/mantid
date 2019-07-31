// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECT_CONVTYPES_H_
#define MANTIDQT_INDIRECT_CONVTYPES_H_

#include "DllConfig.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"

#include <QMap>
#include <QStringList>
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
  LOR2_AMPLITUDE_1,
  LOR2_PEAKCENTRE_1,
  LOR2_FWHM_1,
  LOR2_AMPLITUDE_2,
  LOR2_PEAKCENTRE_2,
  LOR2_FWHM_2,
  FLAT_BG_A0,
  LINEAR_BG_A0,
  LINEAR_BG_A1
};

QString paramName(ParamID id);

inline ParamID &operator++(ParamID &id) {
  id = ParamID(static_cast<std::underlying_type<ParamID>::type>(id) + 1);
  return id;
}

inline void applyToParamIDRange(ParamID from, ParamID to,
                                std::function<void(ParamID)> fun) {
  if (from == ParamID::NONE || to == ParamID::NONE)
    return;
  for (auto i = from; i <= to; ++i)
    fun(i);
}

struct TemplateSubType {
  virtual QString name() const = 0;
  virtual QStringList getTypeNames() const = 0;
  virtual int getTypeIndex(const QString &typeName) const = 0;
  virtual int getNTypes() const = 0;
  virtual QList<ParamID> getParameterIDs(int typeIndex) const = 0;
  virtual QStringList getParameterNames(int typeIndex) const = 0;
  virtual QList<std::string> getParameterDescriptions(int typeIndex) const = 0;
};

struct TemplateSubTypeDescriptor {
  QString name;
  std::string function;
  std::vector<ParamID> blocks;
};

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

struct FitSubType : public TemplateSubTypeImpl<FitType> {
  QString name() const override { return "Fit Type"; }
};

struct BackgroundSubType : public TemplateSubTypeImpl<BackgroundType> {
  QString name() const override { return "Background"; }
};

void applyToFitType(FitType fitType,
                    const std::function<void(ParamID)> &paramFun);
void applyToBackground(BackgroundType bgType,
                       const std::function<void(ParamID)> &paramFun);

} // namespace ConvTypes
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_INDIRECT_CONVTYPES_H_ */
