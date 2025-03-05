// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * IntegratePeakTimeSlices.h
 *
 *  Created on: May 5, 2011
 *      Author: ruth
 */
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/cow_ptr.h"

#include <array>

// Forward Declarations
namespace Mantid {
namespace HistogramData {
class HistogramX;
}
} // namespace Mantid
namespace Mantid {
namespace Crystal {
/**
 Integrates each time slice using the BivariateNormal formula, adding the
 results to the
 peak object

 @author Ruth Mikkelson, SNS, ORNL
 @date 06/06/2011
 */

class DataModeHandler {
public:
  DataModeHandler() { init(); }

  DataModeHandler(const DataModeHandler &handler);
  DataModeHandler(double baseRCRadius, double lastRCRadius, double lastRow, double lastCol, double CellWidth,
                  double CellHeight, bool CalcVariance, int MinCol, int MaxCol, int MinRow, int MaxRow) {
    init();
    this->baseRCRadius = baseRCRadius;
    this->lastRCRadius = lastRCRadius;
    this->lastRow = lastRow;
    this->lastCol = lastCol;
    this->CellWidth = CellWidth;
    this->CellHeight = CellHeight;
    this->CalcVariance = CalcVariance;
    this->MaxCol = MaxCol;
    this->MaxRow = MaxRow;
    this->MinCol = MinCol;
    this->MinRow = MinRow;
  }

  void setTime(double time) { this->time = time; }

  // Returns true if "Edge Peak" otherwise returns false
  bool setStatBase(std::vector<double> const &StatBase);

  bool isEdgePeak(const double *params, int nparams);

  void setHeightHalfWidthInfo(const MantidVec &xvals, const MantidVec &yvals, const MantidVec &counts);

  void setCurrentRadius(double radius) { currentRadius = radius; }
  void setCurrentCenter(const Kernel::V3D &newCenter) {
    Kernel::V3D XX(newCenter);
    currentPosition = XX;
  }

  double getCurrentRadius() { return currentRadius; }
  Kernel::V3D getCurrentCenter() { return currentPosition; }
  void updateEdgeXsize(double newsize) {
    if (EdgeX < 0)

      EdgeX = newsize;

    else if (newsize < EdgeX)

      EdgeX = newsize;
  }

  void updateEdgeYsize(double newsize) {
    if (EdgeY < 0)

      EdgeY = newsize;

    else if (newsize < EdgeY)

      EdgeY = newsize;
  }

  void CalcVariancesFromData(double background, double meanx, double meany, double &Varxx, double &Varxy, double &Varyy,
                             const std::vector<double> &StatBase);

  bool IsEnoughData(const double *ParameterValues, Kernel::Logger &);

  double getNewRCRadius();

  double getInitBackground() { return back_calc; }

  double getInitRow() { return row_calc; }

  double getInitCol() { return col_calc; }

  double getInitIntensity() { return Intensity_calc; }

  double getInitVarx() { return Vx_calc; }

  double getInitVary() { return Vy_calc; }

  double getInitVarxy() { return Vxy_calc; }

  std::string CalcConstraints(std::vector<std::pair<double, double>> &Bounds, bool CalcVariances);
  std::string getTies() { return ""; }

  bool CalcVariances();

  std::vector<double> GetParams(double b);

  double StatBaseVals(int index) {
    if (index < 0 || index >= (int)StatBase.size())
      return 0.0;
    return StatBase[index];
  }

  double CalcISAWIntensity(const double *params);

  double CalcISAWIntensityVariance(const double *params, const double *errs, double chiSqOvDOF);

  /**
   * For Edge Peaks
   */
  double CalcSampleIntensityMultiplier(const double *params) const;

  /**
   * Returns init values with background and variances replaced by arguments.
   *Varxy=0
   *
   */
  std::vector<double> InitValues(double Varx, double Vary, double b);
  double baseRCRadius;
  double lastRCRadius;
  double HalfWidthAtHalfHeightRadius;
  double calcNewRCRadius;

  double lastRow;
  int MinRow, MaxRow;
  double lastCol;
  int MinCol, MaxCol;
  double time;
  double CellWidth;
  double CellHeight;

  double VarxHW, VaryHW;
  double currentRadius;
  Kernel::V3D currentPosition;
  std::vector<double> StatBase;

  double EdgeX, EdgeY;
  double lastISAWIntensity, lastISAWVariance;
  bool CalcVariance;
  bool case4; // if true result of successful merge of dir =1 chan=0 and chan=1
  double back_calc, Intensity_calc, row_calc, col_calc, Vx_calc, Vy_calc, Vxy_calc;

private:
  void init() {
    baseRCRadius = -1;
    lastRCRadius = -1;
    lastRow = -1;
    lastCol = -1;
    EdgeX = EdgeY = -1;
    calcNewRCRadius = -1;
    MaxRow = -1;
    MaxCol = -1;
    MinRow = -1;
    MinCol = -1;
    time = -1;
    CalcVariance = true;
    CellWidth = CellHeight = 0;
    currentRadius = -1;
    lastISAWIntensity = -1;
    lastISAWVariance = -1;
    currentPosition = Kernel::V3D();
    HalfWidthAtHalfHeightRadius = -1;
    case4 = false;

    VarxHW = -1;
    VaryHW = -1;
    back_calc = Intensity_calc = row_calc = col_calc = Vx_calc = Vy_calc = Vxy_calc = -1;
  }
};

class MANTID_CRYSTAL_DLL IntegratePeakTimeSlices : public Mantid::API::Algorithm, public API::DeprecatedAlgorithm {
public:
  /// Default constructor
  IntegratePeakTimeSlices();

