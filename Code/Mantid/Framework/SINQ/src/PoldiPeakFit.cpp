/*WIKI*

toto

 *WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidSINQ/PoldiPeakFit.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FileProperty.h"
#include <limits>
#include <cmath>
#include <boost/shared_ptr.hpp>
#include "MantidNexus/NexusClasses.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>


#include <iostream>
//#include "./xylib/xylib.h"
using namespace std;


namespace Mantid
{
namespace Poldi
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(PoldiPeakFit)

								/// Sets documentation strings for this algorithm
								void PoldiPeakFit::initDocs()
{
	this->setWikiSummary("Load Poldi data file. ");
	this->setOptionalMessage("Load Poldi data file.");
}


using namespace Kernel;
using namespace API;
using Geometry::Instrument;
using namespace::NeXus;
//using namespace NeXus;

///// Empty default constructor
//PoldiLoadChopperSlits::PoldiLoadChopperSlits() :
//	{}

/// Initialisation method.
void PoldiPeakFit::init()
{
  	rad2deg = 180/PI;
	deg2rad = PI/180;
	CONVKV = hbar/m_n;               // m²/s == mm²/µs
	CONVLAMV = CONVKV * TWOPI;       // = 3.956034e-07   unit pb


	// Data
	declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("InputWorkspace", "", Direction::Input),
			"Input workspace containing the data to treat.");
	// Data
	declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("PoldiSampleLogs", "PoldiSampleLogs", Direction::Input),
			"Input workspace containing the log data to treat.");
	// Data
	declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("PoldiDeadWires", "PoldiDeadWires", Direction::Input),
			"Input workspace containing the data to treat.");
	// Data
	declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("PoldiChopperSlits", "PoldiChopperSlits", Direction::Input),
			"Input workspace containing the data to treat.");
	// Data
	declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("PoldiSpectra", "PoldiSpectra", Direction::Input),
			"Input workspace containing the data to treat.");
	// Data
	declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("PoldiIPP", "PoldiIPP", Direction::Input),
			"Input workspace containing the data to treat.");
	// Data
	declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("PoldiAutoCorrelation","",Direction::Input),
			"The output Tableworkspace"
			"with columns containing key summary information about the Poldi spectra.");
	// Data
	declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("PoldiPeak", "", Direction::Input),
			"Input workspace containing the peak to fit.");



	declareProperty("wlenmin", 1.1, "minimal wavelength considered" , Direction::Input);
	// Data
	declareProperty("wlenmax", 5.0, "maximal wavelength considered" , Direction::Input);
	// Data







}

/** ***************************************************************** */



/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw Exception::FileError If the Nexus file cannot be found/opened
 *  @throw std::invalid_argument If the optional properties are set to invalid values
 */
