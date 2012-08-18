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
#include "MantidAPI/IAlgorithm.h"

using Mantid::API::IAlgorithm_sptr;



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

  class DataModeHandler
  {
  public:

     DataModeHandler()
     {
       this->baseRCRadius=-1;
       this->lastRCRadius=-1;
       this->lastRow=-1;
       this->lastCol=-1;
       EdgeX=EdgeY=-1;
       calcNewRCRadius = -1;
       time = -1;
       CalcVariance = true;

       currentRadius =-1;
       currentPosition= Kernel::V3D();
       HalfWidthAtHalfHeightRadius=-1;
       back_calc=Intensity_calc=row_calc=col_calc=Vx_calc= Vy_calc= Vxy_calc=-1;
     }

     DataModeHandler( const DataModeHandler &handler);
     DataModeHandler(double baseRCRadius, double lastRCRadius,
            double lastRow, double lastCol, double CellWidth, double CellHeight,
            bool CalcVariance)
     {
       this->baseRCRadius=baseRCRadius;
       this->lastRCRadius=lastRCRadius;
       this->lastRow=lastRow;
       this->lastCol=lastCol;
       this->CellWidth = CellWidth;
       this->CellHeight = CellHeight;
       this->CalcVariance = CalcVariance;
       this->case4=false;
       EdgeX=EdgeY=-1;
       calcNewRCRadius = -1;
       time = -1;

       this->currentRadius =-1;
       this->currentPosition= Kernel::V3D();
       HalfWidthAtHalfHeightRadius=-1;
       back_calc=Intensity_calc=row_calc=col_calc=Vx_calc= Vy_calc= Vxy_calc=-1;
     }
     void setTime( double time )
     {
          this->time=time;
     }
     void setStatBase(std::vector<double> const &StatBase );
     //Will calc
     void setHeighHalfWidthInfo( Mantid::MantidVecPtr &xvals,
         Mantid::MantidVecPtr &yvals,Mantid::MantidVecPtr &counts,
         double ROW,double COL);

     void setCurrentRadius( double radius)
     {
       currentRadius = radius;
     }
     void setCurrentCenter( Kernel::V3D newCenter)
     {
       currentPosition = newCenter;
     }

     double getCurrentRadius( )
     {
       return currentRadius ;
     }
     Kernel::V3D getCurrentCenter( )
     {
       return currentPosition;
     }
     void updateEdgeXsize( double newsize)
     { if(EdgeX < 0)
          EdgeX = newsize;
        else if( newsize < EdgeX)
         EdgeX = newsize;
     }
     void updateEdgeYsize( double newsize)
     { if(EdgeY < 0)
          EdgeY = newsize;
        else if( newsize < EdgeY)
         EdgeY = newsize;
     }
     void CalcVariancesFromData( double background,  double row,
                           double col, double &Varx, double &Vary, double &Varxy,
                           std::vector<double>&ParameterValues);
     bool IsEnoughData(double *ParameterValues,Kernel::Logger& g_log);

     double getNewRCRadius();
     double getInitBackground(){ return back_calc;}
     double getInitRow(){ return row_calc;}
     double getInitCol(){ return col_calc;}
     double getInitIntensity(){ return Intensity_calc;}
     double getInitVarx(){ return Vx_calc;}
     double getInitVary(){ return Vy_calc;}
     double getInitVarxy(){ return Vxy_calc;}
     std::string CalcConstraints(std::vector< std::pair<double,double> > & Bounds,
        bool CalcVariances );
     std::string getTies(){ return "";}
     bool CalcVariances( );

     double StatBaseVals( int index){return StatBase[index];}

     double CalcISAWIntensity( const double* params) const;

     double CalcISAWIntensityVariance(  const double* params, const double* errs, double chiSqOvDOF) const;

     double CalcSampleIntensityMultiplier( const double* params) const;


     double baseRCRadius;
     double lastRCRadius;
     double HalfWidthAtHalfHeightRadius;
     double calcNewRCRadius;
     double lastRow;
     double lastCol;
     double time;
     double CellWidth;
     double CellHeight;
     double currentRadius;
     Kernel::V3D   currentPosition;
     std::vector<double> StatBase;
     double EdgeX,EdgeY;
     bool CalcVariance;
     bool case4;//if true result of successful merge of dir =1 chan=0 and chan=1
     double back_calc,Intensity_calc,row_calc,col_calc,Vx_calc, Vy_calc, Vxy_calc;
  };

