// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/FindDetectorsPar.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Objects/BoundingBox.h"

#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MultiThreaded.h"

#include <Poco/File.h>
#include <limits>

namespace Mantid::DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(FindDetectorsPar)

using namespace Kernel;
using namespace API;

void FindDetectorsPar::init() {
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<API::InstrumentValidator>();
  wsValidator->add<API::CommonBinsValidator>();
  // input workspace
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the workspace that will be used as input for the algorithm");
  //
  declareProperty("ReturnLinearRanges", false,
                  "if set to true, the algorithm would return linear "
                  "detector's ranges (dx,dy) rather then angular ranges "
                  "(dAzimuthal,dPolar)");
  // optional par or phx file
  const std::vector<std::string> fileExts{".par", ".phx"};

  declareProperty(std::make_unique<FileProperty>("ParFile", "not_used.par", FileProperty::OptionalLoad, fileExts),
                  "An optional file that contains of the list of angular "
                  "parameters for the detectors and detectors groups;\n"
                  "If specified, will use data from file instead of the data, "
                  "calculated from the instument description");

  //
  declareProperty("OutputParTable", "",
                  "If not empty, a name of a table workspace which "
                  " will contain the calculated par or phx values for the detectors");
}

void FindDetectorsPar::exec() {

  // Get the input workspace
  const MatrixWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
  // Number of spectra
  const auto nHist = static_cast<int64_t>(inputWS->getNumberHistograms());

  // try to load par file if one is availible
  std::string fileName = this->getProperty("ParFile");
  if (!(fileName.empty() || fileName == "not_used.par")) {
    if (!Poco::File(fileName).exists()) {
      g_log.error() << " FindDetectorsPar: attempting to load par file: " << fileName << " but it does not exist\n";
      throw(Kernel::Exception::FileError(" file not exist", fileName));
    }
    size_t nPars = loadParFile(fileName);
    if (nPars == static_cast<size_t>(nHist)) {
      this->populate_values_from_file(inputWS);
      this->setOutputTable();
      return;
    } else {
      g_log.warning() << " number of parameters in the file: " << fileName
                      << "  not equal to the number of histograms in the workspace" << inputWS->getName() << '\n';
      g_log.warning() << " calculating detector parameters algorithmically\n";
    }
  }
  m_SizesAreLinear = this->getProperty("ReturnLinearRanges");

  std::vector<DetParameters> Detectors(nHist);
  this->m_nDetectors = 0;

  Progress progress(this, 0.0, 1.0, 100);
  const auto progStep = static_cast<int>(ceil(double(nHist) / 100.0));

  const auto &spectrumInfo = inputWS->spectrumInfo();

  // define the centre of coordinates:
  Kernel::V3D Observer = spectrumInfo.samplePosition();

  // Loop over the spectra
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t i = 0; i < nHist; i++) {
    PARALLEL_START_INTERRUPT_REGION
    // Check that we aren't writing a monitor...
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i))
      continue;

    // valid detector has valid detID
    Detectors[i].detID = spectrumInfo.detector(i).getID();

    // calculate all parameters for current composite detector
    calcDetPar(spectrumInfo.detector(i), Observer, Detectors[i]);

    // make regular progress reports and check for canceling the algorithm

    if (i % progStep == 0) {
      progress.report();
    }
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  this->extractAndLinearize(Detectors);
  // if necessary set up table workspace with detectors parameters.
  this->setOutputTable();
}

/// fills in the ouptput table workspace with calculated values
void FindDetectorsPar::setOutputTable() {
  std::string output = getProperty("OutputParTable");
  if (output.empty())
    return;
  // Store the result in a table workspace
  try {
    declareProperty(
        std::make_unique<WorkspaceProperty<API::ITableWorkspace>>("OutputParTableWS", "", Direction::Output));
  } catch (std::exception &err) {
    g_log.information() << " findDetecotorsPar: unsuccessfully declaring "
                           "property: OutputParTableWS\n";
    g_log.information() << " findDetecotorsPar: the reason is: " << err.what() << '\n';
  }

  // Set the name of the new workspace
  setPropertyValue("OutputParTableWS", output);

  Mantid::API::ITableWorkspace_sptr m_result = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  m_result->addColumn("double", "twoTheta");
  m_result->addColumn("double", "azimuthal");
  m_result->addColumn("double", "secondary_flightpath");
  if (m_SizesAreLinear) {
    m_result->addColumn("double", "det_width");
    m_result->addColumn("double", "det_height");
  } else {
    m_result->addColumn("double", "polar_width");
    m_result->addColumn("double", "azimuthal_width");
  }
  m_result->addColumn("long64", "detID");

  for (size_t i = 0; i < m_nDetectors; i++) {
    Mantid::API::TableRow row = m_result->appendRow();
    row << polar[i] << azimuthal[i] << secondaryFlightpath[i] << polarWidth[i] << azimuthalWidth[i]
        << int64_t(detID[i]);
  }
  setProperty("OutputParTableWS", m_result);
  API::AnalysisDataService::Instance().addOrReplace(output, m_result);
}