void PoldiPeakFit::exec()
{



	g_log.information() << "_Poldi  start conf --------------  "  << std::endl;


	string filename = getPropertyValue("Filename");
	////////////////////////////////////////////////////////////////////////
	// About the workspace
	////////////////////////////////////////////////////////////////////////

	localWorkspace = this->getProperty("InputWorkspace");
	DataObjects::TableWorkspace_sptr ws_sample_logs = this->getProperty("PoldiSampleLogs");
	DataObjects::TableWorkspace_sptr ws_poldi_chopper_slits = this->getProperty("PoldiChopperSlits");
	DataObjects::TableWorkspace_sptr ws_poldi_dead_wires = this->getProperty("PoldiDeadWires");
	DataObjects::TableWorkspace_sptr ws_poldi_spectra = this->getProperty("PoldiSpectra");
	DataObjects::TableWorkspace_sptr ws_poldi_IPP = this->getProperty("PoldiIPP");
	DataObjects::TableWorkspace_sptr ws_poldi_peak = this->getProperty("PoldiPeak");

	ws_auto_corr = this->getProperty("PoldiAutoCorrelation");
	Mantid::MantidVec& X     = ws_auto_corr->dataX(0);
	Mantid::MantidVec& Yall  = ws_auto_corr->dataY(0);
	Mantid::MantidVec& Ypeak = ws_auto_corr->dataY(1);

	size_t nb_d_channel = X.size();

	vector<double> Yfit; Yfit.resize(nb_d_channel);


	g_log.information() << "_Poldi ws loaded --------------  "  << std::endl;

	double wlen_min = this->getProperty("wlenmin");
	double wlen_max = this->getProperty("wlenmax");



	API::Column_const_sptr poldi_IPP_data = ws_poldi_IPP->getColumn("value");

	std::vector<double> time_channels = localWorkspace->dataX(0);

	////////////////////////////////////////////////////////////////////////
	// Chopper configuration
	////////////////////////////////////////////////////////////////////////
	g_log.information() << "____________________________________________________ "  << std::endl;
	g_log.information() << "_Poldi  chopper conf ------------------------------  "  << std::endl;

	double chopper_rot_speed        = this->getTableValueFromLabel(ws_sample_logs,"param","value","ChopperSpeed");

	g_log.information() << "_Poldi -        chopper_rot_speed                  " <<  chopper_rot_speed  << " rpm" << std::endl;


	double time_chopper_tcycle=60./(4.*chopper_rot_speed)*1.e6;    // tcycle

	API::Column_const_sptr col = ws_poldi_chopper_slits->getColumn("position");
	size_t nb_chopper_slits = ws_poldi_chopper_slits->rowCount();
	g_log.information() << "_Poldi -        nb_chopper_slits                   " <<  nb_chopper_slits  << " slits"  << std::endl;

	vector<double> chopper_slits_pos;
	chopper_slits_pos.resize(nb_chopper_slits);
	for (size_t ipk = 0; ipk < nb_chopper_slits; ipk ++)
	{
		chopper_slits_pos[ipk] = static_cast<double>((*col)[ipk]);
		g_log.debug() << "_      -        slits " << ipk
				<< ": pos = " << chopper_slits_pos[ipk]
				                                   << "\t" << chopper_slits_pos[ipk]*time_chopper_tcycle  << "\tµs" << std::endl;
		chopper_slits_pos[ipk] *= time_chopper_tcycle;
	}

	g_log.information() << "_Poldi -        time_chopper_tcycle                " <<  time_chopper_tcycle  << " µs" << std::endl;


	////////////////////////////////////////////////////////////////////////
	// TIME configuration
	////////////////////////////////////////////////////////////////////////
	g_log.information() << "____________________________________________________ "  << std::endl;
	g_log.information() << "_Poldi  time conf ---------------------------------  "  << std::endl;
	double time_delta_t = time_channels[1] - time_channels[0];
	double time_offset = time_channels[0];

	double time_t0 = this->getTableValueFromLabel(ws_poldi_IPP,"param","value","t0");
	g_log.information() << "_Poldi -        time_t0                            " <<  time_t0  << " (as a fraction of tcycle)" << std::endl;
	double time_tconst = this->getTableValueFromLabel(ws_poldi_IPP,"param","value","tconst");
	time_t0=time_t0*time_chopper_tcycle+time_tconst;

	g_log.information() << "_Poldi -        time_delta_t                       " <<  time_delta_t  << " µs" << std::endl;
	g_log.information() << "_Poldi -        time_offset                        " <<  time_offset   << " µs" << std::endl;
	g_log.information() << "_Poldi -        time_tconst                        " <<  time_tconst   << " µs" << std::endl;
	g_log.information() << "_Poldi -        time_t0                            " <<  time_t0       << " µs" << std::endl;



	/////////////////////////////////////////colname///////////////////////////////
	// Detector configuration
	////////////////////////////////////////////////////////////////////////
	g_log.information() << "____________________________________________________ "  << std::endl;
	g_log.information() << "_Poldi  setup conf --------------------------------  "  << std::endl;
	double dist_chopper_sample        = this->getTableValueFromLabel(ws_poldi_IPP,"param","value","dist-chopper-sample");
	double dist_sample_detector       = this->getTableValueFromLabel(ws_poldi_IPP,"param","value","dist-sample-detector");
	double pos_x0_det                 = this->getTableValueFromLabel(ws_poldi_IPP,"param","value","x0det");
	double pos_y0_det                 = this->getTableValueFromLabel(ws_poldi_IPP,"param","value","y0det");
	double ang_twotheta_det_deg       = this->getTableValueFromLabel(ws_poldi_IPP,"param","value","twothet");
	double ang_twotheta_det           = ang_twotheta_det_deg*deg2rad;
	double dist_detector_radius       = this->getTableValueFromLabel(ws_poldi_IPP,"param","value","det_radius");             // # rdet
	size_t nb_det_channel             = int( this->getTableValueFromLabel(ws_poldi_IPP,"param","value","det_nb_channel"));   //   # 400
	size_t nb_time_channels           = time_channels.size();
	int indice_mid_detector           = int((1+nb_det_channel)/2.);                                                          // # mitteldet
	double ang_det_channel_resolution = this->getTableValueFromLabel(ws_poldi_IPP,"param","value","det_channel_resolution");
//	double ang_det_resolution         = this->getTableValue(ws_poldi_IPP,"value",11);              //# resdet

	g_log.information() << "_Poldi -        dist_chopper_sample                " <<  dist_chopper_sample   << " mm"  << std::endl;
	g_log.information() << "_Poldi -        dist_sample_detector               " <<  dist_sample_detector  << " mm"  << std::endl;
	g_log.information() << "_Poldi -        pos_x0_det                         " <<  pos_x0_det            << " mm"  << std::endl;
	g_log.information() << "_Poldi -        pos_y0_det                         " <<  pos_y0_det            << " mm"  << std::endl;
	g_log.information() << "_Poldi -  "  << std::endl;
	g_log.information() << "_Poldi -        ang_twotheta_det_deg               " <<  ang_twotheta_det_deg  << " deg"  << std::endl;
	g_log.debug() << "_Poldi -        dist_detector_radius               " <<  dist_detector_radius  << " mm"  << std::endl;
	g_log.debug() << "_Poldi -        nb_det_channel                     " <<  nb_det_channel        << " wires"  << std::endl;
	g_log.debug() << "_Poldi -        nb_time_channels                   " <<  nb_time_channels      << " time channels"  << std::endl;
	g_log.debug() << "_Poldi -        indice_mid_detector                " <<  indice_mid_detector   << " (mid-time channel)"  << std::endl;
	g_log.information() << "_Poldi -  "  << std::endl;
	g_log.debug() << "_Poldi -        ang_det_channel_resolution         " <<  ang_det_channel_resolution  << " mm"  << std::endl;
//	g_log.debug() << "_Poldi -        ang_det_resolution                 " <<  ang_det_resolution  << std::endl;


	double ang_alpha1 = atan2(pos_y0_det, pos_x0_det);
	if(ang_alpha1<0){ang_alpha1 += PI;}
	g_log.debug() << "_Poldi -        ang_alpha1                         " <<  ang_alpha1*rad2deg          << " deg" << std::endl;

	//	double ang_alpha_sample = PI + ang_alpha1 - ang_twotheta_det;
	double ang_alpha_sample = ang_alpha1 + (PI - ang_twotheta_det);
	g_log.debug() << "_Poldi -        ang_alpha_sample                   " <<  ang_alpha_sample*rad2deg    << " deg" << std::endl;

	double dist_sms = sqrt(pos_x0_det*pos_x0_det + pos_y0_det*pos_y0_det);
	g_log.debug() << "_Poldi -        dist_sms                           " <<  dist_sms                    << " mm"  << std::endl;

	double ang_phi_det_mittel = asin(dist_sms / dist_detector_radius * sin(ang_alpha_sample));
	g_log.debug() << "_Poldi -        ang_phi_det_mittel                 " <<  ang_phi_det_mittel*rad2deg  << " deg" << std::endl;

	double ang_phi_det_mittel_comp = PI - ang_phi_det_mittel - ang_alpha_sample;
	g_log.debug() << "_Poldi -        ang_phi_det_mittel_comp            " <<  ang_phi_det_mittel_comp*rad2deg  << " deg" << std::endl;

	double ang_beta_det_mittel = ang_phi_det_mittel_comp + ang_alpha1;
	g_log.debug() << "_Poldi -        ang_beta_det_mittel                " <<  ang_beta_det_mittel*rad2deg << " deg" << std::endl;

	double csinbeta = dist_sms*sin(ang_alpha_sample);
	double dist_sampl_det_mittel = sqrt(dist_detector_radius*dist_detector_radius - csinbeta*csinbeta) + dist_sms*cos(ang_alpha_sample);
	g_log.debug() << "_Poldi -        dist_sampl_det_mittel              " <<  dist_sampl_det_mittel       << " mm"
			      << " (around " <<	dist_sample_detector << " mm)" << std::endl;
//	g_log.information() << "_Poldi -        b > c sin beta                     " <<  dist_detector_radius << ">" << csinbeta  << std::endl;
	g_log.information() << "_Poldi -  "  << std::endl;



	double ang_wire_apperture = 2*atan2(ang_det_channel_resolution/2., dist_detector_radius); //  # shoulb be = 2.5
	g_log.debug() << "_Poldi -        ang_wire_apperture                 " <<  ang_wire_apperture*rad2deg          << " deg" << std::endl;

	double ang_total_det_apperture = nb_det_channel * ang_wire_apperture;
	g_log.debug() << "_Poldi -        ang_total_det_apperture            " <<  ang_total_det_apperture*rad2deg          << " deg" << std::endl;

	double ang_beta_max = ang_beta_det_mittel + indice_mid_detector * ang_wire_apperture;
	g_log.debug() << "_Poldi -        ang_beta_max                       " <<  ang_beta_max*rad2deg          << " deg" << std::endl;

	double ang_beta_min = ang_beta_max - nb_det_channel * ang_wire_apperture;
	g_log.debug() << "_Poldi -        ang_beta_min                       " <<  ang_beta_min*rad2deg          << " deg" << std::endl;






	////////////////////////////////////////////////////////////////////////
	// dead wires configuration
	////////////////////////////////////////////////////////////////////////
	g_log.information() << "____________________________________________________ "  << std::endl;
	g_log.information() << "_Poldi  dead wires conf ---------------------------  "  << std::endl;

	API::Column_const_sptr col2 = ws_poldi_dead_wires->getColumn("DeadWires");
	size_t nb_dead_wires = ws_poldi_dead_wires->rowCount();
	g_log.information() << "_Poldi -        nb_dead_wires                      " <<  nb_dead_wires  << std::endl;

	vector<bool> table_dead_wires;
	table_dead_wires.resize(nb_det_channel);
	for (size_t ipk = 0; ipk < nb_det_channel; ipk ++){table_dead_wires[ipk]=true;}

	vector<int> list_dead_wires;
	list_dead_wires.resize(nb_dead_wires);
	for (size_t dwire = 0; dwire < nb_dead_wires; dwire ++)
	{
		list_dead_wires[dwire] = static_cast<int>((*col2)[dwire]);
		table_dead_wires[list_dead_wires[dwire]-1] = false;
		g_log.debug() << "_      -        dead wires                     " << list_dead_wires[dwire] << std::endl;
	}


	////////////////////////////////////////////////////////////////////////
	// monitor configuration
	////////////////////////////////////////////////////////////////////////
	g_log.information() << "____________________________________________________ "  << std::endl;
	g_log.information() << "_Poldi  monitor conf -------------------------------  "  << std::endl;
	double int_monitor        = this->getTableValueFromLabel(ws_sample_logs,"param","value","DetMonitor");



	////////////////////////////////////////////////////////////////////////
	// peak fitting configuration
	////////////////////////////////////////////////////////////////////////
	g_log.information() << "____________________________________________________ "  << std::endl;
	g_log.information() << "_Poldi  peak fitting conf --------------------------  "  << std::endl;

	size_t nb_peaks = ws_poldi_peak->getColumn(0)->size();

	vector<double> peak_q_pos;          peak_q_pos.resize(nb_peaks);
	API::Column_const_sptr col_q_pos   = ws_poldi_peak->getColumn("position");
	vector<double> peak_int_max;        peak_int_max.resize(nb_peaks);
	API::Column_const_sptr col_int_max = ws_poldi_peak->getColumn("max");
	vector<double> peak_q_fwhm;         peak_q_fwhm.resize(nb_peaks);
	API::Column_const_sptr col_q_fwhm  = ws_poldi_peak->getColumn("fwhm");
	vector<int> peak_i_min;          peak_i_min.resize(nb_peaks);
	API::Column_const_sptr col_i_min   = ws_poldi_peak->getColumn("imin");
	vector<int> peak_i_pos;          peak_i_pos.resize(nb_peaks);
	API::Column_const_sptr col_i_pos   = ws_poldi_peak->getColumn("ipos");
	vector<int> peak_i_max;          peak_i_max.resize(nb_peaks);
	API::Column_const_sptr col_i_max   = ws_poldi_peak->getColumn("imax");

	for(size_t i = 0; i<nb_peaks; i++){
		peak_q_pos[i]   = (*col_q_pos)[i];
		peak_int_max[i] = (*col_int_max)[i];
		peak_q_fwhm[i]  = (*col_q_fwhm)[i];
		peak_i_min[i]   = int((*col_i_min)[i]);
		peak_i_pos[i]   = int((*col_i_pos)[i]);
		peak_i_max[i]   = int((*col_i_max)[i]);
	}


    double expectsigm = 0.;
    double sumdint = 0.;
    int nsumdint = 0;

	for (size_t i = 1; i<nb_d_channel-1; i++){
		if(Ypeak[i]==0){
            expectsigm = expectsigm + abs(Yall[i-1]-Yall[i]);
            sumdint = sumdint+Yall[i];
            nsumdint++;
		}
	}
    double bgdint = sumdint/float(nsumdint);
    expectsigm = expectsigm/float(nsumdint);
//    Minimum height of the Bragg reflections 2.75 * mean variation plus medium base
    double refintmin = 2.75*expectsigm+bgdint;



	////////////////////////////////////////////////////////////////////////
	// count configuration
	////////////////////////////////////////////////////////////////////////

	// *****  Calculating the number of time elements ******
	g_log.debug() << "____________________________________________________ "  << std::endl;
	g_log.debug() << "_Poldi  time conf ---------------------------------  "  << std::endl;
	double time_peridicity = time_chopper_tcycle / time_delta_t;
	size_t nb_time_elmt = int(time_peridicity+0.01);
	g_log.debug() << "_Poldi -        time_peridicity                    " <<  time_peridicity  << std::endl;
	g_log.debug() << "_Poldi -        nb_time_elmt                       " <<  nb_time_elmt  << std::endl;


	// ****** Calculate the sample scattering angle and distance from the sample for each element of the detector

	vector<double> ang_pw_for_sample;  ang_pw_for_sample.resize(nb_det_channel);
	vector<double> dist_from_sample; dist_from_sample.resize(nb_det_channel);

	for (size_t wire = 0; wire < nb_det_channel; wire ++){

		double ang_phi2det = ang_beta_min+(wire+0.5)*ang_wire_apperture;
		double helpy = dist_detector_radius*sin(ang_phi2det) + pos_y0_det;
		double helpx = dist_detector_radius*cos(ang_phi2det) + pos_x0_det;
		double dist_samp_wire_i = sqrt(helpx*helpx + helpy*helpy);
		double ang_phi2samp = atan2(helpy,helpx);

		ang_pw_for_sample[wire] = ang_phi2samp;
		dist_from_sample[wire] = dist_samp_wire_i;
	}



	// ****    Calculation of the various values of Q
	g_log.information() << "____________________________________________________ "  << std::endl;
	g_log.information() << "_Poldi  diffraction calibration -------------------  "  << std::endl;

	double qmin=2.*(TWOPI/wlen_max) * sin(ang_pw_for_sample[0]/2.);
	double qmax=2.*(TWOPI/wlen_min) * sin(ang_pw_for_sample[nb_det_channel-1]/2.);

	g_log.information() << "_Poldi -        wlen_min                           " <<  wlen_min << " A" << std::endl;
	g_log.information() << "_Poldi -        wlen_max                           " <<  wlen_max << " A"  << std::endl;
	g_log.information() << "_Poldi -        qmin                               " <<  qmin << " A-1"  << std::endl;
	g_log.information() << "_Poldi -        qmax                               " <<  qmax << " A-1"  << std::endl;


	//	double vqmin = CONVKV*qmin / (2.*sin(ang_pw_for_sample[indice_mid_detector]/2.));
	double dist_chop_mid_detector = dist_chopper_sample + dist_from_sample[indice_mid_detector];
	g_log.debug() << "_Poldi -        dist_chop_mid_detector             " <<  dist_chop_mid_detector << " mm" << std::endl;

	double dspace2 = CONVKV / (2.*dist_chop_mid_detector*sin(ang_pw_for_sample[indice_mid_detector]/2.));
	dspace2 *= time_delta_t *1e7 *TWOPI;       // unit [A]
	int n0_dspace = int(TWOPI/qmax/dspace2);
	double dspace1 = n0_dspace*dspace2;
	int n1_dspace = int(TWOPI/qmin/dspace2);

	size_t n_d_space = n1_dspace-n0_dspace;
	g_log.debug() << "_Poldi -        dspace2                            " <<  dspace2  << std::endl;
	g_log.debug() << "_Poldi -        n_d_space                          " <<  n_d_space  << std::endl;


	// ***   Calculate what time an n arrives for a lattice spacing of 1 A

	vector<double> time_TOF_for_1A; time_TOF_for_1A.resize(nb_det_channel);

	for (size_t i = 0; i < nb_det_channel; i ++){
		time_TOF_for_1A[i] = 2./CONVLAMV *1.e-7 * (dist_chopper_sample+dist_from_sample[i]) * sin(ang_pw_for_sample[i]/2.);  // 1e-7 mm = 1 A
		// time unit is µs
	}
	g_log.debug() << "_XXXXX -   dist_from_sample    " <<  dist_from_sample[0]
	              << "\t" << dist_from_sample [indice_mid_detector]   << "\t" << dist_from_sample [nb_det_channel-1]   << std::endl;
	g_log.debug() << "_XXXXX -   ang_pw_for_sample   " <<  ang_pw_for_sample[0]*rad2deg
			      << "\t" << ang_pw_for_sample [indice_mid_detector]*rad2deg  << "\t" << ang_pw_for_sample [nb_det_channel-1]*rad2deg  << std::endl;
	g_log.debug() << "_XXXXX -   time_TOF_for_1A     " <<  time_TOF_for_1A[0]
	              << "\t" << time_TOF_for_1A [indice_mid_detector]    << "\t" << time_TOF_for_1A [nb_det_channel-1]    << std::endl;



	g_log.information() << "_Poldi -        det. apperture for the sample      " <<  (ang_pw_for_sample [nb_det_channel-1] - ang_pw_for_sample [0])*rad2deg << " deg" << std::endl;



	////////////////////////////////////////////////////////////////////////
	// fit setup
	////////////////////////////////////////////////////////////////////////
	g_log.information() << "____________________________________________________ "  << std::endl;
	g_log.information() << "_Poldi  fit conf -----------------------------------  "  << std::endl;

	double expa = 4;
	double q0 = TWOPI/expa;

	size_t stopfit1 = 10.;
	double stopfit2 = 50.;
	double stopfit3 = 1.e10;
	double stopfit4 = 1.e-5;

	double delta_stop_fit = 1.e-5;
	stopfit4 += delta_stop_fit;


	double chisqr;

	// numbref ==  nb_peaks
	// nptsref == npts
	// monitor == int_monitor
	// isame
	// expa
	// q
	// qint
	// expectsigm == expectsigm
	// s_table
	// auindex
	// aindex
	// parref
    // poldipar
	// cinf
	// cubic_structure

	for(size_t peak = 0; peak<nb_peaks; peak++){
		size_t bin_min = peak_i_min[peak];
		size_t npts = int((peak_i_min[peak] - peak_i_min[peak]))+1;


		vector<double> peak_x; peak_x.resize(npts);
		vector<double> peak_y; peak_y.resize(npts);
		for(size_t bin=0; bin<npts; bin++){
			peak_x[bin] = X[bin_min+bin];
			peak_y[bin] = Yall[bin_min+bin];
		}

		this->a1 = peak_y[npts/2];
		this->a2 = (peak_x[npts-1]+peak_x[0])/2.;
		this->a3 = (peak_x[npts-1]-peak_x[0])/20.;
		this->a4 = 0.;
		this->a5 = 0.;
		this->a6 = 0.;


        int nterms = 5;


    	vector<vector<double>> alpha; alpha.resize(nterms);
        for(size_t i=0; i<nterms; i++){
        	alpha[i].resize(nterms);
        }
    	vector<double> beta; beta.resize(nterms);


    	this->deriv.resize(nterms);
        for(size_t i=0; i<nterms; i++){
        	this->deriv[i].resize(npts);
        }



    	vector<int> listfree; listfree.resize(5);
        listfree[0] = 1;
        listfree[1] = 2;
        listfree[2] = 3;
        listfree[3] = 4;
        listfree[4] = 6;

        //**** input of parameters **********************

        double flambda  = 0.001;
        double chisqold = 10000;
        double nabbruch = 0;
        int ncycle = 0;

//****** start of fit ***************************

        for(size_t j = 0; j<stopfit1; j++){
        	double sigmint = expectsigm;
            double sigmintsqr = sigmint*sigmint;
            int nfree = npts-nterms;
            //********* curfit
            if(nfree<=0){chisqr = 0.;}
            else{
            	//*  evaluate alpha and beta matrices
            	//*
            	for(int j1=0; j1<nterms;j1++){
            		beta[j] = 0.;
            		for(int k1=0; k1<j1;k1++){
            			alpha[j1][k1] = 0.;
            		}   //k1
            	}//j1

            	//   ** calculation of chisqr at starting point *********
            	double chisq1 = chisqr ;
            	if(ncycle == 0){
            		chisq1 = 0.;
            		for(size_t i2 = 0; i2<npts; i2++){
            			Yfit[i2] = functn(X[i2]);
            			chisq1 = chisq1 + (Yall[i2]-Yfit[i2])*(Yall[i2]-Yfit[i2]) /sigmintsqr;
            		}
            		chisq1=chisq1/(float(nfree));
            	}


            	//**** main part ****
            	        this->fderiv(X,listfree,nterms,npts);

            	        for(size_t i2 = 0; i2<npts; i2++){
            	        	for(size_t jj = 0; jj<nterms; jj++){
            	                beta[jj] += (Yall[i2]-Yfit[i2])*deriv[jj][i2]/sigmintsqr;
            	                for(size_t kk = 0; kk<jj; kk++){
            	                    alpha[jj][kk] += deriv[jj][i2]*deriv[kk][i2]/sigmintsqr;
            	                } //     !kk
            	        	} //   !jj
            	        } //   !i2
            	        for(size_t jj = 0; jj<nterms; jj++){
            	        	for(size_t kk = 0; kk<jj; kk++){
            	        		alpha[jj][kk] = alpha[kk][jj];
            	        	} //     !kk
            	        } //   !jj



            }
        }


	}













































}








