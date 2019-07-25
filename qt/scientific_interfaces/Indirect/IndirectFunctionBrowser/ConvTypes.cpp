#include "ConvTypes.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
namespace ConvTypes {

using namespace Mantid::API;

std::map<ParamID, QString> g_paramName{{ParamID::LOR1_AMPLITUDE, "Amplitude"},
                                       {ParamID::LOR1_PEAKCENTRE, "PeakCentre"},
                                       {ParamID::LOR1_FWHM, "FWHM"},
                                       {ParamID::LOR2_AMPLITUDE, "Amplitude"},
                                       {ParamID::LOR2_PEAKCENTRE, "PeakCentre"},
                                       {ParamID::LOR2_FWHM, "FWHM"},
                                       {ParamID::BG_A0, "A0"},
                                       {ParamID::BG_A1, "A1"}};

struct TemplateSubTypeDescriptor {
  QString name;
  std::string function;
  ParamID firstParamID;
  ParamID lastParamID;
};

std::map<FitType, TemplateSubTypeDescriptor> g_fitTypeMap{
    {FitType::None,
     {"None", "", ParamID::NONE, ParamID::NONE}},
    {FitType::OneLorentzian,
     {"One Lorentzian", "Lorentzian", ParamID::LOR1_AMPLITUDE, ParamID::LOR1_FWHM}},
    {FitType::TwoLorentzians,
     {"Two Lorentzians", "Lorentzian", ParamID::LOR1_AMPLITUDE, ParamID::LOR2_FWHM}},
};

std::map<BackgroundType, TemplateSubTypeDescriptor> g_backgroundTypeMap{
    {BackgroundType::None, {"None", "", ParamID::NONE, ParamID::NONE}},
    {BackgroundType::Flat, {"FlatBackground", "FlatBackground", ParamID::BG_A0, ParamID::BG_A0}},
    {BackgroundType::Linear,
     {"LinearBackground", "LinearBackground", ParamID::BG_A0, ParamID::BG_A1}},
};

ParamID &operator++(ParamID &id) {
  id = ParamID(static_cast<std::underlying_type<ParamID>::type>(id) + 1);
  return id;
}

void applyToParamIDRange(ParamID from, ParamID to,
                         std::function<void(ParamID)> fun) {
  if (from == ParamID::NONE || to == ParamID::NONE)
    return;
  for (auto i = from; i <= to; ++i)
    fun(i);
}

QString paramName(ParamID id) { return g_paramName.at(id); }

void applyToFitType(FitType fitType, std::function<void(ParamID)> paramFun) {
  applyToParamIDRange(g_fitTypeMap[fitType].firstParamID,
                      g_fitTypeMap[fitType].lastParamID, paramFun);
}

void applyToBackground(BackgroundType bgType,
                       std::function<void(ParamID)> paramFun) {
  applyToParamIDRange(g_backgroundTypeMap[bgType].firstParamID,
                      g_backgroundTypeMap[bgType].lastParamID,
                      paramFun);
}

QStringList FitSubType::getTypeNames() const {
  QStringList out;
  for (auto &&it : g_fitTypeMap) {
    out << it.second.name;
  }
  return out;
}

int FitSubType::getTypeIndex(const QString &typeName) const {
  for (auto &&it : g_fitTypeMap) {
    if (it.second.name == typeName)
      return static_cast<int>(it.first);
  }
  return static_cast<int>(FitType::None);
}

int FitSubType::getNTypes() const { return static_cast<int>(FitType::TwoLorentzians) + 1; }

QList<ParamID> FitSubType::getParameterIDs(int typeIndex) const {
  QList<ParamID> ids;
  auto fillIDs = [&ids](ParamID id) { ids << id; };
  applyToFitType(static_cast<FitType>(typeIndex), fillIDs);
  return ids;
}

QStringList FitSubType::getParameterNames(int typeIndex) const {
  QStringList names;
  auto fillNames = [&names](ParamID id) { names << paramName(id); };
  applyToFitType(static_cast<FitType>(typeIndex), fillNames);
  return names;
}

QList<std::string> FitSubType::getParameterDescriptions(int typeIndex) const {
  auto const fitType = static_cast<FitType>(typeIndex);
  QList<std::string> descriptions;
  auto const function = g_fitTypeMap[fitType].function;
  if (!function.empty()) {
    IFunction_sptr fun = FunctionFactory::Instance().createFunction(function);
    auto fillDescriptions = [&descriptions, &fun](ParamID id) {
      descriptions << fun->parameterDescription(
          fun->parameterIndex(paramName(id).toStdString()));
    };
    applyToParamIDRange(g_fitTypeMap[fitType].firstParamID,
                        g_fitTypeMap[fitType].lastParamID, fillDescriptions);
  }
  return descriptions;
}

QStringList BackgroundSubType::getTypeNames() const {
  QStringList out;
  for (auto &&it : g_backgroundTypeMap) {
    out << it.second.name;
  }
  return out;
}

int BackgroundSubType::getTypeIndex(const QString &typeName) const {
  for (auto &&it : g_backgroundTypeMap) {
    if (it.second.name == typeName)
      return static_cast<int>(it.first);
  }
  return static_cast<int>(BackgroundType::None);
}

int BackgroundSubType::getNTypes() const {
  return static_cast<int>(BackgroundType::Linear) + 1;
}

QList<ParamID> BackgroundSubType::getParameterIDs(int typeIndex) const {
  QList<ParamID> ids;
  applyToBackground(static_cast<BackgroundType>(typeIndex),
                    [&ids](ParamID id) { ids << id; });
  return ids;
}

QStringList BackgroundSubType::getParameterNames(int typeIndex) const {
  QStringList names;
  applyToBackground(static_cast<BackgroundType>(typeIndex),
                    [&names](ParamID id) { names << paramName(id); });
  return names;
}

QList<std::string>
BackgroundSubType::getParameterDescriptions(int typeIndex) const {
  auto const bgType = static_cast<BackgroundType>(typeIndex);
  QList<std::string> descriptions;
  auto const function = g_backgroundTypeMap[bgType].function;
  if (!function.empty()) {
    IFunction_sptr fun = FunctionFactory::Instance().createFunction(function);
    auto fillDescriptions = [&descriptions, &fun](ParamID id) {
      descriptions << fun->parameterDescription(
          fun->parameterIndex(paramName(id).toStdString()));
    };
    applyToParamIDRange(g_backgroundTypeMap[bgType].firstParamID,
                        g_backgroundTypeMap[bgType].lastParamID,
                        fillDescriptions);
  }
  return descriptions;
}

std::string BackgroundSubType::getFunctionName(BackgroundType bgType) const {
  return g_backgroundTypeMap[bgType].function;
}

} // namespace ConvTypes
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
