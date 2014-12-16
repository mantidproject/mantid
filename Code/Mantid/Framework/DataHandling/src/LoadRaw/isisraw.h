#ifndef ISISRAW_H
#define ISISRAW_H

/**
  @file isisraw.h ISIS VMS raw file definitions
  @author Freddie Akeroyd
  @date 4/12/2007
 */

struct ISISCRPT_STRUCT;
#include <stdlib.h>
#include <string.h>
#include "item_struct.h"
#ifdef _WIN32 /* _WIN32 */
#include <time.h>
#endif
#include "MantidKernel/System.h"

/// Run header (80 bytes)
struct HDR_STRUCT {
  char inst_abrv[3]; ///< instrument abbreviated name
  char hd_run[5];    ///< run  number
  char hd_user[20];  ///< user name
  char hd_title[24]; ///< short title
  char hd_date[12];  ///< start date
  char hd_time[8];   ///< start time
  char hd_dur[8];    ///< run duration (uA.hour)
                     /// constructor
  HDR_STRUCT() { memset(this, ' ', sizeof(HDR_STRUCT)); }
};

/// address offsets to various sections (9*4 bytes)
struct ADD_STRUCT {
  int ad_run;  ///<		32 (1+offset_of(ver2))
  int ad_inst; ///<		ad_run + 94
  int ad_se;   ///<		ad_inst + 70 + 2*nmon+(5+nuse)*ndet
  int ad_dae;  ///<		ad_se + 66 + nse*32
  int ad_tcb;  ///<		ad_dae + 65 + 5*ndet
  int ad_user; ///<		ad_tcb + 288 + (ntc1 + 1)
  int ad_data; ///<		ad_user + 2 + ulen
  int ad_log;  ///<		ad_data + 33 +
  int ad_end;  ///<		1+end of file
  /// constructor
  ADD_STRUCT() { memset(this, 0, sizeof(ADD_STRUCT)); }
};

/// user information (8*20 bytes)
struct USER_STRUCT {
  char r_user[20];    ///< name
  char r_daytel[20];  ///< daytime phone
  char r_daytel2[20]; ///< daytime phone
  char r_night[20];   ///< nighttime phone
  char r_instit[20];  ///< institute
  char unused[3][20]; ///< to pad out to 8*20 bytes
                      /// constructor
  USER_STRUCT() { memset(this, ' ', sizeof(USER_STRUCT)); }
};

/// Run parameter block (32*4 bytes)
struct RPB_STRUCT {
  int r_dur;             ///< actual run duration
  int r_durunits;        ///< scaler for above (1=seconds)
  int r_dur_freq;        ///< test interval for above (seconds)
  int r_dmp;             ///< dump interval
  int r_dmp_units;       ///< scaler for above
  int r_dmp_freq;        ///< test interval for above
  int r_freq;            ///< 2**k where source frequency = 50 / 2**k
  float r_gd_prtn_chrg;  ///< good proton charge (uA.hour)
  float r_tot_prtn_chrg; ///< total proton charge (uA.hour)
  int r_goodfrm;         ///< good frames
  int r_rawfrm;          ///< raw frames
  int r_dur_wanted; ///< requested run duration (units as for "duration" above)
  int r_dur_secs;   ///< actual run duration in seconds
  int r_mon_sum1;   ///< monitor sum 1
  int r_mon_sum2;   ///< monitor sum 2
  int r_mon_sum3;   ///< monitor sum 3
  char r_enddate[12]; ///< format DD-MMM-YYYY
  char r_endtime[8];  ///< format HH-MM-SS
  int r_prop;         ///< RB (proposal) number
  int spare[10];      ///< to pad out to 32*4 bytes
                      /// constructor
  RPB_STRUCT() { memset(this, 0, sizeof(RPB_STRUCT)); }
};

