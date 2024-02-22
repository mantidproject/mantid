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
using namespace Mantid::API;

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
  FD_HEIGHT,
  FD_DIFFCOEFF,
  FD_CENTRE,
  CE_HEIGHT,
  CE_TAU,
  CE_L,
  CE_CENTRE,
  HR_HEIGHT,
  HR_TAU,
  HR_L,
  HR_CENTRE,
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
  DP_INTENSITY,
  DP_RADIUS,
  DP_DIFFUSION,
  DP_SHIFT,
  DRDC_INTENSITY,
  DRDC_RADIUS,
  DRDC_DECAY,
  DRDC_SHIFT,
  IDRDC_INTENSITY,
  IDRDC_RADIUS,
  IDRDC_DECAY,
  IDRDC_SHIFT,
  EDRDC_HEIGHT,
  EDRDC_CENTRE,
  EDRDC_RADIUS,
  IRD_HEIGHT,
  IRD_RADIUS,
  IRD_TAU,
  IRD_CENTRE,
  EIRD_HEIGHT,
  EIRD_RADIUS,
  IIRD_HEIGHT,
  IIRD_RADIUS,
  IIRD_TAU,
  IIRD_CENTRE,
  FLAT_BG_A0,
  LINEAR_BG_A0,
  LINEAR_BG_A1,
};

namespace ConvTypes {

enum class FitType {
  None,
  TeixeiraWater,
  FickDiffusion,
  ChudleyElliot,
  HallRoss,
  StretchedExpFT,
  DiffSphere,
  ElasticDiffSphere,
  InelasticDiffSphere,
  DiffRotDiscreteCircle,
  ElasticDiffRotDiscreteCircle,
  InelasticDiffRotDiscreteCircle,
  IsoRotDiff,
  ElasticIsoRotDiff,
  InelasticIsoRotDiff,
};

enum class LorentzianType {
  None,
  OneLorentzian,
  TwoLorentzians,
};

extern std::map<FitType, bool> FitTypeQDepends;
extern std::unordered_map<std::string, FitType> FitTypeStringToEnum;

enum class BackgroundType { None, Flat, Linear };

enum class TempCorrectionType { None, Exponential };

static std::map<ParamID, std::string> g_paramName{
    {ParamID::LOR1_AMPLITUDE, "Amplitude"},
    {ParamID::LOR1_PEAKCENTRE, "PeakCentre"},
    {ParamID::LOR1_FWHM, "FWHM"},
    {ParamID::LOR2_AMPLITUDE_1, "Amplitude"},
    {ParamID::LOR2_PEAKCENTRE_1, "PeakCentre"},
    {ParamID::LOR2_FWHM_1, "FWHM"},
    {ParamID::LOR2_AMPLITUDE_2, "Amplitude"},
    {ParamID::LOR2_PEAKCENTRE_2, "PeakCentre"},
    {ParamID::LOR2_FWHM_2, "FWHM"},
    {ParamID::TW_HEIGHT, "Height"},
    {ParamID::TW_DIFFCOEFF, "DiffCoeff"},
    {ParamID::TW_TAU, "Tau"},
    {ParamID::TW_CENTRE, "Centre"},
    {ParamID::FD_HEIGHT, "Height"},
    {ParamID::FD_DIFFCOEFF, "DiffCoeff"},
    {ParamID::FD_CENTRE, "Centre"},
    {ParamID::CE_HEIGHT, "Height"},
    {ParamID::CE_TAU, "Tau"},
    {ParamID::CE_L, "L"},
    {ParamID::CE_CENTRE, "Centre"},
    {ParamID::HR_HEIGHT, "Height"},
    {ParamID::HR_TAU, "Tau"},
    {ParamID::HR_L, "L"},
    {ParamID::HR_CENTRE, "Centre"},
    {ParamID::DELTA_HEIGHT, "Height"},
    {ParamID::DELTA_CENTER, "Centre"},
    {ParamID::TEMPERATURE, "Temperature"},
    {ParamID::SE_HEIGHT, "Height"},
    {ParamID::SE_TAU, "Tau"},
    {ParamID::SE_BETA, "Beta"},
    {ParamID::SE_CENTRE, "Centre"},
    {ParamID::DP_INTENSITY, "f1.Intensity"},
    {ParamID::DP_RADIUS, "f1.Radius"},
    {ParamID::DP_DIFFUSION, "f1.Diffusion"},
    {ParamID::DP_SHIFT, "f1.Shift"},
    {ParamID::EDP_HEIGHT, "Height"},
    {ParamID::EDP_CENTRE, "Centre"},
    {ParamID::EDP_RADIUS, "Radius"},
    {ParamID::IDP_INTENSITY, "Intensity"},
    {ParamID::IDP_RADIUS, "Radius"},
    {ParamID::IDP_DIFFUSION, "Diffusion"},
    {ParamID::IDP_SHIFT, "Shift"},
    {ParamID::DRDC_INTENSITY, "f1.Intensity"},
    {ParamID::DRDC_RADIUS, "f1.Radius"},
    {ParamID::DRDC_DECAY, "f1.Decay"},
    {ParamID::DRDC_SHIFT, "f1.Shift"},
    {ParamID::IDRDC_INTENSITY, "Intensity"},
    {ParamID::IDRDC_RADIUS, "Radius"},
    {ParamID::IDRDC_DECAY, "Decay"},
    {ParamID::IDRDC_SHIFT, "Shift"},
    {ParamID::EDRDC_HEIGHT, "Height"},
    {ParamID::EDRDC_CENTRE, "Centre"},
    {ParamID::EDRDC_RADIUS, "Radius"},
    {ParamID::IRD_HEIGHT, "f1.Height"},
    {ParamID::IRD_RADIUS, "f1.Radius"},
    {ParamID::IRD_TAU, "f1.Tau"},
    {ParamID::IRD_CENTRE, "f1.Centre"},
    {ParamID::EIRD_HEIGHT, "Height"},
    {ParamID::EIRD_RADIUS, "Radius"},
    {ParamID::IIRD_HEIGHT, "Height"},
    {ParamID::IIRD_RADIUS, "Radius"},
    {ParamID::IIRD_TAU, "Tau"},
    {ParamID::IIRD_CENTRE, "Centre"},
    {ParamID::FLAT_BG_A0, "A0"},
    {ParamID::LINEAR_BG_A0, "A0"},
    {ParamID::LINEAR_BG_A1, "A1"},
};

enum SubTypeIndex {
  Lorentzian = 0,
  Fit = 1,
  Background = 2,
};

struct TemplateSubType {
  virtual std::string name() const = 0;
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