class DLLExport IntegratePeakTimeSlices:  public Mantid::API::Algorithm
{
public:
  /// Default constructor
  IntegratePeakTimeSlices();
  
  /// Destructor
 virtual  ~IntegratePeakTimeSlices();
 
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

  std::string AttributeNames[20];

  std::string ParameterNames[7];

  boost::shared_ptr<DataModeHandler> AttributeValues ;
  double ParameterValues[7] ;

  Mantid::detid2index_map * wi_to_detid_map;

  int*                     NeighborIDs;//Stores IDs of nearest neighbors
  double R0 ;  ///<for Weak Peaks, these can be set using info from close

  Kernel::V3D  center;  ///< for Describing the Plane at the Peak
  Kernel::V3D  xvec;    ///< for Describing the Plane at the Peak
  Kernel::V3D yvec;     ///< for Describing the Plane at the Peak
  double ROW;           ///< for Describing the Row(or 0) describing the center of the  Peak
  double COL;           ///< for Describing the Column(or 0) describing the center of the  Peak
  double CellWidth;     ///< for Describing the Plane at the Peak
  double CellHeight;     ///< for Describing the Plane at the Peak
  int NROWS;
  int NCOLS;

  void SetUpData( API::MatrixWorkspace_sptr          & Data,
                  API::MatrixWorkspace_sptr    const & inpWkSpace,
                  boost::shared_ptr< Geometry::IComponent> comp,
                  const int                       chanMin,
                  const int                       chanMax,
                  double                          CentX,
                  double                          CentY,
                  Kernel::V3D                     &CentNghbr,

                  double                        &neighborRadius,//from CentDet
                  double                         Radius,
                  std::string                    &spec_idList);




  bool getNeighborPixIDs( boost::shared_ptr< Geometry::IComponent> comp,
                          Kernel::V3D                             &Center,
                          double                                  &Radius,
                          int*                                    &ArryofID);

  int  CalculateTimeChannelSpan( DataObjects::Peak     const & peak,
                                 const double                  dQ,
                                 Mantid::MantidVec      const& X,
                                 const int                     specNum,
                                 int                         & Centerchan);

  double CalculatePositionSpan(  DataObjects::Peak const &peak,
                                 const double             dQ );

  void InitializeColumnNamesInTableWorkspace( DataObjects::TableWorkspace_sptr &TabWS) ;


  void SetUpData1( API::MatrixWorkspace_sptr      &Data,
                   API::MatrixWorkspace_sptr     const &inpWkSpace,
                   const int                       chanMin,
                   const int                       chanMax,
                   double                         Radius,
                   Kernel::V3D                    CentPos , ///< Center on Plane,
                   std::string                    &spec_idList

                   ) ;
  void Fit(API::MatrixWorkspace_sptr &Data,double &chisq, bool &done,
        std::vector<std::string>&names, std::vector<double>&params,
        std::vector<double>&errs,double lastRow,double lastCol,double neighborRadius);

  std::string CalculateFunctionProperty_Fit(  ) ;

  bool isGoodFit( std::vector<double >              const & params,
                  std::vector<double >              const & errs,
                  std::vector<std::string >         const &names,
                  double                                   chisq
                ) ;
  //returns last row added
  int UpdateOutputWS( DataObjects::TableWorkspace_sptr         &TabWS,
                       const int                                  dir,
                       const double                                  chan,
                       std::vector<double >                 const &params,
                       std::vector<double >                 const &errs,
                       std::vector<std::string>             const &names,
                       const double                               chisq,
                       const double                              time,
                       std::string                         spec_idList) ;


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



  double CalculateIsawIntegrateError(const double background,
                                     const double backError,
                                     const double ChiSqOverDOF,
                                     const double TotIntensity,
                                     const int ncells);

  void FindPlane( Kernel::V3D & center,  Kernel:: V3D & xvec,    Kernel::V3D& yvec,
                  double &ROW,          double &COL,    int &NROWS,
                  int & NCOLS,        double &pixWidthx,
                  double&pixHeighty,   DataObjects::Peak const &peak) const;

  int find( Mantid::MantidVec const & X,
            const double              time);

  //returns true if Neighborhood list is changed
  bool updateNeighbors( boost::shared_ptr< Geometry::IComponent> &comp,
      Kernel:: V3D CentPos, Kernel::V3D oldCenter,double NewRadius, double &neighborRadius);



  bool    debug;
  static Kernel::Logger& g_log;

};
} // namespace Crystal
} // namespace Mantid

#endif /* INTEGRATEPEAKTIMESLICES_H_ */