/// instrument parameter block (64*4 bytes)
struct IVPB_STRUCT {
  float i_chfreq;     ///< frequency chopper 1 (Hz)
  float freq_c2;      ///< frequency chopper 2 (Hz)
  float freq_c3;      ///< frequency chopper 3 (Hz)
  int delay_c1;       ///< delay chopper 1 (us)
  int delay_c2;       ///< delay chopper 2 (us)
  int delay_c3;       ///< delay chopper 3 (us)
  int delay_error_c1; ///< max delay error chopper 1 (us)
  int delay_error_c2; ///< max delay error chopper 2 (us)
  int delay_error_c3; ///< max delay error chopper 3 (us)
  float i_chopsiz;    ///< dunno
  int aperture_c2;    ///< dunno
  int aperture_c3;    ///< dunno
  int status_c1;      ///< status c1  (run,stopped,stop open)
  int status_c2;      ///< status c2  (run,stopped,stop open)
  int status_c3;      ///< status c3  (run,stopped,stop open)
  int i_mainshut;     ///< main shutter open = 1
  int i_thermshut;    ///< thermal shutter open = 1
  float i_xsect;      ///< beam aperture horizontal (mm)
  float i_ysect;      ///< beam aperture vertical (mm)
  int i_posn;         ///< scattering position (1 or 2, for HRPD)
  int i_mod;          ///< moderator type code
  int i_vacuum;       ///< detector_tank_vacuum 1 = vacuum on
  float i_l1;         ///< L1 scattering length
  int i_rfreq;        ///< rotor_frequency (HET)
  float i_renergy;    ///< (HET)
  float i_rphase;     ///< (HET)
  int i_rslit;        ///< slit package (0="unknown",1="L",2="Med",3="Hi") HET
  int i_slowchop;     ///< (1=on, 0=off) HET
  float i_xcen;       ///< LOQ x centre
  float i_ycen;       ///< LOQ y centre
  int i_bestop;       ///< beam_stop LOQ
  float i_radbest;    ///< beam_stop_radius LOQ
  float i_sddist;     ///< source to detector distance (LOQ)
  float i_foeang;     ///< foe angle LOQ
  float i_aofi;       ///< angle of incidence (CRISP)
  int spare[29];      ///< to pad out to 64*4 bytes
                      /// constructor
  IVPB_STRUCT() { memset(this, 0, sizeof(IVPB_STRUCT)); }
};

/// sample parameter block (64*4 bytes)
struct SPB_STRUCT {
  int e_posn;        ///< sample changer position
  int e_type;        ///< sample type (1=sample+can,2=empty can)
  int e_geom;        ///< sample geometry
  float e_thick;     ///< sample thickness normal to sample (mm)
  float e_height;    ///< sample height (mm)
  float e_width;     ///< sample width (mm)
  float e_omega;     ///< omega sample angle (degrees)
  float e_chi;       ///< chi sample angle (degrees)
  float e_phi;       ///< phi sample angle (degrees)
  float e_scatt;     ///< scattering geometry (1=trans, 2 =reflect)
  float e_xscatt;    ///< sample coherent scattering cross section (barn)
  float samp_cs_inc; ///< sample incoherent cross section
  float samp_cs_abs; ///< sample absorption cross section
  float e_dens;      ///< sample number density (atoms.A-3)
  float e_canthick;  ///< can wall thickness (mm)
  float e_canxsect;  ///< can coherent scattering cross section (barn)
  float can_cs_inc;  ///< dunno
  float can_cs_abs;  ///< dunno
  float can_nd;      ///< can number density (atoms.A-3)
  char e_name[40];   ///< sample name of chemical formula
  int e_equip;       ///< dunno
  int e_eqname;      ///< dunno
  int spare[33];     ///< to bring up to 64*4 bytes
                     /// constructor
  SPB_STRUCT() { memset(this, 0, sizeof(SPB_STRUCT)); }
};

