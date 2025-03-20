// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FitTypes.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"

namespace MantidQt::CustomInterfaces::Inelastic {

template <>
std::map<IqtTypes::ExponentialType, TemplateSubTypeDescriptor>
    TemplateSubTypeImpl<IqtTypes::ExponentialType>::g_typeMap{
        {IqtTypes::ExponentialType::None, {"None", "", {ParamID::NONE, ParamID::NONE}}},
        {IqtTypes::ExponentialType::OneExponential,
         {"One Exponential", "ExpDecay", {ParamID::EXP1_HEIGHT, ParamID::EXP1_LIFETIME}}},
        {IqtTypes::ExponentialType::TwoExponentials,
         {"Two Exponentials", "ExpDecay", {ParamID::EXP1_HEIGHT, ParamID::EXP2_LIFETIME}}},
    };

template <>
std::map<IqtTypes::FitType, TemplateSubTypeDescriptor> TemplateSubTypeImpl<IqtTypes::FitType>::g_typeMap{
    {IqtTypes::FitType::None, {"None", "", {ParamID::NONE, ParamID::NONE}}},
    {IqtTypes::FitType::StretchExponential,
     {"Stretch Exponential", "StretchExp", {ParamID::STRETCH_HEIGHT, ParamID::STRETCH_STRETCHING}}},
    {IqtTypes::FitType::TeixeiraWaterIqt,
     {"Teixeira Water Iqt", "TeixeiraWaterIqt", {ParamID::TWI_AMPLITUDE, ParamID::TWI_GAMMA}}},
};

template <>
std::map<IqtTypes::BackgroundType, TemplateSubTypeDescriptor> TemplateSubTypeImpl<IqtTypes::BackgroundType>::g_typeMap{
    {IqtTypes::BackgroundType::None, {"None", "", {ParamID::NONE, ParamID::NONE}}},
    {IqtTypes::BackgroundType::Flat, {"FlatBackground", "FlatBackground", {ParamID::FLAT_BG_A0, ParamID::FLAT_BG_A0}}},
};

template <>
std::map<IqtTypes::TieIntensitiesType, TemplateSubTypeDescriptor>
    TemplateSubTypeImpl<IqtTypes::TieIntensitiesType>::g_typeMap{
        {IqtTypes::TieIntensitiesType::False, {"None", "", {ParamID::NONE, ParamID::NONE}}},
        {IqtTypes::TieIntensitiesType::True, {"Tie Intensities", "", {ParamID::NONE, ParamID::NONE}}},
    };

namespace ConvTypes {

std::map<FitType, bool> FitTypeQDepends = std::map<FitType, bool>({{FitType::None, false},
                                                                   {FitType::TeixeiraWater, true},
                                                                   {FitType::TeixeiraWaterIqtFT, true},
                                                                   {FitType::FickDiffusion, true},
                                                                   {FitType::ChudleyElliot, true},
                                                                   {FitType::HallRoss, true},
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

std::unordered_map<std::string, FitType>
    FitTypeStringToEnum({{"TeixeiraWaterSQE", FitType::TeixeiraWater},
                         {"TeixeiraWaterIqtFT", FitType::TeixeiraWaterIqtFT},
                         {"FickDiffusionSQE", FitType::FickDiffusion},
                         {"ChudleyElliotSQE", FitType::ChudleyElliot},
                         {"HallRossSQE", FitType::HallRoss},
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

} // namespace ConvTypes

template <>
std::map<ConvTypes::FitType, TemplateSubTypeDescriptor> TemplateSubTypeImpl<ConvTypes::FitType>::g_typeMap{
    {ConvTypes::FitType::None, {"None", "", {ParamID::NONE, ParamID::NONE}}},
    {ConvTypes::FitType::TeixeiraWater,
     {"Teixeira Water SQE", "TeixeiraWaterSQE", {ParamID::TW_HEIGHT, ParamID::TW_CENTRE}}},
    {ConvTypes::FitType::TeixeiraWaterIqtFT,
     {"TeixeiraWaterIqtFT", "TeixeiraWaterIqtFT", {ParamID::TWIF_AMP, ParamID::TWIF_TAU1, ParamID::TWIF_GAMMA}}},
    {ConvTypes::FitType::FickDiffusion,
     {"Fick Diffusion SQE", "FickDiffusionSQE", {ParamID::FD_HEIGHT, ParamID::FD_CENTRE}}},
    {ConvTypes::FitType::ChudleyElliot,
     {"Chudley-Elliot SQE", "ChudleyElliotSQE", {ParamID::CE_HEIGHT, ParamID::CE_CENTRE}}},
    {ConvTypes::FitType::HallRoss, {"Hall-Ross SQE", "HallRossSQE", {ParamID::HR_HEIGHT, ParamID::HR_CENTRE}}},
    {ConvTypes::FitType::StretchedExpFT,
     {"StretchedExpFT", "StretchedExpFT", {ParamID::SE_HEIGHT, ParamID::SE_CENTRE}}},
    {ConvTypes::FitType::DiffSphere, {"DiffSphere", "DiffSphere", {ParamID::DP_INTENSITY, ParamID::DP_SHIFT}}},
    {ConvTypes::FitType::ElasticDiffSphere,
     {"ElasticDiffSphere", "ElasticDiffSphere", {ParamID::EDP_HEIGHT, ParamID::EDP_RADIUS}}},
    {ConvTypes::FitType::InelasticDiffSphere,
     {"InelasticDiffSphere", "InelasticDiffSphere", {ParamID::IDP_INTENSITY, ParamID::IDP_SHIFT}}},
    {ConvTypes::FitType::DiffRotDiscreteCircle,
     {"DiffRotDiscreteCircle", "DiffRotDiscreteCircle", {ParamID::DRDC_INTENSITY, ParamID::DRDC_SHIFT}}},
    {ConvTypes::FitType::InelasticDiffRotDiscreteCircle,
     {"InelasticDiffRotDiscreteCircle",
      "InelasticDiffRotDiscreteCircle",
      {ParamID::IDRDC_INTENSITY, ParamID::IDRDC_SHIFT}}},
    {ConvTypes::FitType::ElasticDiffRotDiscreteCircle,
     {"ElasticDiffRotDiscreteCircle", "ElasticDiffRotDiscreteCircle", {ParamID::EDRDC_HEIGHT, ParamID::EDRDC_RADIUS}}},
    {ConvTypes::FitType::IsoRotDiff, {"IsoRotDiff", "IsoRotDiff", {ParamID::IRD_HEIGHT, ParamID::IRD_CENTRE}}},
    {ConvTypes::FitType::ElasticIsoRotDiff,
     {"ElasticIsoRotDiff", "ElasticIsoRotDiff", {ParamID::EIRD_HEIGHT, ParamID::EIRD_RADIUS}}},
    {ConvTypes::FitType::InelasticIsoRotDiff,
     {"InelasticIsoRotDiff", "InelasticIsoRotDiff", {ParamID::IIRD_HEIGHT, ParamID::IIRD_CENTRE}}},
};
template <>
std::map<ConvTypes::LorentzianType, TemplateSubTypeDescriptor>
    TemplateSubTypeImpl<ConvTypes::LorentzianType>::g_typeMap{
        {ConvTypes::LorentzianType::None, {"None", "", {ParamID::NONE, ParamID::NONE}}},
        {ConvTypes::LorentzianType::OneLorentzian,
         {"One Lorentzian", "Lorentzian", {ParamID::LOR1_AMPLITUDE, ParamID::LOR1_FWHM}}},
        {ConvTypes::LorentzianType::TwoLorentzians,
         {"Two Lorentzians", "Lorentzian", {ParamID::LOR1_AMPLITUDE, ParamID::LOR2_FWHM}}},
    };

template <>
std::map<ConvTypes::BackgroundType, TemplateSubTypeDescriptor>
    TemplateSubTypeImpl<ConvTypes::BackgroundType>::g_typeMap{
        {ConvTypes::BackgroundType::None, {"None", "", {ParamID::NONE, ParamID::NONE}}},
        {ConvTypes::BackgroundType::Flat,
         {"FlatBackground", "FlatBackground", {ParamID::FLAT_BG_A0, ParamID::FLAT_BG_A0}}},
        {ConvTypes::BackgroundType::Linear,
         {"LinearBackground", "LinearBackground", {ParamID::LINEAR_BG_A0, ParamID::LINEAR_BG_A1}}},
    };

template <>
std::map<ConvTypes::DeltaType, TemplateSubTypeDescriptor> TemplateSubTypeImpl<ConvTypes::DeltaType>::g_typeMap{
    {ConvTypes::DeltaType::None, {"None", "", {ParamID::NONE, ParamID::NONE}}},
    {ConvTypes::DeltaType::Delta, {"DeltaFunction", "DeltaFunction", {ParamID::DELTA_HEIGHT, ParamID::DELTA_CENTER}}},
};

template <>
std::map<ConvTypes::TempCorrectionType, TemplateSubTypeDescriptor>
    TemplateSubTypeImpl<ConvTypes::TempCorrectionType>::g_typeMap{
        {ConvTypes::TempCorrectionType::None, {"None", "", {ParamID::NONE, ParamID::NONE}}},
        {ConvTypes::TempCorrectionType::Exponential,
         {"Temp Correction", "ConvTempCorrection", {ParamID::TEMPERATURE, ParamID::TEMPERATURE}}},
    };

template <>
std::map<ConvTypes::TiePeakCentresType, TemplateSubTypeDescriptor>
    TemplateSubTypeImpl<ConvTypes::TiePeakCentresType>::g_typeMap{
        {ConvTypes::TiePeakCentresType::False, {"None", "", {ParamID::NONE, ParamID::NONE}}},
        {ConvTypes::TiePeakCentresType::True, {"Tie Peak Centres", "", {ParamID::NONE, ParamID::NONE}}},
    };
} // namespace MantidQt::CustomInterfaces::Inelastic
