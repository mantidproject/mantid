#pragma once

#include "MantidAPI/Algorithm.h"

class DLLExport MyAlg : public Mantid::API::Algorithm {
public:
  /// (Empty) Constructor
  MyAlg() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~MyAlg() = default;
  /// Algorithm's name
  const std::string name() const override { return "MyAlg"; }
  /// Algorithm's purpose/summary
  const std::string summary() const override { return "Summary"; }
  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "UserDefined"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};