// Constant for converting Radians to Degrees
constexpr double rad2deg = 180.0 / M_PI;

/** method calculates an angle closest to the initial one taken on a ring
  * e.g. given inital angle 179 deg and another one -179 closest one to 179 is
  181
  @param baseAngle -- the angle to be close to
  @param anAngle   -- the angle which ring image may be requested

  @returns         -- the angle closest to the initial on a ring.
*/
double AvrgDetector::nearAngle(const double &baseAngle, const double &anAngle) {
  double dist = baseAngle - anAngle;
  if (dist > 180.) {
    return (anAngle + 360.);
  } else if (dist < -180.) {
    return (anAngle - 360.);
  } else
    return anAngle;
}

/** method to cacluate the detectors parameters and add them to the detectors
 *averages
 *@param det      -- reference to the Mantid Detector
 *@param Observer -- sample position or the centre of the polar system of
 *coordinates to calculate detector's parameters.
 */
void AvrgDetector::addDetInfo(const Geometry::IDetector &det, const Kernel::V3D &Observer) {
  m_nComponents++;
  Kernel::V3D detPos = det.getPos();
  Kernel::V3D toDet = (detPos - Observer);

  double dist2Det, Polar, Azimut, ringPolar, ringAzim;
  // identify the detector' position in the beam coordinate system:
  toDet.getSpherical(dist2Det, Polar, Azimut);
  if (m_nComponents <= 1) {
    m_FlightPathSum = dist2Det;
    m_PolarSum = Polar;
    m_AzimutSum = Azimut;

    m_AzimBase = Polar;
    m_PolarBase = Azimut;
    ringPolar = Polar;
    ringAzim = Azimut;
  } else {
    ringPolar = nearAngle(m_AzimBase, Polar);
    ringAzim = nearAngle(m_PolarBase, Azimut);
    m_FlightPathSum += dist2Det;
    m_PolarSum += ringPolar;
    m_AzimutSum += ringAzim;
  }

  // centre of the azimuthal ring (the ring  detectors form around the beam)
  Kernel::V3D ringCentre(0, 0, toDet.Z());

  // Get the bounding box
  Geometry::BoundingBox bbox;
  std::vector<Kernel::V3D> coord(3);

  // ez along beamline, which is always oz;
  Kernel::V3D er(0, 1, 0), ez(0, 0, 1);
  if (dist2Det != 0.0)
    er = toDet / dist2Det; // direction to the detector
  // tangential to the ring and anticlockwise.
  Kernel::V3D e_tg = er.cross_prod(ez);
  if (e_tg.nullVector(1e-12)) {
    e_tg = V3D(1., 0., 0.);
  } else {
    e_tg.normalize();
  }
  // make orthogonal -- projections are calculated in this coordinate system
  ez = e_tg.cross_prod(er);

  coord[0] = er;   // new X
  coord[1] = ez;   // new y
  coord[2] = e_tg; // new z
  bbox.setBoxAlignment(ringCentre, coord);

  det.getBoundingBox(bbox);

  // linear extensions of the bounding box orientied tangentially to the equal
  // scattering angle circle
  double azimMin = bbox.zMin();
  double azimMax = bbox.zMax();
  double polarMin = bbox.yMin(); // bounding box has been rotated according to
                                 // coord above, so z is along e_tg
  double polarMax = bbox.yMax();

  if (m_useSphericalSizes) {
    if (dist2Det == 0)
      dist2Det = 1;

    // convert to angular units
    double polarHalfSize = rad2deg * atan2(0.5 * (polarMax - polarMin), dist2Det);
    double azimHalfSize = rad2deg * atan2(0.5 * (azimMax - azimMin), dist2Det);

    polarMin = ringPolar - polarHalfSize;
    polarMax = ringPolar + polarHalfSize;
    azimMin = ringAzim - azimHalfSize;
    azimMax = ringAzim + azimHalfSize;
  }
  if (m_AzimMin > azimMin)
    m_AzimMin = azimMin;
  if (m_AzimMax < azimMax)
    m_AzimMax = azimMax;

  if (m_PolarMin > polarMin)
    m_PolarMin = polarMin;
  if (m_PolarMax < polarMax)
    m_PolarMax = polarMax;
}

