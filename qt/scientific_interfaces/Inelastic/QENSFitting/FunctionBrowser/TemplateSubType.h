// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/WarningSuppressions.h"

#include "ParamID.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

#include <QList>
#include <QStringList>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

struct TemplateSubType {
  virtual std::string name() const = 0;
  virtual bool isType(const std::type_info &type) const = 0;
  virtual QStringList getTypeNames() const = 0;
  virtual int getTypeIndex(const QString &typeName) const = 0;
  virtual int getNTypes() const = 0;
  virtual QList<ParamID> getParameterIDs(int typeIndex) const = 0;
  virtual std::vector<std::string> getParameterNames(int typeIndex) const = 0;
  virtual QList<std::string> getParameterDescriptions(int typeIndex) const = 0;
  virtual ~TemplateSubType() = default;
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
  virtual bool isType(const std::type_info &type) const override {
    (void)type;
    return false;
  };

  QStringList getTypeNames() const override {
    QStringList out;
    for (auto &&it : g_typeMap) {
      out << it.second.name;
    }
    return out;
  }
  int getTypeIndex(const QString &typeName) const override {
    const auto it = std::find_if(g_typeMap.cbegin(), g_typeMap.cend(),
                                 [&typeName](auto &&it) { return it.second.name == typeName; });

    assert(it != g_typeMap.cend());
    return static_cast<int>((*it).first);
  }
  int getNTypes() const override { return static_cast<int>(g_typeMap.size()); }
  QList<ParamID> getParameterIDs(int typeIndex) const override {
    QList<ParamID> ids;
    auto fillIDs = [&ids](ParamID id) { ids << id; };
    applyToType(static_cast<Type>(typeIndex), fillIDs);
    return ids;
  }
  std::vector<std::string> getParameterNames(int typeIndex) const override {
    std::vector<std::string> names;
    auto fillNames = [&names](ParamID id) { names.emplace_back(g_paramName.at(id)); };
    applyToType(static_cast<Type>(typeIndex), fillNames);
    return names;
  }
  QList<std::string> getParameterDescriptions(int typeIndex) const override {
    auto const type = static_cast<Type>(typeIndex);
    QList<std::string> descriptions;
    auto const &function = g_typeMap[type].function;
    if (!function.empty()) {
      Mantid::API::IFunction_sptr fun = Mantid::API::FunctionFactory::Instance().createFunction(function);
      auto fillDescriptions = [&descriptions, &fun](ParamID id) {
        descriptions << fun->parameterDescription(fun->parameterIndex(g_paramName.at(id)));
      };
      applyToParamIDRange(g_typeMap[type].blocks.front(), g_typeMap[type].blocks.back(), fillDescriptions);
    }
    return descriptions;
  }

  std::string getFunctionName(Type type) const { return g_typeMap[type].function; }

  void applyToType(Type type, std::function<void(ParamID)> paramFun) const {
    applyToParamIDRange(g_typeMap[type].blocks.front(), g_typeMap[type].blocks.back(), paramFun);
  }
  static std::map<Type, TemplateSubTypeDescriptor> g_typeMap;
};

template <typename TemplateSubType, typename Type>
void applyToFitFunction(Type functionType, const std::function<void(ParamID)> &paramFun) {
  applyToParamIDRange(TemplateSubType::g_typeMap[functionType].blocks.front(),
                      TemplateSubType::g_typeMap[functionType].blocks.back(), paramFun);
}
GNU_DIAG_ON("undefined-var-template")

using TemplateSubTypes = std::vector<std::unique_ptr<TemplateSubType>>;

template <typename... Args> std::unique_ptr<TemplateSubTypes> packTemplateSubTypes(Args &&...others) {
  std::unique_ptr<TemplateSubTypes> subTypes = std::make_unique<TemplateSubTypes>();
  (subTypes->emplace_back(std::forward<Args>(others)), ...);
  return subTypes;
}

struct TemplateBrowserCustomizations {
  std::unique_ptr<TemplateSubTypes> templateSubTypes = nullptr;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt