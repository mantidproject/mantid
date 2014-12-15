#ifndef DATAHANDLING_FIND_DETPAR_H_
#define DATAHANDLING_FIND_DETPAR_H_
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include <fstream>

namespace Mantid
{
  namespace DataHandling
  {
    /**  file types currently supported by ASCII loader are:
     *1) an ASCII Tobyfit par file
     *     Syntax:
     *     >> par = get_ascii_file(filename,['par'])
     *
     *     filename            name of par file
     *
     *     par(5,ndet)         contents of array
     *
     *         1st column      sample-detector distance
     *         2nd  &quot;          scattering angle (deg)
     *         3rd  &quot;          azimuthal angle (deg)
     *                     (west bank = 0 deg, north bank = -90 deg etc.)
     *                     (Note the reversed sign convention cf .phx files)
     *         4th  &quot;          width (m)
     *         5th  &quot;          height (m)
     *-----------------------------------------------------------------------
     *2) load an ASCII phx file
     *
     *
     *     phx(7,ndet)         contents of array
     *
     *     Recall that only the 3,4,5,6 columns in the file (rows in the
     *     output of this routine) contain useful information
     *         3rd column      scattering angle (deg)
     *         4th  &quot;          azimuthal angle (deg)
     *                     (west bank = 0 deg, north bank = 90 deg etc.)
     *         5th  &quot;          angular width (deg)
     *         6th  &quot;          angular height (deg)
     *-----------------------------------------------------------------------
     */
    enum fileTypes
    {
      PAR_type, //< ASCII PAR file
      PHX_type, //< ASCII phx file
      SPE_type, //< spe file, this loader would not work with spe file, left for compartibility with old algorithms.
      BIN_file, //< binary file is not an ASCII file, so ascii loader would not work on it
      NumFileTypes
    };

    /**
     *   Description of the ASCII data header, common for all ASCII PAR and PHX files
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

    /**
    An algorithm to calculate the angular coordinates of the workspace's detectors, as they can be viewed from a sample (par or phx data)

    Properties:
    <UL>
    <LI> Workspace - The name of the input Workspace2D on which to perform the algorithm.
         Detectors or detectors groups have to be loaded into this workspace </LI>
    <LI> OutputTable workspace name - if present, identify the name of the output table workspace with provided detectors parameters </LI>
    <LI> Par or phx file name - if present, used to define the detectors parameters from the file instead of the parameters
         calculated from the instrument description</LI>
    </UL>

    Output Properties:
    Optional: OutputTableWorkspace - the workspace which contains five columns with the following values:
    <UL>
    <LI> azimuthal             - A columnt  containing the detectors azimutal angles</LI>
    <LI> polar                 - A column  containing the detectors polar angles</LI>
    <LI> secondary_flightpath  - A column containing the distance from detectors to the sample center</LI>
    <LI> azimuthal_width       - A column  containing the detectors azimuthal angular width</LI>
    <LI> polar_width           - A column  containing the detectors polar angular width</LI>
    </UL>

    When OutputTable workspace name is empty, the tabled workspace is not defined. To get access to the resulting arrays,
    the algorithm user has to deploy accessors (getAzimuthal(), getPolar() etc.), defined below, which allows avoiding the
    transformation of these arrays into strings.

    @author Alex Buts ISIS; initially extracted from Stuart Campbell's SaveNXSPE algorithm,
    @date 17/05/2012

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
     */
    // predefine class, used to cashe precalculated detector's parameters
    class DetParameters;

