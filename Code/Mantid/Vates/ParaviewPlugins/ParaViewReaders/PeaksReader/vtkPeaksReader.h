#ifndef _vtkEventNexusReader_h
#define _vtkEventNexusReader_h
#include "vtkUnstructuredGridAlgorithm.h"
#include "MantidVatesAPI/MultiDimensionalDbPresenter.h"
#include "MantidMDAlgorithms/WidthParameter.h"
#include "MantidVatesAPI/EscalatingRebinningActionManager.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"

class vtkImplicitFunction;
class VTK_EXPORT vtkPeaksReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkPeaksReader *New();
  vtkTypeRevisionMacro(vtkPeaksReader,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  int CanReadFile(const char* fname);
  void SetWidth(double width);

  /// Called by presenter to force progress information updating.
  void updateAlgorithmProgress(double progress);

  void SetAppliedGeometryXML(std::string xml);

  const char* GetInputGeometryXML();

protected:
  vtkPeaksReader();
  ~vtkPeaksReader();
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int Canreadfile(const char *fname);
  ///Handle time variation.
  unsigned long GetMTime();
  
private:
  
  vtkPeaksReader(const vtkPeaksReader&);
  
  void operator = (const vtkPeaksReader&);

  void doRebinning();

  /// File name from which to read.
  char *FileName;

  /// Width of each peak
  double m_width;

  /// Controller/Presenter.
  Mantid::VATES::MultiDimensionalDbPresenter m_presenter;

  /// Flag indicates when set up is complete wrt  the conversion of the nexus file to a MDEventWorkspace stored in ADS.
  bool m_isSetup;

  /// MD Event Workspace id. strictly could be made static rather than an instance member.
  const std::string m_mdEventWsId;

  /// MD Histogram(IMD) Workspace id. strictly could be made static rather than an instance member.
  const std::string m_histogrammedWsId;

  /// Abstracts the handling of rebinning states and rules govening when those states should apply.
  Mantid::VATES::EscalatingRebinningActionManager m_actionManager;

  /// Converts dimension objects into well-formed xml describing the overall geometry
  const Mantid::Geometry::MDGeometryBuilderXML<Mantid::Geometry::StrictDimensionPolicy> m_geometryXmlBuilder;

  /// Sets the rebinning action to rebin if the number of bins has changed on a dimension.
  void formulateRequestUsingNBins(Mantid::VATES::Dimension_sptr newDim);
//
//  /// the dimension information applied to the XDimension Mapping.
//  Mantid::VATES::Dimension_sptr m_appliedXDimension;
//
//  /// the dimension information applied to the yDimension Mapping.
//  Mantid::VATES::Dimension_sptr m_appliedYDimension;
//
//  // the dimension information applied to the zDimension Mapping.
//  Mantid::VATES::Dimension_sptr m_appliedZDimension;
//
//  /// the dimension information applied to the tDimension Mapping.
//  Mantid::VATES::Dimension_sptr m_appliedTDimension;
};
#endif