double PoldiPeakFit::functn(double xi){
	double relative_pos = (xi-this->a2);
      double fct = this->a4+ this->a5*xi+ this->a6*relative_pos*relative_pos;
      double z2 = (relative_pos/this->a3);
      if(z2<8.){
    	  fct = fct + a1*exp(-z2*z2/2.);
      }
      return fct;
}





double PoldiPeakFit::fderiv(Mantid::MantidVec& X, vector<int> &listfree, int nterms, int npts){

    for(size_t j=0; j<nterms; j++){
    	for(size_t i2=0; i2<npts; i2++){

            this->deriv[j][i2] = 0.;
            double xi = X[i2];

            if(listfree[j]<4){

                double z = (xi-a2)/a3;
                double z2 = z*z;
                if(z2<50.){
                    if(listfree[j]==1) deriv[j][i2] = exp(-z2/2.);
                    if(listfree[j]==2) deriv[j][i2] = a1*exp(-z2/2.)*z/a3;
                    if(listfree[j]==3) deriv[j][i2] = a1*exp(-z2/2.)*z2/a3;
                }
            }

            if(listfree[j]==2) deriv[j][i2] = deriv[j][i2] + 2.*a6*(a2-xi);
            if(listfree[j]==4) deriv[j][i2] = 1.;
            if(listfree[j]==5) deriv[j][i2] = xi;
            if(listfree[j]==6) deriv[j][i2] = (xi-a2)*(xi-a2);

    	} //     !i2
//****** tests
        double su = 0;
    	for(size_t i2=0; i2<npts; i2++){
            su += abs(deriv[j][i2]);
    	}
        //if(su<1.e-4)write(6,*)'Ableitung ',listfree(j),' ist Null'
//**** ende test

    } //     !j  nterms
}



















