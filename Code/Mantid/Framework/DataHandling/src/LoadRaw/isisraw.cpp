#include <iostream>
#include <cstdio>
#include "isisraw.h"
#include "vms_convert.h"
#include "byte_rel_comp.h"

#define SUCCESS 0
#define FAILURE 1

/// stuff
ISISRAW::ISISRAW() : m_crpt(0), dat1(0) {
  int i, j;
  // section 1
  frmt_ver_no = 2; // format version number VER1 (=2)
  data_format = 0; // data section format (0 = by TC, 1 = by spectrum)
  // section 2
  ver2 = 1;                              // run section version number VER2 (=1)
  r_number = 0;                          // run number
  memset(r_title, ' ', sizeof(r_title)); // run title
  // section 3
  ver3 = 2;                            // instrument section version number (=2)
  memset(i_inst, ' ', sizeof(i_inst)); // instrument name
  i_det = 1;                           // number of detectors NDET
  i_mon = 1;                           // number of monitors NMON
  i_use = 1;                           // number of user defined UTn tables NUSE
  // I_TABLES is address of MDET
  mdet = new int[i_mon]; // detector number for monitors (size NMON)
  monp = new int[i_mon]; // prescale value for each monitor (size NMON)
  for (i = 0; i < i_mon; i++) {
    mdet[i] = i + 1;
    monp[i] = 1;
  }
  spec = new int[i_det];   // spectrum number table (size NDET)
  delt = new float[i_det]; // hold off table (size NDET)
  len2 = new float[i_det]; // L2 table (size NDET)
  code = new int[i_det];   // code for UTn tables (size NDET)
  tthe = new float[i_det]; // 2theta scattering angle (size NDET)
  ut = new float[i_use *
                 i_det]; // nuse UT* user tables (total size NUSE*NDET) ut01=phi
  for (i = 0; i < i_det; i++) {
    spec[i] = i + 1; // we have t_nsp1 = i_det
    delt[i] = (float)i;
    len2[i] = (float)i;
    code[i] = i + 1;
    tthe[i] = (float)i;
  }
  for (i = 0; i < i_use; i++) {
    for (j = 0; j < i_det; j++) {
      ut[i * i_det + j] = (float)j;
    }
  }
  // section 4
  ver4 = 2;                         // SE section version number (=2)
  e_nse = 1;                        // number of controlled SEPs NSEP
  e_seblock = new SE_STRUCT[e_nse]; // NSEP SE parameter blocks (total size
                                    // NSEP*32*4 bytes)
  // section 5
  ver5 = 2;              // DAE section version number (=2)
  crat = new int[i_det]; // crate number for each detector (size NDET)
  modn = new int[i_det]; // module number for each detector (size NDET)
  mpos = new int[i_det]; // module position for each detector (size NDET)
  timr = new int[i_det]; // time regime for each detector (size NDET)
  udet = new int[i_det]; // user detector number for each detector (size NDET)
  for (i = 0; i < i_det; i++) {
    crat[i] = 1;
    modn[i] = 1;
    mpos[i] = i;
    timr[i] = 1;
    udet[i] = i;
  }
  // section 6
  ver6 = 1;   // TCB section version number (=1)
  t_ntrg = 1; // number of time regimes (=1)
  t_nfpp = 1; // number of frames per period
  t_nper = 1; // number of periods
  for (i = 0; i < 256; i++) {
    t_pmap[i] = 1; // period number for each basic period
  }
  t_nsp1 = i_det;                    // number of spectra in time regime 1
  t_ntc1 = 10;                       // number of time channels in time regime 1
  memset(t_tcm1, 0, sizeof(t_tcm1)); // time channel mode
  memset(t_tcp1, 0, sizeof(t_tcp1)); // time channel parameters
  t_pre1 = 1;                        // prescale for 32MHz clock
  t_tcb1 = new int[t_ntc1 +
                   1]; // time channel boundaries in clock pulses (size NTC1+1)
  for (i = 0; i < t_ntc1 + 1; i++) {
    t_tcb1[i] = i;
  }
  // section 7
  // write NCHAN = NTC1+1 time channel boundaries
  ver7 = 1; // user version number (=1)
  u_len = 1;
  u_dat = new float[u_len]; // user defined data (ULEN, max size 400 words)
  for (i = 0; i < u_len; i++) {
    u_dat[i] = (float)i;
  }
  // section 8
  ver8 = 2; // data version number (=2)
  // D_DATA points at ddes
  ddes = new DDES_STRUCT[(t_nsp1 + 1) * t_nper]; // (NSP1+1)*NPER items, totoal
                                                 // size (NSP1+1)*NPER*2*4 bytes
  dat1 =
      new uint32_t[(t_ntc1 + 1) * (t_nsp1 + 1) *
                   t_nper]; // compressed data for (NTC1+1)*(NSP1+1)*NPER values
  for (i = 0; i < (t_ntc1 + 1) * (t_nsp1 + 1) * t_nper; i++) {
    dat1[i] = i;
  }
#if defined(_WIN32)
// disable warning about strcpy
#pragma warning(disable : 4996)
#endif
  logsect.nlines = 1;
  logsect.lines = new LOG_LINE[logsect.nlines];
  for (i = 0; i < logsect.nlines; i++) {
    // logsect.lines[i].data = "test log line"; //Deprecated
    logsect.lines[i].data = (char *)malloc(16);
    strcpy(logsect.lines[i].data, "test log line");
    logsect.lines[i].len = static_cast<int>(strlen(logsect.lines[i].data));
  }
  addItems();
}

