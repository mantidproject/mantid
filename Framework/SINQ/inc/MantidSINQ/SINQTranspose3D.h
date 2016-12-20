/**
 * This algorithm takes a 3D MD workspace and performs certain axis transposings
 on it.
 * Essentially this fixes some mess which developed at SINQ when being to hasty
 taking the
 * EMBL detectors into operation.
 *
 * I am afraid that this code has grown to do something else: I suspect that
 Mantids MDHistoWorkspace
 * is acting in F77 storage order. This, then, is also fixed here.
 *
 * Original contributor: Mark Koennecke: mark.koennecke@psi.ch
 *
 * Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

 * This file is part of Mantid.

 * Mantid is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mantid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.

 * File change history is stored at: <https://github.com/mantidproject/mantid>
 * Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
#ifndef TRANSPOSE3D_H_
#define TRANSPOSE3D_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/IMDHistoWorkspace_fwd.h"

class MANTID_SINQ_DLL SINQTranspose3D
    : public Mantid::API::Algorithm,
      public Mantid::API::DeprecatedAlgorithm {
public:
  /// Constructor
  SINQTranspose3D() { this->useAlgorithm("TransposeMD", 1); }
  /// Algorithm's name
  const std::string name() const override { return "Transpose3D"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "SINQ specific MD data reordering";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "MDAlgorithms\\Transforms";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  void doYXZ(Mantid::API::IMDHistoWorkspace_sptr inws);
  void doXZY(Mantid::API::IMDHistoWorkspace_sptr inws);
  void doTRICS(Mantid::API::IMDHistoWorkspace_sptr inws);
  void doAMOR(Mantid::API::IMDHistoWorkspace_sptr inws);

  void copyMetaData(Mantid::API::IMDHistoWorkspace_sptr inws,
                    Mantid::API::IMDHistoWorkspace_sptr outws);
};

#endif /*TRANSPOSE3D_H_*/
