#ifndef _vtkRebinningCutter_h
#define _vtkRebinningCutter_h
#include "vtkUnstructuredGridAlgorithm.h"
#include "MantidVatesAPI/RebinningCutterPresenter.h"

class vtkImplicitFunction;
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
  void SetAppliedXDimensionXML(std::string xml);
  void SetAppliedYDimensionXML(std::string xml);
  void SetAppliedZDimensionXML(std::string xml);
  void SetAppliedTDimensionXML(std::string xml);
  const char* GetInputGeometryXML();
  /// Paraview Related Commands. See *.xml proxy/property file --------------------------------

  /// Called by presenter to force progress information updating.
  void UpdateAlgorithmProgress(int progressPercent);

protected:

  vtkRebinningCutter();

  ~vtkRebinningCutter();

  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int RequestUpdateExtent(vtkInformation*,
                                    vtkInformationVector**,
                                    vtkInformationVector* );
  unsigned long GetMTime();

  int FillInputPortInformation(int port, vtkInformation* info);

private:

  vtkRebinningCutter(const vtkRebinningCutter&);

  void operator = (const vtkRebinningCutter&);

  /// Executor peforms the logic associated with running rebinning operations.
  Mantid::VATES::RebinningCutterPresenter m_presenter;
  /// Get the x dimension form the input dataset.
  Mantid::VATES::Dimension_sptr getDimensionX(vtkDataSet* in_ds) const;
  /// Get the y dimension form the input dataset.
  Mantid::VATES::Dimension_sptr getDimensionY(vtkDataSet* in_ds) const;
  /// Get the z dimension form the input dataset.
  Mantid::VATES::Dimension_sptr getDimensionZ(vtkDataSet* in_ds) const;
  /// Get the t dimension form the input dataset.
  Mantid::VATES::Dimension_sptr getDimensiont(vtkDataSet* in_ds) const;

  boost::shared_ptr<Mantid::API::ImplicitFunction> constructBox(
          Mantid::VATES::Dimension_sptr spDimX,
          Mantid::VATES::Dimension_sptr spDimY,
          Mantid::VATES::Dimension_sptr spDimZ) const;

  /// Helper method. Selects the dataset factory to use.
  boost::shared_ptr<Mantid::VATES::vtkDataSetFactory> createDataSetFactory(Mantid::MDDataObjects::MDWorkspace_sptr spRebinnedWs) const;

  /// Decides on the necessary iteration action that is to be performed.
  Mantid::VATES::RebinningIterationAction decideIterationAction(const int timestep);

  /// creates a hash of arguments considered as flags for redrawing the visualisation dataset.
  std::string createRedrawHash() const;

  /// Clip function provided by ClipFunction ProxyProperty
  vtkImplicitFunction* m_clipFunction;
  /// Cached vtkDataSet. Enables fast visualisation where possible.
  vtkDataSet* m_cachedVTKDataSet;
  /// Arguments that cause redrawing are hashed and cached for rapid comparison regarding any changes.
  std::string m_cachedRedrawArguments;
  /// Flag indicating that rebinning must be performed.
  bool m_Rebin;
  /// Flag indicating whether set up has occured or not.
  bool m_isSetup;
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

};
#endif