double PoldiPeakFit::nhe3(int a, int b){
	//	return (*this->poldi_nhe3[a])[b];
	return this->localWorkspace->dataY(399-a)[b];
}


double PoldiPeakFit::dblSqrt(double in)
{
  return sqrt(in);
}


/*
 * Get one specific table value
 */
double PoldiPeakFit::getTableValue(DataObjects::TableWorkspace_sptr tableWS, std::string colname, size_t index)
{
	API::Column_const_sptr col = tableWS->getColumn(colname);

	if (!col)
	{
		g_log.error() << "Column with name " << colname << " does not exist" << std::endl;
		throw std::invalid_argument("Non-exist column name");
	}

	if (index >= col->size())
	{
		g_log.error() << "Try to access index " << index << " out of boundary = " << col->size() << std::endl;
		throw std::runtime_error("Access column array out of boundary");
	}

	return (*col)[index];
}

/*
 * Get one specific table value
 */
double PoldiPeakFit::getTableValueFromLabel(DataObjects::TableWorkspace_sptr tableWS, string colNameLabel, string colNameValue, string label)
{
	API::Column_const_sptr colvalue = tableWS->getColumn(colNameValue);
	API::Column_const_sptr collabel = tableWS->getColumn(colNameLabel);

	if (!colvalue)
	{
		g_log.error() << "Column with name " << colNameValue << " does not exist" << std::endl;
		throw std::invalid_argument("Non-exist column name");
	}
	if (!collabel)
	{
		g_log.error() << "Column with name " << colNameLabel << " does not exist" << std::endl;
		throw std::invalid_argument("Non-exist column name");
	}


	size_t l = collabel->size();
	size_t indice = 999;
	for(size_t i = 0; i<l; i++){
		string toto = (collabel).get()->cell<string>(i);
		if(toto ==  label){
			indice = i;
			break;
		}
	}

	if (indice == 999)
	{
		g_log.error() << "Parameter with label " << label << " does not exist" << std::endl;
		throw std::invalid_argument("Non-exist label parameter");
	}

	if (indice >= colvalue->size())
	{
		g_log.error() << "Try to access index " << indice << " out of boundary = " << colvalue->size() << std::endl;
		throw std::runtime_error("Access column array out of boundary");
	}

	return (*colvalue)[indice];
}



} // namespace DataHandling
} // namespace Mantid
