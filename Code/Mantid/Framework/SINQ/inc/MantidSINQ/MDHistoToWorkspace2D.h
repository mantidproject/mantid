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

#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidDataObjects/Workspace2D.h"

class MANTID_SINQ_DLL MDHistoToWorkspace2D : public Mantid::API::Algorithm {
public:
  /// (Empty) Constructor
  MDHistoToWorkspace2D() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~MDHistoToWorkspace2D() {}
  /// Algorithm's name
  virtual const std::string name() const { return "MDHistoToWorkspace2D"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Flattens a n dimensional MDHistoWorkspace into a Workspace2D with "
           "many spectra";
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

  size_t rank;
  size_t currentSpectra;
  size_t calculateNSpectra(Mantid::API::IMDHistoWorkspace_sptr inws);
  void recurseData(Mantid::API::IMDHistoWorkspace_sptr inWS,
                   Mantid::DataObjects::Workspace2D_sptr outWS,
                   size_t currentDim, Mantid::coord_t *pos);

  void checkW2D(Mantid::DataObjects::Workspace2D_sptr outWS);

  void copyMetaData(Mantid::API::IMDHistoWorkspace_sptr inWS,
                    Mantid::DataObjects::Workspace2D_sptr outWS);
};

#endif /*MDHISTOTOWORKSPACE2D_H_*/