/// stuff
int ISISRAW::addItems() {
  static const int hdr_size = sizeof(hdr) / sizeof(char);
  static const int rrpb_size = sizeof(rpb) / sizeof(float);
  static const int irpb_size = sizeof(rpb) / sizeof(int);
  m_char_items.addItem("HDR", (const char *)&hdr, false, &hdr_size);
  m_real_items.addItem("RRPB", (float *)&rpb, false, &rrpb_size);
  m_int_items.addItem("IRPB", (int *)&rpb, false, &irpb_size);
  return 0;
}

// create one bound to a CRPT
/// stuff
ISISRAW::ISISRAW(ISISCRPT_STRUCT *crpt)
    : m_crpt(crpt), frmt_ver_no(0), data_format(0), ver2(0), r_number(0),
      ver3(0), i_det(0), i_mon(0), i_use(0), mdet(0), monp(0), spec(0), delt(0),
      len2(0), code(0), tthe(0), ut(0), ver4(0), ver5(0), crat(0), modn(0),
      mpos(0), timr(0), udet(0), ver6(0), t_ntrg(0), t_nfpp(0), t_nper(0),
      t_nsp1(0), t_ntc1(0), t_pre1(0), t_tcb1(0), ver7(0), u_dat(0), ver8(0),
      ddes(0), dat1(0) {
  memset(r_title, ' ', sizeof(r_title));
  memset(i_inst, ' ', sizeof(i_inst));
  for (int i = 0; i < 256; i++) {
    t_pmap[i] = 1; // period number for each basic period
  }
  memset(t_tcm1, 0, sizeof(t_tcm1)); // time channel mode
  memset(t_tcp1, 0, sizeof(t_tcp1)); // time channel parameters
  e_nse = 0;
  e_seblock = 0;
  u_len = 0;
  logsect.nlines = 0;
  logsect.lines = 0;
  addItems();
  updateFromCRPT();
}

// create one bound to a CRPT
/// stuff
ISISRAW::ISISRAW(ISISCRPT_STRUCT *crpt, bool doUpdateFromCRPT)
    : m_crpt(crpt), frmt_ver_no(0), data_format(0), ver2(0), r_number(0),
      ver3(0), i_det(0), i_mon(0), i_use(0), mdet(0), monp(0), spec(0), delt(0),
      len2(0), code(0), tthe(0), ut(0), ver4(0), ver5(0), crat(0), modn(0),
      mpos(0), timr(0), udet(0), ver6(0), t_ntrg(0), t_nfpp(0), t_nper(0),
      t_nsp1(0), t_ntc1(0), t_pre1(0), t_tcb1(0), ver7(0), u_dat(0), ver8(0),
      ddes(0), dat1(0) {
  memset(r_title, ' ', sizeof(r_title));
  memset(i_inst, ' ', sizeof(i_inst));
  for (int i = 0; i < 256; i++) {
    t_pmap[i] = 1; // period number for each basic period
  }
  memset(t_tcm1, 0, sizeof(t_tcm1)); // time channel mode
  memset(t_tcp1, 0, sizeof(t_tcp1)); // time channel parameters
  e_nse = 0;
  e_seblock = 0;
  u_len = 0;
  logsect.nlines = 0;
  logsect.lines = 0;
  addItems();
  if (doUpdateFromCRPT) {
    updateFromCRPT();
  }
}

