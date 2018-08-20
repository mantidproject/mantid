#ifndef _vtkMDEWNexusReader_h
#define _vtkMDEWNexusReader_h
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidMDAlgorithms/WidthParameter.h"
#include "MantidVatesAPI/EscalatingRebinningActionManager.h"
#include "MantidVatesAPI/MultiDimensionalDbPresenter.h"
#include "MantidVatesAPI/ThresholdRange.h"
#include "vtkUnstructuredGridAlgorithm.h"

class vtkImplicitFunction;
class VTK_EXPORT vtkMDEWNexusReader : public vtkUnstructuredGridAlgorithm {
public:
  static vtkMDEWNexusReader *New();
  vtkTypeMacro(vtkMDEWNexusReader, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent);
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  int CanReadFile(const char *fname);
  void SetXBins(int x);
  void SetYBins(int y);
  void SetZBins(int z);
  void SetMaxThreshold(double maxThreshold);
  void SetMinThreshold(double minThreshold);
  void SetWidth(double width);
  void SetApplyClip(bool applyClip);
  void SetClipFunction(vtkImplicitFunction *func);
  void SetThresholdRangeStrategyIndex(std::string selectedStrategyIndex);
  double GetInputMinThreshold();
  double GetInputMaxThreshold();
  /// Called by presenter to force progress information updating.
  void UpdateAlgorithmProgress(double progress);

  void SetAppliedGeometryXML(std::string xml);

  const char *GetInputGeometryXML();

protected:
  vtkMDEWNexusReader();
  ~vtkMDEWNexusReader();
  int RequestInformation(vtkInformation *, vtkInformationVector **,
                         vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *);
  int Canreadfile(const char *fname);
  /// Handle time variation.
  vtkMTimeType GetMTime() override;

private:
  vtkMDEWNexusReader(const vtkMDEWNexusReader &);

  void operator=(const vtkMDEWNexusReader &);

  std::string extractFormattedPropertyFromDimension(
      Mantid::Geometry::IMDDimension_sptr dimension) const;

  void doRebinning();

  /// Set-up a strategy for thresholding.
  void configureThresholdRangeMethod();

  /**
   Detect wheter x dimension is available.
   @return true available, false otherwise.
 */
  bool hasXDimension() const { return NULL != m_appliedXDimension.get(); }

  /**
   Detect wheter y dimension is available.
   @return true available, false otherwise.
 */
  bool hasYDimension() const { return NULL != m_appliedYDimension.get(); }

  /**
   Detect wheter z dimension is available.
   @return true available, false otherwise.
 */
  bool hasZDimension() const { return NULL != m_appliedZDimension.get(); }

  /**
   Detect wheter t dimension is available.
   @return true available, false otherwise.
 */
  bool hasTDimension() const { return NULL != m_appliedTDimension.get(); }

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

  /// Flag indicates when set up is complete wrt  the conversion of the nexus
  /// file to a MDEventWorkspace stored in ADS.
  bool m_isSetup;

  /// The maximum threshold of counts for the visualisation.
  double m_maxThreshold;

  /// The minimum threshold of counts for the visualisation.
  double m_minThreshold;

  /// Flag indicating that clipping of some kind should be considered.
  bool m_applyClip;

  /// vtkImplicit function from which to determine how the cut is to be made.
  vtkImplicitFunction *m_clipFunction;

  /// With parameter (applied to plane with width).
  Mantid::MDAlgorithms::WidthParameter m_width;

  /// MD Event Workspace id. strictly could be made static rather than an
  /// instance member.
  const std::string m_mdEventWsId;

  /// MD Histogram(IMD) Workspace id. strictly could be made static rather than
  /// an instance member.
  const std::string m_histogrammedWsId;

  /// Abstracts the handling of rebinning states and rules govening when those
  /// states should apply.
  Mantid::VATES::EscalatingRebinningActionManager m_actionManager;

  /// Converts dimension objects into well-formed xml describing the overall
  /// geometry
  Mantid::Geometry::MDGeometryBuilderXML<
      Mantid::Geometry::StrictDimensionPolicy>
      m_geometryXmlBuilder;

  /// Sets the rebinning action to rebin if the number of bins has changed on a
  /// dimension.
  void formulateRequestUsingNBins(Mantid::VATES::Dimension_sptr newDim);

  /// the dimension information applied to the XDimension Mapping.
  Mantid::VATES::Dimension_sptr m_appliedXDimension;

  /// the dimension information applied to the yDimension Mapping.
  Mantid::VATES::Dimension_sptr m_appliedYDimension;

  // the dimension information applied to the zDimension Mapping.
  Mantid::VATES::Dimension_sptr m_appliedZDimension;

  /// the dimension information applied to the tDimension Mapping.
  Mantid::VATES::Dimension_sptr m_appliedTDimension;

  int m_thresholdMethodIndex;

  Mantid::VATES::ThresholdRange_scptr m_ThresholdRange;

  /// Mutex for thread-safe progress reporting.
  Mantid::Kernel::Mutex progressMutex;
};
#endif
