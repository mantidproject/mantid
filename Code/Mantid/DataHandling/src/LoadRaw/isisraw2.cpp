#include <exception>
#include <iostream>
#include <cstdio>
#include "isisraw2.h"
#include "vms_convert.h"
#include "byte_rel_comp.h"

#define SUCCESS 0
#define FAILURE 1

/// stuff
int ISISRAW2::addItems()
{
	static const int hdr_size = sizeof(hdr) / sizeof(char);
	static const int rrpb_size = sizeof(rpb) / sizeof(float);
	static const int irpb_size = sizeof(rpb) / sizeof(int);
	m_char_items.addItem("HDR", (const char*)&hdr, false, &hdr_size);
	m_real_items.addItem("RRPB", (float*)&rpb, false, &rrpb_size);
	m_int_items.addItem("IRPB", (int*)&rpb, false, &irpb_size);
	return 0;
}

// create one bound to a CRPT
/// stuff
ISISRAW2::ISISRAW2() : m_crpt(NULL),m_ntc1(0),m_nsp1(0), m_nper(0),
						mdet(0),monp(0),spec(0),delt(0),len2(0),code(0),
						tthe(0),ut(0),crat(0),modn(0),mpos(0),timr(0),udet(0),
						t_tcb1(0),u_dat(0),ddes(0),dat1(0),m_file(0),outbuff(0)
{
	e_nse = 0;
	e_seblock = 0;
	u_len = 0;
	logsect.nlines = 0;
	logsect.lines = 0;
	addItems();
	//updateFromCRPT();
}


/// stuff
int ISISRAW2::ioRAW(FILE* file, bool from_file, bool read_data)
{
  m_file = file;
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
  ioRAW(file, &t_tcb1, t_ntc1+1, from_file);
  ioRAW(file, &ver7, 1, from_file);
// it appear that the VMS ICP traditionally sets u_len to 1 regardless
// of its real size; thus we cannot rely on it for reading and must instead
// use section offsets
  i = 0;
  ioRAW(file, &i, 1, from_file);
//		ioRAW(file, &u_len, 1, from_file);
  if (from_file)
  {
    u_len = add.ad_data - add.ad_user - 2; 
  }
  ioRAW(file, &u_dat, u_len, from_file);
  ioRAW(file, &ver8, 1, from_file);
  fgetpos(file, &dhdr_pos);
  ioRAW(file, &dhdr, 1, from_file);

  int outbuff_size = 100000;
  outbuff = new char[outbuff_size];
  ndes = t_nper * (t_nsp1+1);
  ioRAW(file, &ddes, ndes, true);
  dat1 = new uint32_t[ t_ntc1+1 ];  //  space for just one spectrum
  memset(outbuff, 0, outbuff_size); // so when we round up words we get a zero written

  return 0; // stop reading here
}

void ISISRAW2::skipData(int i)
{
    if (i < ndes)
        fseek(m_file,4*ddes[i].nwords,SEEK_CUR);
}

void ISISRAW2::readData(int i)
{
    if (i >= ndes) return;
    int nwords = 4*ddes[i].nwords;
    ioRAW(m_file, outbuff, nwords, true);
    byte_rel_expn(outbuff, nwords, 0, (int*)dat1, t_ntc1+1);
}



/// stuff
	int ISISRAW2::ioRAW(FILE* file, HDR_STRUCT* s, int len, bool from_file) 
		{
			ioRAW(file, (char*)s, sizeof(HDR_STRUCT) * len, from_file);
			return 0;
		}

/// stuff
	int ISISRAW2::ioRAW(FILE* file, ADD_STRUCT* s, int len, bool from_file)
		{
			ioRAW(file, (int*)s, (sizeof(ADD_STRUCT) * len / sizeof(int)), from_file);
			return 0;
		}

/// stuff
		int ISISRAW2::ioRAW(FILE* file, USER_STRUCT* s, int len, bool from_file) 
		{
			ioRAW(file, (char*)s, sizeof(USER_STRUCT) * len, from_file);
			return 0;
		}

/// stuff
		int ISISRAW2::ioRAW(FILE* file, RPB_STRUCT* s, int len, bool from_file) 
		{
			int i;
			for(i=0; i<len; i++)
			{
				ioRAW(file, &(s[i].r_dur), 7, from_file);
				ioRAW(file, &(s[i].r_gd_prtn_chrg), 2, from_file);
				ioRAW(file, &(s[i].r_goodfrm), 7, from_file);
				ioRAW(file, s[i].r_enddate, 20, from_file);
				ioRAW(file, &(s[i].r_prop), 11, from_file);
			}
			return 0;
		}