// update from bound CRPT
/// stuff
int ISISRAW::updateFromCRPT() {
  if (m_crpt == NULL) {
    return 0;
  }
#ifndef REAL_CRPT
  return 0;
#else
  int i;
  char buffer[256];
  spacePadCopy(hdr.inst_abrv, m_crpt->inst_abrv, sizeof(hdr.inst_abrv));
  sprintf(buffer, "%05d", m_crpt->run_number);
  spacePadCopy(hdr.hd_run, buffer, sizeof(hdr.hd_run));
  spacePadCopy(hdr.hd_user, m_crpt->user_name, sizeof(hdr.hd_user));
  //	spacePadCopy(hdr.hd_title, m_crpt->short_title, sizeof(hdr.hd_title));
  spacePadCopy(hdr.hd_title, m_crpt->long_title, sizeof(hdr.hd_title));
  vmstime(buffer, sizeof(buffer), m_crpt->start_time);
  spacePadCopy(hdr.hd_date, buffer, sizeof(hdr.hd_date) + sizeof(hdr.hd_time));
  sprintf(buffer, "%8.2f", m_crpt->good_uamph);
  spacePadCopy(hdr.hd_dur, buffer, sizeof(hdr.hd_dur));

  // section 1
  frmt_ver_no = 2; // format version number VER1 (=2)
  data_format = 0; // data section format (0 = by TC, 1 = by spectrum)
  // section 2
  ver2 = 1;                      // run section version number VER2 (=1)
  r_number = m_crpt->run_number; // run number
  spacePadCopy(r_title, m_crpt->long_title, sizeof(r_title)); // run title
  spacePadCopy(user.r_user, m_crpt->user_name, sizeof(user.r_user));
  spacePadCopy(user.r_instit, m_crpt->institute, sizeof(user.r_instit));
  // rpb
  rpb.r_dur = m_crpt->duration;
  rpb.r_durunits = 1; // seconds
  rpb.r_freq = 1;     // 50Hz
  rpb.r_gd_prtn_chrg = m_crpt->good_uamph;
  rpb.r_tot_prtn_chrg = m_crpt->total_uamph;
  rpb.r_goodfrm = m_crpt->good_frames;
  rpb.r_rawfrm = m_crpt->total_frames;
  rpb.r_dur_secs = m_crpt->duration;
  // TODO
  rpb.r_mon_sum1 = 0; // m_crpt->monitor_sum[0];
  rpb.r_mon_sum2 = 0;
  rpb.r_mon_sum3 = 0;
  vmstime(buffer, sizeof(buffer), m_crpt->stop_time);
  spacePadCopy(rpb.r_enddate, buffer,
               sizeof(rpb.r_enddate) + sizeof(rpb.r_endtime));
  rpb.r_prop = m_crpt->rb_number;

  // section 3
  ver3 = 2; // instrument section version number (=2)
  spacePadCopy(i_inst, m_crpt->inst_name, sizeof(i_inst)); // instrument name
  ivpb.i_l1 = m_crpt->i_l1;
  if (!strcmp(m_crpt->beamstop_position, "IN")) {
    ivpb.i_bestop = 0;
  } else {
    ivpb.i_bestop = 1;
  }
  ivpb.i_xsect = m_crpt->aperture1;
  ivpb.i_ysect = m_crpt->aperture2;
  ivpb.i_sddist = m_crpt->sdd;

  ivpb.i_foeang = m_crpt->foe_mirror_angle;
  ivpb.i_xcen = m_crpt->beam_centre_x;
  ivpb.i_ycen = m_crpt->beam_centre_y;
  ivpb.i_chopsiz = m_crpt->chopper_opening_angle;
  ivpb.i_aofi = m_crpt->angle_of_incidence;

  spb.e_phi = m_crpt->sample_phi_angle;
  spb.e_height = m_crpt->sample_height;
  spb.e_width = m_crpt->sample_width;
  spb.e_thick = m_crpt->sample_thickness;
  if (!stricmp(m_crpt->sample_type, "sample+can")) {
    spb.e_type = 1;
  } else if (!stricmp(m_crpt->sample_type, "empty can")) {
    spb.e_type = 2;
  } else if (!stricmp(m_crpt->sample_type, "vanadium")) {
    spb.e_type = 3;
  } else if (!stricmp(m_crpt->sample_type, "absorber")) {
    spb.e_type = 4;
  } else if (!stricmp(m_crpt->sample_type, "nothing")) {
    spb.e_type = 5;
  } else if (!stricmp(m_crpt->sample_type, "sample, no can")) {
    spb.e_type = 6;
  } else {
    spb.e_type = 1;
  }

  if (!stricmp(m_crpt->sample_geometry, "cylinder") ||
      !stricmp(m_crpt->sample_geometry, "cylindrical")) {
    spb.e_geom = 1;
  } else if (!stricmp(m_crpt->sample_geometry, "flat plate")) {
    spb.e_geom = 2;
  } else if (!stricmp(m_crpt->sample_geometry, "disc")) {
    spb.e_geom = 3;
  } else if (!stricmp(m_crpt->sample_geometry, "single crystal")) {
    spb.e_geom = 4;
  } else {
    spb.e_geom = 1;
  }

  i_det = m_crpt->ndet; // number of detectors NDET
  i_mon = m_crpt->nmon; // number of monitors NMON
  i_use = m_crpt->nuse; // number of user defined UTn tables NUSE
  // I_TABLES is address of MDET
  mdet = m_crpt->mdet; // detector number for monitors (size NMON)
  monp = m_crpt->monp; // prescale value for each monitor (size NMON)
  spec = m_crpt->spec; // spectrum number table (size NDET)
  delt = m_crpt->delt; // hold off table (size NDET)
  len2 = m_crpt->len2; // L2 table (size NDET)
  code = m_crpt->code; // code for UTn tables (size NDET)
  tthe = m_crpt->tthe; // 2theta scattering angle (size NDET)
  ut = (float *)
       m_crpt->ut; // nuse UT* user tables (total size NUSE*NDET) ut01=phi

  // section 4
  ver4 = 2;  // SE section version number (=2)
  e_nse = 1; // number of controlled SEPs NSEP
  delete[] e_seblock;
  e_seblock = new SE_STRUCT[e_nse]; // NSEP SE parameter blocks (total size
                                    // NSEP*32*4 bytes)
  // section 5
  ver5 = 2;        // DAE section version number (=2)
  daep.a_pars = 4; // 32bit dae memory
  daep.mem_size =
      daep.a_pars * (m_crpt->nsp1 + 1) * (m_crpt->ntc1 + 1) * m_crpt->nper_daq;

  daep.ppp_good_high = m_crpt->good_ppp_high;
  daep.ppp_good_low = m_crpt->good_ppp_low;
  daep.ppp_raw_high = m_crpt->total_ppp_high;
  daep.ppp_raw_low = m_crpt->total_ppp_low;
  daep.mon1_detector = 0; // m_crpt->udet[m_crpt->mdet[0] - 1];
  daep.mon1_module = 0;
  daep.mon1_crate = 0;
  daep.mon1_mask = 0;
  daep.mon2_detector = 0;
  daep.mon2_module = 0;
  daep.mon2_crate = 0;
  daep.mon2_mask = 0;
  daep.a_smp = (m_crpt->vetos[SMPVeto].enabled != 0);
  daep.ext_vetos[0] = (m_crpt->vetos[Ext0Veto].enabled != 0);
  daep.ext_vetos[1] = (m_crpt->vetos[Ext1Veto].enabled != 0);
  daep.ext_vetos[2] = (m_crpt->vetos[Ext2Veto].enabled != 0);
  daep.a_delay = m_crpt->tcb_delay / 4;
  if (m_crpt->tcb_sync == FrameSyncInternalTest) {
    daep.a_sync = 1;
  } else {
    daep.a_sync = 0; // external
  }
  // two time regime mods
  if (m_crpt->ntrg > 1) {
    daep.n_tr_shift = m_crpt->ntrg;
    for (i = 0; i < m_crpt->ntrg; i++) {
      daep.tr_shift[i] = m_crpt->tcb_trdelay[i];
    }
  } else {
    daep.n_tr_shift = 0;
  }
  crat = m_crpt->crat; // crate number for each detector (size NDET)
  modn = m_crpt->modn; // module number for each detector (size NDET)
  mpos = m_crpt->mpos; // module position for each detector (size NDET)
  timr = m_crpt->timr; // time regime for each detector (size NDET)
  udet = m_crpt->udet; // user detector number for each detector (size NDET)
  // section 6
  ver6 = 1;                  // TCB section version number (=1)
  t_ntrg = 1;                // number of time regimes (=1)
  t_nfpp = 1;                // number of frames per period
  t_nper = m_crpt->nper_daq; // number of periods
  for (i = 0; i < 256; i++) {
    t_pmap[i] = 1; // period number for each basic period
  }
  t_nsp1 = m_crpt->nsp1;             // number of spectra in time regime 1
  t_ntc1 = m_crpt->ntc1;             // number of time channels in time regime 1
                                     // TODO
  memset(t_tcm1, 0, sizeof(t_tcm1)); // time channel mode
  memset(t_tcp1, 0, sizeof(t_tcp1)); // time channel parameters
  t_pre1 = 1;                        // prescale for 32MHz clock
  t_tcb1 =
      m_crpt->tcb1; // time channel boundaries in clock pulses (size NTC1+1)
  // section 7
  // write NCHAN = NTC1+1 time channel boundaries
  ver7 = 1; // user version number (=1)
  u_len = 1;
  delete[] u_dat;
  u_dat = new float[u_len]; // user defined data (ULEN, max size 400 words)
  for (i = 0; i < u_len; i++) {
    u_dat[i] = (float)i;
  }
  // section 8
  ver8 = 2; // data version number (=2)
  // D_DATA points at ddes
  if ((t_nsp1 != m_nsp1) || (t_nper != m_nper)) {
    delete[] ddes;
    ddes = new DDES_STRUCT[(t_nsp1 + 1) * t_nper]; // (NSP1+1)*NPER items,
                                                   // totoal size
                                                   // (NSP1+1)*NPER*2*4 bytes
  }
  dat1 = (uint32_t *)m_crpt->raw_data;
  if (m_crpt->log_nlines > 0) {
    logsect.nlines = m_crpt->log_nlines;
    delete[] logsect.lines;
    logsect.lines = new LOG_LINE[logsect.nlines];
    for (i = 0; i < logsect.nlines; i++) {
      logsect.lines[i].data = &(m_crpt->log_data[i][0]);
      logsect.lines[i].len = strlen(logsect.lines[i].data);
    }
  } else {
    logsect.nlines = 0;
    logsect.lines = NULL;
  }
  // for caching on updates
  m_ntc1 = t_ntc1;
  m_nsp1 = t_nsp1;
  m_nper = t_nper;
  return 0;
#endif /* REAL_CRPT */
}

