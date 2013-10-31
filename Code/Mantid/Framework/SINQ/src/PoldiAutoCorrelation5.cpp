/*WIKI*


== How to use algorithm with other algorithms ==
This algorithm is designed to work with other algorithms to
proceed POLDI data. The introductions can be found in the
wiki page of [[PoldiProjectRun]].


 *WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidSINQ/PoldiAutoCorrelation5.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

#include <boost/shared_ptr.hpp>


using namespace std;


namespace Mantid
{
namespace Poldi
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(PoldiAutoCorrelation5)

/// Sets documentation strings for this algorithm
void PoldiAutoCorrelation5::initDocs()
{
	this->setWikiSummary("Proceed to autocorrelation on Poldi data.");
	this->setOptionalMessage("Proceed to autocorrelation on Poldi data.");
}


using namespace Kernel;
using namespace API;
using namespace PhysicalConstants;

/// Initialisation method.
void PoldiAutoCorrelation5::init()
{

  CONVKV = h_bar/NeutronMass;
  CONVLAMV = CONVKV*2.*M_PI;


	// Input workspace containing the raw data.
	declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("InputWorkspace", "", Direction::InOut),
			"Input workspace containing the raw data.");
	// Input workspace containing the log data.
	declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("PoldiSampleLogs", "PoldiSampleLogs", Direction::InOut),
			"Input workspace containing the log data.");
	// Input workspace containing the dead wires data.
	declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("PoldiDeadWires", "PoldiDeadWires", Direction::InOut),
			"Input workspace containing the dead wires data.");
	// Input workspace containing the choppers' slits data.
	declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("PoldiChopperSlits", "PoldiChopperSlits", Direction::InOut),
			"Input workspace containing the choppers' slits data.");
	// Input workspace containing the Poldi caracteristic spectra.
	declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("PoldiSpectra", "PoldiSpectra", Direction::InOut),
			"Input workspace containing the Poldi caracteristic spectra.");
	// Input workspace containing the Poldi setup data.
	declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("PoldiIPP", "PoldiIPP", Direction::InOut),
			"Input workspace containing the Poldi setup data.");

	// the minimal value of the wavelength to consider
	declareProperty("wlenmin", 1.1, "minimal wavelength considered" , Direction::Input);
	// the maximal value of the wavelength to consider
	declareProperty("wlenmax", 5.0, "maximal wavelength considered" , Direction::Input);

	// The output Workspace2D containing the Poldi data autocorrelation function.
	declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output),
			"The output Workspace2D"
			"containing the Poldi data autocorrelation function."
			"Index 1 and 2 ws will be used later by the peak detection algorithm.");





}

/** ***************************************************************** */



/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw Exception::NotFoundError Error when saving the PoldiDeadWires Results data to Workspace
 *  @throw std::runtime_error Error when saving the PoldiDeadWires Results data to Workspace
 */