    class DLLExport FindDetectorsPar : public API::Algorithm
    {
    public:
      FindDetectorsPar();
      virtual ~FindDetectorsPar();

      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "FindDetectorsPar";};
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "The algorithm returns the angular parameters and second flight path for a workspace detectors (data, usually availble in par or phx file)";}

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;};
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Instrument";}
      /// the accessors, used to return algorithm results when called as Child Algorithm, without setting the properties;
      std::vector<double>const & getAzimuthal()const{return azimuthal;}
      std::vector<double>const & getPolar()const{return polar;}
      std::vector<double>const & getAzimWidth()const{return azimuthalWidth;}
      std::vector<double>const & getPolarWidth()const{return polarWidth;}
      std::vector<double>const & getFlightPath()const{return secondaryFlightpath;}
      std::vector<size_t>const& getDetID()const{return detID;}
      /// number of real detectors, calculated by algorithm
      size_t getNDetectors()const{return m_nDetectors;}
    private:
      
      // Implement abstract Algorithm methods
      void init();
      void exec();
      /**  the variable defines if algorithm needs to calculate linear ranges for the detectors (dX,dY)
       *    instead of azimuthal_width and polar_width */
      bool m_SizesAreLinear;
      
      // numner of real(valid and non-monitor) detectors calculated by the alvorithm
      size_t m_nDetectors;
      // the vectors which represent detector's parameters as linear structure
      std::vector<double> azimuthal;
      std::vector<double> polar;
      std::vector<double> azimuthalWidth;
      std::vector<double> polarWidth;
      std::vector<double> secondaryFlightpath;
      std::vector<size_t> detID;

      // calculate generic detectors parameters:
      void calcDetPar(const Geometry::IDetector_const_sptr &spDet,const Kernel::V3D &GroupCenter,DetParameters &Detector);

      /// if ASCII file is selected as the datasource, this structure describes the type of this file.
      FileTypeDescriptor current_ASCII_file;
      /// internal function which sets the output table according to the algorithms properties
      void setOutputTable();
      /// extract valid detectors parameters into vectors above 
      void extractAndLinearize(const std::vector<DetParameters> &detPar);
      /// functions used to populate data from the phx or par file
      void   populate_values_from_file(const API::MatrixWorkspace_sptr & inputWS);
      /// load data from par or phx file;
      size_t loadParFile(const std::string &fileName);
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

/**Small helper class-holder used to precalculate the detectors parameters in spherical coordinate system */
class DetParameters
{
public:
  /// azimuthal detector's angle in spherical coordinate system alighned with the beam
  double azimutAngle;
  /// polar detector's angle in spherical coordinate system alighned with the beam
  double polarAngle;
  /// scattering source -- detector' distance
  double secondaryFlightPath;
  /// linear or angular size of the bounding box encapsulating detector and alighned tangentially to the constant scattering angle circle 
  double azimWidth,polarWidth;
  /// the detector's ID
  int64_t detID;
  // default detector ID -- -1 means undefined
  DetParameters():detID(-1){}

};

/** helper class-collection to keep together the parameters, which characterize average composite detector 
    and help to calculate these parameters*/
class AvrgDetector
{
  double m_AzimutSum;
  double m_PolarSum;
  double m_FlightPathSum;
  double m_AzimBase,m_PolarBase;
  // if azimuthal and polar sizes expressed in angular or linear units
  bool m_useSphericalSizes;
  double m_AzimMin,m_PolarMin,m_AzimMax,m_PolarMax;
  /// numbr of primary detectors, contributing into this detector
  size_t m_nComponents;
public:
  AvrgDetector():m_AzimutSum(0),m_PolarSum(0),m_FlightPathSum(0),
                 m_AzimBase(0),m_PolarBase(0),
                 m_useSphericalSizes(false),
                 m_AzimMin(FLT_MAX),m_PolarMin(FLT_MAX),m_AzimMax(-FLT_MAX),m_PolarMax(-FLT_MAX),
                 m_nComponents(0)
  { }
  void addDetInfo(const Geometry::IDetector_const_sptr &spDet,const Kernel::V3D &Observer);
  void returnAvrgDetPar(DetParameters &det);

  void setUseSpherical(bool shouldWe=true)
  {m_useSphericalSizes = shouldWe;}

  static double nearAngle(const double &baseAngle,const double &anAngle);
};


  } //end namespace DataHandling
} //end namespace Mandid


#endif