/** Method processes accumulated averages and return them in preexistent avrgDet
class
 */
void AvrgDetector::returnAvrgDetPar(DetParameters &avrgDet) {

  // return undefined detector parameters if no average detector is defined;
  if (m_nComponents == 0)
    return;

  avrgDet.azimutAngle = m_AzimutSum / double(m_nComponents);
  avrgDet.polarAngle = m_PolarSum / double(m_nComponents);
  avrgDet.secondaryFlightPath = m_FlightPathSum / double(m_nComponents);

  avrgDet.azimWidth = (m_AzimMax - m_AzimMin);
  avrgDet.polarWidth = (m_PolarMax - m_PolarMin);
}
/** Method calculates averaged polar coordinates of the detector's group
(which may consist of one detector)
*@param detector -- reference to the Mantid Detector
*@param observer -- sample position or the centre of the polar system of
coordinates to calculate detector's parameters.

*@param detParameters -- return Detector class containing averaged polar coordinates
of the detector or detector's group in
                     spherical coordinate system with centre at Observer
*/
void FindDetectorsPar::calcDetPar(const Geometry::IDetector &detector, const Kernel::V3D &observer,
                                  DetParameters &detParameters) {

  // get number of basic detectors within the composit detector
  size_t nDetectors = detector.nDets();
  // define summator
  AvrgDetector detSum;
  // do we want spherical or linear box sizes?
  detSum.setUseSpherical(!m_SizesAreLinear);

  if (nDetectors == 1) {
    detSum.addDetInfo(detector, observer);
  } else {
    // access contributing detectors;
    auto detGroup = dynamic_cast<const Geometry::DetectorGroup *>(&detector);
    if (!detGroup) {
      g_log.error() << "calc_cylDetPar: can not downcast IDetector_sptr to "
                       "detector group for detector->ID: "
                    << detector.getID() << '\n';
      throw(std::bad_cast());
    }
    for (const auto &det : detGroup->getDetectors()) {
      detSum.addDetInfo(*det, observer);
    }
  }
  // calculate averages and return the detector parameters
  detSum.returnAvrgDetPar(detParameters);
}
/**Method to convert vector of Detector's classes into vectors of doubles with
   all correspondent information
   also drops non-existent detectors and monitors */
void FindDetectorsPar::extractAndLinearize(const std::vector<DetParameters> &detPar) {
  size_t nDetectors;

  // provisional number
  nDetectors = detPar.size();

  this->azimuthal.resize(nDetectors);
  this->polar.resize(nDetectors);
  this->azimuthalWidth.resize(nDetectors);
  this->polarWidth.resize(nDetectors);
  this->secondaryFlightpath.resize(nDetectors);
  this->detID.resize(nDetectors);

  nDetectors = 0;
  for (const auto &parameter : detPar) {
    if (parameter.detID < 0)
      continue;

    azimuthal[nDetectors] = parameter.azimutAngle;
    polar[nDetectors] = parameter.polarAngle;
    azimuthalWidth[nDetectors] = parameter.azimWidth;
    polarWidth[nDetectors] = parameter.polarWidth;
    secondaryFlightpath[nDetectors] = parameter.secondaryFlightPath;
    detID[nDetectors] = static_cast<size_t>(parameter.detID);
    nDetectors++;
  }
  // store caluclated value
  m_nDetectors = nDetectors;

  // resize to actual detector's number
  this->azimuthal.resize(nDetectors);
  this->polar.resize(nDetectors);
  this->azimuthalWidth.resize(nDetectors);
  this->polarWidth.resize(nDetectors);
  this->secondaryFlightpath.resize(nDetectors);
  this->detID.resize(nDetectors);
}

