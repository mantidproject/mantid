// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FunctionBrowser/FitTypes.h"
#include "FunctionBrowser/TemplateSubType.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace MantidQt::CustomInterfaces::IDA {

static const auto FUNCTION_STRINGS =
    std::unordered_map<std::string, std::string>({{"ExpDecay", "E"},
                                                  {"StretchExp", "S"},
                                                  {"Lorentzian", "L"},
                                                  {"StretchedExpFT", "SFT"},
                                                  {"TeixeiraWater", "TxWater"},
                                                  {"TeixeiraWaterSQE", "TxWater"},
                                                  {"FickDiffusionSQE", "FickDiff"},
                                                  {"ChudleyElliotSQE", "ChudElliot"},
                                                  {"HallRoss", "HallRoss"},
                                                  {"HallRossSQE", "HallRoss"},
                                                  {"DiffRotDiscreteCircle", "DC"},
                                                  {"ElasticDiffRotDiscreteCircle", "EDC"},
                                                  {"InelasticDiffRotDiscreteCircle", "IDC"},
                                                  {"DiffSphere", "DS"},
                                                  {"ElasticDiffSphere", "EDS"},
                                                  {"InelasticDiffSphere", "IDS"},
                                                  {"IsoRotDiff", "IRD"},
                                                  {"ElasticIsoRotDiff", "EIRD"},
                                                  {"InelasticIsoRotDiff", "IIRD"},
                                                  {"MsdGauss", "Gauss"},
                                                  {"MsdPeters", "Peters"},
                                                  {"MsdYi", "Yi"},
                                                  {"FickDiffusion", "FickDiffusion"},
                                                  {"ChudleyElliot", "ChudleyElliot"},
                                                  {"EISFDiffCylinder", "EISFDiffCylinder"},
                                                  {"EISFDiffSphere", "EISFDiffSphere"},
                                                  {"EISFDiffSphereAlkyl", "EISFDiffSphereAlkyl"}});

namespace MSDFit {
static const auto TAB_NAME = "MSDFit";
static const auto HAS_RESOLUTION = false;
static const auto HIDDEN_PROPS =
    std::vector<std::string>({"CreateOutput", "LogValue", "PassWSIndexToFunction", "ConvolveMembers",
                              "OutputCompositeMembers", "OutputWorkspace", "Output", "PeakRadius", "PlotParameter"});

static const auto ALL_FITS =
    std::map<std::string, std::string>({{"None", ""},
                                        {"Gauss", "name=MsdGauss,Height=1,Msd=0.05,constraints=(Height>0, Msd>0)"},
                                        {"Peters", "name=MsdPeters,Height=1,Msd=0.05,Beta=1,constraints=(Height>0, "
                                                   "Msd>0, Beta>0)"},
                                        {"Yi", "name=MsdYi,Height=1,Msd=0.05,Sigma=1,constraints=(Height>0, Msd>0, "
                                               "Sigma>0)"}});
} // namespace MSDFit

namespace IqtFit {
static const auto TAB_NAME = "IqtFit";
static const auto HAS_RESOLUTION = false;
static const auto HIDDEN_PROPS =
    std::vector<std::string>({"CreateOutput", "LogValue", "PassWSIndexToFunction", "ConvolveMembers",
                              "OutputCompositeMembers", "OutputWorkspace", "Output", "PeakRadius", "PlotParameter"});
} // namespace IqtFit

namespace ConvFit {
static const auto TAB_NAME = "ConvFit";
static const auto HAS_RESOLUTION = true;
static const auto HIDDEN_PROPS = std::vector<std::string>(
    {"CreateOutput", "LogValue", "PassWSIndexToFunction", "OutputWorkspace", "Output", "PeakRadius", "PlotParameter"});

static auto TEMPLATE_SUB_TYPES =
    packTemplateSubTypes(std::make_unique<ConvTypes::LorentzianSubType>(), std::make_unique<ConvTypes::FitSubType>(),
                         std::make_unique<ConvTypes::DeltaSubType>(), std::make_unique<ConvTypes::TempSubType>(),
                         std::make_unique<ConvTypes::BackgroundSubType>());

} // namespace ConvFit

namespace FqFit {
static const auto TAB_NAME = "FQFit";
static const auto HAS_RESOLUTION = false;
static const auto HIDDEN_PROPS =
    std::vector<std::string>({"CreateOutput", "LogValue", "PassWSIndexToFunction", "ConvolveMembers",
                              "OutputCompositeMembers", "OutputWorkspace", "Output", "PeakRadius", "PlotParameter"});
static const auto X_BOUNDS = std::pair<double, double>{0.0, 2.0};

enum class DataType {
  WIDTH,
  EISF,
  ALL,
};

static const auto WIDTH_FITS = std::map<std::string, std::string>(
    {{"None", ""},
     {"ChudleyElliot", "name=ChudleyElliot, Tau=1, L=1.5, constraints=(Tau>0, L>0)"},
     {"HallRoss", "name=Hallross, Tau=1, L=0.2, constraints=(Tau>0, L>0)"},
     {"FickDiffusion", "name=FickDiffusion, D=1, constraints=(D>0)"},
     {"TeixeiraWater", "name=TeixeiraWater, Tau=1, L=1.5, constraints=(Tau>0, L>0)"}});

static const auto EISF_FITS = std::map<std::string, std::string>(
    {{"None", ""},
     {"EISFDiffCylinder", "name=EISFDiffCylinder, A=1, R=1, L=2, constraints=(A>0, R>0, L>0)"},
     {"EISFDiffSphere", "name=EISFDiffSphere, A=1, R=1, constraints=(A>0, R>0)"},
     {"EISFDiffSphereAlkyl", "name=EISFDiffSphereAlkyl, A=1, Rmin=1, Rmax=2, constraints=(A>0, Rmin>0, Rmax>0)"}});

static const auto ALL_FITS = std::map<std::string, std::string>(
    {{"None", ""},
     {"ChudleyElliot", "name=ChudleyElliot, Tau=1, L=1.5, constraints=(Tau>0, L>0)"},
     {"HallRoss", "name=Hallross, Tau=1, L=0.2, constraints=(Tau>0, L>0)"},
     {"FickDiffusion", "name=FickDiffusion, D=1, constraints=(D>0)"},
     {"TeixeiraWater", "name=TeixeiraWater, Tau=1, L=1.5, constraints=(Tau>0, L>0)"},
     {"EISFDiffCylinder", "name=EISFDiffCylinder, A=1, R=1, L=2, constraints=(A>0, R>0, L>0)"},
     {"EISFDiffSphere", "name=EISFDiffSphere, A=1, R=1, constraints=(A>0, R>0)"},
     {"EISFDiffSphereAlkyl", "name=EISFDiffSphereAlkyl, A=1, Rmin=1, Rmax=2, constraints=(A>0, Rmin>0, Rmax>0)"}});

static const std::unordered_map<DataType, std::map<std::string, std::string>> availableFits{
    {{DataType::WIDTH, WIDTH_FITS}, {DataType::EISF, EISF_FITS}, {DataType::ALL, ALL_FITS}}};

} // namespace FqFit

} // namespace MantidQt::CustomInterfaces::IDA