/// sample environment block (32*4 bytes)
struct SE_STRUCT {
  char sep_name[8];     ///< SE block name
  int sep_value;        ///< dunno
  int sep_exponent;     ///< dunno
  char sep_units[8];    ///< units of value
  int sep_low_trip;     ///< dunno
  int sep_high_trip;    ///< dunno
  int sep_cur_val;      ///< dunno
  int sep_status;       ///< are we in bounds
  int sep_control;      ///< controlled parameter  (true/false)
  int sep_run;          ///< run control parameter (true/false)
  int sep_log;          ///< logged parameter (true/false)
  float sep_stable;     ///< units per sec
  float sep_period;     ///< monitor repeat period
  int sep_cam_addr;     ///< camac location N
  int sep_cam_sub;      ///< camac location A
  int sep_offset;       ///< CAMAC offset (added to value)
  int sep_cam_rgrp;     ///< camac register group (1 or 2)
  int sep_pre_proc;     ///< pre process routine number
  int sep_cam_vals[12]; ///< camac values
                        /// constructor
  SE_STRUCT() { memset(this, 0, sizeof(SE_STRUCT)); }
};

/// DAE parameters block (64*4 bytes)
struct DAEP_STRUCT {
  int a_pars;           ///<          Word length in bulk store memory
  int mem_size;         ///<          Length of bulk store memory (bytes)**A
  int a_minppp;         ///<          PPP minimum value                  ***B
  int ppp_good_high;    ///<          good PPP total (high 32 bits)   ***B
  int ppp_good_low;     ///<          good PPP total (low  32 bits)   ***B
  int ppp_raw_high;     ///<          raw  PPP total (high 32 bits)   ***B
  int ppp_raw_low;      ///<          raw  PPP total (low  32 bits)   ***B
  int neut_good_high;   ///<          good ext. neut tot (high 32bits)***B
  int neut_good_low;    ///<          good ext. neut tot (low  32 bits)***B
  int neut_raw_high;    ///<          raw  ext. neut tot (high 32 bits)***B
  int neut_raw_low;     ///<          raw  ext. neut tot (low  32 bits)***B
  int neut_gate_t1;     ///<          ext. neutron gate (t1) (�s)   ***B
  int neut_gate_t2;     ///<          ext. neutron gate (t2) (�s)   ***B
  int mon1_detector;    ///<          detector for MON 1  (12 bits)   ***B
  int mon1_module;      ///<          module   for MON 1  ( 4 bits)   ***B
  int mon1_crate;       ///<          crate    for MON 1  ( 4 bits)   ***B
  int mon1_mask;        ///<          mask     for MON 1  (c4:m4:d12) ***B
  int mon2_detector;    ///<          detector for MON 2  (12 bits)   ***B
  int mon2_module;      ///<          module   for MON 2  ( 4 bits)   ***B
  int mon2_crate;       ///<          crate    for MON 2  ( 4 bits)   ***B
  int mon2_mask;        ///<          mask     for MON 2  (c4:m4:d12) ***B
  int events_good_high; ///<  total GOOD EVENTS (high 32 bits)***B
  int events_good_low;  ///<  total GOOD EVENTS (low  32 bits)***B
  int a_delay;          ///<  frame synch delay (4�s steps) ***B
  int a_sync;           ///<          frm snch origin(0:none/1:ext/2:int)***B
  int a_smp;            ///<			    Secondary Master Pulse (0:en,1:dis)
  int ext_vetos[3];     ///<			External vetoes 0,1,2 (0 dis,1
  /// en)
  // extra bits for new PC DAE
  int n_tr_shift;  ///< set to number of shifted time regimes
  int tr_shift[3]; ///< set to shift value (us) of each TR if (using_tr_shift >
  /// 0)
  int spare[31]; ///< to pad to 64*4 bytes
                 /// constructor
  DAEP_STRUCT() { memset(this, 0, sizeof(DAEP_STRUCT)); }
};