//
size_t FindDetectorsPar::loadParFile(const std::string &fileName) {
  // load ASCII par or phx file
  std::ifstream dataStream;
  std::vector<double> result;
  this->current_ASCII_file = get_ASCII_header(fileName, dataStream);
  load_plain(dataStream, result, current_ASCII_file);

  m_nDetectors = current_ASCII_file.nData_records;

  dataStream.close();
  // transfer par data into internal algorithm parameters;
  azimuthal.resize(m_nDetectors);
  polar.resize(m_nDetectors);
  detID.resize(m_nDetectors);

  int Block_size, shift;

  if (current_ASCII_file.Type == PAR_type) {
    m_SizesAreLinear = true;
    Block_size = 5; // this value coinside with the value defined in load_plain
    shift = 0;
    azimuthalWidth.resize(m_nDetectors);
    polarWidth.resize(m_nDetectors);
    secondaryFlightpath.resize(m_nDetectors, std::numeric_limits<double>::quiet_NaN());

    for (size_t i = 0; i < m_nDetectors; i++) {
      azimuthal[i] = result[shift + 2 + i * Block_size];
      polar[i] = result[shift + 1 + i * Block_size];
      azimuthalWidth[i] = -result[shift + 3 + i * Block_size];
      polarWidth[i] = result[shift + 4 + i * Block_size];
      secondaryFlightpath[i] = result[shift + 0 + i * Block_size];
      detID[i] = i + 1;
    }

  } else if (current_ASCII_file.Type == PHX_type) {
    m_SizesAreLinear = false;
    Block_size = 6; // this value coinside with the value defined in load_plain
    shift = 1;
    azimuthalWidth.resize(m_nDetectors);
    polarWidth.resize(m_nDetectors);
    for (size_t i = 0; i < m_nDetectors; i++) {
      azimuthal[i] = result[shift + 2 + i * Block_size];
      polar[i] = result[shift + 1 + i * Block_size];
      azimuthalWidth[i] = result[shift + 4 + i * Block_size];
      polarWidth[i] = result[shift + 3 + i * Block_size];
      detID[i] = i + 1;
    }
  } else {
    g_log.error() << " unsupported type of ASCII parameter file: " << fileName << '\n';
    throw(std::invalid_argument("unsupported ASCII file type"));
  }

  return m_nDetectors;
}
//
void FindDetectorsPar::populate_values_from_file(const API::MatrixWorkspace_sptr &inputWS) {
  size_t nHist = inputWS->getNumberHistograms();

  if (this->current_ASCII_file.Type == PAR_type) {
    // in this case data in azimuthal width and polar width are in fact real
    // sizes in meters; have to transform it in into angular values
    for (size_t i = 0; i < nHist; i++) {
      azimuthalWidth[i] = atan2(azimuthalWidth[i], secondaryFlightpath[i]) * rad2deg;
      polarWidth[i] = atan2(polarWidth[i], secondaryFlightpath[i]) * rad2deg;
    }
    m_SizesAreLinear = false;
  } else {
    const auto &spectrumInfo = inputWS->spectrumInfo();
    secondaryFlightpath.resize(nHist);
    for (size_t i = 0; i < nHist; i++) {
      if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i))
        continue;
      /// this is the only value, which is not defined in phx file, so we
      /// calculate it
      secondaryFlightpath[i] = spectrumInfo.l2(i);
    }
  }
}
//
int FindDetectorsPar::count_changes(const char *const Buf, size_t buf_size) {
  bool is_symbol(false), is_space(true);
  int space_to_symbol_change(0);
  size_t symbols_start(0);
  // supress leading spaces;
  for (size_t i = 0; i < buf_size; i++) {
    if (Buf[i] == 0)
      break;
    if (Buf[i] == ' ') {
      continue;
    } else {
      symbols_start = i;
      break;
    }
  }
  // calculate number of changes from space to symbol assuming start from
  // symbol;
  for (size_t i = symbols_start; i < buf_size; i++) {
    if (Buf[i] == 0)
      break;
    if (Buf[i] >= '+' && Buf[i] <= 'z') { // this is a symbol
      if (is_space) {
        is_space = false;
        space_to_symbol_change++;
      }
      is_symbol = true;
    }
    if (Buf[i] == ' ') { // this is a space
      is_space = true;
    }
  }
  return space_to_symbol_change;
}

