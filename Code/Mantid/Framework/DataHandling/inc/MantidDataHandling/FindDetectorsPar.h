#ifndef DATAHANDLING_FIND_DETPAR_H_
#define DATAHANDLING_FIND_DETPAR_H_
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include <fstream>
/** An algorithm to calculate workspace detectors angular coordinates, as they can be viewed from a sample (par or phx data)

    Required Properties:
    <UL>
    <LI> Workspace - The name of the input Workspace2D on which to perform the algorithm. 
	     Detectors or detectors groups have to be loaded into workspace </LI>
    </UL>

    Output Properties:
	When algorithm runs as subalgorithm, these properties are not defined. To get access to the resulting arrays, 
	the algorithm user has to deploy accessors (getAzimuthal(), getPolar() etc.), defined below, which allows avoiding the
	transformation of these arrays into strings. A property, which control this behaviour can be easy introduced but not at the moment. 

	<UL><LI> azimuthal            - An array property containing the detectors azimutal angles</LI> </UL>
    <UL><LI> polar                - An array property containing the detectors polar angles</LI>    </UL>
    <UL><LI> azimuthal_width      - An array property containing the detectors azimuthal angular width</LI></UL>
    <UL><LI> polar_width          - An array property containing the detectors polar angular width</LI></UL>
    <UL><LI> secondary_flightpath - An array property containing the distance from detectors to the sample center</LI></UL>




    @author Alex Buts ISIS; initially extracted from Stuart Campbell's SaveNXSPE algorithm,
    @date 17/05/2012

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

namespace Mantid
{
namespace DataHandling
{
//
/*!  file types currently supported by ASCII loader are:
*1) an ASCII Tobyfit par file
*     Syntax:
*     >> par = get_ascii_file(filename,['par'])
*
*     filename            name of par file
*
*     par(5,ndet)         contents of array
*
*         1st column      sample-detector distance
*         2nd  "          scattering angle (deg)
*         3rd  "          azimuthal angle (deg)
*                     (west bank = 0 deg, north bank = -90 deg etc.)
*                     (Note the reversed sign convention cf .phx files)
*         4th  "          width (m)
*         5th  "          height (m)
*-----------------------------------------------------------------------
*2) load an ASCII phx file
*     Syntax:
*     >> phx = get_ascii_file(filename,['phx'])
*
*     filename            name of phx file
*
*     phx(7,ndet)         contents of array
*
*     Recall that only the 3,4,5,6 columns in the file (rows in the
*     output of this routine) contain useful information
*         3rd column      scattering angle (deg)
*         4th  "          azimuthal angle (deg)
*                     (west bank = 0 deg, north bank = 90 deg etc.)
*         5th  "          angular width (deg)
*         6th  "          angular height (deg)
*-----------------------------------------------------------------------
*/
enum fileTypes{
	PAR_type, //< ASCII PAR file
	PHX_type, //< ASCII phx file
	SPE_type, //< spe file, this loader would not work with spe file, left for compartibility with old algorithms. 
    BIN_file, //< binary file is not an ASCII file, so ascii loader would not work on it
	NumFileTypes
};
/*!
*   Description of the data header, common for all files
*/
struct FileTypeDescriptor{
	fileTypes Type;
	std::streampos data_start_position; //< the position in the file where the data structure starts
	size_t 	  nData_records,            //< number of data records -- actually nDetectors
		      nData_blocks;             //< nEnergy bins for SPE file, 5 or 6 for PAR file and 7 for PHX file
	char      line_end ;                //< the character which ends line in current ASCII file 0x0A (LF)
	    //Unix, 0x0D (CR) Mac and 0x0D 0x0A (CR LF) Win, but the last is interpreted as 0x0A here 
    FileTypeDescriptor():Type(BIN_file),data_start_position(0),nData_records(0),nData_blocks(0),line_end(0x0A){}
};
// Algorighm body itself
class DLLExport FindDetectorsPar : public API::Algorithm
{
public:
  FindDetectorsPar();
  virtual ~FindDetectorsPar();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "FindDetectorsPar";};
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;};
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling\\Detectors";}
  /// the accessors, used to return algorithm results when called as sub-algorithm, without setting the properties;
  std::vector<double>const & getAzimuthal()const{return azimuthal;}
  std::vector<double>const & getPolar()const{return polar;}
  std::vector<double>const & getAzimWidth()const{return azimuthal_width;}
  std::vector<double>const & getPolarWidth()const{return polar_width;}
  std::vector<double>const & getFlightPath()const{return secondary_flightpath;}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Implement abstract Algorithm methods
  void init();
  void exec();

  std::vector<double> azimuthal;
  std::vector<double> polar;
  std::vector<double> azimuthal_width;
  std::vector<double> polar_width;
  std::vector<double> secondary_flightpath;
  /// logger -> to provide logging, for MD workspaces
  static Kernel::Logger& g_log;

  /// auxiliary function, which transforms array into a string --> it seems, lexical cast already does this job?
  void fill_property(Kernel::Property *const,std::vector<double> const &);
  /// calculates par values for a detectors ring;
  void calc_cylDetPar(const Geometry::IDetector_sptr spDet,
                      const Geometry::IObjComponent_const_sptr sample,
                      double &azim, double &polar, double &azim_width, double &polar_width,double &dist);
  /// calculates par values for a detectors block or a detector;
  void calc_rectDetPar(const API::MatrixWorkspace_sptr inputWS,
                       const Geometry::IDetector_sptr spDet,
                       const Geometry::IObjComponent_const_sptr sample,
                       double &azim, double &polar, double &azim_width, double &polar_width,double &dist);
  size_t loadParFile(const std::string &fileName);

  /// functions used to populate data from the phx or par file
  void   populate_values_from_file();
  /// if ASCII file is selected as the datasource, this structure describes the type of this file. 
  FileTypeDescriptor current_ASCII_file;

protected: // for testing purposes
/**!  function calculates number of colums in an ASCII file, assuming that colums are separated by spaces */
int count_changes(const char *const Buf,size_t buf_size);
/**! The function reads line from input stream and puts it into buffer. 
*   It behaves like std::ifstream getline but the getline reads additional symbol from a row in a Unix-formatted file under windows;*/
size_t get_my_line(std::ifstream &in, char *buf, size_t buf_size, const char DELIM);
/// load file header and identify which file (PHX,PAR or SPE) it belongs to. It also identifies the position of the begining of the data
FileTypeDescriptor get_ASCII_header(std::string const &fileName, std::ifstream &data_stream);
/// load PAR or PHX file
void load_plain(std::ifstream &stream,std::vector<double> &Data,FileTypeDescriptor const &FILE_TYPE);
};


} //end namespace DataHandling
} //end namespace Mandid


#endif
