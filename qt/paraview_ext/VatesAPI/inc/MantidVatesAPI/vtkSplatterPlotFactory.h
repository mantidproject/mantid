// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_vtkSplatterPlotFactory_H_
#define MANTID_VATES_vtkSplatterPlotFactory_H_

#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidAPI/IMDHistoWorkspace_fwd.h"
#include "MantidAPI/IMDNode.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidVatesAPI/MetaDataExtractorUtils.h"
#include "MantidVatesAPI/MetadataJsonManager.h"
#include "MantidVatesAPI/VatesConfigurations.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <vtkPoints.h>

namespace Mantid {
namespace VATES {

using SigFuncIMDNodePtr = Mantid::signal_t (Mantid::API::IMDNode::*)() const;

/**
 * Factory that creates a simple "splatter plot" data set composed of points
 * of a selection of the events in a MDEventWorkspace.
 *
 * @date August 16, 2011
 *
 *
 */

class DLLExport vtkSplatterPlotFactory : public vtkDataSetFactory {
public:
  /// Constructor
  vtkSplatterPlotFactory(const std::string &scalarName,
                         const size_t numPoints = 150000,
                         const double percentToUse = 5.0);

  /// Destructor
  ~vtkSplatterPlotFactory() override;

  /// Factory Method. Should also handle delegation to successors.
  vtkSmartPointer<vtkDataSet>
  create(ProgressAction &progressUpdating) const override;

  /// Initalize with a target workspace.
  void initialize(const Mantid::API::Workspace_sptr &workspace) override;

  /// Get the name of the type.
  std::string getFactoryTypeName() const override {
    return "vtkSplatterPlotFactory";
  }

  /// Set upper limit on the number of points that will be plotted
  virtual void SetNumberOfPoints(size_t points);

  /// Set percentage of boxes from which points will be plotted
  virtual void SetPercentToUse(double percentToUse);

  /// Set the time value.
  void setTime(double timeStep);

  /// Getter for the instrument
  virtual const std::string &getInstrument();

  /// Set the appropriate field data
  virtual void setMetadata(vtkFieldData *fieldData, vtkDataSet *dataSet);

private:
  template <typename MDE, size_t nd>
  void doCreate(
      typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws) const;

  /// Check if the MDHisto workspace is 3D or 4D in nature
  bool doMDHisto4D(const Mantid::API::IMDHistoWorkspace *workspace) const;

  /// Generate the vtkDataSet from the objects input MDHistoWorkspace
  void doCreateMDHisto(const Mantid::API::IMDHistoWorkspace &workspace) const;

  /// Set the signals and the valid points which are to be displayed
  signal_t extractScalarSignal(const Mantid::API::IMDHistoWorkspace &workspace,
                               bool do4D, const int x, const int y,
                               const int z) const;

  /// Template Method pattern to validate the factory before use.
  void validate() const override;

  /// Add metadata
  void addMetadata() const;

  /// Scalar name to provide on dataset.
  const std::string m_scalarName;

  /// Member workspace to generate vtkdataset from.
  Mantid::API::IMDWorkspace_sptr m_workspace;

  /// Approximate number of points to plot
  size_t m_numPoints;

  /// Size of the initial portion of the sorted list of boxes to use.
  double m_percentToUse;

  /// Flag indicating whether or not the sorted list must be built
  mutable bool m_buildSortedList;

  /// Save name of current workspace so we can re-sort if it changes
  mutable std::string m_wsName;

  /// Data set that will be generated
  mutable vtkSmartPointer<vtkDataSet> dataSet;

  /// We are slicing down from > 3 dimensions
  mutable bool slice;

  /// Mask for choosing along which dimensions to slice
  mutable std::unique_ptr<bool[]> sliceMask;

  /// Implicit function to define which boxes to render.
  mutable boost::shared_ptr<Mantid::Geometry::MDImplicitFunction>
      sliceImplicitFunction;

  /// Variable to hold sorted list, so sort doesn't have to be repeated
  mutable std::vector<Mantid::API::IMDNode *> m_sortedBoxes;

  /// Time value.
  double m_time;

  /// Instrument
  mutable std::string m_instrument;

  /// Meta data extractor
  boost::scoped_ptr<MetaDataExtractorUtils> m_metaDataExtractor;

  /// Meata data json manager
  boost::scoped_ptr<MetadataJsonManager> m_metadataJsonManager;

  /// Vates configuration
  boost::scoped_ptr<VatesConfigurations> m_vatesConfigurations;

  /// Sort boxes by normalized signal value
  virtual void sortBoxesByDecreasingSignal(const bool VERBOSE) const;
};
} // namespace VATES
} // namespace Mantid

#endif
