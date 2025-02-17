// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/NexusHDF5Descriptor.h"

#include <H5Cpp.h>

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
*/

/* Base class to Load a sassena dataset into a MatrixWorkspace
 * Derived implementations will load different scattering functions

class LoadDataSet
{

};
*/

class MANTID_DATAHANDLING_DLL LoadSassena : public API::IFileLoader<Kernel::NexusHDF5Descriptor> {
public:
  /// Algorithm's name
  const std::string name() const override { return "LoadSassena"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return " load a Sassena output file into a group workspace."; }

  /// Algorithm's version
  int version() const override { return 1; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Sassena"; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusHDF5Descriptor &descriptor) const override;

protected:
  /// Add a workspace to the group and register in the analysis data service
  void registerWorkspace(const API::WorkspaceGroup_sptr &gws, const std::string &wsName,
                         const DataObjects::Workspace2D_sptr &ws, const std::string &description);
  /// Read info about one HDF5 dataset, log if error
  void dataSetInfo(const H5::H5File &h5file, const std::string &setName, hsize_t *dims) const;
  /// Read dataset data to a buffer ot type double
  void dataSetDouble(const H5::H5File &h5file, const std::string &setName, std::vector<double> &buf);
  /// Load qvectors dataset, calculate modulus of vectors
  HistogramData::Points loadQvectors(const H5::H5File &h5file, const API::WorkspaceGroup_sptr &gws,
                                     std::vector<int> &sorting_indexes);
  /// Load structure factor asa function of q-vector modulus
  void loadFQ(const H5::H5File &h5file, const API::WorkspaceGroup_sptr &gws, const std::string &setName,
              const HistogramData::Points &qvmod, const std::vector<int> &sorting_indexes);
  /// Load time-dependent structure factor
  void loadFQT(const H5::H5File &h5file, const API::WorkspaceGroup_sptr &gws, const std::string &setName,
               const HistogramData::Points &qvmod, const std::vector<int> &sorting_indexes);

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