/// data section header (32*4 bytes)
struct DHDR_STRUCT {
  int d_comp;         ///< compression type (0=none, 1 = byte relative)
  int reserved;       ///< unused
  int d_offset;       ///< offset to spectrum descriptor array
  float d_crdata;     ///< compression ratio for data
  float d_crfile;     ///< compression ratio for whole file
  int d_exp_filesize; ///< equivalent version 1 filesize
  int unused[26];     ///< to bring size to 32*4 bytes
  /// constructor
  DHDR_STRUCT() {
    memset(this, 0, sizeof(DHDR_STRUCT));
    d_comp = 1;
    d_offset = 1 + 32;
  }
};

/// 2*4 bytes
struct DDES_STRUCT {
  int nwords; ///< number of compressed words in spectrum
  int offset; ///< offset to compressed spectrum
  /// constructor
  DDES_STRUCT() : nwords(0), offset(0) {}
};

/// log line entry
struct LOG_LINE {
  int len;    ///< real length of data
  char *data; ///< padded to multiple of 4 bytes
  /// constructor
  LOG_LINE() : len(0), data(0) {}
};

/// log line entry
struct LOG_STRUCT {
  int ver;         ///< = 2
  int nlines;      ///< number of lines
  LOG_LINE *lines; ///< size nlines
  /// constructor
  LOG_STRUCT() : ver(2), nlines(0), lines(0) {}
};

/// isis raw file.
//  isis raw
class ISISRAW {
private:
  ISISCRPT_STRUCT *m_crpt;         ///< CRPT from ICP
  item_struct<char> m_char_items;  ///< dunno
  item_struct<float> m_real_items; ///< dunno
  item_struct<int> m_int_items;    ///< dunno
  int addItems();

public:
  // section 1
  HDR_STRUCT hdr;        ///< header block (80 bytes)
  int frmt_ver_no;       ///< format version number VER1 (=2)
  struct ADD_STRUCT add; ///< 9*4 bytes
  int data_format;       ///< data section format (0 = by TC, 1 = by spectrum)
  // section 2
  int ver2;         ///< run section version number VER2 (=1)
  int r_number;     ///< run number
  char r_title[80]; ///< run title
  USER_STRUCT user; ///< user information (8*20 bytes)
  RPB_STRUCT rpb;   ///< run parameter block (32*4 bytes)
  // section 3
  int ver3;         ///< instrument section version number (=2)
  char i_inst[8];   ///< instrument name
  IVPB_STRUCT ivpb; ///< instrument parameter block (64*4 bytes)
  int i_det;        ///< number of detectors NDET
  int i_mon;        ///< number of monitors NMON
  int i_use;        ///< number of user defined UTn tables NUSE
  // I_TABLES is address of MDET
  int *mdet;   ///< detector number for monitors (size NMON)
  int *monp;   ///< prescale value for each monitor (size NMON)
  int *spec;   ///< spectrum number table (size NDET)
  float *delt; ///< hold off table (size NDET)
  float *len2; ///< L2 table (size NDET)
  int *code;   ///< code for UTn tables (size NDET)
  float *tthe; ///< 2theta scattering angle (size NDET)
  float *ut;   ///< nuse UT* user tables (total size NUSE*NDET) ut01=phi
  // section 4
  int ver4;       ///< SE section version number (=2)
  SPB_STRUCT spb; ///< sample parameter block (64*4 bytes)
  int e_nse;      ///< number of controlled SEPs NSEP
  SE_STRUCT *
      e_seblock; ///< NSEP SE parameter blocks (total size NSEP*32*4 bytes)
  // section 5
  int ver5;         ///< DAE section version number (=2)
  DAEP_STRUCT daep; ///< DAE parameter block (size 64*4 bytes)
  // A_TABLES points at CRAT
  int *crat; ///< crate number for each detector (size NDET)
  int *modn; ///< module number for each detector (size NDET)
  int *mpos; ///< module position for each detector (size NDET)
  int *timr; ///< time regime for each detector (size NDET)
  int *udet; ///< user detector number for each detector (size NDET)
  // section 6
  int ver6;           ///< TCB section version number (=1)
  int t_ntrg;         ///< number of time regimes (=1)
  int t_nfpp;         ///< number of frames per period
  int t_nper;         ///< number of periods
  int t_pmap[256];    ///< period number for each basic period
  int t_nsp1;         ///< number of spectra in time regime 1
  int t_ntc1;         ///< number of time channels in time regime 1
  int t_tcm1[5];      ///< time channel mode
  float t_tcp1[5][4]; ///< time channel parameters
  int t_pre1;         ///< prescale for 32MHz clock
  int *t_tcb1;        ///< time channel boundaries in clock pulses (size NTC1+1)
  // section 7
  // write NCHAN = NTC1+1 time channel boundaries
  int ver7;     ///< user version number (=1)
  int u_len;    ///< length of user data section
  float *u_dat; ///< user defined data (ULEN, max size 400 words)
  // section 8
  int ver8;         ///< data version number (=2)
  DHDR_STRUCT dhdr; ///< size 32*4 bytes
  // D_DATA points at ddes
  DDES_STRUCT *
      ddes;       ///< (NSP1+1)*NPER items, totoal size (NSP1+1)*NPER*2*4 bytes
  uint32_t *dat1; ///< compressed data for (NTC1+1)*(NSP1+1)*NPER values
  LOG_STRUCT logsect; ///< log section

public:
  static int vmstime(char *timbuf, int len, time_t time_value);
  int size_check();
  ISISRAW();
  virtual ~ISISRAW();
  ISISRAW(ISISCRPT_STRUCT *crpt);
  ISISRAW(ISISCRPT_STRUCT *crpt, bool doUpdateFromCRPT);
  int updateFromCRPT();

