// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

class ToyAlgorithm : public Algorithm {
public:
  ToyAlgorithm() : Algorithm() {}
  ~ToyAlgorithm() override = default;
  const std::string name() const override { return "ToyAlgorithm"; } ///< Algorithm's name for identification
  int version() const override { return 1; }                         ///< Algorithm's version for identification
  const std::string category() const override { return "Cat"; }      ///< Algorithm's category for identification
  const std::string alias() const override { return "Dog"; }
  const std::string summary() const override { return "Test summary"; }
  const std::vector<std::string> seeAlso() const override { return {"rabbit", "goldfish", "Spotted Hyena"}; }

  void init() override {
    declareProperty("prop1", "value");
    declareProperty("prop2", 1);
  }
  void exec() override {}

  bool existsProperty(const std::string &name) const override { return Algorithm::existsProperty(name); }
  const std::vector<Property *> &getProperties() const override { return Algorithm::getProperties(); }
};

class ToyAlgorithmTwo : public Algorithm {
public:
  ToyAlgorithmTwo() : Algorithm() {}
  ~ToyAlgorithmTwo() override = default;

  const std::string name() const override { return "ToyAlgorithm"; } ///< Algorithm's name for identification
  int version() const override { return 2; }                         ///< Algorithm's version for identification
  const std::string category() const override { return "Cat,Leopard,Mink"; }
  const std::string categorySeparator() const override { return ","; } ///< testing the ability to change the seperator
  const std::string alias() const override { return "Dog"; }
  const std::string summary() const override { return "Test summary"; }
  void init() override {
    declareProperty("prop1", "value");
    declareProperty("prop2", 1);
    declareProperty("prop3", 10.5);
    std::vector<double> binning{1.0, 0.1, 2.0};
    declareProperty(std::make_unique<ArrayProperty<double>>("Binning", std::move(binning),
                                                            std::make_shared<RebinParamsValidator>()));
  }
  void exec() override {}
};

class ToyAlgorithmThree : public Algorithm {
public:
  ToyAlgorithmThree() : Algorithm() {}
  ~ToyAlgorithmThree() override = default;

  const std::string name() const override { return "ToyAlgorithm"; } ///< Algorithm's name for identification
  int version() const override { return 2; }                         ///< Algorithm's version for identification
  const std::string category() const override { return "Cat;Leopard;Mink"; }
  const std::string alias() const override { return "Dog"; }
  const std::string summary() const override { return "Test summary"; }
  void init() override {
    declareProperty("prop1", "value");
    declareProperty("prop2", 1);
    declareProperty("prop3", 10.5);
  }
  void exec() override {}
};

class CategoryAlgorithm : public Algorithm {
public:
  CategoryAlgorithm() : Algorithm() {}
  ~CategoryAlgorithm() override = default;

  const std::string name() const override { return "CategoryAlgorithm"; } ///< Algorithm's name for identification
  int version() const override { return 1; }                              ///< Algorithm's version for identification
  const std::string category() const override { return "Fake"; }
  const std::string alias() const override { return "CategoryTester"; }
  const std::string summary() const override { return "Test summary"; }
  void init() override {
    declareProperty("prop1", "value");
    declareProperty("prop2", 1);
    declareProperty("prop3", 10.5);
  }
  void exec() override {}
};

class LowerCaseAliasAlgorithm : public Algorithm {
public:
  LowerCaseAliasAlgorithm() : Algorithm() {}
  ~LowerCaseAliasAlgorithm() override = default;
  const std::string name() const override { return "Lower"; }
  int version() const override { return 1; }                          ///< Algorithm's version for identification
  const std::string category() const override { return "Lowercase"; } ///< Algorithm's category for identification
  const std::string alias() const override { return "lower"; }
  const std::string summary() const override { return "Test summary"; }

  void init() override {
    declareProperty("prop1", "value");
    declareProperty("prop2", 1);
  }
  void exec() override {}
};

class CoolAlgorithm1 : public Algorithm {
public:
  CoolAlgorithm1() : Algorithm() {}
  ~CoolAlgorithm1() override = default;
  const std::string name() const override { return "CoolAlgorithm"; }
  int version() const override { return 1; }
  const std::string alias() const override { return "TheCoolestAlgorithm"; }
  const std::string summary() const override { return "Test summary"; }

  void init() override {}
  void exec() override {}
};

class CoolAlgorithm2 : public Algorithm {
public:
  CoolAlgorithm2() : Algorithm() {}
  ~CoolAlgorithm2() override = default;
  const std::string name() const override { return "CoolAlgorithm"; }
  int version() const override { return 2; }
  const std::string summary() const override { return "Test summary"; }

  void init() override {}
  void exec() override {}
};