/// stuff
int ISISRAW::ioRAW(FILE *file, bool from_file, bool read_data) {
  int ndata, len_log, i;
  fpos_t add_pos, dhdr_pos, keep_pos;
  if (!from_file) {
    add.ad_run = 32;
    add.ad_inst = add.ad_run + 94;
    add.ad_se = add.ad_inst + 70 + 2 * i_mon + (5 + i_use) * i_det;
    add.ad_dae = add.ad_se + 66 + e_nse * 32;
    add.ad_tcb = add.ad_dae + 65 + 5 * i_det;
    add.ad_user = add.ad_tcb + 288 + (t_ntc1 + 1);
    add.ad_data = add.ad_user + 2 + u_len;
    add.ad_log = 0; // we don't know it yet
    add.ad_end = 0;
  }
  ioRAW(file, &hdr, 1, from_file);
  ioRAW(file, &frmt_ver_no, 1, from_file);
  fgetpos(file, &add_pos);
  ioRAW(file, &add, 1, from_file);
  ioRAW(file, &data_format, 3, from_file);
  ioRAW(file, r_title, 80, from_file);
  ioRAW(file, &user, 1, from_file);
  ioRAW(file, &rpb, 1, from_file);
  ioRAW(file, &ver3, 1, from_file);
  ioRAW(file, i_inst, 8, from_file);
  ioRAW(file, &ivpb, 1, from_file);
  ioRAW(file, &i_det, 3, from_file);
  ioRAW(file, &mdet, i_mon, from_file);
  ioRAW(file, &monp, i_mon, from_file);
  ioRAW(file, &spec, i_det, from_file);
  ioRAW(file, &delt, i_det, from_file);
  ioRAW(file, &len2, i_det, from_file);
  ioRAW(file, &code, i_det, from_file);
  ioRAW(file, &tthe, i_det, from_file);
  ioRAW(file, &ut, i_use * i_det, from_file);
  ioRAW(file, &ver4, 1, from_file);
  ioRAW(file, &spb, 1, from_file);
  ioRAW(file, &e_nse, 1, from_file);
  ioRAW(file, &e_seblock, e_nse, from_file);
  ioRAW(file, &ver5, 1, from_file);
  ioRAW(file, &daep, 1, from_file);
  ioRAW(file, &crat, i_det, from_file);
  ioRAW(file, &modn, i_det, from_file);
  ioRAW(file, &mpos, i_det, from_file);
  ioRAW(file, &timr, i_det, from_file);
  ioRAW(file, &udet, i_det, from_file);
  ioRAW(file, &ver6, 267, from_file);
  ioRAW(file, &(t_tcp1[0][0]), 20, from_file);
  ioRAW(file, &t_pre1, 1, from_file);
  ioRAW(file, &t_tcb1, t_ntc1 + 1, from_file);
  ioRAW(file, &ver7, 1, from_file);
  // it appear that the VMS ICP traditionally sets u_len to 1 regardless
  // of its real size; thus we cannot rely on it for reading and must instead
  // use section offsets
  i = 0;
  ioRAW(file, &i, 1, from_file);
  //		ioRAW(file, &u_len, 1, from_file);
  if (from_file) {
    u_len = add.ad_data - add.ad_user - 2;
  }
  ioRAW(file, &u_dat, u_len, from_file);
  ioRAW(file, &ver8, 1, from_file);
  fgetpos(file, &dhdr_pos);
  ioRAW(file, &dhdr, 1, from_file);
  int ndes, nout, nwords, outbuff_size = 100000, offset;
  char *outbuff = new char[outbuff_size];
  if (!read_data) {
    ndes = ndata = 0;
    dat1 = NULL;
    // seek to position right after the data if we want to read the log
    if (from_file) {
      ndes = t_nper * (t_nsp1 + 1);
      ioRAW(file, &ddes, ndes, from_file);
      for (i = 0; i < ndes; i++)
        fseek(file, 4 * ddes[i].nwords, SEEK_CUR);
    }
  } else if (dhdr.d_comp == 0) {
    ndata = t_nper * (t_nsp1 + 1) * (t_ntc1 + 1);
    ndes = 0;
    ioRAW(file, &dat1, ndata, from_file);
  } else {
    ndata = 0;
    ndes = t_nper * (t_nsp1 + 1);
    ioRAW(file, &ddes, ndes, from_file);
    if (from_file) {
      dat1 = new uint32_t[ndes * (t_ntc1 + 1)];
    }
    offset = 33 + ndes * 2;
    memset(outbuff, 0,
           outbuff_size); // so when we round up words we get a zero written
    for (i = 0; i < ndes; i++) {
      if (from_file) {
        nwords = ddes[i].nwords;
        ioRAW(file, outbuff, 4 * nwords, from_file);
        byte_rel_expn(outbuff, 4 * nwords, 0, (int *)&dat1[i * (t_ntc1 + 1)],
                      t_ntc1 + 1);
      } else {
        byte_rel_comp((int *)&dat1[i * (t_ntc1 + 1)], t_ntc1 + 1, outbuff,
                      outbuff_size, nout);
        nwords = (3 + nout) / 4; // round up to words
        ddes[i].nwords = nwords;
        ddes[i].offset = offset + ndata;
        ndata += nwords;
        ioRAW(file, outbuff, 4 * nwords, from_file);
      }
    }
  }
  delete[] outbuff;
  // log section
  ioRAW(file, &logsect, 1, from_file);
  len_log = 2 + logsect.nlines;
  for (i = 0; i < logsect.nlines; i++) {
    len_log += (1 + (logsect.lines[i].len - 1) / 4);
  }
  if (!from_file) {
    add.ad_log = add.ad_data + 33 + 2 * ndes + ndata;
    add.ad_end = add.ad_log + len_log;
    int curr_data_size = add.ad_log - add.ad_data;
    int uncomp_data_size = 33 + t_nper * (t_nsp1 + 1) * (t_ntc1 + 1);
    int curr_filesize = add.ad_end - 1;
    int uncomp_filesize = add.ad_data - 1 + uncomp_data_size + len_log;
    dhdr.d_crdata = (float)uncomp_data_size / (float)curr_data_size;
    dhdr.d_crfile = (float)uncomp_filesize / (float)curr_filesize;
    dhdr.d_exp_filesize =
        uncomp_filesize /
        128; // in 512 byte blocks (vms default allocation unit)
    fgetpos(file, &keep_pos);
    // update section addresses
    fsetpos(file, &add_pos);
    ioRAW(file, &add, 1, from_file);
    // update data header and descriptors etc.
    fsetpos(file, &dhdr_pos);
    ioRAW(file, &dhdr, 1, from_file);
    ioRAW(file, &ddes, ndes, from_file);
    fsetpos(file, &keep_pos);
  }
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, HDR_STRUCT *s, int len, bool from_file) {
  ioRAW(file, (char *)s, sizeof(HDR_STRUCT) * len, from_file);
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, ADD_STRUCT *s, int len, bool from_file) {
  ioRAW(file, (int *)s, (sizeof(ADD_STRUCT) * len / sizeof(int)), from_file);
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, USER_STRUCT *s, int len, bool from_file) {
  ioRAW(file, (char *)s, sizeof(USER_STRUCT) * len, from_file);
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, RPB_STRUCT *s, int len, bool from_file) {
  int i;
  for (i = 0; i < len; i++) {
    ioRAW(file, &(s[i].r_dur), 7, from_file);
    ioRAW(file, &(s[i].r_gd_prtn_chrg), 2, from_file);
    ioRAW(file, &(s[i].r_goodfrm), 7, from_file);
    ioRAW(file, s[i].r_enddate, 20, from_file);
    ioRAW(file, &(s[i].r_prop), 11, from_file);
  }
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, IVPB_STRUCT *s, int len, bool from_file) {
  int i;
  for (i = 0; i < len; i++) {
    ioRAW(file, &(s[i].i_chfreq), 3, from_file);
    ioRAW(file, &(s[i].delay_c1), 14, from_file);
    ioRAW(file, &(s[i].i_xsect), 2, from_file);
    ioRAW(file, &(s[i].i_posn), 3, from_file);
    ioRAW(file, &(s[i].i_l1), 1, from_file);
    ioRAW(file, &(s[i].i_rfreq), 1, from_file);
    ioRAW(file, &(s[i].i_renergy), 2, from_file);
    ioRAW(file, &(s[i].i_rslit), 2, from_file);
    ioRAW(file, &(s[i].i_xcen), 2, from_file);
    ioRAW(file, &(s[i].i_bestop), 1, from_file);
    ioRAW(file, &(s[i].i_radbest), 4, from_file);
    ioRAW(file, s[i].spare, 29, from_file);
  }
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, SPB_STRUCT *s, int len, bool from_file) {
  int i;
  for (i = 0; i < len; i++) {
    ioRAW(file, &(s[i].e_posn), 3, from_file);
    ioRAW(file, &(s[i].e_thick), 16, from_file);
    ioRAW(file, s[i].e_name, 40, from_file);
    ioRAW(file, &(s[i].e_equip), 35, from_file);
  }
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, SE_STRUCT *s, int len, bool from_file) {
  int i;
  for (i = 0; i < len; i++) {
    ioRAW(file, s[i].sep_name, 8, from_file);
    ioRAW(file, &(s[i].sep_value), 2, from_file);
    ioRAW(file, s[i].sep_units, 8, from_file);
    ioRAW(file, &(s[i].sep_low_trip), 7, from_file);
    ioRAW(file, &(s[i].sep_stable), 2, from_file);
    ioRAW(file, &(s[i].sep_cam_addr), 17, from_file);
  }
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, DAEP_STRUCT *s, int len, bool from_file) {
  ioRAW(file, (int *)s, sizeof(DAEP_STRUCT) * len / sizeof(int), from_file);
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, DHDR_STRUCT *s, int len, bool from_file) {
  int i;
  for (i = 0; i < len; i++) {
    ioRAW(file, &(s[i].d_comp), 3, from_file);
    ioRAW(file, &(s[i].d_crdata), 2, from_file);
    ioRAW(file, &(s[i].d_exp_filesize), 27, from_file);
  }
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, DDES_STRUCT *s, int len, bool from_file) {
  int i;
  for (i = 0; i < len; i++) {
    ioRAW(file, &(s[i].nwords), 2, from_file);
  }
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, LOG_STRUCT *s, int len, bool from_file) {
  int i;
  for (i = 0; i < len; i++) {
    ioRAW(file, &(s[i].ver), 2, from_file);
    ioRAW(file, &(s[i].lines), s[i].nlines, from_file);
  }
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, LOG_LINE *s, int len, bool from_file) {
  char padding[5];
  memset(padding, ' ', sizeof(padding));
  int i, nbytes_rounded;
  for (i = 0; i < len; i++) {
    ioRAW(file, &(s[i].len), 1, from_file);
    nbytes_rounded = 4 * (1 + (s[i].len - 1) / 4);
    ioRAW(file, &(s[i].data), s[i].len, from_file);
    ioRAW(file, padding, nbytes_rounded - s[i].len, from_file);
  }
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, char *s, int len, bool from_file) {
  size_t n;
  if ((len <= 0) || (s == 0)) {
    return 0;
  }
  if (from_file) {
    n = fread(s, sizeof(char), len, file);
    return static_cast<int>(n - len);
  } else {
    n = fwrite(s, sizeof(char), len, file);
  }
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, int *s, int len, bool from_file) {
  if ((len <= 0) || (s == 0)) {
    return 0;
  }
  if (from_file) {
    fread(s, sizeof(int), len, file);
  } else {
    fwrite(s, sizeof(int), len, file);
  }
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, uint32_t *s, int len, bool from_file) {
  if ((len <= 0) || (s == 0)) {
    return 0;
  }
  if (from_file) {
    fread(s, sizeof(uint32_t), len, file);
  } else {
    fwrite(s, sizeof(uint32_t), len, file);
  }
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, float *s, int len, bool from_file) {
  int errcode = 0;
  if ((len <= 0) || (s == 0)) {
    return 0;
  }
  if (from_file) {
    fread(s, sizeof(float), len, file);
    vaxf_to_local(s, &len, &errcode);
  } else {
    local_to_vaxf(s, &len, &errcode);
    fwrite(s, sizeof(float), len, file);
    vaxf_to_local(s, &len, &errcode);
  }
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, char **s, int len, bool from_file) {
  if (from_file) {
    if (len > 0) {
      *s = new char[len];
      ioRAW(file, *s, len, from_file);
    } else {
      *s = 0;
    }
  } else {
    if (*s != 0) {
      ioRAW(file, *s, len, from_file);
    }
  }
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, int **s, int len, bool from_file) {
  if (from_file) {
    if (len > 0) {
      *s = new int[len];
      ioRAW(file, *s, len, from_file);
    } else {
      *s = 0;
    }
  } else {
    if (*s != 0) {
      ioRAW(file, *s, len, from_file);
    }
  }
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, uint32_t **s, int len, bool from_file) {
  if (from_file) {
    if (len > 0) {
      *s = new uint32_t[len];
      ioRAW(file, *s, len, from_file);
    } else {
      *s = 0;
    }
  } else {
    if (*s != 0) {
      ioRAW(file, *s, len, from_file);
    }
  }
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, float **s, int len, bool from_file) {
  if (from_file) {
    if (len > 0) {
      *s = new float[len];
      ioRAW(file, *s, len, from_file);
    } else {
      *s = 0;
    }
  } else {
    if (*s != 0) {
      ioRAW(file, *s, len, from_file);
    }
  }
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, SE_STRUCT **s, int len, bool from_file) {
  if (from_file) {
    if (len > 0) {
      *s = new SE_STRUCT[len];
      ioRAW(file, *s, len, from_file);
    } else {
      *s = 0;
    }
  } else {
    if (*s != 0) {
      ioRAW(file, *s, len, from_file);
    }
  }
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, DDES_STRUCT **s, int len, bool from_file) {
  if (from_file) {
    if (len > 0) {
      *s = new DDES_STRUCT[len];
      ioRAW(file, *s, len, from_file);
    } else {
      *s = 0;
    }
  } else {
    if (*s != 0) {
      ioRAW(file, *s, len, from_file);
    }
  }
  return 0;
}

