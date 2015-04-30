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
#include "MantidAPI/IMDHistoWorkspace.h"

class MANTID_SINQ_DLL SINQTranspose3D : public Mantid::API::Algorithm {
public:
  /// (Empty) Constructor
  SINQTranspose3D() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~SINQTranspose3D() {}
  /// Algorithm's name
  virtual const std::string name() const { return "Transpose3D"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "SINQ specific MD data reordering";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MDAlgorithms"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

  void doYXZ(Mantid::API::IMDHistoWorkspace_sptr inws);
  void doXZY(Mantid::API::IMDHistoWorkspace_sptr inws);
  void doTRICS(Mantid::API::IMDHistoWorkspace_sptr inws);
  void doAMOR(Mantid::API::IMDHistoWorkspace_sptr inws);

  void copyMetaData(Mantid::API::IMDHistoWorkspace_sptr inws,
                    Mantid::API::IMDHistoWorkspace_sptr outws);
};

#endif /*TRANSPOSE3D_H_*/
