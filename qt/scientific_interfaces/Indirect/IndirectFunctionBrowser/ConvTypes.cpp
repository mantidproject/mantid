#include "ConvTypes.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
namespace ConvTypes {

std::map<ParamID, QString> g_paramName{
    {ParamID::LOR1_AMPLITUDE, "Amplitude"},
    {ParamID::LOR1_PEAKCENTRE, "PeakCentre"},
    {ParamID::LOR1_FWHM, "FWHM"},
    {ParamID::LOR2_AMPLITUDE, "Amplitude"},
    {ParamID::LOR2_PEAKCENTRE, "PeakCentre"},
    {ParamID::LOR2_FWHM, "FWHM"},
    {ParamID::BG_A0, "A0"},
    {ParamID::BG_A0, "A1"}};

std::map<QString, FitType> g_fitTypeMap{
    {"None", FitType::None},
    {"One Lorentzian", FitType::OneLorentzian},
    {"Two Lorentzians", FitType::TwoLorentzians},
};

QStringList getFitTypes() {
  QMap<FitType, QString> inverseMap;
  for (auto const it : g_fitTypeMap) {
    inverseMap[it.second] = it.first;
  }
  return inverseMap.values();
}

FitType fitTypeId(const QString &fitType) { return g_fitTypeMap[fitType]; }

QString paramName(ParamID id) { return g_paramName.at(id); }

QStringList getParameterNames(FitType fitType) { return QStringList(); }

} // namespace ConvTypes
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
