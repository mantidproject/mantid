#include "EnggDiffCalibrationPresenter.h"

#include <Poco/Path.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

namespace {

boost::optional<std::string>
validateCalibPath(const boost::optional<std::string> &filename) {
  if (!filename) {
    return boost::make_optional<std::string>("No file selected");
  }

  Poco::Path pocoPath;
  const bool pathValid = pocoPath.tryParse(*filename);

  if (!pathValid) {
    return "\"" + *filename + "\" is not a valid filename";
  }
  return boost::none;
}
}

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffCalibrationPresenter::EnggDiffCalibrationPresenter(
    std::unique_ptr<IEnggDiffCalibrationModel> model,
    boost::shared_ptr<IEnggDiffCalibrationView> view,
    boost::shared_ptr<EnggDiffUserSettings> userSettings)
    : m_model(std::move(model)), m_userSettings(userSettings), m_view(view) {}

void EnggDiffCalibrationPresenter::notify(
    IEnggDiffCalibrationPresenter::Notification notif) {
  switch (notif) {

  case IEnggDiffCalibrationPresenter::Notification::LoadCalibration:
    processLoadCalibration();
    break;
  }
}

std::tuple<std::string, std::string, std::string>
EnggDiffCalibrationPresenter::parseCalibPath(const std::string &path) const {
  Poco::Path fullPath(path);
  auto filenameWithExtension = fullPath.getFileName();
  const auto filename = filenameWithExtension.erase(
      filenameWithExtension.find_last_of("."), std::string::npos);

  const std::string explMsg =
      "Expected a file name like 'INSTR_vanNo_ceriaNo_....par', "
      "where INSTR is the instrument name and vanNo and ceriaNo are the "
      "numbers of the Vanadium and calibration sample (Ceria, CeO2) runs.";

  std::vector<std::string> parts;
  boost::split(parts, filename, boost::is_any_of("_"));
  if (parts.size() < 3) {
    throw std::invalid_argument(
        "Failed to find at least the 3 required parts of the file name.\n\n" +
        explMsg);
  }

  const auto instName = m_userSettings->getInstName();
  if (instName != parts[0]) {
    throw std::invalid_argument("The first component of the file name is not "
                                "the expected instrument name: " +
                                instName + ".\n\n" + explMsg);
  }
  const std::string castMsg =
      "It is not possible to interpret as an integer number ";
  try {
    boost::lexical_cast<int>(parts[1]);
  } catch (std::runtime_error &) {
    throw std::invalid_argument(
        castMsg + "the Vanadium number part of the file name.\n\n" + explMsg);
  }
  try {
    boost::lexical_cast<int>(parts[2]);
  } catch (std::runtime_error &) {
    throw std::invalid_argument(
        castMsg + "the Ceria number part of the file name.\n\n" + explMsg);
  }
  return std::make_tuple(parts[0], parts[1], parts[2]);
}

void EnggDiffCalibrationPresenter::processLoadCalibration() {
  const auto filename = m_view->getInputFilename();
  const auto fileInvalid = validateCalibPath(filename);
  if (fileInvalid) {
    m_view->userWarning("Invalid calibration file", *fileInvalid);
    return;
  }

  std::string instName;
  std::string vanadiumRunNumber;
  std::string ceriaRunNumber;

  try {
    std::tie(instName, vanadiumRunNumber, ceriaRunNumber) =
        parseCalibPath(*filename);
  } catch (std::invalid_argument &ia) {
    m_view->userWarning("Invalid calibration filename", ia.what());
    return;
  }

  const auto calibParams = m_model->parseCalibrationFile(*filename);
  m_view->displayLoadedVanadiumRunNumber(vanadiumRunNumber);
  m_view->displayLoadedCeriaRunNumber(ceriaRunNumber);

  m_model->setCalibrationParams(calibParams);
}

} // CustomInterfaces
} // MantidQt
