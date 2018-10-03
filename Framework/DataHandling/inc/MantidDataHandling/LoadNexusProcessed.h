// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADNEXUSPROCESSED_H_
#define MANTID_DATAHANDLING_LOADNEXUSPROCESSED_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidKernel/cow_ptr.h"

#include "MantidNexus/NexusClasses.h"

#include <map>

namespace NeXus {
class File;
}

namespace Mantid {

namespace DataHandling {

/**

Loads a workspace from a NeXus Processed entry in a NeXus file.
LoadNexusProcessed is an algorithm and as such inherits
from the Algorithm class, via DataHandlingCommand, and overrides
the init() & exec() methods.

Required Properties:
<UL>
<LI> Filename - The name of the input NeXus file (must exist) </LI>
<LI> InputWorkspace - The name of the workspace to put the data </LI>
</UL>
*/
class DLLExport LoadNexusProcessed
    : public API::IFileLoader<Kernel::NexusDescriptor> {

public:
  /// Default constructor
  LoadNexusProcessed();
  /// Destructor
  ~LoadNexusProcessed() override;
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadNexusProcessed"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The LoadNexusProcessed algorithm will read the given Nexus "
           "Processed data file containing a Mantid Workspace. The data is "
           "placed in the named workspace. LoadNexusProcessed may be invoked "
           "by LoadNexus if it is given a Nexus file of this type.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"LoadNexus"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Nexus"; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  /// Validates the input Min < Max and Max < Maximum_Int
  std::map<std::string, std::string> validateInputs() override;
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;

  /// Create the workspace name if it's part of a group workspace
  std::string buildWorkspaceName(const std::string &name,
                                 const std::string &baseName, size_t wsIndex);

  /// Add an index to the name if it already exists in the workspace
  void correctForWorkspaceNameClash(std::string &wsName);

  /// Extract the workspace name
  std::vector<std::string> extractWorkspaceNames(Mantid::NeXus::NXRoot &root,
                                                 size_t nWorkspaceEntries);

  /// Load the workspace name attribute if it exists
  std::string loadWorkspaceName(Mantid::NeXus::NXRoot &root,
                                const std::string &entry_name);

  /// Load a single entry
  API::Workspace_sptr loadEntry(Mantid::NeXus::NXRoot &root,
                                const std::string &entry_name,
                                const double &progressStart,
                                const double &progressRange);

  API::Workspace_sptr loadTableEntry(Mantid::NeXus::NXEntry &entry);

  /// Load a numeric column to the TableWorkspace.
  template <typename ColumnType, typename NexusType>
  void loadNumericColumn(const Mantid::NeXus::NXData &tableData,
                         const std::string &dataSetName,
                         const API::ITableWorkspace_sptr &tableWs,
                         const std::string &columnType);

  /// Loads a vector column to the TableWorkspace.
  template <typename Type>
  void loadVectorColumn(const Mantid::NeXus::NXData &tableData,
                        const std::string &dataSetName,
                        const API::ITableWorkspace_sptr &tableWs,
                        const std::string &columnType);

  /// Loads a V3D column to the TableWorkspace.
  void loadV3DColumn(Mantid::NeXus::NXDouble &data,
                     const API::ITableWorkspace_sptr &tableWs);

  API::Workspace_sptr loadPeaksEntry(Mantid::NeXus::NXEntry &entry);

  API::MatrixWorkspace_sptr loadEventEntry(Mantid::NeXus::NXData &wksp_cls,
                                           Mantid::NeXus::NXDouble &xbins,
                                           const double &progressStart,
                                           const double &progressRange);
  API::MatrixWorkspace_sptr
  loadNonEventEntry(Mantid::NeXus::NXData &wksp_cls,
                    Mantid::NeXus::NXDouble &xbins, const double &progressStart,
                    const double &progressRange,
                    const Mantid::NeXus::NXEntry &mtd_entry, const int xlength,
                    std::string &workspaceType);

  /// Read the data from the sample group
  void readSampleGroup(Mantid::NeXus::NXEntry &mtd_entry,
                       API::MatrixWorkspace_sptr local_workspace);
  /// Add a property to the sample object
  bool addSampleProperty(Mantid::NeXus::NXMainClass &sample_entry,
                         const std::string &entryName,
                         API::Sample &sampleDetails);
  /// Read the spectra
  void readInstrumentGroup(Mantid::NeXus::NXEntry &mtd_entry,
                           API::MatrixWorkspace_sptr local_workspace);
  /// Splits a string of exactly three words into the separate words
  void getWordsInString(const std::string &words3, std::string &w1,
                        std::string &w2, std::string &w3);
  /// Splits a string of exactly four words into the separate words
  void getWordsInString(const std::string &words4, std::string &w1,
                        std::string &w2, std::string &w3, std::string &w4);

  /// Read the bin masking information
  void readBinMasking(Mantid::NeXus::NXData &wksp_cls,
                      API::MatrixWorkspace_sptr local_workspace);

  /// Load a block of data into the workspace where it is assumed that the x
  /// bins have already been cached
  void loadBlock(Mantid::NeXus::NXDataSetTyped<double> &data,
                 Mantid::NeXus::NXDataSetTyped<double> &errors,
                 Mantid::NeXus::NXDataSetTyped<double> &farea, bool hasFArea,
                 Mantid::NeXus::NXDouble &xErrors, bool hasXErrors,
                 int blocksize, int nchannels, int &hist,
                 API::MatrixWorkspace_sptr local_workspace);

  /// Load a block of data into the workspace where it is assumed that the x
  /// bins have already been cached
  void loadBlock(Mantid::NeXus::NXDataSetTyped<double> &data,
                 Mantid::NeXus::NXDataSetTyped<double> &errors,
                 Mantid::NeXus::NXDataSetTyped<double> &farea, bool hasFArea,
                 Mantid::NeXus::NXDouble &xErrors, bool hasXErrors,
                 int blocksize, int nchannels, int &hist, int &wsIndex,
                 API::MatrixWorkspace_sptr local_workspace);
  /// Load a block of data into the workspace
  void loadBlock(Mantid::NeXus::NXDataSetTyped<double> &data,
                 Mantid::NeXus::NXDataSetTyped<double> &errors,
                 Mantid::NeXus::NXDataSetTyped<double> &farea, bool hasFArea,
                 Mantid::NeXus::NXDouble &xErrors, bool hasXErrors,
                 Mantid::NeXus::NXDouble &xbins, int blocksize, int nchannels,
                 int &hist, int &wsIndex,
                 API::MatrixWorkspace_sptr local_workspace);

  /// Load the data from a non-spectra axis (Numeric/Text) into the workspace
  void loadNonSpectraAxis(API::MatrixWorkspace_sptr local_workspace,
                          Mantid::NeXus::NXData &data);

  /// Validates the optional 'spectra to read' properties, if they have been set
  void checkOptionalProperties(const std::size_t numberofspectra);

  /// calculates the workspace size
  std::size_t calculateWorkspaceSize(const std::size_t numberofspectra,
                                     bool gen_filtered_list = false);

  /// Accellerated multiperiod loading
  Mantid::API::Workspace_sptr doAccelleratedMultiPeriodLoading(
      Mantid::NeXus::NXRoot &root, const std::string &entryName,
      Mantid::API::MatrixWorkspace_sptr &tempMatrixWorkspace,
      const size_t nWorkspaceEntries, const size_t p);

  /// Does the current workspace have uniform binning
  bool m_shared_bins;
  /// The cached x binning if we have bins
  HistogramData::BinEdges m_xbins;
  /// Numeric values for the second axis, if applicable
  MantidVec m_axis1vals;

  /// Flag set if list of spectra to save is specifed
  bool m_list;
  /// Flag set if interval of spectra to write is set
  bool m_interval;

  /// The value of the spectrum_min property
  int m_spec_min;
  /// The value of the spectrum_max property
  int m_spec_max;

  /// The value of the spectrum_list property
  std::vector<int> m_spec_list;
  /// list of spectra filtered by min/max/list, currently
  /// used only when loading data into event_workspace
  std::vector<int> m_filtered_spec_idxs;

  // C++ interface to the NXS file
  ::NeXus::File *m_cppFile;
};
/// to sort the algorithmhistory vector
bool UDlesserExecCount(Mantid::NeXus::NXClassInfo elem1,
                       Mantid::NeXus::NXClassInfo elem2);

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADNEXUSPROCESSED_H_*/
