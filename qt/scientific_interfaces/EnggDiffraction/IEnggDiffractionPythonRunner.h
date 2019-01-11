// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONPYTHONRUNNER_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONPYTHONRUNNER_H_

namespace MantidQt {
namespace CustomInterfaces {

/**
Interface to the python script runner functionality of the Engineering
Diffraction (EnggDiffraction) GUI. This can be used in different
tabs/widgets as well as in the main/central view. Normally the
individual / area specific tabs/widgets will forward to the main view.
*/
class IEnggDiffractionPythonRunner {
public:
  virtual ~IEnggDiffractionPythonRunner() = default;

  /**
   * Run Python code received as a script string. This is used for
   * example to write GSAS instrument parameters file, to plot using
   * the 'plotSpectrum' python functions, or other code included with
   * the Engg scripts. In the first case, this is done temporarily
   * here until we have a more final way of generating these files. A
   * SaveGSAS algorithm that can handle ENGIN-X files would be ideal.
   *
   * @param pyCode Python script as a string
   *
   * @return status string from running the code
   */
  virtual std::string enggRunPythonCode(const std::string &pyCode) = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIOPYTHONRUNNER_H_
