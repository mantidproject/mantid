// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * SetCrystalLocation.h
 *
 *  Created on: Dec 12, 2018
 *      Author: Brendan Sullivan
 */

#ifndef SETCRYSTALLOCATION_H_
#define SETCRYSTALLOCATION_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {

/** SetCrystalLocation

Description:
This algorithm basically indexes peaks with the crystal orientation matrix
stored in the peaks workspace.
The optimization is on the goniometer settings for the runs in the peaks
workspace and also the sample
orientation .
@author Ruth Mikkelson, SNS,ORNL
@date 01/26/2013
*/
class DLLExport SetCrystalLocation : public API::Algorithm {
public:
  const std::string name() const override {
    return "SetCrystalLocation";
  };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm sets the sample location of the "
           "input peaks workspace.";
  }

  int version() const override { return 1; };

  const std::string category() const override {
    return "Crystal\\Corrections";
  };

private:
  void init() override;

  void exec() override;
};
} // namespace Crystal
} // namespace Mantid

#endif /* SETCRYSTALLOCATION_H_ */