    if (it != g_typeMap.cend())
      return static_cast<int>((*it).first);

    return static_cast<int>(FitType::None);
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
    auto fillNames = [&names](ParamID id) { names.emplace_back(ConvTypes::g_paramName.at(id)); };
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
        descriptions << fun->parameterDescription(fun->parameterIndex(ConvTypes::g_paramName.at(id)));
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
GNU_DIAG_ON("undefined-var-template")

struct FitSubType : public TemplateSubTypeImpl<FitType> {
  std::string name() const override { return "Fit Type"; }
};

struct LorentzianSubType : public TemplateSubTypeImpl<LorentzianType> {
  std::string name() const override { return "Lorentzians"; }
};

struct BackgroundSubType : public TemplateSubTypeImpl<BackgroundType> {
  std::string name() const override { return "Background"; }
};

struct DeltaSubType : public TemplateSubTypeImpl<bool> {
  std::string name() const override { return "Delta"; }
};

struct TempSubType : public TemplateSubTypeImpl<TempCorrectionType> {
  std::string name() const override { return "ConvTempCorrection"; }
};

} // namespace ConvTypes

inline ParamID &operator++(ParamID &id) {
  id = ParamID(static_cast<std::underlying_type<ParamID>::type>(id) + 1);
  return id;
}

inline void applyToParamIDRange(ParamID from, ParamID to, const std::function<void(ParamID)> &fun) {
  if (from == ParamID::NONE || to == ParamID::NONE)
    return;
  for (auto i = from; i <= to; ++i)
    fun(i);
}

template <typename TemplateSubType, typename Type>
void applyToFitFunction(Type functionType, const std::function<void(ParamID)> &paramFun) {
  applyToParamIDRange(TemplateSubType::g_typeMap[functionType].blocks.front(),
                      TemplateSubType::g_typeMap[functionType].blocks.back(), paramFun);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
