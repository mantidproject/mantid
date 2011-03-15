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
  void SetClipFunction( vtkImplicitFunction * func);
  void SetMaxThreshold(double maxThreshold);
  void SetMinThreshold(double minThreshold);
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

  Mantid::VATES::Dimension_sptr getDimensionX(vtkDataSet* in_ds) const;
  Mantid::VATES::Dimension_sptr getDimensionY(vtkDataSet* in_ds) const;
  Mantid::VATES::Dimension_sptr getDimensionZ(vtkDataSet* in_ds) const;
  Mantid::VATES::Dimension_sptr getDimensiont(vtkDataSet* in_ds) const;

  boost::shared_ptr<Mantid::API::ImplicitFunction> constructBox(
          Mantid::VATES::Dimension_sptr spDimX,
          Mantid::VATES::Dimension_sptr spDimY,
          Mantid::VATES::Dimension_sptr spDimZ) const;

  /// Helper method. Selects the dataset factory to use.
  boost::shared_ptr<Mantid::VATES::vtkDataSetFactory> createDataSetFactory(Mantid::MDDataObjects::MDWorkspace_sptr spRebinnedWs) const;

  Mantid::VATES::RebinningIterationAction decideIterationAction(const int timestep);
  std::string createHash() const;

  vtkImplicitFunction* m_clipFunction;
  vtkDataSet* m_cachedVTKDataSet;
  std::string m_cachedHashedArguments;
  int m_timestep;
  double m_thresholdMax;
  double m_thresholdMin;

};
#endif
