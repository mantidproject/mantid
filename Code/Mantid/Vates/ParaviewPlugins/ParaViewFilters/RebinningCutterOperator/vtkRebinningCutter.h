#ifndef _vtkRebinningCutter_h
#define _vtkRebinningCutter_h
#include <boost/scoped_ptr.hpp>
#include "vtkUnstructuredGridAlgorithm.h"
#include "MantidVatesAPI/RebinningCutterPresenter.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"

/**
 *
 * Paraview Filter implementing rebinning/cutting operations.

    @author Owen Arnold, RAL ISIS
    @date 18/03/2011

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

namespace Mantid
{
  namespace MDAlgorithms
  {
    //Forward declaration
    class BoxImplicitFunction;
  }
  namespace VATES
  {
    class RebinningActionManager;
  }
}
///Typedef shared pointer to a box implicit function.
typedef boost::shared_ptr<Mantid::MDAlgorithms::BoxImplicitFunction> BoxFunction_sptr;
///Typedef scoped pointer to a rebinning action manager.
typedef boost::scoped_ptr<Mantid::VATES::RebinningActionManager> RebinningActionManager_scptr;
///Type marks setup status
enum SetupStatus{ Pending, SetupDone};
///Type marks wheter clipping is to be applied or ignored
enum Clipping{ ApplyClipping, IgnoreClipping};
///Type marks wheter original extents should be used over box extents.
enum OrignalExtents{ ApplyOriginal, IgnoreOriginal};

class vtkBox;
class VTK_EXPORT vtkRebinningCutter : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkRebinningCutter *New();
  vtkTypeRevisionMacro(vtkRebinningCutter,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Paraview Related Commands. See *.xml proxy/property file --------------------------------
  void SetClipFunction( vtkImplicitFunction * func);
  void SetMaxThreshold(double maxThreshold);
  void SetMinThreshold(double minThreshold);
  void SetAppliedGeometryXML(std::string xml);
  void SetApplyClip(int applyClip);
  const char* GetInputGeometryXML();
  /// Paraview Related Commands. See *.xml proxy/property file --------------------------------

  /// Called by presenter to force progress information updating.
  void UpdateAlgorithmProgress(double progress);

protected:

  ///Constructor
  vtkRebinningCutter();

  ///Destructor
  ~vtkRebinningCutter();

  ///Request information prior to execution.
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  ///Execution.
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  ///Update extents.
  int RequestUpdateExtent(vtkInformation*,
                                    vtkInformationVector**, vtkInformationVector* );

  ///Handle time variation.
  unsigned long GetMTime();

  ///Overriden fill imports so that vtkDataSets may be specified.
  int FillInputPortInformation(int port, vtkInformation* info);

private:

    /**
   Detect wheter x dimension is available.
   @return true available, false otherwise.
 */
  bool hasXDimension() const
  {
    return NULL != m_appliedXDimension.get();
  }

  /**
   Detect wheter y dimension is available.
   @return true available, false otherwise.
 */
  bool hasYDimension() const
  {
    return NULL != m_appliedYDimension.get();
  }

  /**
   Detect wheter z dimension is available.
   @return true available, false otherwise.
 */
  bool hasZDimension() const
  {
    return NULL != m_appliedZDimension.get();
  }

  /**
   Detect wheter t dimension is available.
   @return true available, false otherwise.
 */
  bool hasTDimension() const
  {
    return NULL != m_appliedTDimension.get();
  }

  vtkRebinningCutter(const vtkRebinningCutter&);

  void operator = (const vtkRebinningCutter&);

  /// Executor pefroms the logic associated with running rebinning operations.
  Mantid::VATES::RebinningCutterPresenter m_presenter;


  BoxFunction_sptr constructBox(vtkDataSet* ) const;

  /// Selects the dataset factory to use.
  Mantid::VATES::vtkDataSetFactory_sptr createDataSetFactory(Mantid::API::IMDWorkspace_sptr spRebinnedWs) const;

  /// DataSet handles re-mappings, so allows regeneration of a visual dataset in rapid time.
  Mantid::VATES::vtkDataSetFactory_sptr createQuickChangeDataSetFactory(Mantid::API::IMDWorkspace_sptr spRebinnedWs) const;

  /// Dataset does not handle remappings and is therefore may be generated quickly.
  Mantid::VATES::vtkDataSetFactory_sptr createQuickRenderDataSetFactory(Mantid::API::IMDWorkspace_sptr spRebinnedWs) const;

  /// Decides on the necessary iteration action that is to be performed.
  void determineAnyCommonExecutionActions(const int timestep, BoxFunction_sptr box);

  /// creates a hash of arguments considered as flags for redrawing the visualisation dataset.
  std::string createRedrawHash() const;

  /// Sets the rebinning action to rebin if the number of bins has changed on a dimension.
  void formulateRequestUsingNBins(Mantid::VATES::Dimension_sptr newDim);

  /// handles overwriting of time ranges.
  void setTimeRange(vtkInformationVector* outputVector);

  /// Clip function provided by ClipFunction ProxyProperty
  vtkBox * m_clipFunction;
  /// Cached vtkDataSet. Enables fast visualization where possible.
  vtkDataSet* m_cachedVTKDataSet;
  /// Arguments that cause redrawing are hashed and cached for rapid comparison regarding any changes.
  std::string m_cachedRedrawArguments;
  /// Flag indicating that the clip boundaries should be use to construct the rebinning region.
  Clipping m_clip;
  /// Original extents should be used.
  OrignalExtents m_originalExtents;
  /// Flag indicating whether set up has occurred or not
  SetupStatus m_setup;
  /// Flag containing the timestep.
  int m_timestep;
  /// Threshold maximum for signal values to be rendered as cells.
  double m_thresholdMax;
  /// Threshold minimum for signal values to be rendered as cells.
  double m_thresholdMin;
  /// the dimension information applied to the xDimension Mapping.
  Mantid::VATES::Dimension_sptr m_appliedXDimension;
  /// the dimension information applied to the yDimension Mapping.
  Mantid::VATES::Dimension_sptr m_appliedYDimension;
  /// the dimension information applied to the zDimension Mapping.
  Mantid::VATES::Dimension_sptr m_appliedZDimension;
  /// the dimension information applied to the tDimension Mapping.
  Mantid::VATES::Dimension_sptr m_appliedTDimension;
  /// holds all applied dimensions.
  Mantid::Geometry::VecIMDDimension_sptr m_vecAppliedDimensions;
  /// Manages the precedence of rebinning related actions.
  RebinningActionManager_scptr m_actionRequester;
  /// Box implicit function, used to determine when the clipping has changed.
  BoxFunction_sptr m_box;


};
#endif
