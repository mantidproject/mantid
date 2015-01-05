/**
 * This Algorithms inverts the dimensions of a MD data set. The
 * application area is when fixing up MD workspaces which had to have the
 * dimensions inverted because they were delivered in C storage order.
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
#ifndef INVERTMDDIM_H_
#define INVERTMDDIM_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

class MANTID_SINQ_DLL InvertMDDim : public Mantid::API::Algorithm {
public:
  /// (Empty) Constructor
  InvertMDDim() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~InvertMDDim() {}
  /// Algorithm's name
  virtual const std::string name() const { return "InvertMDDim"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Inverts dimensions of a MDHistoWorkspace";
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

  void copyMetaData(Mantid::API::IMDHistoWorkspace_sptr inws,
                    Mantid::API::IMDHistoWorkspace_sptr outws);
  void recurseDim(Mantid::API::IMDHistoWorkspace_sptr inWS,
                  Mantid::API::IMDHistoWorkspace_sptr outWS, int currentDim,
                  int *idx, int rank);

  unsigned int calcIndex(Mantid::API::IMDHistoWorkspace_sptr ws, int *dim);
  unsigned int calcInvertedIndex(Mantid::API::IMDHistoWorkspace_sptr ws,
                                 int *dim);
};

#endif /*INVERTMDDIM_H_*/