/// stuff
int ISISRAW::ioRAW(FILE *file, LOG_LINE **s, int len, bool from_file) {
  if (from_file) {
    if (len > 0) {
      *s = new LOG_LINE[len];
      ioRAW(file, *s, len, from_file);
    } else {
      *s = 0;
    }
  } else {
    if (*s != 0) {
      ioRAW(file, *s, len, from_file);
    }
  }
  return 0;
}

/// stuff
int ISISRAW::size_check() {
  static int size_check_array[] = {
      sizeof(HDR_STRUCT),  80,     sizeof(ADD_STRUCT),  9 * 4,
      sizeof(USER_STRUCT), 8 * 20, sizeof(RPB_STRUCT),  32 * 4,
      sizeof(IVPB_STRUCT), 64 * 4, sizeof(SPB_STRUCT),  64 * 4,
      sizeof(SE_STRUCT),   32 * 4, sizeof(DAEP_STRUCT), 64 * 4,
      sizeof(DHDR_STRUCT), 32 * 4, sizeof(DDES_STRUCT), 2 * 4};
  for (unsigned i = 0; i < sizeof(size_check_array) / sizeof(int); i += 2) {
    if (size_check_array[i] != size_check_array[i + 1]) {
      std::cerr << "size check failed" << std::endl;
    }
  }
  return 0;
}
/// stuff
int ISISRAW::vmstime(char *timbuf, int len, time_t time_value) {
  /*
   * get time in VMS format 01-JAN-1970 00:00:00
   */
  size_t i, n;
  struct tm *tmstruct = NULL;
#ifdef MS_VISUAL_STUDIO
  errno_t err = localtime_s(tmstruct, &time_value);
  if (err) {
    return FAILURE;
  }
#else  //_WIN32
  tmstruct = localtime((time_t *)&time_value);
#endif //_WIN32
  n = strftime(timbuf, len, "%d-%b-%Y %H:%M:%S", tmstruct);
  for (i = 0; i < n; i++) {
    timbuf[i] = toupper(timbuf[i]);
  }
  return SUCCESS;
}