/// stuff
		int ISISRAW2::ioRAW(FILE* file, IVPB_STRUCT* s, int len, bool from_file) 
		{
			int i;
			for(i=0; i<len; i++)
			{
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
		int ISISRAW2::ioRAW(FILE* file, SPB_STRUCT* s, int len, bool from_file) 
		{
			int i;
			for(i=0; i<len; i++)
			{
				ioRAW(file, &(s[i].e_posn), 3, from_file);
				ioRAW(file, &(s[i].e_thick), 16, from_file);
				ioRAW(file, s[i].e_name, 40, from_file);
				ioRAW(file, &(s[i].e_equip), 35, from_file);
			}
			return 0;
		}

/// stuff
		int ISISRAW2::ioRAW(FILE* file, SE_STRUCT* s, int len, bool from_file) 
		{
			int i;
			for(i=0; i<len; i++)
			{
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
		int ISISRAW2::ioRAW(FILE* file, DAEP_STRUCT* s, int len, bool from_file) 
		{
			ioRAW(file, (int*)s, sizeof(DAEP_STRUCT) * len / sizeof(int), from_file);
			return 0;
		}

/// stuff
		int ISISRAW2::ioRAW(FILE* file, DHDR_STRUCT* s, int len, bool from_file) 
		{
			int i;
			for(i=0; i<len; i++)
			{
				ioRAW(file, &(s[i].d_comp), 3, from_file);
				ioRAW(file, &(s[i].d_crdata), 2, from_file);
				ioRAW(file, &(s[i].d_exp_filesize), 27, from_file);
			}
			return 0;
		}

/// stuff
		int ISISRAW2::ioRAW(FILE* file, DDES_STRUCT* s, int len, bool from_file) 
		{
			int i;
			for(i=0; i<len; i++)
			{
				ioRAW(file, &(s[i].nwords), 2, from_file);
			}
			return 0;
		}

/// stuff
		int ISISRAW2::ioRAW(FILE* file, LOG_STRUCT* s, int len, bool from_file) 
		{
			int i;
			for(i=0; i<len; i++)
			{
				ioRAW(file, &(s[i].ver), 2, from_file);
				ioRAW(file, &(s[i].lines), s[i].nlines, from_file);
			}
			return 0;
		}

/// stuff
		int ISISRAW2::ioRAW(FILE* file, LOG_LINE* s, int len, bool from_file) 
		{
			char padding[5];
			memset(padding, ' ', sizeof(padding)); 
			int i, nbytes_rounded; 
			for(i=0; i<len; i++)
			{
				ioRAW(file, &(s[i].len), 1, from_file);
				nbytes_rounded = 4 * (1 + (s[i].len-1)/4);
				ioRAW(file, &(s[i].data), s[i].len, from_file);
				ioRAW(file, padding, nbytes_rounded - s[i].len, from_file);
			}
			return 0;
		}

/// stuff
		int ISISRAW2::ioRAW(FILE* file, char* s, int len, bool from_file) 
		{
			size_t n;
			if ( (len <= 0) || (s == 0) )
			{
				return 0;
			}
			if (from_file)
			{
				n = fread(s, sizeof(char), len, file);
			}
			else
			{
				n = fwrite(s, sizeof(char), len, file);
			}
			return 0;
		}

/// stuff
		int ISISRAW2::ioRAW(FILE* file, int* s, int len, bool from_file) 
		{
			size_t n;
			if ( (len <= 0) || (s == 0) )
			{
				return 0;
			}
			if (from_file)
			{
				n = fread(s, sizeof(int), len, file);
			}
			else
			{
				n = fwrite(s, sizeof(int), len, file);
			}
			return 0;
		}

/// stuff
		int ISISRAW2::ioRAW(FILE* file, uint32_t* s, int len, bool from_file) 
		{
			size_t n;
			if ( (len <= 0) || (s == 0) )
			{
				return 0;
			}
			if (from_file)
			{
				n = fread(s, sizeof(uint32_t), len, file);
			}
			else
			{
				n = fwrite(s, sizeof(uint32_t), len, file);
			}
			return 0;
		}

/// stuff
		int ISISRAW2::ioRAW(FILE* file, float* s, int len, bool from_file) 
		{
			size_t n;
			int errcode = 0;
			if ( (len <= 0) || (s == 0) )
			{
				return 0;
			}
			if (from_file)
			{
				n = fread(s, sizeof(float), len, file);
				vaxf_to_local(s, &len, &errcode);
			}
			else
			{
				local_to_vaxf(s, &len, &errcode);
				n = fwrite(s, sizeof(float), len, file);
				vaxf_to_local(s, &len, &errcode);
			}
			return 0;
		}

/// stuff
		int ISISRAW2::ioRAW(FILE* file, char** s, int len, bool from_file) 
		{
			if (from_file)
			{
				if (len > 0)
				{
					*s = new char[len];
					ioRAW(file, *s, len, from_file);
				}
				else
				{
					*s = 0;
				}
			}
			else
			{
				if (*s != 0)
				{
					ioRAW(file, *s, len, from_file);
				}
			}
			return 0;
		}

/// stuff
		int ISISRAW2::ioRAW(FILE* file, int** s, int len, bool from_file) 
		{
			if (from_file)
			{
				if (len > 0)
				{
					*s = new int[len];
					ioRAW(file, *s, len, from_file);
				}
				else
				{
					*s = 0;
				}
			}
			else
			{
				if (*s != 0)
				{
					ioRAW(file, *s, len, from_file);
				}	
			}
			return 0;
		}

/// stuff
		int ISISRAW2::ioRAW(FILE* file, uint32_t** s, int len, bool from_file) 
		{
			if (from_file)
			{
				if (len > 0)
				{
					*s = new uint32_t[len];
					ioRAW(file, *s, len, from_file);
				}
				else
				{
					*s = 0;
				}
			}
			else
			{
				if (*s != 0)
				{
					ioRAW(file, *s, len, from_file);
				}	
			}
			return 0;
		}


/// stuff
		int ISISRAW2::ioRAW(FILE* file, float** s, int len, bool from_file) 
		{
			if (from_file)
			{
				if (len > 0)
				{
					*s = new float[len];
					ioRAW(file, *s, len, from_file);
				}
				else
				{
					*s = 0;
				}
			}
			else
			{
				if (*s != 0)
				{
					ioRAW(file, *s, len, from_file);
				}
			}
			return 0;
		}
		
/// stuff
		int ISISRAW2::ioRAW(FILE* file, SE_STRUCT** s, int len, bool from_file) 
		{
			if (from_file)
			{
				if (len > 0)
				{
					*s = new SE_STRUCT[len];
					ioRAW(file, *s, len, from_file);
				}
				else
				{
					*s = 0;
				}
			}
			else
			{
				if (*s != 0)
				{
					ioRAW(file, *s, len, from_file);
				}
			}
			return 0;
		}

/// stuff
		int ISISRAW2::ioRAW(FILE* file, DDES_STRUCT** s, int len, bool from_file) 
		{
			if (from_file)
			{
				if (len > 0)
				{
					*s = new DDES_STRUCT[len];
					ioRAW(file, *s, len, from_file);
				}
				else
				{
					*s = 0;
				}
			}
			else
			{
				if (*s != 0)
				{
					ioRAW(file, *s, len, from_file);
				}
			}
			return 0;
		}

/// stuff
		int ISISRAW2::ioRAW(FILE* file, LOG_LINE** s, int len, bool from_file)
		{
			if (from_file)
			{
				if (len > 0)
				{
					*s = new LOG_LINE[len];
					ioRAW(file, *s, len, from_file);
				}
				else
				{
					*s = 0;
				}
			}
			else
			{
				if (*s != 0)
				{
					ioRAW(file, *s, len, from_file);
				}
			}
			return 0;
		}

/// stuff
	int ISISRAW2::size_check()
	{
		static int size_check_array[] = { 
			sizeof(HDR_STRUCT), 80, 
			sizeof(ADD_STRUCT), 9*4, 
			sizeof(USER_STRUCT), 8*20, 
			sizeof(RPB_STRUCT), 32*4,
			sizeof(IVPB_STRUCT), 64*4, 
			sizeof(SPB_STRUCT), 64*4, 
			sizeof(SE_STRUCT), 32*4,
			sizeof(DAEP_STRUCT), 64*4, 
			sizeof(DHDR_STRUCT), 32*4, 
			sizeof(DDES_STRUCT), 2*4
		};
		for(unsigned i=0; i<sizeof(size_check_array)/sizeof(int); i += 2)
		{
			if (size_check_array[i] != size_check_array[i+1])
			{
				std::cerr << "size check failed" << std::endl;
			}
		}
		return 0;
	}
/// stuff
int ISISRAW2::vmstime(char* timbuf, int len, time_t time_value)
{
/* 
 * get time in VMS format 01-JAN-1970 00:00:00
 */
	int i, n;
	struct tm *tmstruct = NULL;
#ifdef MS_VISUAL_STUDIO
  errno_t err = localtime_s( tmstruct, &time_value ); 
  if (err)
  {
      return FAILURE;
  }
#else //_WIN32
	tmstruct = localtime((time_t*)&time_value);
#endif //_WIN32
	n = strftime(timbuf, len, "%d-%b-%Y %H:%M:%S", tmstruct);
	for(i=0; i<n; i++)
	{
		timbuf[i] = toupper(timbuf[i]);
	}
	return SUCCESS;
}


/// stuff
int ISISRAW2::readFromFile(const char* filename, bool read_data)
{
#ifdef MS_VISUAL_STUDIO
  FILE* input_file=NULL;
  if(fopen_s( &input_file, filename, "rb" ) !=0 )
  {
      return -1;
  }
#else //_WIN32
	FILE* input_file = fopen(filename,"rb");
#endif //_WIN32
	if (input_file != NULL)
	{
		ioRAW(input_file, true, read_data);
		fclose(input_file);
		return 0;
	}
	else
	{
		return -1;
	}
}


/// stuff
int ISISRAW2::writeToFile(const char* filename)
{
	unsigned char zero_pad[512];
	int npad;
	long pos;
	memset(zero_pad, 0, sizeof(zero_pad));
	remove(filename);
#ifdef MS_VISUAL_STUDIO
  FILE* output_file=NULL;
  if(fopen_s( &output_file, filename, "w+bc" ) !=0 )
  {
      return -1;
  }
#else //_WIN32
	FILE* output_file = fopen(filename,"w+bc");
#endif //_WIN32
	if (output_file != NULL)
	{
		ioRAW(output_file, false,0);
		fflush(output_file);
		// we need to pad to a multiple of 512 bytes for VMS compatibility
		fseek(output_file, 0, SEEK_END);
		pos = ftell(output_file);
		if (pos % 512 > 0)
		{
			npad = 512 - pos % 512;
			fwrite(zero_pad, 1, npad, output_file);
		}
		fclose(output_file);
		return 0;
	}
	else
	{
		return -1;
	}
}

/// stuff
int ISISRAW2::printInfo(std::ostream& os)
{
	int i;
	long offset;
	os << "INST section at " << add.ad_inst << " 0x" << std::hex << 4*add.ad_inst << std::dec << std::endl;
	os << "SE section at " << add.ad_se << " 0x" << std::hex << 4*add.ad_se << std::dec << std::endl;
	os << "Dae section at " << add.ad_dae << " 0x" << std::hex << 4*add.ad_dae << std::dec << std::endl;
	os << "Tcb section at " << add.ad_tcb << " 0x" << std::hex << 4*add.ad_tcb << std::dec << std::endl;
	os << "User section at " << add.ad_user << " 0x" << std::hex << 4*add.ad_user << std::dec << std::endl;
	os << "Data section at " << add.ad_data << " 0x" << std::hex << 4*add.ad_data << std::dec << std::endl;
	os << "Log section at " << add.ad_log << " 0x" << std::hex << 4*add.ad_log << std::dec << std::endl;
	os << "End section at " << add.ad_end << " 0x" << std::hex << 4*add.ad_end << std::dec << std::endl;
	os << "User data len " << u_len << std::endl;
	os << "Compression is " << (dhdr.d_comp == 0 ? "NONE" : "BYTE-RELATIVE") << std::endl;
	os << "Compression ratio of data = " << dhdr.d_crdata << std::endl;
	os << "Offsets of spectrum data" << std::endl;
	offset = add.ad_data;
	for(i=0; i < ((t_nsp1+1) * t_nper); i++)
	{
		os << i << " " << ddes[i].nwords << " words at offset " << ddes[i].offset << std::endl;
	}
	return 0;
}

/// stuff
ISISRAW2::~ISISRAW2()
{
    //fclose(m_file);
    delete[] outbuff;
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
    for(int i=0; i<logsect.nlines; i++)
    {
	delete[] logsect.lines[i].data;
    }
    delete[] logsect.lines;
}

// rtcb1 is of size t_ntc1+1
int ISISRAW2::getTimeChannels(float* rtcb1, int n)
{
    if (n != t_ntc1+1)
    {
	return -1;
    }
    float extra;
    if (frmt_ver_no > 1)
    {
	extra = 4 * daep.a_delay; // add on frame sync delay
    }
    else
    {
	extra = 0.0;		  // old files did not have this
    }
    int i;
    for(i=0; i<t_ntc1+1; i++)
    {
	rtcb1[i] = t_tcb1[i] * t_pre1 / 32.0 + extra;
    }
    return 0;
}
