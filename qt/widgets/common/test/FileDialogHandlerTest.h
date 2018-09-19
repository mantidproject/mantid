#ifndef MANTIDQT_API_FILEDIALOGHANDLERTEST_H_
#define MANTIDQT_API_FILEDIALOGHANDLERTEST_H_

#include "MantidQtWidgets/Common/FileDialogHandler.h"
#include <cxxtest/TestSuite.h>

class FileDialogHandlerTest : public CxxTest::TestSuite {
public:
  void test_addExtension() {
    // --- single extensions
    const QString singleExt(".nxs (*.nxs)");
    const QString nexusResult("/tmp/testing.nxs");

    auto result1 = MantidQt::API::FileDialogHandler::addExtension(
        QString::fromStdString("/tmp/testing"), singleExt);
    TS_ASSERT_EQUALS(nexusResult.toStdString(), result1.toStdString());

    auto result2 = MantidQt::API::FileDialogHandler::addExtension(
        QString::fromStdString("/tmp/testing."), singleExt);
    TS_ASSERT_EQUALS(nexusResult.toStdString(), result2.toStdString());

    auto result3 =
        MantidQt::API::FileDialogHandler::addExtension(nexusResult, singleExt);
    TS_ASSERT_EQUALS(nexusResult.toStdString(), result3.toStdString());

    // don't override if it is already specified
    const QString singleH5("/tmp/testing.h5");
    auto result4 =
        MantidQt::API::FileDialogHandler::addExtension(singleH5, singleExt);
    TS_ASSERT_EQUALS(singleH5.toStdString(), result4.toStdString());

    // --- double extensions
    const QString doubleExt("JPEG (*.jpg *.jpeg)");
    const QString jpegResult("/tmp/testing.jpg");

    // this can't work because you can't determine one extension
    TS_ASSERT_THROWS(MantidQt::API::FileDialogHandler::addExtension(
                         QString::fromStdString("/tmp/testing"), doubleExt),
                     std::runtime_error);

    // this shouldn't do anything
    auto result5 =
        MantidQt::API::FileDialogHandler::addExtension(jpegResult, doubleExt);
    TS_ASSERT_EQUALS(jpegResult.toStdString(), result5.toStdString());
  }

  void test_getFileDialogFilter() {
    std::vector<std::string> exts({"*.h5", "*.nxs"});

    const auto result1 =
        MantidQt::API::FileDialogHandler::getFilter(std::vector<std::string>());
    TS_ASSERT_EQUALS(std::string("All Files (*)"), result1.toStdString());

    const auto result2 = MantidQt::API::FileDialogHandler::getFilter(exts);
    TS_ASSERT_EQUALS(std::string("Data Files ( *.h5 *.nxs );;*.h5 "
                                 "(**.h5);;*.nxs (**.nxs);;All Files (*)"),
                     result2.toStdString());
  }

  void test_formatExtension() {
    const std::string bare_ext = "ext";
    const std::string no_star = ".ext";
    const std::string no_dot = "*ext";
    const std::string valid = "*.ext";
    const QString expected("*.ext");

    const auto result1 =
        MantidQt::API::FileDialogHandler::formatExtension(bare_ext);
    TS_ASSERT_EQUALS(expected, result1);

    const auto result2 =
        MantidQt::API::FileDialogHandler::formatExtension(no_star);
    TS_ASSERT_EQUALS(expected, result2);

    const auto result3 =
        MantidQt::API::FileDialogHandler::formatExtension(no_dot);
    TS_ASSERT_EQUALS(expected, result3);

    const auto result4 =
        MantidQt::API::FileDialogHandler::formatExtension(valid);
    TS_ASSERT_EQUALS(expected, result4);
  }
};

#endif /* MANTIDQT_API_FILEDIALOGHANDLERTEST_H_ */
