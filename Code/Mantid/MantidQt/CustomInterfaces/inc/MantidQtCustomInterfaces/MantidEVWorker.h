
#include <MantidKernel/System.h>
#include "MantidKernel/V3D.h"
#include <vector>

#ifndef  INTERFACES_MANTID_EV_WORKER_H
#define  INTERFACES_MANTID_EV_WORKER_H

/**
   @class MantidEVWorker 
  
      This class has methods that call Mantid algorithms to do most of the
   calculations for MantidEV.  These methods are typically called by code in
   the MantidEV_Connections class, in response to user input.

    @author Dennis Mikkelson 
    @date   2013-02-19 
     
    Copyright © 2013 ORNL, STFC Rutherford Appleton Laboratories
  
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
    
    Code Documentation is available at 
                 <http://doxygen.mantidproject.org>
*/

namespace MantidQt
{
namespace CustomInterfaces
{

class DLLExport MantidEVWorker
{

public:

  /// Default constructor
  MantidEVWorker();


  /// Default destructor
 ~MantidEVWorker();

  
  /// check for existence of MD workspace in analysis data service
  bool isMDWorkspace( const std::string & md_ws_name );

  /// check for existence of peaks workspace in analysis data service
  bool isPeaksWorkspace( const std::string & peaks_ws_name );

  /// check for existence of event workspace in analysis data service
  bool isEventWorkspace( const std::string & event_ws_name );

  /// Load and event file and convert to MD workspace
  bool loadAndConvertToMD(const std::string & file_name,
                          const std::string & ev_ws_name,
                          const std::string & md_ws_name,
                          const double        minQ,
                          const double        maxQ,
                          const bool          do_lorentz_corr,
                          const bool          load_data,
                          const bool          load_det_cal,
                          const std::string & det_cal_file,
                          const std::string & det_cal_file2 );

  /// Find peaks in MD workspace and set peaks into peaks workspace
  bool findPeaks( const std::string & ev_ws_name,
		          const std::string & md_ws_name,
                  const std::string & peaks_ws_name,
                        double        max_abc,
                        size_t        num_to_find,
                        double        min_intensity );

  /// Predict peaks and overwrite the peaks workspace
  bool predictPeaks( const std::string & peaks_ws_name,
                                           double        min_pred_wl,
                                           double        max_pred_wl,
                                           double        min_pred_dspacing,
                                           double        max_pred_dspacing );

  /// Load the peaks workspace from a .peaks or .integrate file
  bool loadIsawPeaks( const std::string & peaks_ws_name,
                      const std::string & file_name );

  bool loadNexusPeaks( const std::string & peaks_ws_name,
                                      const std::string & file_name );

  /// Save the peaks workspace to a .peaks or .integrate file
  bool saveIsawPeaks( const std::string & peaks_ws_name,
                      const std::string & file_name,
                            bool          append );

  /// Save the peaks workspace to a .nxs file
  bool saveNexusPeaks( const std::string & peaks_ws_name,
                      const std::string & file_name,
                            bool          append );

  /// Index the peaks using the FFT method
  bool findUBUsingFFT( const std::string & peaks_ws_name,
                       double              min_abc,
                       double              max_abc,
                       double              tolerance );

  /// Index the peaks using the indexing of the peaks in the peaks workspace
  bool findUBUsingIndexedPeaks(const std::string & peaks_ws_name, double tolerance );

  /// Load the UB matrix from a file
  bool loadIsawUB( const std::string & peaks_ws_name,
                   const std::string & file_name );

  /// Save the UB matrix to a file
  bool saveIsawUB( const std::string & peaks_ws_name,
                   const std::string & file_name );

  /// Optimize the phi, chi, omega angles in the peaks wkspace, using stored UB
  bool optimizePhiChiOmega( const std::string & peaks_ws_name, 
                                  double        max_change );

  /// Actually index the peaks, using the stored UB
  bool indexPeaksWithUB( const std::string & peaks_ws_name,
                               double        tolerance,
                               bool          round_hkls );

  /// Show the possible conventional cells for a Niggli cell
  bool showCells( const std::string & peaks_ws_name,
                        double        max_scalar_error,
                        bool          best_only,
                        bool          allow_perm);

  /// Select conventional cell using the cell type and centering
  bool selectCellOfType( const std::string & peaks_ws_name,
                         const std::string & cell_type,
                         const std::string & centering );

  /// Select conventional cell using the form number from the Mighell paper
  bool selectCellWithForm( const std::string & peaks_ws_name,
                                 size_t        form_num );

  /// Apply a mapping to the h,k,l indices and the UB matrix 
  bool changeHKL( const std::string & peaks_ws_name,
                  const std::string & row_1_str,
                  const std::string & row_2_str,
                  const std::string & row_3_str );

  /// Integrate an MD event workspace using spherical integration 
  bool sphereIntegrate( const std::string & peaks_ws_name,
                        const std::string & event_ws_name,
                              double        peak_radius,
                              double        inner_radius,
                              double        outer_radius,
                              bool          integrate_edge,
                              bool          use_cylinder_integration,
                              double        cylinder_length,
                              double        cylinder_percent_bkg,
                        const std::string & cylinder_profile_fit);

  /// Integrate an event workspace using 2-D peak fitting integration 
  bool fitIntegrate( const std::string & peaks_ws_name,
                     const std::string & event_ws_name,
                     const std::string & rebin_param_str,
                           size_t        n_bad_edge_pix,
                           bool          use_ikeda_carpenter );

  /// Integrate an event workspace using 3D ellipsoids 
  bool ellipsoidIntegrate( const std::string & peaks_ws_name,
                           const std::string & event_ws_name,
                                 double        region_radius,
                                 bool          specify_size,
                                 double        peak_size,
                                 double        inner_size,
                                 double        outer_size );

  /// Display UB and lattice parameters in MantidPlot
  bool showUB( const std::string & peaks_ws_name );

  /// Get the current UB matrix from the peaks workspace in sample or lab coords
  bool getUB( const std::string & peaks_ws_name,
                    bool          lab_coords,
                    Mantid::Kernel::Matrix<double> & UB );

  /// Copy the oriented lattice from the peaks workspace to the ND workspace
  bool copyLattice(const std::string & peaks_ws_name,
                    const std::string & md_ws_name , const std::string &event_ws_name);

  /// Get Info about a Q-Vector from a PeaksWorkspace
  std::vector< std::pair< std::string, std::string > >
                               PointInfo( const std::string & peaks_ws_name, 
                                                bool          lab_coords,
                                          Mantid::Kernel::V3D Q);


private:

  /// Utility to get workspace ID from ADS, blank if none
  std::string workspaceType( const std::string & ws_name );  
};

}  // namespace CustomInterfaces 
}  // namespace MantidQt

#endif  // INTERFACES_MANTID_EV_WORKER_H
