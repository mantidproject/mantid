#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMGGFORMATSCONVERTPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMGGFORMATSCONVERTPRESENTER_H_

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Tomography/IImggFormatsConvertPresenter.h"
#include "MantidQtCustomInterfaces/Tomography/IImggFormatsConvertView.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

// forward declarations for Poco classes
namespace Poco {
class File;
class Path;
}

namespace MantidQt {

namespace API {
class BatchAlgorithmRunner;
}

namespace CustomInterfaces {

/**
Presenter for the widget to convert images and stacks of images
between formats.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTIDQT_CUSTOMINTERFACES_DLL ImggFormatsConvertPresenter
    : public IImggFormatsConvertPresenter {

public:
  /// Default constructor - normally used from the concrete view
  ImggFormatsConvertPresenter(IImggFormatsConvertView *view);
  ~ImggFormatsConvertPresenter() override;

  void notify(IImggFormatsConvertPresenter::Notification notif) override;

protected:
  void initialize();

  /// clean shut down of model, view, etc.
  void cleanup();

  // Methods that process notifications from view->presenter
  void processInit();
  void processConvert();
  void processShutDown();

private:
  size_t goThroughDirRecur(const Poco::File &inFilePath,
                           const std::string &inFormat,
                           const Poco::File &outFilePath,
                           const std::string &outExt, size_t depth);

  void convert(const std::string &inputName, const std::string &inFormat,
               const std::string &outputName,
               const std::string &outFormat) const;

  void convertToFITS(const std::string &inputName, const std::string &inFormat,
                     const std::string &outputName) const;

  void convertToNXTomo(const std::string &inputName,
                       const std::string &inFormat,
                       const std::string &outputName) const;

  Mantid::API::MatrixWorkspace_sptr loadImg(const std::string &inputName,
                                            const std::string inFormat) const;

  Mantid::API::MatrixWorkspace_sptr
  loadFITS(const std::string &inputName) const;

  void saveFITS(Mantid::API::MatrixWorkspace_sptr image,
                const std::string &outputName) const;

  void saveNXTomo(Mantid::API::MatrixWorkspace_sptr image,
                  const std::string &outputName) const;

  /// Associated view for this presenter (MVP pattern)
  IImggFormatsConvertView *const m_view;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMGGGFORMATSCONVERTPRESENTER_H_
