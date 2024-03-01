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

enum class BackgroundType { None, Flat, Linear };

enum class TempCorrectionType { None, Exponential };

enum SubTypeIndex {
  Lorentzian = 0,
  Fit = 1,
  Delta = 2,
  Background = 3,
};

extern std::map<FitType, bool> FitTypeQDepends;
extern std::unordered_map<std::string, FitType> FitTypeStringToEnum;

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
  std::string name() const override { return "Delta Function"; }
};

struct TempSubType : public TemplateSubTypeImpl<TempCorrectionType> {
  std::string name() const override { return "ConvTempCorrection"; }
};

} // namespace ConvTypes
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