  virtual int ioRAW(FILE *file, bool from_file, bool do_data = true);
  int ioRAW(FILE *file, HDR_STRUCT *s, int len, bool from_file);
  int ioRAW(FILE *file, ADD_STRUCT *s, int len, bool from_file);
  int ioRAW(FILE *file, USER_STRUCT *s, int len, bool from_file);
  int ioRAW(FILE *file, RPB_STRUCT *s, int len, bool from_file);
  int ioRAW(FILE *file, IVPB_STRUCT *s, int len, bool from_file);
  int ioRAW(FILE *file, SPB_STRUCT *s, int len, bool from_file);
  int ioRAW(FILE *file, SE_STRUCT *s, int len, bool from_file);
  int ioRAW(FILE *file, DAEP_STRUCT *s, int len, bool from_file);
  int ioRAW(FILE *file, DHDR_STRUCT *s, int len, bool from_file);
  int ioRAW(FILE *file, DDES_STRUCT *s, int len, bool from_file);
  int ioRAW(FILE *file, LOG_STRUCT *s, int len, bool from_file);
  int ioRAW(FILE *file, LOG_LINE *s, int len, bool from_file);
  int ioRAW(FILE *file, char *val, int len, bool from_file);
  int ioRAW(FILE *file, int *val, int len, bool from_file);
  int ioRAW(FILE *file, uint32_t *val, int len, bool from_file);
  int ioRAW(FILE *file, float *val, int len, bool from_file);
  // allocate
  int ioRAW(FILE *file, char **val, int len, bool from_file);
  int ioRAW(FILE *file, int **val, int len, bool from_file);
  int ioRAW(FILE *file, uint32_t **val, int len, bool from_file);
  int ioRAW(FILE *file, float **val, int len, bool from_file);
  int ioRAW(FILE *file, SE_STRUCT **s, int len, bool from_file);
  int ioRAW(FILE *file, DDES_STRUCT **s, int len, bool from_file);
  int ioRAW(FILE *file, LOG_LINE **s, int len, bool from_file);
  int readFromFile(const char *filename, bool read_data = true);
  int writeToFile(const char *filename);
  int printInfo(std::ostream &os);
  int getTimeChannels(float *rtcb1, int n);
};

#endif /* ISISRAW_H */
