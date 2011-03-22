#include <exception>
#include <iostream>
#include <cstdio>
#include "isisraw2.h"
#include "byte_rel_comp.h"



/// No arg Constructor
ISISRAW2::ISISRAW2() : ISISRAW(NULL,false),outbuff(0)
{
}

/** Loads the headers of the file, leaves the file pointer at a specific position
*   @param file :: The file handle to use
*   @param from_file :: Wether to read from or write to a file
*   @param read_data :: Wether to go on to read the data
*   @return file readin exit code, 0 is OK
**/
int ISISRAW2::ioRAW(FILE* file, bool from_file, bool read_data)
{
  (void) read_data; // Avoid compiler warning

  int i;
  fpos_t add_pos, dhdr_pos; 
  if (!from_file)
  {
    add.ad_run = 32;
    add.ad_inst = add.ad_run + 94;
    add.ad_se = add.ad_inst + 70 + 2*i_mon + (5+i_use)*i_det;
    add.ad_dae = add.ad_se + 66 + e_nse*32;
    add.ad_tcb = add.ad_dae + 65 + 5*i_det;
    add.ad_user = add.ad_tcb + 288 + (t_ntc1 + 1);
    add.ad_data = add.ad_user + 2 + u_len; 
    add.ad_log = 0; // we don't know it yet 
    add.ad_end = 0;
  }
  ISISRAW::ioRAW(file, &hdr, 1, from_file);
  ISISRAW::ioRAW(file, &frmt_ver_no, 1, from_file);
  fgetpos(file, &add_pos);
  ISISRAW::ioRAW(file, &add, 1, from_file);
  ISISRAW::ioRAW(file, &data_format, 3, from_file);
  ISISRAW::ioRAW(file, r_title, 80, from_file);
  ISISRAW::ioRAW(file, &user, 1, from_file);
  ISISRAW::ioRAW(file, &rpb, 1, from_file);
  ISISRAW::ioRAW(file, &ver3, 1, from_file);
  ISISRAW::ioRAW(file, i_inst, 8, from_file);
  ISISRAW::ioRAW(file, &ivpb, 1, from_file);
  ISISRAW::ioRAW(file, &i_det, 3, from_file);
  ISISRAW::ioRAW(file, &mdet, i_mon, from_file);
  ISISRAW::ioRAW(file, &monp, i_mon, from_file);
  ISISRAW::ioRAW(file, &spec, i_det, from_file);
  ISISRAW::ioRAW(file, &delt, i_det, from_file);
  ISISRAW::ioRAW(file, &len2, i_det, from_file);
  ISISRAW::ioRAW(file, &code, i_det, from_file);
  ISISRAW::ioRAW(file, &tthe, i_det, from_file);
  ISISRAW::ioRAW(file, &ut, i_use * i_det, from_file);
  ISISRAW::ioRAW(file, &ver4, 1, from_file);
  ISISRAW::ioRAW(file, &spb, 1, from_file);
  ISISRAW::ioRAW(file, &e_nse, 1, from_file);
  ISISRAW::ioRAW(file, &e_seblock, e_nse, from_file);
  ISISRAW::ioRAW(file, &ver5, 1, from_file);
  ISISRAW::ioRAW(file, &daep, 1, from_file);
  ISISRAW::ioRAW(file, &crat, i_det, from_file);
  ISISRAW::ioRAW(file, &modn, i_det, from_file);
  ISISRAW::ioRAW(file, &mpos, i_det, from_file);
  ISISRAW::ioRAW(file, &timr, i_det, from_file);
  ISISRAW::ioRAW(file, &udet, i_det, from_file);
  ISISRAW::ioRAW(file, &ver6, 267, from_file);
  ISISRAW::ioRAW(file, &(t_tcp1[0][0]), 20, from_file);
  ISISRAW::ioRAW(file, &t_pre1, 1, from_file);
  ISISRAW::ioRAW(file, &t_tcb1, t_ntc1+1, from_file);
  ISISRAW::ioRAW(file, &ver7, 1, from_file);
// it appear that the VMS ICP traditionally sets u_len to 1 regardless
// of its real size; thus we cannot rely on it for reading and must instead
// use section offsets
  i = 0;
  ISISRAW::ioRAW(file, &i, 1, from_file);
//		ISISRAW::ioRAW(file, &u_len, 1, from_file);
  if (from_file)
  {
    u_len = add.ad_data - add.ad_user - 2; 
  }
  ISISRAW::ioRAW(file, &u_dat, u_len, from_file);
  ISISRAW::ioRAW(file, &ver8, 1, from_file);
  fgetpos(file, &dhdr_pos);
  ISISRAW::ioRAW(file, &dhdr, 1, from_file);

  int outbuff_size = 100000;
  outbuff = new char[outbuff_size];
  ndes = t_nper * (t_nsp1+1);
  ISISRAW::ioRAW(file, &ddes, ndes, true);
  dat1 = new uint32_t[ t_ntc1+1 ];  //  space for just one spectrum
  memset(outbuff, 0, outbuff_size); // so when we round up words we get a zero written

  return 0; // stop reading here
}

/// Skip data
/// @param file :: The file pointer
/// @param i :: The amount of data to skip
void ISISRAW2::skipData(FILE* file, int i)
{
    if (i < ndes)
        fseek(file,4*ddes[i].nwords,SEEK_CUR);
}

/// Read data
/// @param file :: The file pointer
/// @param i :: The amount of data to read
/// @return true on success
bool ISISRAW2::readData(FILE* file, int i)
{
    if (i >= ndes) return false;
    int nwords = 4*ddes[i].nwords;
    int res = ISISRAW::ioRAW(file, outbuff, nwords, true);
    if (res != 0) return false;
    byte_rel_expn(outbuff, nwords, 0, (int*)dat1, t_ntc1+1);
    return true;
}

ISISRAW2::~ISISRAW2()
{
    //fclose(m_file);
    if (outbuff) delete[] outbuff;
}

///Clears the output buffer
void ISISRAW2::clear()
{
  if (outbuff) delete[] outbuff;
}