void PoldiAutoCorrelation5::exec()
{



	g_log.information() << "_Poldi  start conf --------------  "  << std::endl;


//	string filename = getPropertyValue("Filename");
	////////////////////////////////////////////////////////////////////////
	// About the workspace
	////////////////////////////////////////////////////////////////////////

	localWorkspace = this->getProperty("InputWorkspace");
	DataObjects::TableWorkspace_sptr ws_sample_logs = this->getProperty("PoldiSampleLogs");
	DataObjects::TableWorkspace_sptr ws_poldi_chopper_slits = this->getProperty("PoldiChopperSlits");
	DataObjects::TableWorkspace_sptr ws_poldi_dead_wires = this->getProperty("PoldiDeadWires");
	DataObjects::TableWorkspace_sptr ws_poldi_spectra = this->getProperty("PoldiSpectra");
	DataObjects::TableWorkspace_sptr ws_poldi_IPP = this->getProperty("PoldiIPP");


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

	double chopper_rot_speed        = getTableValueFromLabel(ws_sample_logs,"param","value","ChopperSpeed");

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

	double time_t0 = getTableValueFromLabel(ws_poldi_IPP,"param","value","t0");
	g_log.information() << "_Poldi -        time_t0                            " <<  time_t0  << " (as a fraction of tcycle)" << std::endl;
	double time_tconst = getTableValueFromLabel(ws_poldi_IPP,"param","value","tconst");
	time_t0=time_t0*time_chopper_tcycle+time_tconst;

	g_log.information() << "_Poldi -        time_delta_t                       " <<  time_delta_t  << " µs" << std::endl;
	g_log.information() << "_Poldi -        time_offset                        " <<  time_offset   << " µs" << std::endl;
	g_log.information() << "_Poldi -        time_tconst                        " <<  time_tconst   << " µs" << std::endl;
	g_log.information() << "_Poldi -        time_t0                            " <<  time_t0       << " µs" << std::endl;



	////////////////////////////////////////////////////////////////////////
	// Detector configuration
	////////////////////////////////////////////////////////////////////////
	g_log.information() << "____________________________________________________ "  << std::endl;
	g_log.information() << "_Poldi  setup conf --------------------------------  "  << std::endl;
	double dist_chopper_sample        = getTableValueFromLabel(ws_poldi_IPP,"param","value","dist-chopper-sample");
	double dist_sample_detector       = getTableValueFromLabel(ws_poldi_IPP,"param","value","dist-sample-detector");
	double pos_x0_det                 = getTableValueFromLabel(ws_poldi_IPP,"param","value","x0det");
	double pos_y0_det                 = getTableValueFromLabel(ws_poldi_IPP,"param","value","y0det");
	double ang_twotheta_det_deg       = getTableValueFromLabel(ws_poldi_IPP,"param","value","twothet");
	double ang_twotheta_det           = ang_twotheta_det_deg*deg2rad;
	double dist_detector_radius       = getTableValueFromLabel(ws_poldi_IPP,"param","value","det_radius");             // # rdet
	size_t nb_det_channel             = int( getTableValueFromLabel(ws_poldi_IPP,"param","value","det_nb_channel"));   //   # 400
	size_t nb_time_channels           = time_channels.size();
	int indice_mid_detector           = int((1+nb_det_channel)/2);                                                          // # mitteldet
	double ang_det_channel_resolution = getTableValueFromLabel(ws_poldi_IPP,"param","value","det_channel_resolution");

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


	double ang_alpha1 = atan2(pos_y0_det, pos_x0_det);
	if(ang_alpha1<0){ang_alpha1 += M_PI;}
	g_log.debug() << "_Poldi -        ang_alpha1                         " <<  ang_alpha1*rad2deg          << " deg" << std::endl;

	double ang_alpha_sample = ang_alpha1 + (M_PI - ang_twotheta_det);
	g_log.debug() << "_Poldi -        ang_alpha_sample                   " <<  ang_alpha_sample*rad2deg    << " deg" << std::endl;

	double dist_sms = sqrt(pos_x0_det*pos_x0_det + pos_y0_det*pos_y0_det);
	g_log.debug() << "_Poldi -        dist_sms                           " <<  dist_sms                    << " mm"  << std::endl;

	double ang_phi_det_mittel = asin(dist_sms / dist_detector_radius * sin(ang_alpha_sample));
	g_log.debug() << "_Poldi -        ang_phi_det_mittel                 " <<  ang_phi_det_mittel*rad2deg  << " deg" << std::endl;

	double ang_phi_det_mittel_comp = M_PI - ang_phi_det_mittel - ang_alpha_sample;
	g_log.debug() << "_Poldi -        ang_phi_det_mittel_comp            " <<  ang_phi_det_mittel_comp*rad2deg  << " deg" << std::endl;

	double ang_beta_det_mittel = ang_phi_det_mittel_comp + ang_alpha1;
	g_log.debug() << "_Poldi -        ang_beta_det_mittel                " <<  ang_beta_det_mittel*rad2deg << " deg" << std::endl;

	double csinbeta = dist_sms*sin(ang_alpha_sample);
	double dist_sampl_det_mittel = sqrt(dist_detector_radius*dist_detector_radius - csinbeta*csinbeta) + dist_sms*cos(ang_alpha_sample);
	g_log.debug() << "_Poldi -        dist_sampl_det_mittel              " <<  dist_sampl_det_mittel       << " mm"
			      << " (around " <<	dist_sample_detector << " mm)" << std::endl;
	g_log.information() << "_Poldi -  "  << std::endl;



	double ang_wire_apperture = 2*atan2(ang_det_channel_resolution/2., dist_detector_radius); //  # shoulb be = 2.5
	g_log.debug() << "_Poldi -        ang_wire_apperture                 " <<  ang_wire_apperture*rad2deg          << " deg" << std::endl;

	double ang_total_det_apperture = static_cast<double>(nb_det_channel) * ang_wire_apperture;
	g_log.debug() << "_Poldi -        ang_total_det_apperture            " <<  ang_total_det_apperture*rad2deg          << " deg" << std::endl;

	double ang_beta_max = ang_beta_det_mittel + indice_mid_detector * ang_wire_apperture;
	g_log.debug() << "_Poldi -        ang_beta_max                       " <<  ang_beta_max*rad2deg          << " deg" << std::endl;

	double ang_beta_min = ang_beta_max - static_cast<double>(nb_det_channel) * ang_wire_apperture;
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

		double ang_phi2det = ang_beta_min+(static_cast<double>(wire)+0.5)*ang_wire_apperture;
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

	double qmin=2.*(2.*M_PI/wlen_max) * sin(ang_pw_for_sample[0]/2.);
	double qmax=2.*(2.*M_PI/wlen_min) * sin(ang_pw_for_sample[nb_det_channel-1]/2.);

	g_log.information() << "_Poldi -        wlen_min                           " <<  wlen_min << " A" << std::endl;
	g_log.information() << "_Poldi -        wlen_max                           " <<  wlen_max << " A"  << std::endl;
	g_log.information() << "_Poldi -        qmin                               " <<  qmin << " A-1"  << std::endl;
	g_log.information() << "_Poldi -        qmax                               " <<  qmax << " A-1"  << std::endl;


	//	double vqmin = CONVKV*qmin / (2.*sin(ang_pw_for_sample[indice_mid_detector]/2.));
	double dist_chop_mid_detector = dist_chopper_sample + dist_from_sample[indice_mid_detector];
	g_log.debug() << "_Poldi -        dist_chop_mid_detector             " <<  dist_chop_mid_detector << " mm" << std::endl;

	double dspace2 = CONVKV / (2.*dist_chop_mid_detector*sin(ang_pw_for_sample[indice_mid_detector]/2.));
	dspace2 *= time_delta_t *1e7 *2.*M_PI;       // unit [A]
	int n0_dspace = int(2.*M_PI/qmax/dspace2);
	double dspace1 = n0_dspace*dspace2;
	int n1_dspace = int(2.*M_PI/qmin/dspace2);

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
	// analyse
	////////////////////////////////////////////////////////////////////////
	g_log.information() << "____________________________________________________ "  << std::endl;
	g_log.information() << "_Poldi  time conf ---------------------------------  "  << std::endl;

	double summdbr=0.;
	for (size_t i = 0; i < nb_det_channel; i ++){
		if(table_dead_wires[i]){
			summdbr += time_TOF_for_1A[i];
		}
	}
	summdbr *=  dspace2 * static_cast<double>(n_d_space)/time_delta_t;
	g_log.debug() << "_Poldi -        summdbr                            " <<  summdbr  << std::endl;



	double sudbr2 = 0;
	vector<double> subr; subr.resize(n_d_space);
	vector<double> dint; dint.resize(n_d_space);
	for (size_t d_i = 0; d_i < n_d_space; d_i ++){
		subr[d_i] = 0.;
		dint[d_i] = 0.;
		for (size_t wire = 0; wire < nb_det_channel; wire ++){
			if(table_dead_wires[wire]){
				subr[d_i] += time_TOF_for_1A[wire] ;
			}
		}
		subr[d_i] *= dspace2 / time_delta_t;
		sudbr2 += subr[d_i];
	}
	g_log.debug() << "_Poldi -        summdbr-sudbr2                     " << (summdbr-sudbr2) << " : " << sudbr2 << std::endl;



	g_log.debug() << "_XXXXX -        subr                  " <<  subr[0] << " " << subr [n_d_space]  << std::endl;




	// *******  Calculation of the deviation of the measured value ***********

	vector<double> deld; deld.resize(n_d_space);
	vector<double> wzw; wzw.resize(n_d_space);
	vector<double> cmess; cmess.resize(nb_chopper_slits);
	vector<double> csigm; csigm.resize(nb_chopper_slits);
	vector<double> width; width.resize(nb_det_channel);


	for(size_t wire=0; wire<nb_det_channel; wire++){
		if(table_dead_wires[wire]){
			// ****  Calculation of the sensing elements to be considered
			width[wire] = time_TOF_for_1A[wire]*dspace2/time_delta_t;                               // [A]
		}
	}



	for (size_t d_i = 0; d_i < n_d_space; d_i ++){

		double dspace_i = dspace1+float(d_i)*dspace2;                                            // [A]

		for(size_t slit=0; slit<nb_chopper_slits; slit++){
			cmess[slit]=0;
			csigm[slit]=0;

			for(size_t wire=0; wire<nb_det_channel; wire++){
				if(table_dead_wires[wire]){
					// ****  Calculation of the sensing elements to be considered

					double center = time_t0;                          // offset time             // [mysec]
					center += chopper_slits_pos[slit];                // wait for the right slit // [mysec]
					center += time_TOF_for_1A[wire]*dspace_i;         // tof as fct of d         // [mysec]
					center /= time_delta_t;                                                      // unitless

					double cmin = center-width[wire]/2.;
					double cmax = center+width[wire]/2.;
					double icmin = cmin - static_cast<double>(int(cmin/static_cast<double>(nb_time_elmt))*nb_time_elmt);        // modulo for a float
					double icmax = cmax - static_cast<double>(int(cmax/static_cast<double>(nb_time_elmt))*nb_time_elmt);
					int iicmin = static_cast<int>(int(cmin)%nb_time_elmt);                     // modulo for the equivalent int
					int iicmax = static_cast<int>(int(cmax)%nb_time_elmt);

					double I(0.0), delta_q(0.0);
					if(iicmax>iicmin){
						for(int k = iicmin; k<iicmax; k++){
							delta_q = min(k+1,icmax) - max(k,icmin);                   // range hight limit - range low limit
							I = nhe3(static_cast<int>(wire),k);
							cmess[slit] += I*delta_q/float(max(1.,I));
							csigm[slit] +=   delta_q/float(max(1.,I));
						}
					}
				}
			}
		}



		double csigm0 = csigm[0];
		for(size_t j2=0; j2<nb_chopper_slits; j2++){
			csigm0 = min(csigm0,csigm[j2]);
		}


		deld[d_i] = 0;
//		if(csigm0>0.00001){
		if(true){
			double ivzwzw = 0;
			for(size_t j2=0; j2<nb_chopper_slits; j2++){
				wzw[j2] = cmess[j2]/csigm[j2];
				if(wzw[j2]>0){
					ivzwzw = ivzwzw+1;
				}
				if(wzw[j2]<0){
					ivzwzw = ivzwzw-1;
				}
			}
			if(abs(ivzwzw) == nb_chopper_slits){
				for(size_t j2=0; j2<nb_chopper_slits; j2++){
					deld[d_i] += 1./wzw[j2];
				}
				deld[d_i] = float(nb_chopper_slits*nb_chopper_slits)/deld[d_i] * subr[d_i];
			}
		}
	}


	//    	*** Background subtraction and calculate the correlation of calculated values
	//    	*** Falls bei jedem Q-Wert 1*subr(i) stehen wuerde, waere Summe
	//    	***   gesammte berechnete Detektorintegral = summqdr
	//    	***   If there are for each Q-value 1 * subr (i) would sum would be calculated whole detector integral = summqdr


	double summdet = 0;
	for(size_t i=0; i<nb_det_channel; i++){
		if(table_dead_wires[i]){
			for(size_t j=0; j<nb_time_elmt; j++){
				summdet += nhe3(static_cast<int>(i),static_cast<int>(j));
			}
		}
	}


	g_log.debug() << "_XXXXX -        deld                  " <<  deld[5] << " " << deld [100]  << std::endl;

	double sudeld=0;
	for (size_t i = 0; i < n_d_space; i ++){
		sudeld=sudeld+deld[i];
	}
	double diffdel = sudeld-summdet;

	//    	** Dividing the difference in the individual Q values
	for(size_t i=0; i<n_d_space; i++){
		deld[i] = deld[i]-diffdel*subr[i]/sudbr2;
	}

	double sukor = 0;
	for(size_t i=0; i<n_d_space; i++){
		sukor=sukor+deld[i];
	}

	g_log.debug() << "_Poldi -        sukor-summdet         " <<  (sukor - summdet) << " : " << sukor << std::endl;

	for(size_t i=0; i<n_d_space; i++){
		dint[i] = deld[i];
		deld[i] = 0;
	}

	g_log.debug() << "_Poldi -        dint                  " <<  dint[5] << " " << dint [100]  << std::endl;
	vector<double> qi; qi.resize(n_d_space);
	vector<double> qj; qj.resize(n_d_space);
	for(size_t i=0; i<n_d_space; i++){
		qi[i] = 2.*M_PI/(dspace1+float(i)*dspace2);
		qj[n_d_space-1-i] = 2.*M_PI/(dspace1+float(i)*dspace2);
	}

	try
	{
		Mantid::DataObjects::Workspace2D_sptr outputws;
		outputws = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
			(WorkspaceFactory::Instance().create("Workspace2D",3,n_d_space,n_d_space));

		Mantid::MantidVec& Y = outputws->dataY(0);
		for(size_t j = 0; j < n_d_space; j++){
			Y[j] = dint[n_d_space-1-j];
		}
		// Create and fill another vector for the errors, containing sqrt(count)
//		Mantid::MantidVec& E = outputws->dataE(0);
//		std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
		outputws->setX(0, qj);
		outputws->setX(1, qj);
		outputws->setX(2, qj);

		outputws->setYUnit("Counts");
		outputws->getAxis(0)->title() = "q space [1/nm]";


		setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(outputws));

	}
	catch(Mantid::Kernel::Exception::NotFoundError& )
	{
		throw std::runtime_error("Error when saving the PoldiIPP Results data to Workspace : NotFoundError");
	}
	catch(std::runtime_error &)
	{
		throw std::runtime_error("Error when saving the PoldiIPP Results data to Workspace : runtime_error");
	}


}

