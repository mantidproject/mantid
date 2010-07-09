#ifndef ISISRAW2_H
#define ISISRAW2_H


#include "isisraw.h"

/// isis raw file.
//  isis raw
class ISISRAW2 : public ISISRAW
{
public:
	ISISRAW2();
	virtual ~ISISRAW2();
	
	virtual int ioRAW(FILE* file, bool from_file, bool do_data = true);

	void skipData(FILE* file, int i);
	bool readData(FILE* file, int i);
  void clear();  

	int ndes; ///<ndes
private:
	char* outbuff;  ///<output buffer
};



#endif /* ISISRAW2_H */
