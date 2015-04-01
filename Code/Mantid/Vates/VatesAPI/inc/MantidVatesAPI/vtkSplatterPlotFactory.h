#ifndef MANTID_VATES_vtkSplatterPlotFactory_H_
#define MANTID_VATES_vtkSplatterPlotFactory_H_

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidVatesAPI/MetaDataExtractorUtils.h"
#include "MantidAPI/IMDNode.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidVatesAPI/ThresholdRange.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidVatesAPI/MetaDataExtractorUtils.h"
#include "MantidVatesAPI/MetadataJsonManager.h"
#include "MantidVatesAPI/VatesConfigurations.h"
#include <vtkPoints.h>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>


using Mantid::DataObjects::MDEventWorkspace;

namespace Mantid
{
namespace VATES
{

/**
 * Factory that creates a simple "splatter plot" data set composed of points
 * of a selection of the events in a MDEventWorkspace.
 *
 * @date August 16, 2011
 *
 * Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source
 *
 * This file is part of Mantid.
 *
 * Mantid is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mantid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * File change history is stored at: <https://github.com/mantidproject/mantid>
 * Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

class DLLExport vtkSplatterPlotFactory : public vtkDataSetFactory
{
public:

  /// Constructor
  vtkSplatterPlotFactory(ThresholdRange_scptr thresholdRange,
                         const std::string& scalarName,
                         const size_t numPoints = 150000,
                         const double percentToUse = 5.0);

  /// Destructor
  virtual ~vtkSplatterPlotFactory();

  /// Factory Method. Should also handle delegation to successors.
  virtual vtkDataSet* create(ProgressAction& progressUpdating) const;
  
  /// Initalize with a target workspace.
  virtual void initialize(Mantid::API::Workspace_sptr);

  /// Get the name of the type.
  virtual std::string getFactoryTypeName() const
  {
    return "vtkSplatterPlotFactory";
  }

  /// Set upper limit on the number of points that will be plotted
  virtual void SetNumberOfPoints(size_t points);

  /// Set percentage of boxes from which points will be plotted
  virtual void SetPercentToUse(double percentToUse);

  /// Set the time value.
  void setTime(double timeStep);

  /// Get the max value of the data set
  virtual double getMinValue();

  /// Get the min value of the data set
  virtual double getMaxValue();

  /// Getter for the instrument
  virtual const std::string& getInstrument();

  /// Set the appropriate field data
  virtual void setMetadata(vtkFieldData* fieldData, vtkDataSet* dataSet);

private:

  template<typename MDE, size_t nd>
  void doCreate(typename MDEventWorkspace<MDE, nd>::sptr ws) const;

  ///Check if the MDHisto workspace is 3D or 4D in nature
  bool doMDHisto4D(Mantid::API::IMDHistoWorkspace_sptr workspace) const;

  /// Generate the vtkDataSet from the objects input MDHistoWorkspace
  void doCreateMDHisto(Mantid::API::IMDHistoWorkspace_sptr workspace) const;

  /// Set the signals and the valid points which are to be displayed
  signal_t extractScalarSignal(Mantid::API::IMDHistoWorkspace_sptr workspace, bool do4D, const int x, const int y, const int z) const;

  /// Template Method pattern to validate the factory before use.
  virtual void validate() const;

  /// Add metadata
  void addMetadata() const;

  /// Threshold range strategy.
  ThresholdRange_scptr m_thresholdRange;

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
  mutable vtkDataSet *dataSet;

  /// We are slicing down from > 3 dimensions
  mutable bool slice;

  /// Mask for choosing along which dimensions to slice
  mutable bool *sliceMask;

  /// Implicit function to define which boxes to render.
  mutable Mantid::Geometry::MDImplicitFunction *sliceImplicitFunction;

  /// Variable to hold sorted list, so sort doesn't have to be repeated
  mutable std::vector< Mantid::API::IMDNode * > m_sortedBoxes;

  /// Time value.
  double m_time;

  /// Min data value
  mutable double m_minValue;

  /// Max data value;
  mutable double m_maxValue;

  /// Instrument
  mutable std::string m_instrument;

  /// Meta data extractor
  boost::scoped_ptr<MetaDataExtractorUtils> m_metaDataExtractor;

  /// Meata data json manager
  boost::scoped_ptr<MetadataJsonManager> m_metadataJsonManager;

  /// Vates configuration
  boost::scoped_ptr<VatesConfigurations> m_vatesConfigurations;
};

}
}


#endif