/**
   Return the detector intensity for the wire 'wire' at position 'timeChannel'

   @param wire :: IndexIterator object
   @param timeChannel :: single pass through to determine if has key (only for virtual base object)
   @returns The value of the intensity (as a double)
  */
double PoldiAutoCorrelation5::nhe3(int wire, int timeChannel){
	return this->localWorkspace->dataY(399-wire)[timeChannel];
}


/**
   Return the squre root of the input

   @param in :: input value
   @returns The square root of the input value : sqrt(in)
  */
double PoldiAutoCorrelation5::dblSqrt(double in)
{
  return sqrt(in);
}


/**
   Get one specific table value from column name and index.
   Inspired from Poldi pool of algorithm

   @param tableWS :: TableWorkspace containing the data
   @param colname :: name of the colon containing the data
   @param index :: index of the data
   @throw std::invalid_argument Non-exist column name
   @throw std::runtime_error Access column array out of boundary
   @returns The double value store at colname[index]
  */
double PoldiAutoCorrelation5::getTableValue
(
		DataObjects::TableWorkspace_sptr tableWS,
		std::string colname,
		size_t index
		)
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

/**
   Get one specific table value from colone name and label.
   Inspired from Poldi pool of algorithm

   @param tableWS :: TableWorkspace containing the data
   @param colNameLabel :: name of the colon containing the label
   @param colNameValue :: name of the colon containing the data
   @param label :: label of the data
   @throw std::invalid_argument Non-exist column name
   @throw std::runtime_error Access column array out of boundary
   @returns The double value store at colname[index]
  */
double PoldiAutoCorrelation5::getTableValueFromLabel
(
		DataObjects::TableWorkspace_sptr tableWS,
		string colNameLabel,
		string colNameValue,
		string label
		)
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



} // namespace Poldi
} // namespace Mantid
