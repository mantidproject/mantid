/**
 * This algorithm flattens a MDHistoWorkspace to a Workspace2D. Mantid has far
 more tools
 * to deal with W2D then for MD ones.
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
#ifndef MDHISTOTOWORKSPACE2D_H_
#define MDHISTOTOWORKSPACE2D_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDHistoWorkspace_fwd.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidSINQ/DllConfig.h"

class MANTID_SINQ_DLL MDHistoToWorkspace2D : public Mantid::API::Algorithm {
public:
  /// Default constructor
  MDHistoToWorkspace2D();
  /// Algorithm's name
  const std::string name() const override { return "MDHistoToWorkspace2D"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Flattens a n dimensional MDHistoWorkspace into a Workspace2D with "
           "many spectra";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertMDHistoToMatrixWorkspace"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "MDAlgorithms\\Transforms";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  size_t m_rank;
  size_t m_currentSpectra;
  size_t calculateNSpectra(Mantid::API::IMDHistoWorkspace_sptr inws);
  void recurseData(Mantid::API::IMDHistoWorkspace_sptr inWS,
                   Mantid::DataObjects::Workspace2D_sptr outWS,
                   size_t currentDim, Mantid::coord_t *pos);

  void checkW2D(Mantid::DataObjects::Workspace2D_sptr outWS);

  void copyMetaData(Mantid::API::IMDHistoWorkspace_sptr inWS,
                    Mantid::DataObjects::Workspace2D_sptr outWS);
};

#endif /*MDHISTOTOWORKSPACE2D_H_*/
