#include "ConvTypes.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
namespace ConvTypes {

using namespace Mantid::API;

std::map<ParamID, QString> g_paramName{
    {ParamID::LOR1_AMPLITUDE, "Amplitude"},
    {ParamID::LOR1_PEAKCENTRE, "PeakCentre"},
    {ParamID::LOR1_FWHM, "FWHM"},
    {ParamID::LOR2_AMPLITUDE_1, "Amplitude"},
    {ParamID::LOR2_PEAKCENTRE_1, "PeakCentre"},
    {ParamID::LOR2_FWHM_1, "FWHM"},
    {ParamID::LOR2_AMPLITUDE_2, "Amplitude"},
    {ParamID::LOR2_PEAKCENTRE_2, "PeakCentre"},
    {ParamID::LOR2_FWHM_2, "FWHM"},
    {ParamID::FLAT_BG_A0, "A0"},
    {ParamID::LINEAR_BG_A0, "A0"},
    {ParamID::LINEAR_BG_A1, "A1"}};

template<>
std::map<FitType, TemplateSubTypeDescriptor> TemplateSubTypeImpl<FitType>::g_typeMap{
    {FitType::None, {"None", "", {ParamID::NONE, ParamID::NONE}}},
    {FitType::OneLorentzian,
     {"One Lorentzian",
      "Lorentzian",
      {ParamID::LOR1_AMPLITUDE, ParamID::LOR1_FWHM}}},
    {FitType::TwoLorentzians,
     {"Two Lorentzians",
      "Lorentzian",
      {ParamID::LOR2_AMPLITUDE_1, ParamID::LOR2_FWHM_1, ParamID::LOR2_FWHM_2}}},
};

template<>
std::map<BackgroundType, TemplateSubTypeDescriptor>
    TemplateSubTypeImpl<BackgroundType>::g_typeMap{
        {BackgroundType::None, {"None", "", {ParamID::NONE, ParamID::NONE}}},
        {BackgroundType::Flat,
         {"FlatBackground",
          "FlatBackground",
          {ParamID::FLAT_BG_A0, ParamID::FLAT_BG_A0}}},
        {BackgroundType::Linear,
         {"LinearBackground",
          "LinearBackground",
          {ParamID::LINEAR_BG_A0, ParamID::LINEAR_BG_A1}}},
    };

QString paramName(ParamID id) { return g_paramName.at(id); }

void applyToFitType(FitType fitType,
                    const std::function<void(ParamID)> &paramFun) {
  applyToParamIDRange(FitSubType::g_typeMap[fitType].blocks.front(),
                      FitSubType::g_typeMap[fitType].blocks.back(), paramFun);
}

void applyToBackground(BackgroundType bgType,
                       const std::function<void(ParamID)> &paramFun) {
  applyToParamIDRange(BackgroundSubType::g_typeMap[bgType].blocks.front(),
                      BackgroundSubType::g_typeMap[bgType].blocks.back(),
                      paramFun);
}

} // namespace ConvTypes
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
