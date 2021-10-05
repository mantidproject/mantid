// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvTypes.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"

namespace MantidQt::CustomInterfaces::IDA::ConvTypes {

using namespace Mantid::API;

std::map<FitType, bool> FitTypeQDepends = std::map<FitType, bool>({{FitType::None, false},
                                                                   {FitType::TeixeiraWater, true},
                                                                   {FitType::StretchedExpFT, false},
                                                                   {FitType::DiffSphere, true},
                                                                   {FitType::ElasticDiffSphere, true},
                                                                   {FitType::InelasticDiffSphere, true},
                                                                   {FitType::DiffRotDiscreteCircle, true},
                                                                   {FitType::InelasticDiffRotDiscreteCircle, true},
                                                                   {FitType::ElasticDiffRotDiscreteCircle, true},
                                                                   {FitType::IsoRotDiff, true},
                                                                   {FitType::ElasticIsoRotDiff, true},
                                                                   {FitType::InelasticIsoRotDiff, true}});

std::unordered_map<FitType, std::string>
    FitTypeEnumToString({{FitType::TeixeiraWater, "TeixeiraWaterSQE"},
                         {FitType::StretchedExpFT, "StretchedExpFT"},
                         {FitType::DiffSphere, "DiffSphere"},
                         {FitType::ElasticDiffSphere, "ElasticDiffSphere"},
                         {FitType::InelasticDiffSphere, "InelasticDiffSphere"},
                         {FitType::DiffRotDiscreteCircle, "DiffRotDiscreteCircle"},
                         {FitType::InelasticDiffRotDiscreteCircle, "InelasticDiffRotDiscreteCircle"},
                         {FitType::ElasticDiffRotDiscreteCircle, "ElasticDiffRotDiscreteCircle"},
                         {FitType::IsoRotDiff, "IsoRotDiff"},
                         {FitType::ElasticIsoRotDiff, "ElasticIsoRotDiff"},
                         {FitType::InelasticIsoRotDiff, "InelasticIsoRotDiff"}});

std::unordered_map<std::string, FitType>
    FitTypeStringToEnum({{"TeixeiraWaterSQE", FitType::TeixeiraWater},
                         {"StretchedExpFT", FitType::StretchedExpFT},
                         {"DiffSphere", FitType::DiffSphere},
                         {"ElasticDiffSphere", FitType::ElasticDiffSphere},
                         {"InelasticDiffSphere", FitType::InelasticDiffSphere},
                         {"DiffRotDiscreteCircle", FitType::DiffRotDiscreteCircle},
                         {"InelasticDiffRotDiscreteCircle", FitType::InelasticDiffRotDiscreteCircle},
                         {"ElasticDiffRotDiscreteCircle", FitType::ElasticDiffRotDiscreteCircle},
                         {"IsoRotDiff", FitType::IsoRotDiff},
                         {"ElasticIsoRotDiff", FitType::ElasticIsoRotDiff},
                         {"InelasticIsoRotDiff", FitType::InelasticIsoRotDiff}});

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

template <>
std::map<FitType, TemplateSubTypeDescriptor> TemplateSubTypeImpl<FitType>::g_typeMap{
    {FitType::None, {"None", "", {ParamID::NONE, ParamID::NONE}}},
    {FitType::TeixeiraWater, {"Teixeira Water", "TeixeiraWaterSQE", {ParamID::TW_HEIGHT, ParamID::TW_CENTRE}}},
    {FitType::StretchedExpFT, {"StretchedExpFT", "StretchedExpFT", {ParamID::SE_HEIGHT, ParamID::SE_CENTRE}}},
    {FitType::DiffSphere, {"DiffSphere", "DiffSphere", {ParamID::DP_INTENSITY, ParamID::DP_SHIFT}}},
    {FitType::ElasticDiffSphere,
     {"ElasticDiffSphere", "ElasticDiffSphere", {ParamID::EDP_HEIGHT, ParamID::EDP_RADIUS}}},
    {FitType::InelasticDiffSphere,
     {"InelasticDiffSphere", "InelasticDiffSphere", {ParamID::IDP_INTENSITY, ParamID::IDP_SHIFT}}},
    {FitType::DiffRotDiscreteCircle,
     {"DiffRotDiscreteCircle", "DiffRotDiscreteCircle", {ParamID::DRDC_INTENSITY, ParamID::DRDC_SHIFT}}},
    {FitType::InelasticDiffRotDiscreteCircle,
     {"InelasticDiffRotDiscreteCircle",
      "InelasticDiffRotDiscreteCircle",
      {ParamID::IDRDC_INTENSITY, ParamID::IDRDC_SHIFT}}},
    {FitType::ElasticDiffRotDiscreteCircle,
     {"ElasticDiffRotDiscreteCircle", "ElasticDiffRotDiscreteCircle", {ParamID::EDRDC_HEIGHT, ParamID::EDRDC_RADIUS}}},
    {FitType::IsoRotDiff, {"IsoRotDiff", "IsoRotDiff", {ParamID::IRD_HEIGHT, ParamID::IRD_CENTRE}}},
    {FitType::ElasticIsoRotDiff,
     {"ElasticIsoRotDiff", "ElasticIsoRotDiff", {ParamID::EIRD_HEIGHT, ParamID::EIRD_RADIUS}}},
    {FitType::InelasticIsoRotDiff,
     {"InelasticIsoRotDiff", "InelasticIsoRotDiff", {ParamID::IIRD_HEIGHT, ParamID::IIRD_CENTRE}}},
};
template <>
std::map<LorentzianType, TemplateSubTypeDescriptor> TemplateSubTypeImpl<LorentzianType>::g_typeMap{
    {LorentzianType::None, {"None", "", {ParamID::NONE, ParamID::NONE}}},
    {LorentzianType::OneLorentzian, {"One Lorentzian", "Lorentzian", {ParamID::LOR1_AMPLITUDE, ParamID::LOR1_FWHM}}},
    {LorentzianType::TwoLorentzians,
     {"Two Lorentzians", "Lorentzian", {ParamID::LOR2_AMPLITUDE_1, ParamID::LOR2_FWHM_1, ParamID::LOR2_FWHM_2}}},
};

template <>
std::map<BackgroundType, TemplateSubTypeDescriptor> TemplateSubTypeImpl<BackgroundType>::g_typeMap{
    {BackgroundType::None, {"None", "", {ParamID::NONE, ParamID::NONE}}},
    {BackgroundType::Flat, {"FlatBackground", "FlatBackground", {ParamID::FLAT_BG_A0, ParamID::FLAT_BG_A0}}},
    {BackgroundType::Linear, {"LinearBackground", "LinearBackground", {ParamID::LINEAR_BG_A0, ParamID::LINEAR_BG_A1}}},
};

template <>
std::map<bool, TemplateSubTypeDescriptor> TemplateSubTypeImpl<bool>::g_typeMap{
    {false, {"None", "", {ParamID::NONE, ParamID::NONE}}},
    {true, {"DeltaFunction", "DeltaFunction", {ParamID::DELTA_HEIGHT, ParamID::DELTA_CENTER}}},
};

template <>
std::map<TempCorrectionType, TemplateSubTypeDescriptor> TemplateSubTypeImpl<TempCorrectionType>::g_typeMap{
    {TempCorrectionType::None, {"None", "", {ParamID::NONE}}},
    {TempCorrectionType::Exponential,
     {"Temp Correction", "ConvTempCorrection", {ParamID::TEMPERATURE, ParamID::TEMPERATURE}}},
};

QString paramName(ParamID id) { return g_paramName.at(id); }

void applyToFitType(FitType fitType, const std::function<void(ParamID)> &paramFun) {
  applyToParamIDRange(FitSubType::g_typeMap[fitType].blocks.front(), FitSubType::g_typeMap[fitType].blocks.back(),
                      paramFun);
}

void applyToLorentzianType(LorentzianType lorentzianType, const std::function<void(ParamID)> &paramFun) {
  applyToParamIDRange(LorentzianSubType::g_typeMap[lorentzianType].blocks.front(),
                      LorentzianSubType::g_typeMap[lorentzianType].blocks.back(), paramFun);
}

void applyToBackground(BackgroundType bgType, const std::function<void(ParamID)> &paramFun) {
  applyToParamIDRange(BackgroundSubType::g_typeMap[bgType].blocks.front(),
                      BackgroundSubType::g_typeMap[bgType].blocks.back(), paramFun);
}

void applyToDelta(bool hasDeltaFunction, const std::function<void(ParamID)> &paramFun) {
  applyToParamIDRange(DeltaSubType::g_typeMap[hasDeltaFunction].blocks.front(),
                      DeltaSubType::g_typeMap[hasDeltaFunction].blocks.back(), paramFun);
}

void applyToTemp(TempCorrectionType tempCorrectionType, const std::function<void(ParamID)> &paramFun) {
  applyToParamIDRange(TempSubType::g_typeMap[tempCorrectionType].blocks.front(),
                      TempSubType::g_typeMap[tempCorrectionType].blocks.back(), paramFun);
}

} // namespace MantidQt::CustomInterfaces::IDA::ConvTypes
