/*
 * IntegratePeakTimeSlices.h
 *
 *  Created on: May 5, 2011
 *      Author: ruth
 */
#ifndef INTEGRATEPEAKTIMESLICES_H_
#define INTEGRATEPEAKTIMESLICES_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IPeak.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/V3D.h"



namespace Mantid
{
namespace Crystal
{
/**
 Integrates each time slice using the BivariateNormal formula, adding the results to the
 peak object

 @author Ruth Mikkelson, SNS, ORNL
 @date 06/06/2011

 Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport IntegratePeakTimeSlices:  public Mantid::API::Algorithm
{
public:
  /// Default constructor
  IntegratePeakTimeSlices();
  
  /// Destructor
 ~IntegratePeakTimeSlices();
 
  /// Algorithm's name for identification overriding a virtual method
 virtual const std::string name() const 
 {
    return "IntegratePeakTimeSlices";
 }
 
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const 
 {
     return 1;
 }
  
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const
  {
    return "Crystal";
  }

private:


  void init();
  void exec();

  virtual void initDocs();

  Mantid::API::MatrixWorkspace_sptr inputW;  ///< A pointer to the input workspace, the data set
  Mantid::DataObjects::TableWorkspace_sptr outputW; ///< A pointer to the output workspace

  bool EdgePeak;

  std::string AttributeNames[19];

  std::string ParameterNames[7];

  double AttributeValues[19] ;
  double ParameterValues[7] ;

  Mantid::detid2index_map * wi_to_detid_map;
  Mantid::index2spec_map * wsIndx2specNo_map;
  double R0 ;  ///<for Weak Peaks, these can be set using info from close

  Kernel::V3D  center;  ///< for Describing the Plane at the Peak
  Kernel::V3D  xvec;    ///< for Describing the Plane at the Peak
  Kernel::V3D yvec;     ///< for Describing the Plane at the Peak
  double ROW;           ///< for Describing the Row(or 0) describing the center of the  Peak
  double COL;           ///< for Describing the Column(or 0) describing the center of the  Peak
  double CellWidth;     ///< for Describing the Plane at the Peak
  double CellHeight;     ///< for Describing the Plane at the Peak

  void SetUpData( API::MatrixWorkspace_sptr          & Data,
                  API::MatrixWorkspace_sptr    const & inpWkSpace,
                  const int                       chan,
                  double                          CentX,
                  double                          CentY,
                  specid_t                      &CentDet,
                  std::map< specid_t, Kernel::V3D >     &neighbors,
                  double                        &neighborRadius,//from CentDet
                  double                         Radius);





  int  CalculateTimeChannelSpan( DataObjects::Peak     const & peak,
                                 const double                  dQ,
                                 Mantid::MantidVec      const& X,
                                 const int                     specNum,
                                 int                         & Centerchan);

  double CalculatePositionSpan(  DataObjects::Peak const &peak,
                                 const double             dQ );

  void InitializeColumnNamesInTableWorkspace( DataObjects::TableWorkspace_sptr &TabWS) ;


  void SetUpData1( API::MatrixWorkspace_sptr                                    &Data,
                   API::MatrixWorkspace_sptr                              const &inpWkSpace,
                   const int                                              chan,
                   std::map< specid_t, Kernel::V3D > neighbors,
                   double                     Radius,
                   Kernel::V3D               CentPos,  ///< Center on Plane
                   Kernel::V3D                CentDetPos,///< closes detector to center
                   Kernel::Logger                                               & g_log ) ;


  std::string CalculateFunctionProperty_Fit(  ) ;

  bool isGoodFit( std::vector<double >              const & params,
                  std::vector<double >              const & errs,
                  std::vector<std::string >         const &names,
                  double                                   chisq
                ) ;

  void UpdateOutputWS( DataObjects::TableWorkspace_sptr         &TabWS,
                       const int                                  dir,
                       const int                                  chan,
                       std::vector<double >                 const &params,
                       std::vector<double >                 const &errs,
                       std::vector<std::string>             const &names,
                       const double                               chisq,
                       const double                              time) ;


  void updatePeakInformation( std::vector<double >     const &params,
                              std::vector<double >     const &errs,
                              std::vector<std::string >const &names,
                              double                        &TotVariance,
                              double                        &TotIntensity,
                              double const                   TotSliceIntensity,
                              double const                   TotSliceVariance,
                              double const                   chisqdivDOF,
                              const int                      ncelss) ;


  void updateStats( const double          intensity,
                    const double          variance,
                    const double          row,
                    const double          col,
                    std::vector<double> & StatBase );


  int find( std::string              const &oneName,
            std::vector<std::string> const &nameList);


  void getInitParamValues( std::vector<double> const &StatBase,
                           const double               TotBoundaryIntensities,
                           const int                  nBoundaryCells);

  double CalculateIsawIntegrateError(const double background,
                                     const double backError,
                                     const double ChiSqOverDOF,
                                     const double TotIntensity,
                                     const int ncells);

  void FindPlane( Kernel::V3D & center,Kernel:: V3D & xvec, Kernel::V3D& yvec,
                  double &ROW, double &COL,
                 double &pixWidthx, double&pixHeighty, DataObjects::Peak const &peak) const;

  int find( Mantid::MantidVec const & X,
            const double              time);


  bool IsEnoughData(   ) ;


  bool    debug;
  static Kernel::Logger& g_log;

};
} // namespace Crystal
} // namespace Mantid

#endif /* INTEGRATEPEAKTIMESLICES_H_ */
