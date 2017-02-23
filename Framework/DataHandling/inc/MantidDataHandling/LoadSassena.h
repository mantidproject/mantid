#ifndef MANTID_DATAHANDLING_LOADSASSENA_H_
#define MANTID_DATAHANDLING_LOADSASSENA_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidDataObjects/Workspace2D.h"
#include <hdf5.h>

namespace Mantid {

namespace DataHandling {

/** Load Sassena Output files.

Required Properties:
<UL>
<LI> Filename - The name of and path to the Sassena file </LI>
<LI> Workspace - The name of the group workspace to output
</UL>

@author Jose Borreguero
@date 2012-04-24

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Base class to Load a sassena dataset into a MatrixWorkspace
 * Derived implementations will load different scattering functions

class LoadDataSet
{

};
*/

class DLLExport LoadSassena : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  /// Algorithm's name
  const std::string name() const override { return "LoadSassena"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return " load a Sassena output file into a group workspace.";
  }

  /// Algorithm's version
  int version() const override { return 1; }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling\\Sassena";
  }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

protected:
  /// Add a workspace to the group and register in the analysis data service
  void registerWorkspace(API::WorkspaceGroup_sptr gws, const std::string wsName,
                         DataObjects::Workspace2D_sptr ws,
                         const std::string &description);
  /// Read info about one HDF5 dataset, log if error
  herr_t dataSetInfo(const hid_t &h5file, const std::string setName,
                     hsize_t *dims) const;
  /// Read dataset data to a buffer ot type double
  herr_t dataSetDouble(const hid_t &h5file, const std::string setName,
                       std::vector<double> &buf);
  /// Load qvectors dataset, calculate modulus of vectors
  HistogramData::Points loadQvectors(const hid_t &h5file,
                                     API::WorkspaceGroup_sptr gws,
                                     std::vector<int> &sorting_indexes);
  /// Load structure factor asa function of q-vector modulus
  void loadFQ(const hid_t &h5file, API::WorkspaceGroup_sptr gws,
              const std::string setName, const HistogramData::Points &qvmod,
              const std::vector<int> &sorting_indexes);
  /// Load time-dependent structure factor
  void loadFQT(const hid_t &h5file, API::WorkspaceGroup_sptr gws,
               const std::string setName, const HistogramData::Points &qvmod,
               const std::vector<int> &sorting_indexes);

private:
  /// Initialization code
  void init() override; // Overwrites Algorithm method.
  /// Execution code
  void exec() override; // Overwrites Algorithm method
  /// Loads one dataset
  void loadSet(const std::string &version, const std::string &setName);

  /// valid datasets
  std::vector<std::string> m_validSets;
  /// name and path of input file
  std::string m_filename;

}; // class LoadSassena

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADSASSENA_H_*/
