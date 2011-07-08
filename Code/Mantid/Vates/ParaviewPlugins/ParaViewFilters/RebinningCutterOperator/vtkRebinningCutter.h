#ifndef _vtkRebinningCutter_h
#define _vtkRebinningCutter_h
#include <boost/scoped_ptr.hpp>
#include "vtkUnstructuredGridAlgorithm.h"
#include "MantidVatesAPI/ThresholdRange.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include <string>

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
    class MDRebinningPresenter;
    class RebinningActionManager;
  }
}


enum SetupStatus{ Pending, SetupDone};
///Type marks wheter clipping is to be applied or ignored
enum Clipping{ ApplyClipping, IgnoreClipping};
///Type marks wheter original extents should be used over box extents.
enum OrignalExtents{ ApplyOriginal, IgnoreOriginal};

class vtkBox;
class vtkImplicitFunction;
class VTK_EXPORT vtkRebinningCutter : public vtkUnstructuredGridAlgorithm//, public Mantid::VATES::MDRebinningView
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
  void SetThresholdRangeStrategyIndex(std::string selectedStrategyIndex);  
  double GetInputMinThreshold();
  double GetInputMaxThreshold();
  /// Paraview Related Commands. See *.xml proxy/property file --------------------------------

  /// Called by presenter to force progress information updating.
  void UpdateAlgorithmProgress(double progress);



  virtual vtkImplicitFunction* getImplicitFunction() const;
  virtual double getMaxThreshold() const;
  virtual double getMinThreshold() const;
  virtual bool getApplyClip() const;
  virtual double getTimeStep() const;
  virtual const char* getAppliedGeometryXML() const;

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

  Mantid::VATES::MDRebinningPresenter* m_presenter;
  std::string m_appliedGeometryXML;

  vtkRebinningCutter(const vtkRebinningCutter&);
  void operator = (const vtkRebinningCutter&);

  void configureThresholdRangeMethod();

  /// handles overwriting of time ranges.
  void setTimeRange(vtkInformationVector* outputVector);

  /// Clip function provided by ClipFunction ProxyProperty
  vtkBox * m_clipFunction;
  /// Cached vtkDataSet. Enables fast visualization where possible.

  /// Flag indicating that the clip boundaries should be use to construct the rebinning region.
  Clipping m_clip;
  /// Original extents should be used.
  OrignalExtents m_originalExtents;
  /// Flag indicating whether set up has occurred or not
  SetupStatus m_setup;
  /// Flag containing the timestep.
  int m_timestep;
  Mantid::signal_t m_thresholdMax;
  Mantid::signal_t m_thresholdMin;
  /// Threshold range calculator.
  Mantid::VATES::ThresholdRange_scptr m_ThresholdRange;
  
  int m_thresholdMethodIndex;

};
#endif
