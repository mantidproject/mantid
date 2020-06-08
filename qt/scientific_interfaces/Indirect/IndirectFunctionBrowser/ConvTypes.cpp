// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvTypes.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
namespace ConvTypes {

using namespace Mantid::API;

std::map<FitType, bool> FitTypeQDepends =
    std::map<FitType, bool>({{FitType::None, false},
                             {FitType::OneLorentzian, false},
                             {FitType::TwoLorentzians, false},
                             {FitType::TeixeiraWater, true},
                             {FitType::StretchedExpFT, false},
                             {FitType::ElasticDiffSphere, true},
                             {FitType::InelasticDiffSphere, true},
                             {FitType::InelasticDiffRotDiscreteCircle, true},
                             {FitType::ElasticDiffRotDiscreteCircle, true}});

std::unordered_map<FitType, std::string> FitTypeEnumToString(
    {{FitType::OneLorentzian, "Lorentzian"},
     {FitType::TwoLorentzians, "Lorentzian"},
     {FitType::TeixeiraWater, "TeixeiraWaterSQE"},
     {FitType::StretchedExpFT, "StretchedExpFT"},
     {FitType::ElasticDiffSphere, "ElasticDiffSphere"},
     {FitType::InelasticDiffSphere, "InelasticDiffSphere"},
     {FitType::InelasticDiffRotDiscreteCircle,
      "InelasticDiffRotDiscreteCircle"},
     {FitType::ElasticDiffRotDiscreteCircle, "ElasticDiffRotDiscreteCircle"}});

std::unordered_map<std::string, FitType> FitTypeStringToEnum(
    {{"Lorentzian", FitType::OneLorentzian},
     {"TeixeiraWaterSQE", FitType::TeixeiraWater},
     {"StretchedExpFT", FitType::StretchedExpFT},
     {"ElasticDiffSphere", FitType::ElasticDiffSphere},
     {"InelasticDiffSphere", FitType::InelasticDiffSphere},
     {"InelasticDiffRotDiscreteCircle",
      FitType::InelasticDiffRotDiscreteCircle},
     {"ElasticDiffRotDiscreteCircle", FitType::ElasticDiffRotDiscreteCircle}});

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
    {ParamID::TW_HEIGHT, "Height"},
    {ParamID::TW_DIFFCOEFF, "DiffCoeff"},
    {ParamID::TW_TAU, "Tau"},
    {ParamID::TW_CENTRE, "Centre"},
    {ParamID::DELTA_HEIGHT, "Height"},
    {ParamID::DELTA_CENTER, "Centre"},
    {ParamID::TEMPERATURE, "Temp"},
    {ParamID::SE_HEIGHT, "Height"},
    {ParamID::SE_TAU, "Tau"},
    {ParamID::SE_BETA, "Beta"},
    {ParamID::SE_CENTRE, "Centre"},
    {ParamID::EDP_HEIGHT, "Height"},
    {ParamID::EDP_CENTRE, "Centre"},
    {ParamID::EDP_RADIUS, "Radius"},
    {ParamID::IDP_INTENSITY, "Intensity"},
    {ParamID::IDP_RADIUS, "Radius"},
    {ParamID::IDP_DIFFUSION, "Diffusion"},
    {ParamID::IDP_SHIFT, "Shift"},
    {ParamID::IDRDC_INTENSITY, "Intensity"},
    {ParamID::IDRDC_RADIUS, "Radius"},
    {ParamID::IDRDC_DECAY, "Decay"},
    {ParamID::IDRDC_SHIFT, "Shift"},
    {ParamID::EDRDC_HEIGHT, "Height"},
    {ParamID::EDRDC_CENTRE, "Centre"},
    {ParamID::EDRDC_RADIUS, "Radius"},
    {ParamID::FLAT_BG_A0, "A0"},
    {ParamID::LINEAR_BG_A0, "A0"},
    {ParamID::LINEAR_BG_A1, "A1"},
};

template <>
std::map<FitType, TemplateSubTypeDescriptor>
    TemplateSubTypeImpl<FitType>::g_typeMap{
        {FitType::None, {"None", "", {ParamID::NONE, ParamID::NONE}}},
        {FitType::OneLorentzian,
         {"One Lorentzian",
          "Lorentzian",
          {ParamID::LOR1_AMPLITUDE, ParamID::LOR1_FWHM}}},
        {FitType::TwoLorentzians,
         {"Two Lorentzians",
          "Lorentzian",
          {ParamID::LOR2_AMPLITUDE_1, ParamID::LOR2_FWHM_1,
           ParamID::LOR2_FWHM_2}}},
        {FitType::TeixeiraWater,
         {"Teixeira Water",
          "TeixeiraWaterSQE",
          {ParamID::TW_HEIGHT, ParamID::TW_CENTRE}}},
        {FitType::StretchedExpFT,
         {"StretchedExpFT",
          "StretchedExpFT",
          {ParamID::SE_HEIGHT, ParamID::SE_CENTRE}}},
        {FitType::ElasticDiffSphere,
         {"ElasticDiffSphere",
          "ElasticDiffSphere",
          {ParamID::EDP_HEIGHT, ParamID::EDP_RADIUS}}},
        {FitType::InelasticDiffSphere,
         {"InelasticDiffSphere",
          "InelasticDiffSphere",
          {ParamID::IDP_INTENSITY, ParamID::IDP_SHIFT}}},
        {FitType::InelasticDiffRotDiscreteCircle,
         {"InelasticDiffRotDiscreteCircle",
          "InelasticDiffRotDiscreteCircle",
          {ParamID::IDRDC_INTENSITY, ParamID::IDRDC_SHIFT}}},
        {FitType::ElasticDiffRotDiscreteCircle,
         {"ElasticDiffRotDiscreteCircle",
          "ElasticDiffRotDiscreteCircle",
          {ParamID::EDRDC_HEIGHT, ParamID::EDRDC_RADIUS}}},
    };

template <>
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

template <>
std::map<bool, TemplateSubTypeDescriptor> TemplateSubTypeImpl<bool>::g_typeMap{
    {false, {"None", "", {ParamID::NONE, ParamID::NONE}}},
    {true,
     {"DeltaFunction",
      "DeltaFunction",
      {ParamID::DELTA_HEIGHT, ParamID::DELTA_CENTER}}},
};

template <>
std::map<TempCorrectionType, TemplateSubTypeDescriptor>
    TemplateSubTypeImpl<TempCorrectionType>::g_typeMap{
        {TempCorrectionType::None,
         {"None", "", {ParamID::NONE, ParamID::NONE}}},
        {TempCorrectionType::Exponential,
         {"Temp Correction",
          "UserFunction",
          {ParamID::TEMPERATURE, ParamID::TEMPERATURE}}},
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

void applyToDelta(bool hasDeltaFunction,
                  const std::function<void(ParamID)> &paramFun) {
  applyToParamIDRange(DeltaSubType::g_typeMap[hasDeltaFunction].blocks.front(),
                      DeltaSubType::g_typeMap[hasDeltaFunction].blocks.back(),
                      paramFun);
}

void applyToTemp(TempCorrectionType tempCorrectionType,
                 const std::function<void(ParamID)> &paramFun) {
  applyToParamIDRange(TempSubType::g_typeMap[tempCorrectionType].blocks.front(),
                      TempSubType::g_typeMap[tempCorrectionType].blocks.back(),
                      paramFun);
}

} // namespace ConvTypes
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
