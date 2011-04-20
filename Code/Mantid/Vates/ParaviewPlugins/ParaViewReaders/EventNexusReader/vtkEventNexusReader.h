#ifndef _vtkEventNexusReader_h
#define _vtkEventNexusReader_h
#include "vtkUnstructuredGridAlgorithm.h"
#include "MantidVatesAPI/MultiDimensionalDbPresenter.h"
#include "MantidMDAlgorithms/WidthParameter.h"
#include "MantidVatesAPI/EscalatingRebinningActionManager.h"

class vtkImplicitFunction;
class VTK_EXPORT vtkEventNexusReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkEventNexusReader *New();
  vtkTypeRevisionMacro(vtkEventNexusReader,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  int CanReadFile(const char* fname);
  void SetXBins(int x);
  void SetYBins(int y);
  void SetZBins(int z);
  void SetMaxThreshold(double maxThreshold);
  void SetMinThreshold(double minThreshold);
  void SetWidth(double width);
  void SetApplyClip(bool applyClip);
  void SetClipFunction( vtkImplicitFunction * func);
  /// Called by presenter to force progress information updating.
  void UpdateAlgorithmProgress(double progress);

protected:
  vtkEventNexusReader();
  ~vtkEventNexusReader();
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int Canreadfile(const char *fname);
  ///Handle time variation.
  unsigned long GetMTime();
  
private:
  vtkEventNexusReader(const vtkEventNexusReader&);
  void operator = (const vtkEventNexusReader&);

  /// File name from which to read.
  char *FileName;

  /// Controller/Presenter.
  Mantid::VATES::MultiDimensionalDbPresenter m_presenter;

  /// Number of x bins set
  int m_nXBins;

  /// Number of y bins set.
  int m_nYBins;

  /// Number of z bins set.
  int m_nZBins;

  /// Flag indicates when set up is complete wrt  the conversion of the nexus file to a MDEventWorkspace stored in ADS.
  bool m_isSetup;

  /// The maximum threshold of counts for the visualisation.
  double m_maxThreshold;

  /// The minimum threshold of counts for the visualisation.
  double m_minThreshold;

  /// Flag indicating that clipping of some kind should be considered. 
  bool m_applyClip;

  /// vtkImplicit function from which to determine how the cut is to be made.
  vtkImplicitFunction* m_clipFunction;

  /// With parameter (applied to plane with width).
  Mantid::MDAlgorithms::WidthParameter m_width;

  /// MD Event Workspace id. strictly could be made static rather than an instance member.
  const std::string m_mdEventWsId;

  /// MD Histogram(IMD) Workspace id. strictly could be made static rather than an instance member.
  const std::string m_histogrammedWsId;

  /// Abstracts the handling of rebinning states and rules govening when those states should apply.
  Mantid::VATES::EscalatingRebinningActionManager m_actionManager;
};
#endif