  /// Destructor
  ~IntegratePeakTimeSlices() override;

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "IntegratePeakTimeSlices"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The algorithm uses CurveFitting::BivariateNormal for fitting a "
           "time slice";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"PeakIntegration"}; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Crystal\\Integration"; }

private:
  void init() override;
  void exec() override;

  bool m_EdgePeak;

  std::string m_AttributeNames[20];

  std::string m_ParameterNames[7];

  std::shared_ptr<DataModeHandler> m_AttributeValues;
  std::array<double, 7> m_ParameterValues;

  Mantid::detid2index_map m_wi_to_detid_map;

  int *m_NeighborIDs; // Stores IDs of nearest neighbors
  double m_R0;        ///< for Weak Peaks, these can be set using info from close

  Kernel::V3D m_center; ///< for Describing the Plane at the Peak
  Kernel::V3D m_xvec;   ///< for Describing the Plane at the Peak
  Kernel::V3D m_yvec;   ///< for Describing the Plane at the Peak
  double m_ROW;         ///< for Describing the Row(or 0) describing the center of the Peak
  double m_COL;         ///< for Describing the Column(or 0) describing the center of the
  /// Peak
  double m_cellWidth;  ///< for Describing the Plane at the Peak
  double m_cellHeight; ///< for Describing the Plane at the Peak
  int m_NROWS;
  int m_NCOLS;

  void SetUpData(API::MatrixWorkspace_sptr &Data, API::MatrixWorkspace_const_sptr const &inpWkSpace,
                 const std::shared_ptr<Geometry::IComponent> &comp, const int chanMin, const int chanMax, double CentX,
                 double CentY, Kernel::V3D &CentNghbr,

                 double &neighborRadius, // from CentDet
                 double Radius, std::string &spec_idList);

  bool getNeighborPixIDs(const std::shared_ptr<Geometry::IComponent> &comp, const Kernel::V3D &Center, double &Radius,
                         int *&ArryofID);

  int CalculateTimeChannelSpan(Geometry::IPeak const &peak, const double dQ, const Mantid::HistogramData::HistogramX &X,
                               const int specNum, int &Centerchan);

  double CalculatePositionSpan(DataObjects::Peak const &peak, const double dQ);

  void InitializeColumnNamesInTableWorkspace(DataObjects::TableWorkspace_sptr &TabWS);

  /// Prepares the data for futher analysis adding meta data and marking data on
  /// the edges of detectors
  void SetUpData1(API::MatrixWorkspace_sptr &Data, API::MatrixWorkspace_const_sptr const &inpWkSpace, const int chanMin,
                  const int chanMax, double Radius, const Kernel::V3D &CentPos, std::string &spec_idList

  );

  /**
   *  Tests several starting points in the Marquardt algorithm then calls Fit.
   */
  void PreFit(const API::MatrixWorkspace_sptr &Data, double &chisqOverDOF, bool &done, std::vector<std::string> &names,
              std::vector<double> &params, std::vector<double> &errs, double lastRow, double lastCol,
              double neighborRadius);

  void Fit(const API::MatrixWorkspace_sptr &Data, double &chisqOverDOF, bool &done, std::vector<std::string> &names,
           std::vector<double> &params, std::vector<double> &errs, double lastRow, double lastCol,
           double neighborRadius);

  std::string CalculateFunctionProperty_Fit();

  bool isGoodFit(std::vector<double> const &params, std::vector<double> const &errs,
                 std::vector<std::string> const &names, double chisqOverDOF);
  // returns last row added
  int UpdateOutputWS(DataObjects::TableWorkspace_sptr &TabWS, const int dir, const double chan,
                     std::vector<double> const &params, std::vector<double> const &errs,
                     std::vector<std::string> const &names, const double Chisq, const double time,
                     std::string spec_idList);

  void updatePeakInformation(std::vector<double> const &params, std::vector<double> const &errs,
                             std::vector<std::string> const &names, double &TotVariance, double &TotIntensity,
                             double const TotSliceIntensity, double const TotSliceVariance, double const chisqdivDOF,
                             const int ncells);

  void updateStats(const double intensity, const double variance, const double row, const double col,
                   std::vector<double> &StatBase);

  int findNameInVector(std::string const &oneName, std::vector<std::string> const &nameList);

  double CalculateIsawIntegrateError(const double background, const double backError, const double ChiSqOverDOF,
                                     const double TotVariance, const int ncells);

  void FindPlane(Kernel::V3D &center, Kernel::V3D &xvec, Kernel::V3D &yvec, double &ROW, double &COL, int &NROWS,
                 int &NCOLS, double &pixWidthx, double &pixHeighty, DataObjects::Peak const &peak) const;

  int findTimeChannel(const Mantid::HistogramData::HistogramX &X, const double time);

  // returns true if Neighborhood list is changed
  bool updateNeighbors(const std::shared_ptr<Geometry::IComponent> &comp, const Kernel::V3D &CentPos,
                       const Kernel::V3D &oldCenter, double NewRadius, double &neighborRadius);
};
} // namespace Crystal
} // namespace Mantid
