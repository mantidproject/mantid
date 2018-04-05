#include "EnggDiffCalibrationPresenter.h"

#include <Poco/Path.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <algorithm>
#include <cctype>

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

/// Validate a run input, which should either be a numeric run number or a file
bool isDigit(const std::string &runInput) {
  return std::all_of(runInput.cbegin(), runInput.cend(),
                     [](const auto &c) { return std::isdigit(c); });
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

  case IEnggDiffCalibrationPresenter::Notification::Calibrate:
    processCalibrate();
    break;

  case IEnggDiffCalibrationPresenter::Notification::CalibrateCropped:
    processCalibrateCropped();
    break;

  case IEnggDiffCalibrationPresenter::Notification::LoadCalibration:
    processLoadCalibration();
    break;
  }
}

void EnggDiffCalibrationPresenter::displayCalibOutput(
    const GSASCalibrationParameters &calibParams) {
  m_view->setCurrentCalibVanadiumRunNumber(calibParams.vanadiumRunNumber);
  m_view->setCurrentCalibCeriaRunNumber(calibParams.ceriaRunNumber);
  m_view->setCalibFilePath(calibParams.filePath);
}

boost::optional<std::string>
EnggDiffCalibrationPresenter::getAndValidateCeriaInput() const {
  auto ceriaInput = m_view->getNewCalibCeriaInput();
  if (ceriaInput.empty()) {
    m_view->userWarning("No ceria entered",
                        "Please enter a ceria run number to calibrate against");
    return boost::none;
  }
  if (isDigit(ceriaInput)) {
    return m_userSettings->getInstName() + ceriaInput;
  }
  return ceriaInput;
}

boost::optional<std::string>
EnggDiffCalibrationPresenter::getAndValidateVanadiumInput() const {
  auto vanInput = m_view->getNewCalibVanadiumInput();
  if (vanInput.empty()) {
    m_view->userWarning(
        "No vanadium entered",
        "Please enter a vanadium run number to calibrate against");
    return boost::none;
  }
  if (isDigit(vanInput)) {
    return m_userSettings->getInstName() + vanInput;
  }
  return vanInput;
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

void EnggDiffCalibrationPresenter::processCalibrate() {
  const auto vanadiumInput = getAndValidateVanadiumInput();
  if (!vanadiumInput) {
    return;
  }
  const auto ceriaInput = getAndValidateCeriaInput();
  if (!ceriaInput) {
    return;
  }

  std::vector<GSASCalibrationParameters> newCalib;
  try {
    newCalib = m_model->createCalibration(*vanadiumInput, *ceriaInput);
  } catch (std::runtime_error &ex) {
    m_view->userWarning("Calibration failed", ex.what());
    return;
  }
  m_model->setCalibrationParams(newCalib);

  displayCalibOutput(newCalib[0]);
}

void EnggDiffCalibrationPresenter::processCalibrateCropped() {
  const auto vanadiumInput = getAndValidateVanadiumInput();
  if (!vanadiumInput) {
    return;
  }
  const auto ceriaInput = getAndValidateCeriaInput();
  if (!ceriaInput) {
    return;
  }

  const auto cropType = m_view->getCalibCropType();
  std::vector<GSASCalibrationParameters> newCalib;

  try {
    switch (cropType) {
    case CalibCropType::NORTH_BANK:
      newCalib =
          m_model->createCalibrationByBank(1, *vanadiumInput, *ceriaInput);
      break;

    case CalibCropType::SOUTH_BANK:
      newCalib =
          m_model->createCalibrationByBank(2, *vanadiumInput, *ceriaInput);
      break;

    case CalibCropType::SPEC_NUMS:
      const auto specNums = m_view->getSpectrumNumbers();
      if (specNums.empty()) {
        m_view->userWarning(
            "No spectrum numbers",
            "Please enter a set of spectrum numbers to use for focusing");
        return;
      }
      const auto bankName = m_view->getCustomBankName();
      newCalib = m_model->createCalibrationBySpectra(
          specNums, bankName, *vanadiumInput, *ceriaInput);
      break;
    }
  } catch (std::runtime_error &ex) {
    m_view->userWarning("Calibration failed", ex.what());
  }

  displayCalibOutput(newCalib[0]);
  m_model->setCalibrationParams(newCalib);
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

  displayCalibOutput(calibParams[0]);
  m_model->setCalibrationParams(calibParams);
}

} // CustomInterfaces
} // MantidQt