/**! The function reads line from inout stream and puts it into buffer.
 *   It behaves like std::ifstream getline but the getline reads additional
 * symbol from a row in a Unix-formatted file under windows;
 */
size_t FindDetectorsPar::get_my_line(std::ifstream &in, char *buf, size_t buf_size, const char DELIM) {
  size_t i;
  for (i = 0; i < buf_size; i++) {
    in.get(buf[i]);
    if (buf[i] == DELIM) {
      buf[i] = 0;
      return i;
    }
  }
  buf[buf_size - 1] = 0;
  g_log.information() << " data obtained from ASCII data file trunkated to " << buf_size << " characters\n";
  return buf_size;
}
/**!
 *  The function loads ASCII file header and tries to identify the type of the
 *header.
 *  Possible types are
 *  SPE, PAR or PHS
 *
 *  if none three above identified, returns "undefined" type
 *  it also returns the FileTypeDescriptor, which identifyes the position of the
 *data in correcponding ASCII file
 *  plus characteristics of the data extracted from correspondent data header.
 */
FileTypeDescriptor FindDetectorsPar::get_ASCII_header(std::string const &fileName, std::ifstream &data_stream) {
  std::vector<char> buffer(1024);
  FileTypeDescriptor file_descriptor;
  file_descriptor.Type = NumFileTypes; // set the autotype to invalid

  data_stream.open(fileName.c_str(), std::ios_base::in | std::ios_base::binary);
  if (!data_stream.is_open()) {
    g_log.error() << " can not open existing ASCII data file: " << fileName << '\n';
    throw(Kernel::Exception::FileError(" Can not open existing input data file", fileName));
  }
  // let's identify the EOL symbol; As the file may have been prepared on
  // different OS, from where you are reading it
  // and no conversion have been performed;
  char symbol;
  data_stream.get(symbol);
  while (symbol > 0x1F) {
    data_stream.get(symbol);
  }
  char EOL;
  if (symbol == 0x0D) { // Win or old Mac file
    data_stream.get(symbol);
    if (symbol == 0x0A) { // Windows file
      EOL = 0x0A;
    } else { // Mac
      EOL = 0x0D;
      data_stream.putback(symbol);
    }
  } else if (symbol == 0x0A) { // unix file.
    EOL = 0x0A;
  } else {
    g_log.error() << " Error reading the first row of the input ASCII data file: " << fileName
                  << " as it contains unprintable characters\n";
    throw(Kernel::Exception::FileError(" Error reading the first row of the "
                                       "input ASCII data file, as it contains "
                                       "unprintable characters",
                                       fileName));
  }

  file_descriptor.line_end = EOL;
  data_stream.seekg(0, std::ios::beg);

  get_my_line(data_stream, buffer.data(), buffer.size(), EOL);
  if (!data_stream.good()) {
    g_log.error() << " Error reading the first row of the input data file " << fileName
                  << ", It may be bigger then 1024 symbols\n";
    throw(Kernel::Exception::FileError(" Error reading the first row of the "
                                       "input data file, It may be bigger then "
                                       "1024 symbols",
                                       fileName));
  }

  // let's find if there is one or more groups of symbols inside of the buffer;
  int space_to_symbol_change = count_changes(buffer.data(), buffer.size());
  if (space_to_symbol_change > 1) { // more then one group of symbols in the string, spe file
    int nData_records(0), nData_blocks(0);

    int nDatas = sscanf(buffer.data(), " %d %d ", &nData_records, &nData_blocks);
    file_descriptor.nData_records = static_cast<size_t>(nData_records);
    file_descriptor.nData_blocks = static_cast<size_t>(nData_blocks);
    if (nDatas != 2) {
      g_log.error() << " File " << fileName
                    << " iterpreted as SPE but does "
                       "not have two numbers in the "
                       "first row\n";
      throw(Kernel::Exception::FileError(" File iterpreted as SPE but does not "
                                         "have two numbers in the first row",
                                         fileName));
    }
    file_descriptor.Type = SPE_type;
    get_my_line(data_stream, buffer.data(), buffer.size(), EOL);
    if (buffer.front() != '#') {
      g_log.error() << " File " << fileName << "iterpreted as SPE does not have symbol # in the second row\n";
      throw(Kernel::Exception::FileError(" File iterpreted as SPE does not have symbol # in the second row", fileName));
    }
    file_descriptor.data_start_position = data_stream.tellg(); // if it is SPE file then the data begin after the
                                                               // second line;
  } else {
    file_descriptor.data_start_position = data_stream.tellg(); // if it is PHX or PAR file then the data begin
                                                               // after the first line;
    file_descriptor.nData_records = std::stoi(buffer.data());
    file_descriptor.nData_blocks = 0;

    // let's ifendify now if is PHX or PAR file;
    data_stream.getline(buffer.data(), buffer.size(), EOL);

    space_to_symbol_change = count_changes(buffer.data(), buffer.size());
    if (space_to_symbol_change == 6 || space_to_symbol_change == 5) { // PAR file
      file_descriptor.Type = PAR_type;
      file_descriptor.nData_blocks = space_to_symbol_change;
    } else if (space_to_symbol_change == 7) { // PHX file
      file_descriptor.Type = PHX_type;
      file_descriptor.nData_blocks = space_to_symbol_change;
    } else { // something unclear or damaged
      g_log.error() << " can not identify format of the input data file " << fileName << '\n';
      throw(Kernel::Exception::FileError(" can not identify format of the input data file", fileName));
    }
  }
  return file_descriptor;
}

