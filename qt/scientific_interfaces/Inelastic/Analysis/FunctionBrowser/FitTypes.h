// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ParamID.h"
#include "TemplateSubType.h"

#include <map>
#include <unordered_map>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
namespace IqtTypes {

enum class ExponentialType {
  None,
  OneExponential,
  TwoExponentials,
};

enum class StretchExpType { None, StretchExponential };

enum class BackgroundType { None, Flat };

enum class TieIntensitiesType { False, True };

enum SubTypeIndex {
  Exponential = 0,
  StretchExponential = 1,
  Background = 2,
  TieIntensities = 3,
};

struct ExponentialSubType : public TemplateSubTypeImpl<ExponentialType> {
  std::string name() const override { return "Exponentials"; }
  bool isType(const std::type_info &type) const override { return type == typeid(int); }
};

struct StretchExpSubType : public TemplateSubTypeImpl<StretchExpType> {
  std::string name() const override { return "Stretch Exponential"; }
  bool isType(const std::type_info &type) const override { return type == typeid(bool); }
};

struct BackgroundSubType : public TemplateSubTypeImpl<BackgroundType> {
  std::string name() const override { return "Background"; }
};

struct TieIntensitiesSubType : public TemplateSubTypeImpl<TieIntensitiesType> {
  std::string name() const override { return "Tie Intensities"; }
  bool isType(const std::type_info &type) const override { return type == typeid(bool); }
};

} // namespace IqtTypes

namespace ConvTypes {

enum class LorentzianType {
  None,
  OneLorentzian,
  TwoLorentzians,
};

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

enum class DeltaType { None, Delta };

enum class TempCorrectionType { None, Exponential };

enum class BackgroundType { None, Flat, Linear };

enum SubTypeIndex {
  Lorentzian = 0,
  Fit = 1,
  Delta = 2,
  TempCorrection = 3,
  Background = 4,
};

extern std::map<FitType, bool> FitTypeQDepends;
extern std::unordered_map<std::string, FitType> FitTypeStringToEnum;

struct LorentzianSubType : public TemplateSubTypeImpl<LorentzianType> {
  std::string name() const override { return "Lorentzians"; }
  bool isType(const std::type_info &type) const override { return type == typeid(int); }
};

struct FitSubType : public TemplateSubTypeImpl<FitType> {
  std::string name() const override { return "Fit Type"; }
};

struct DeltaSubType : public TemplateSubTypeImpl<DeltaType> {
  std::string name() const override { return "Delta Function"; }
  bool isType(const std::type_info &type) const override { return type == typeid(bool); }
};

struct TempSubType : public TemplateSubTypeImpl<TempCorrectionType> {
  std::string name() const override { return "ConvTempCorrection"; }
  bool isType(const std::type_info &type) const override { return type == typeid(bool); }
};

struct BackgroundSubType : public TemplateSubTypeImpl<BackgroundType> {
  std::string name() const override { return "Background"; }
};

} // namespace ConvTypes
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
