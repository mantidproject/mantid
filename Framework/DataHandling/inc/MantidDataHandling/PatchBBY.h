// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//---------------------------------------------------
// Includes
//---------------------------------------------------

#include "LoadANSTOHelper.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {
/*
Patches a Bilby data file. Implements API::Algorithm and its file check methods
to recognise a file as the one containing Bilby data.

@author David Mannicke (ANSTO), Anders Markvardsen (ISIS), Roman Tolchenov
(Tessella plc)
@date 22/01/2016
*/

class DLLExport PatchBBY : public API::Algorithm {
public:
  // description
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LoadBBY"}; }
  const std::string name() const override { return "PatchBBY"; }
  const std::string category() const override { return "DataHandling\\ANSTO"; }
  const std::string summary() const override { return "Patches a BilBy data file."; }

protected:
  // initialisation
  void init() override;
  // execution
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