/// stuff
int ISISRAW::readFromFile(const char *filename, bool read_data) {
#ifdef MS_VISUAL_STUDIO
  FILE *input_file = NULL;
  if (fopen_s(&input_file, filename, "rb") != 0) {
    return -1;
  }
#else  //_WIN32
  FILE *input_file = fopen(filename, "rb");
#endif //_WIN32
  if (input_file != NULL) {
    ioRAW(input_file, true, read_data);
    fclose(input_file);
    return 0;
  } else {
    return -1;
  }
}

/// stuff
int ISISRAW::writeToFile(const char *filename) {
  unsigned char zero_pad[512];
  memset(zero_pad, 0, sizeof(zero_pad));
  remove(filename);
#ifdef MS_VISUAL_STUDIO
  FILE *output_file = NULL;
  if (fopen_s(&output_file, filename, "w+bc") != 0) {
    return -1;
  }
#else  //_WIN32
  FILE *output_file = fopen(filename, "w+bc");
#endif //_WIN32
  if (output_file != NULL) {
    ioRAW(output_file, false, 0);
    fflush(output_file);
    // we need to pad to a multiple of 512 bytes for VMS compatibility
    fseek(output_file, 0, SEEK_END);
    long pos = ftell(output_file);
    if (pos % 512 > 0) {
      int npad = 512 - pos % 512;
      fwrite(zero_pad, 1, npad, output_file);
    }
    fclose(output_file);
    return 0;
  } else {
    return -1;
  }
}