/*!
 *  function to load PHX or PAR file
 *  the file should be already opened and the FILE_TYPE structure properly
 *  defined using get_ASCII_header function
 */
void FindDetectorsPar::load_plain(std::ifstream &stream, std::vector<double> &Data,
                                  FileTypeDescriptor const &FILE_TYPE) {
  std::vector<char> BUF(1024, 0);
  char par_format[] = " %g %g %g %g %g";
  char phx_format[] = " %g %g %g %g %g %g";
  float data_buf[7];
  const char *format;
  int BlockSize;
  char EOL = FILE_TYPE.line_end;

  switch (FILE_TYPE.Type) {
  case (PAR_type): {
    format = par_format;
    BlockSize = 5;
    break;
  }
  case (PHX_type): {
    format = phx_format;
    BlockSize = 6;
    break;
  }
  default: {
    g_log.error() << " trying to load data in FindDetectorsPar::load_plain but "
                     "the data type is not recognized\n";
    throw(std::invalid_argument(" trying to load data but the data type is not recognized"));
  }
  }
  Data.resize(BlockSize * FILE_TYPE.nData_records);

  stream.seekg(FILE_TYPE.data_start_position, std::ios_base::beg);
  if (!stream.good()) {
    g_log.error() << " can not rewind the file to the initial position where "
                     "the data begin\n";
    throw(std::invalid_argument(" can not rewind the file to the initial "
                                "position where the data begin"));
  }

  int nRead_Data(0);
  for (unsigned int i = 0; i < FILE_TYPE.nData_records; i++) {
    stream.getline(BUF.data(), BUF.size(), EOL);
    if (!stream.good()) {
      g_log.error() << " error reading input file\n";
      throw(std::invalid_argument(" error reading input file"));
    }

    switch (FILE_TYPE.Type) {
    case (PAR_type): {
      nRead_Data = sscanf(BUF.data(), format, data_buf, data_buf + 1, data_buf + 2, data_buf + 3, data_buf + 4);
      break;
    }
    case (PHX_type): {
      nRead_Data =
          sscanf(BUF.data(), format, data_buf, data_buf + 1, data_buf + 2, data_buf + 3, data_buf + 4, data_buf + 5);
      break;
    }
    default: {
      g_log.error() << " unsupported value of FILE_TYPE.Type: " << FILE_TYPE.Type << '\n';
      throw(std::invalid_argument(" unsupported value of FILE_TYPE.Type"));
    }
    }
    if (nRead_Data != BlockSize) {
      g_log.error() << " Error reading data at file, row " << i + 1 << " column " << nRead_Data << " from total "
                    << FILE_TYPE.nData_records << " rows, " << BlockSize << " columns\n";
      throw(std::invalid_argument("error while interpreting data "));
    }
    for (int j = 0; j < nRead_Data; j++) {
      Data[i * BlockSize + j] = static_cast<double>(data_buf[j]);
    }
  }
}

} // namespace Mantid::DataHandling
