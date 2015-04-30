/**
 * This algorithm takes a MDHistoWorkspace and allows to select a slab out of
 * it which is storeed into the result workspace.
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
#ifndef SLICEMDHISTO_H_
#define SLICEMDHISTO_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDHistoWorkspace.h"

class MANTID_SINQ_DLL SliceMDHisto : public Mantid::API::Algorithm {
public:
  /// (Empty) Constructor
  SliceMDHisto() : Mantid::API::Algorithm(), dim() {}
  /// Virtual destructor
  virtual ~SliceMDHisto() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SliceMDHisto"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Extracts a hyperslab of data from a MDHistoWorkspace";
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

  unsigned int rank;
  std::vector<int> dim;
  void cutData(Mantid::API::IMDHistoWorkspace_sptr inWS,
               Mantid::API::IMDHistoWorkspace_sptr outWS,
               Mantid::coord_t *sourceDim, Mantid::coord_t *targetDim,
               std::vector<int> start, std::vector<int> end, unsigned int dim);

  void copyMetaData(Mantid::API::IMDHistoWorkspace_sptr inws,
                    Mantid::API::IMDHistoWorkspace_sptr outws);
};

#endif /*SLICEMDHISTO_H_*/