/// stuff
int ISISRAW::printInfo(std::ostream &os) {
  int i;
  os << "INST section at " << add.ad_inst << " 0x" << std::hex
     << 4 * add.ad_inst << std::dec << std::endl;
  os << "SE section at " << add.ad_se << " 0x" << std::hex << 4 * add.ad_se
     << std::dec << std::endl;
  os << "Dae section at " << add.ad_dae << " 0x" << std::hex << 4 * add.ad_dae
     << std::dec << std::endl;
  os << "Tcb section at " << add.ad_tcb << " 0x" << std::hex << 4 * add.ad_tcb
     << std::dec << std::endl;
  os << "User section at " << add.ad_user << " 0x" << std::hex
     << 4 * add.ad_user << std::dec << std::endl;
  os << "Data section at " << add.ad_data << " 0x" << std::hex
     << 4 * add.ad_data << std::dec << std::endl;
  os << "Log section at " << add.ad_log << " 0x" << std::hex << 4 * add.ad_log
     << std::dec << std::endl;
  os << "End section at " << add.ad_end << " 0x" << std::hex << 4 * add.ad_end
     << std::dec << std::endl;
  os << "User data len " << u_len << std::endl;
  os << "Compression is " << (dhdr.d_comp == 0 ? "NONE" : "BYTE-RELATIVE")
     << std::endl;
  os << "Compression ratio of data = " << dhdr.d_crdata << std::endl;
  os << "Offsets of spectrum data" << std::endl;
  for (i = 0; i < ((t_nsp1 + 1) * t_nper); i++) {
    os << i << " " << ddes[i].nwords << " words at offset " << ddes[i].offset
       << std::endl;
  }
  return 0;
}

/// stuff
ISISRAW::~ISISRAW() {
  delete[] dat1;
  delete[] ut;
  delete[] mdet;
  delete[] monp;
  delete[] spec;
  delete[] delt;
  delete[] len2;
  delete[] code;
  delete[] tthe;
  delete[] e_seblock;
  delete[] crat;
  delete[] modn;
  delete[] mpos;
  delete[] timr;
  delete[] udet;
  delete[] t_tcb1;
  delete[] u_dat;
  delete[] ddes;
  for (int i = 0; i < logsect.nlines; i++) {
    delete[] logsect.lines[i].data;
  }
  delete[] logsect.lines;
}

/// rtcb1 is of size t_ntc1+1
int ISISRAW::getTimeChannels(float *rtcb1, int n) {
  if (n != t_ntc1 + 1) {
    return -1;
  }
  float extra;
  if (frmt_ver_no > 1) {
    extra = float(4.0) * daep.a_delay; // add on frame sync delay
  } else {
    extra = 0.0; // old files did not have this
  }
  int i;
  for (i = 0; i < t_ntc1 + 1; i++) {
    rtcb1[i] = t_tcb1[i] * t_pre1 / float(32.0) + extra;
  }
  return 0;
}
