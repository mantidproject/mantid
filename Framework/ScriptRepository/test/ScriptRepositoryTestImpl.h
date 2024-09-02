// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ConfigService.h"
#include "MantidScriptRepository/ScriptRepositoryImpl.h"
#include <Poco/DateTimeFormatter.h>
#include <Poco/File.h>
#include <Poco/FileStream.h>
#include <Poco/Path.h>
#include <Poco/TemporaryFile.h>
#include <boost/algorithm/string.hpp>
#include <cxxtest/TestSuite.h>

#include <algorithm>
#include <filesystem>

using Mantid::API::ScriptRepoException;
using Mantid::API::ScriptRepositoryImpl;
using Mantid::Kernel::ConfigService;
using Mantid::Kernel::ConfigServiceImpl;
using Mantid::Types::Core::DateAndTime;

constexpr auto REPOSITORYJSON = "{\n"
                                "\"TofConv\":\n"
                                "{\n"
                                " \"pub_date\": \"2012-Feb-13 10:00:50\",\n"
                                " \"description\": \"the description\",\n"
                                " \"directory\": true \n"
                                "},\n"
                                "\"TofConv/README.txt\":\n"
                                "{\n"
                                " \"pub_date\": \"2012-Feb-13 10:02:50\",\n"
                                " \"description\": \"tofconv description\",\n"
                                " \"directory\": false \n"
                                "},\n"
                                "\"TofConv/TofConverter.py\":\n"
                                "{\n"
                                "  \"pub_date\": \"2012-Feb-10 10:00:50\",\n"
                                "  \"description\": \"tofconverter description\",\n"
                                "  \"directory\": false	\n"
                                "},\n"
                                "\"reflectometry\":\n"
                                "{\n"
                                "	\"pub_date\": \"2012-Jan-13 10:00:50\",\n"
                                "  \"directory\": true	\n"
                                "},\n"
                                "\"reflectometry/Quick.py\":\n"
                                "{\n"
                                "  \"pub_date\": \"2012-Feb-13 10:00:00\",\n"
                                "  \"description\": \"quick description\",\n"
                                "\"directory\": false	\n"
                                "}\n"
                                "}\n";

constexpr auto TOFCONV_README = "This is the content of TOFCONV_README";
constexpr auto TOFCONV_CONVERTER = "print 'hello world'";
constexpr auto WEBSERVER_URL = "https://localhost";

/** The ScriptRepositoryTest aims to ensure and protect the logic and
the interfaces described for ScriptRepository without requiring
the connection to the internet.

In order to be able to do this, the ::ScriptRepositoryTestImpl
uses the ::ScriptRepositoryImplLocal, which is a derived class
of ::ScriptRepositoryImpl, but adds the following:

 - It overrides the doDownloadFile method, scaping from downloading
   to copy the files locally.

Through this strategy, all the logic and the behavior of the
::ScriptRepositoryImpl can be tested.

It provides some public attributes repository_json_content,
tofconf_readme_content, tofconf_tofconverter_content that allows
to simulate changes and new values for the downloading.

*/

class ScriptRepositoryImplLocal : public ScriptRepositoryImpl {
public:
  ScriptRepositoryImplLocal(const std::string &a = "", const std::string &b = "") : ScriptRepositoryImpl(a, b) {
    repository_json_content = REPOSITORYJSON;
    tofconv_readme_content = TOFCONV_README;
    tofconv_tofconverter_content = TOFCONV_CONVERTER;
    fail = false;
  }
  std::string repository_json_content;
  std::string tofconv_readme_content;
  std::string tofconv_tofconverter_content;

  /** Override the ScriptRepositoryImpl::doDownloadFile in order
      to mock its functioning avoiding the internet connection.

      It allows the following requests:

      @code
      // download repository.json file
      doDownloadFile("<remote_url>/repository.json","<localpath>/repository.json);
      // ping the site
      doDownloadFile("<remote_url>","");
      // download file TofConv/README.txt
      doDownloadFile("<remote_url>/TofConv/README.txt","<localpath>/TofConv/README.txt");
      // download file TofConv/README.txt
      doDownloadFile("<remote_url>/TofConv/TofConverter.py","<localpath>/TofConv/TofConverter.py");

      @endcode

      It also make it public, in order to be able to test this method itself.
   */
  void doDownloadFile(const std::string &url_file, const std::string &local_file_path) override {

    // answer when the download it to 'forget' the downloaded file
    // request to ping the site
    if (local_file_path.empty())
      return;
    if (url_file.find("https://") == std::string::npos) {
      throw ScriptRepoException("Invalid url to download");
    }
    Poco::FileStream _out(local_file_path);

    if (url_file.find("repository.json") != std::string::npos) {
      // request to download repository.json
      _out << repository_json_content;
      _out.close();
      return;
    }

    if (url_file.find("TofConv/README.txt") != std::string::npos) {
      // request to download TofConv/README.txt
      _out << tofconv_readme_content;
      _out.close();
      return;
    }

    if (url_file.find("TofConv/TofConverter.py") != std::string::npos) {
      // request to download TofConv/TofConverter.py
      _out << tofconv_tofconverter_content;
      _out.close();
      return;
    }

    if (url_file == remote_url) {
      // request to ping the site
      _out << "<html><body>Site Alive</body></html>";
      _out.close();
      return;
    }

    std::stringstream ss;
    ss << "Failed to download this file : " << url_file << " to " << local_file_path << std::ends;
    throw ScriptRepoException(ss.str());
  };

  /** Override the ScriptRepositoryImpl::doDeleteRemoteFile in order
      to mock its functioning avoiding the internet connection.

      Its answer depends on the public attribute fail. If fail is false
     (default)
      then, it will return a json string that simulate the answer from the
     remote host
      when it succeed in deleting one file. If fail is true, than, it will
     return
      a json file that simulate the answer from the remote host when the
     operation fails.

      @code
      // delete success
      std::string json_ans =
     doDeleteRemoteFile("<remote_url>/README.md","README.md",
                "noone","noone@nowhere.com","Remove this useless file");

      // delete failure
      fail = true;
      std::string json_fail_ans =
     doDeleteRemoteFile("<remote_url>/README.md","README.md",
                "noone","noone@nowhere.com","Remove this useless file");
      @endcode

      It also make it public, in order to be able to test this method itself.
   */
  bool fail;
  std::string doDeleteRemoteFile(const std::string & /*url*/, const std::string & /*file_path*/,
                                 const std::string & /*author*/, const std::string & /*email*/,
                                 const std::string & /*comment*/) override {
    if (fail)
      return "{\n  \"message\": \"Invalid author: \"\n}";
    else
      return "{\n  \"message\": \"success\"\n}";
  };
};

/** Protect the logic and behavior of ScriptRepositoryImpl without requiring
internet connection.

These tests do no depend on the internet connection
ctest -j8 -R ScriptRepositoryTestImpl_  --verbose

**/
class ScriptRepositoryTestImpl : public CxxTest::TestSuite {
  std::unique_ptr<ScriptRepositoryImplLocal> repo;
  std::string local_rep;
  std::string backup_local_repository_path;

public:
  static ScriptRepositoryTestImpl *createSuite() { return new ScriptRepositoryTestImpl(); }
  static void destroySuite(ScriptRepositoryTestImpl *suite) { delete suite; }

  // ensure that all tests will be perfomed in a fresh repository
  void setUp() override {
    ConfigServiceImpl &config = ConfigService::Instance();
    backup_local_repository_path = config.getString("ScriptLocalRepository");
    local_rep = std::string(Poco::Path::current()).append("mytemprepository/");
    TS_ASSERT_THROWS_NOTHING(repo = std::make_unique<ScriptRepositoryImplLocal>(local_rep, WEBSERVER_URL));
  }

  // ensure that the local files are free from the test created.
  void tearDown() override {
    repo = nullptr;
    try {
      Poco::File f(local_rep);
      f.remove(true);
    } catch (Poco::Exception &ex) {
      TS_WARN(ex.displayText());
    }
    ConfigServiceImpl &config = ConfigService::Instance();
    config.setString("ScriptLocalRepository", backup_local_repository_path);
    config.saveConfig(config.getUserFilename());
  }

  /*****************
   * ENSURING  ScriptRepositoryImplLocal::doDownloadFile do not introduce errors
   ******************/
  void test_doDownloadFile() {
    // ensure it can ping the remote url
    TS_ASSERT_THROWS_NOTHING(repo->doDownloadFile(WEBSERVER_URL, ""));

    // simulate the installation.
    Poco::File dir(local_rep);
    dir.createDirectories();

    {
      // ensure it can download repository.json
      std::string local_j_file = std::string(local_rep).append("/.repository.json");
      TS_ASSERT_THROWS_NOTHING(
          repo->doDownloadFile(std::string(WEBSERVER_URL).append("/repository.json"), local_j_file));
    }
    {
      // ensure it can download TofConv/README.txt
      std::string local_j_file = std::string(local_rep).append("/TofConv/README.txt");
      Poco::File dir(std::string(local_rep).append("/TofConv"));
      dir.createDirectories();
      TS_ASSERT_THROWS_NOTHING(
          repo->doDownloadFile(std::string(WEBSERVER_URL).append("/TofConv/README.txt"), local_j_file));
      Poco::File f(local_j_file);
      TS_ASSERT(f.exists());
    }
  }

  /*************************************
   *   INSTALL
   *************************************/

  /**
     Testing the installation of the Repository Service:

     The normal test, it should be able to create the new folder and put inside
     the
     repository.json and local.json files.
   */
  void test_normal_installation_procedure() {
    // before installing the repository, ScriptRepositoryImpl will be always
    // invalid
    TSM_ASSERT("Why valid?", !repo->isValid());
    // the installation should throw nothing
    TSM_ASSERT_THROWS_NOTHING("Installation should not throw", repo->install(local_rep));
    // the repository must be valid
    TSM_ASSERT("Now should be valid!", repo->isValid());
    // checking that repository.json and local.json exists
    {
      Poco::File l(std::string(local_rep).append("/.repository.json"));
      TSM_ASSERT("Failed to create repository.json", l.exists());
      Poco::File r(std::string(local_rep).append("/.local.json"));
      TSM_ASSERT("Failed to create local.json", r.exists());
    }

    // after the installation, all the others instances of ScriptRepositoryImpl
    // should be valid,
    // by geting the information from the ScriptRepository settings.
    auto other = std::make_unique<ScriptRepositoryImplLocal>();
    TSM_ASSERT("All the others should recognize that this is a valid repository", other->isValid());
  }

  /**
      Installation may install on non-empty directory. If the directory is
     already a ScriptRepository,
      the installation should just return. If it is not, the installation,
     should install the
      two hidden files in that directory.
   */
  void test_installation_do_not_install_on_non_empty_directory() {
    // fill the local_rep path with files
    Poco::File d(local_rep);
    d.createDirectories();
    Poco::FileStream f(std::string(local_rep).append("/myfile"));
    f << "nothing";
    f.close();
    // now, local_rep is not empty!

    // before installing the repository, ScriptRepositoryImpl will be always
    // invalid
    TSM_ASSERT("Why valid?", !repo->isValid());
    // the installation should throw, directory is not empty
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
  }

  void test_checkLocalInstallIsPresent_local_json() {
    TS_ASSERT(!repo->checkLocalInstallIsPresent());
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    TS_ASSERT(repo->checkLocalInstallIsPresent());
    std::filesystem::remove(local_rep.append("/.local.json"));
    TS_ASSERT(!repo->checkLocalInstallIsPresent());
  }

  void test_checkLocalInstallIsPresent_repository_json() {
    TS_ASSERT(!repo->checkLocalInstallIsPresent());
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    TS_ASSERT(repo->checkLocalInstallIsPresent());
    std::filesystem::remove(local_rep.append("/.repository.json"));
    TS_ASSERT(!repo->checkLocalInstallIsPresent());
  }

  /*************************************
   *   List Files
   *************************************/

  /**
     List Files must list all the files at central repository
   */
  void test_listFiles_must_list_all_files_at_central_repository() {
    const char *test_entries[] = {"TofConv", "TofConv/README.txt", "TofConv/TofConverter.py", "reflectometry",
                                  "reflectometry/Quick.py"};
    std::vector<std::string> list_files;
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    TS_ASSERT_THROWS_NOTHING(list_files = repo->listFiles());
    TS_ASSERT_EQUALS(list_files.size(), 5);
    // check that all the files at the central repository are inside
    for (auto &test_entry : test_entries)
      TSM_ASSERT_THROWS_NOTHING(test_entry, repo->info(test_entry));
  }

  /**
     List File must list all the local files as well.
   */
  void test_listFiles_must_list_all_local_files() {
    // will create the folder
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));

    // creating a file to test the list Files
    std::string local_file = std::string(local_rep).append("/myfile");
    Poco::FileStream f(local_file);
    f << "nothing";
    f.close();

    //
    std::vector<std::string> files;
    TS_ASSERT_THROWS_NOTHING(files = repo->listFiles());

    // checking that the local_file was listed in listFiles.
    TSM_ASSERT_THROWS_NOTHING(local_file, repo->info("myfile"));
    // MUST ACCEPT AN ABSOLUTE PATH AS WELL
    TSM_ASSERT_THROWS_NOTHING(local_file, repo->info(local_file));
  }

  /*************************************
   *   File Info
   *************************************/
  void test_info_correctly_parses_the_repository_json() {
    using Mantid::API::ScriptInfo;
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    ScriptInfo information = repo->info("TofConv/TofConverter.py");
    TS_ASSERT(repo->description("TofConv/TofConverter.py") == "tofconverter description");
    TS_ASSERT(information.author.empty());
    TSM_ASSERT("check time", information.pub_date == DateAndTime("2012-02-10 10:00:50"));
    TS_ASSERT(!information.auto_update);
  }

  /*************************************
   *   Download
   *************************************/

  /** Test that we are able to download files from the remote repository
   */
  void test_download_new_files_from_repository() {
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    TS_ASSERT_THROWS_NOTHING(repo->download("TofConv/README.txt"));
  }

  /**
      Test that we are able to download folders from the remote repository
   */
  void test_download_new_folder_from_repository() {
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    TS_ASSERT_THROWS_NOTHING(repo->download("TofConv"));
  }

  /** Test that we can download files inside folder one at once
   */
  void test_downloading_single_files() {
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    TS_ASSERT_THROWS_NOTHING(repo->download("TofConv/README.txt"));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    TS_ASSERT_THROWS_NOTHING(repo->download("TofConv/TofConverter.py"));
  }

  /**
      There is no point downloading files if they have only local_changes,
      so, this test that it is not possible to download the same file
      twice, witouth a new version.
   */
  void tnoest_downloading_twice_the_same_file() {
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    TS_ASSERT_THROWS_NOTHING(repo->download("TofConv/README.txt"));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    // there is no new version, so there is no point to download it again.
    // it throws that the file has not changed.
    TS_ASSERT_THROWS(repo->download("TofConv/README.txt"), const ScriptRepoException &);
  }

  /*************************************
   *  UPDATE
   *************************************/
  void test_update() {
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    std::vector<std::string> list_of_files;
    TS_ASSERT_THROWS_NOTHING(list_of_files = repo->listFiles());
    TS_ASSERT(list_of_files.size() == 5);

    // simulate remove of cleaning up the repository, and having just a
    // README.md file
    //
    repo->repository_json_content = "{\n"
                                    "\"README.md\":\n"
                                    "{\n"
                                    " \"pub_date\": \"2012-02-20 10:00:50\",\n"
                                    " \"description\": \"Script Repository Script\",\n"
                                    " \"directory\": false \n"
                                    "}\n"
                                    "}\n";

    TS_ASSERT_THROWS_NOTHING(repo->check4Update());
    TS_ASSERT_THROWS_NOTHING(list_of_files = repo->listFiles());

    std::cout << "After update, the files are: ";
    for (auto &list_of_file : list_of_files) {
      std::cout << list_of_file << ", ";
    }
    std::cout << '\n';
    TS_ASSERT(list_of_files.size() == 1);
    TS_ASSERT(list_of_files[0] == "README.md");
  }

  void test_auto_update() {
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    std::vector<std::string> list_of_files;
    TS_ASSERT_THROWS_NOTHING(list_of_files = repo->listFiles());
    TS_ASSERT(list_of_files.size() == 5);
    std::string file_name = "TofConv/README.txt";

    // before downloading the file is REMOTE_ONLY
    TS_ASSERT(repo->fileStatus(file_name) == Mantid::API::REMOTE_ONLY);

    // do download
    TS_ASSERT_THROWS_NOTHING(repo->download(file_name));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());

    // after downloading the file is BOTH_UNCHANGED
    TS_ASSERT(repo->fileStatus(file_name) == Mantid::API::BOTH_UNCHANGED);

    // set this file for AutoUpdate
    TS_ASSERT_THROWS_NOTHING(repo->setAutoUpdate(file_name, true));

    // simulate a new version of the file inside the central repository
    //
    std::string original_time = "2012-Feb-13 10:02:50";
    std::string change_to_ = "2012-Mar-13 10:02:50";

    // simulate new version of the file
    boost::replace_all(repo->repository_json_content, original_time, change_to_);

    // execute a check4updte
    TS_ASSERT_THROWS_NOTHING(list_of_files = repo->check4Update());

    // ensure that it has downloaded the file again
    TS_ASSERT(list_of_files.size() == 1);
    TS_ASSERT(list_of_files[0] == file_name);
  }

  void test_auto_update_cascade() {
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    std::vector<std::string> list_of_files;
    TS_ASSERT_THROWS_NOTHING(list_of_files = repo->listFiles());
    TS_ASSERT(list_of_files.size() == 5);
    std::string folder_name = "TofConv";
    std::string file_name_readme = folder_name + "/README.txt";
    std::string file_name_conv = folder_name + "/TofConverter.py";
    // before downloading the file is REMOTE_ONLY
    TS_ASSERT(repo->fileStatus(file_name_readme) == Mantid::API::REMOTE_ONLY);

    // do download
    TS_ASSERT_THROWS_NOTHING(repo->download(file_name_readme));
    TS_ASSERT_THROWS_NOTHING(repo->download(file_name_conv));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());

    // after downloading the file is BOTH_UNCHANGED
    TS_ASSERT(repo->fileStatus(file_name_readme) == Mantid::API::BOTH_UNCHANGED);
    TS_ASSERT(repo->fileStatus(file_name_conv) == Mantid::API::BOTH_UNCHANGED);

    TS_ASSERT(repo->setAutoUpdate(file_name_readme, true) == 1);

    // set this file for AutoUpdate (return 3: cascaded to 3 entries)
    TS_ASSERT(repo->setAutoUpdate(folder_name, true) == 3);
    TS_ASSERT(repo->fileInfo(folder_name).auto_update == true);
    TS_ASSERT(repo->fileInfo(file_name_readme).auto_update == true);
    TS_ASSERT(repo->fileInfo(file_name_conv).auto_update == true);

    // remove the folder
    {
      std::string path_to_folder = std::string(local_rep).append(folder_name);
      Poco::File f(path_to_folder);
      f.remove(true);
    }

    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    TS_ASSERT(repo->fileInfo(folder_name).auto_update == false);
    TS_ASSERT(repo->fileInfo(file_name_readme).auto_update == false);
    TS_ASSERT(repo->fileInfo(file_name_conv).auto_update == false);

    // download recursively
    TS_ASSERT_THROWS_NOTHING(repo->download(folder_name));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());

    TS_ASSERT(repo->fileInfo(folder_name).auto_update == false);
    TS_ASSERT(repo->fileInfo(file_name_readme).auto_update == false);
    TS_ASSERT(repo->fileInfo(file_name_conv).auto_update == false);
  }

  void test_auto_update_cascade_remove_all_internal_files() {
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    std::vector<std::string> list_of_files;
    TS_ASSERT_THROWS_NOTHING(list_of_files = repo->listFiles());
    TS_ASSERT(list_of_files.size() == 5);
    std::string folder_name = "TofConv";
    TS_ASSERT_THROWS_NOTHING(repo->download(folder_name));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    TS_ASSERT(repo->setAutoUpdate(folder_name, true) == 3);

    std::string file_name_readme = folder_name + "/README.txt";
    std::string file_name_conv = folder_name + "/TofConverter.py";

    // remove the folder
    std::cout << "Removing children files\n";
    {

      {
        std::string path_to_readme = std::string(local_rep).append(file_name_readme);
        Poco::File f(path_to_readme);
        f.remove();
      }
      {
        std::string path_to_conv = std::string(local_rep).append(file_name_conv);
        Poco::File f(path_to_conv);
        f.remove();
      }
    }
    std::cout << "children files removed\n";

    // without internal files, the folder should lose the auto_update flag.
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    TS_ASSERT(repo->fileInfo(folder_name).auto_update == false);
    TS_ASSERT(repo->fileInfo(file_name_readme).auto_update == false);
    TS_ASSERT(repo->fileInfo(file_name_conv).auto_update == false);

    // download recursively
    TS_ASSERT_THROWS_NOTHING(repo->download(folder_name));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());

    TS_ASSERT(repo->fileInfo(folder_name).auto_update == false);
    TS_ASSERT(repo->fileInfo(file_name_readme).auto_update == false);
    TS_ASSERT(repo->fileInfo(file_name_conv).auto_update == false);
  }

  /*************************************
   *   FILE STATUS
   *************************************/
  void test_info_of_one_file() {
    std::string file_name = "TofConv/README.txt";
    std::string dir_name = "TofConv";
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    // before downloading the file is REMOTE_ONLY
    TS_ASSERT(repo->fileStatus(file_name) == Mantid::API::REMOTE_ONLY);
    TS_ASSERT(repo->fileStatus(dir_name) == Mantid::API::REMOTE_ONLY);

    // do download
    TS_ASSERT_THROWS_NOTHING(repo->download(file_name));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());

    // after downloading the file is BOTH_UNCHANGED
    TS_ASSERT(repo->fileStatus(file_name) == Mantid::API::BOTH_UNCHANGED);
    TS_ASSERT(repo->fileStatus(dir_name) == Mantid::API::BOTH_UNCHANGED);

    std::string original_time = "2012-Feb-13 10:02:50";
    std::string change_to_ = "2012-Mar-13 10:02:50";

    // simulate new version of the file
    boost::replace_all(repo->repository_json_content, original_time, change_to_);

    TS_ASSERT_THROWS_NOTHING(repo->check4Update());

    // should change to REMOTE_CHANGED
    TS_ASSERT(repo->fileStatus(file_name) == Mantid::API::REMOTE_CHANGED);
    TS_ASSERT(repo->fileStatus(dir_name) == Mantid::API::REMOTE_CHANGED);

    // restore the file
    repo->repository_json_content = REPOSITORYJSON;

    TS_ASSERT_THROWS_NOTHING(repo->check4Update());

    // after downloading the file is BOTH_UNCHANGED
    TS_ASSERT(repo->fileStatus(file_name) == Mantid::API::BOTH_UNCHANGED);
    TS_ASSERT(repo->fileStatus(dir_name) == Mantid::API::BOTH_UNCHANGED);

    // we will simulate the change of the file, by, changin the local.json
    // file

    Poco::FileStream ss(std::string(local_rep).append("/local.json"));
    ss << "{\n"
       << "\"TofConv/README.txt\":\n"
       << "{\n"
       << "\"downloaded_date\": \"2013-Mar-07 14:30:09\",\n"
       << "\"downloaded_pubdate\": \"2012-Feb-13 10:02:50\"\n"
       << "}\n"
       << "}";
    ss.close();
    std::string localjson = std::string(local_rep).append("/.local.json");
    Poco::File f(std::string(local_rep).append("/local.json"));

#if defined(_WIN32) || defined(_WIN64)
    // set the .repository.json and .local.json hidden
    SetFileAttributes(localjson.c_str(), FILE_ATTRIBUTE_NORMAL);
#endif
    f.moveTo(localjson);
#if defined(_WIN32) || defined(_WIN64)
    // set the .repository.json and .local.json hidden
    SetFileAttributes(localjson.c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif

    TS_ASSERT_THROWS_NOTHING(repo->listFiles());

    // file has local changes
    TS_ASSERT(repo->fileStatus(file_name) == Mantid::API::LOCAL_CHANGED);
    TS_ASSERT(repo->fileStatus(dir_name) == Mantid::API::LOCAL_CHANGED);

    // simulate new version of the file
    boost::replace_all(repo->repository_json_content, original_time, change_to_);

    TS_ASSERT_THROWS_NOTHING(repo->check4Update());

    // file has local and remote changes
    TS_ASSERT(repo->fileStatus(file_name) == Mantid::API::BOTH_CHANGED);
    TS_ASSERT(repo->fileStatus(dir_name) == Mantid::API::BOTH_CHANGED);
  }

  /*************************************
   *   FILE STATUS
   *************************************/
  void test_info_of_downloaded_folder() {
    std::string file_name = "TofConv/TofConverter.py";
    std::string folder_name = "TofConv";
    // install
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    // list files
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    // download
    TS_ASSERT_THROWS_NOTHING(repo->download(folder_name));
    // it must be unchanged
    TS_ASSERT(repo->fileStatus(file_name) == Mantid::API::BOTH_UNCHANGED);
    // it
    TS_ASSERT(repo->fileStatus(folder_name) == Mantid::API::BOTH_UNCHANGED);
  }

  void test_status_of_empty_local_folder() {
    std::string folder_name = "LocalFolder";
    // install
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));

    Poco::File dir(std::string(local_rep).append(folder_name));
    dir.createDirectories();

    // list files
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    // it should be local only
    TS_ASSERT(repo->fileStatus(folder_name) == Mantid::API::LOCAL_ONLY);
  }

  void test_downloading_and_removing_files() {
    std::string file_name = "TofConv/TofConverter.py";
    // install
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    // list files
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    // download
    TS_ASSERT_THROWS_NOTHING(repo->download(file_name));
    // it must be unchanged
    TS_ASSERT(repo->fileStatus(file_name) == Mantid::API::BOTH_UNCHANGED);

    // now, lets delete this file from the repository

    {
      Poco::File f(std::string(local_rep).append(file_name));
      f.remove();
    }

    // so, the file should be remote_only and not Mantid::API::BOTH_CHANGED
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    TS_ASSERT(repo->fileStatus(file_name) == Mantid::API::REMOTE_ONLY);
  }

  /** If a file has local changes, than, download should create a backup
   **/
  void test_downloading_locally_modified_file() {
    std::string file_name = "TofConv/README.txt";
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    // do download
    TS_ASSERT_THROWS_NOTHING(repo->download(file_name));

    // we will simulate the change of the file, by, changin the local.json
    // file

    Poco::FileStream ss(std::string(local_rep).append("/local.json"));
    ss << "{\n"
       << "\"TofConv/README.txt\":\n"
       << "{\n"
       << "\"downloaded_date\": \"2013-Mar-07 14:30:09\",\n"
       << "\"downloaded_pubdate\": \"2012-Feb-13 10:02:50\"\n"
       << "}\n"
       << "}";
    ss.close();
    std::string localjson = std::string(local_rep).append("/.local.json");
    Poco::File f(std::string(local_rep).append("/local.json"));

#if defined(_WIN32) || defined(_WIN64)
    // set the .repository.json and .local.json hidden
    SetFileAttributes(localjson.c_str(), FILE_ATTRIBUTE_NORMAL);
#endif
    f.moveTo(localjson);
#if defined(_WIN32) || defined(_WIN64)
    // set the .repository.json and .local.json hidden
    SetFileAttributes(localjson.c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif

    // now, we will simulate a new version of the file
    std::string original_time = "2012-Feb-13 10:02:50";
    std::string change_to_ = "2012-Mar-13 10:02:50";

    // simulate new version of the file
    boost::replace_all(repo->repository_json_content, original_time, change_to_);

    TS_ASSERT_THROWS_NOTHING(repo->check4Update());

    // should change to REMOTE_CHANGED
    TS_ASSERT(repo->fileStatus(file_name) == Mantid::API::BOTH_CHANGED);

    // download the file
    TS_ASSERT_THROWS_NOTHING(repo->download(file_name));

    // ensure that a backup was created
    {
      Poco::File bckf(std::string(local_rep).append(file_name).append("_bck"));
      TSM_ASSERT("No backup file was created!", bckf.exists());
    }
  }

  void test_list_files_after_download_repository() {
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    TS_ASSERT_THROWS_NOTHING(repo->download("TofConv/TofConverter.py"));
    TS_ASSERT_THROWS_NOTHING(repo->check4Update());
    TS_ASSERT_THROWS_NOTHING(repo->download("TofConv/TofConverter.py"));
  }

  void test_download_add_folder_to_python_scripts() {
    ConfigServiceImpl &config = ConfigService::Instance();
    std::string backup_python_directories = config.getString("pythonscripts.directories");

    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    TS_ASSERT_THROWS_NOTHING(repo->download("TofConv/TofConverter.py"));

    std::string curr_python_direc = config.getString("pythonscripts.directories");
    std::string direc = std::string(local_rep).append("TofConv/");
    // make all the back slashs direct slashs, for comparing the path
    // required for windows.
    boost::replace_all(curr_python_direc, "\\", "/");
    boost::replace_all(direc, "\\", "/");

    TS_ASSERT(curr_python_direc.find(direc) != std::string::npos);

    config.setString("pythonscripts.directories", backup_python_directories);
    config.saveConfig(config.getUserFilename());
  }

  /*************************************
   *   SET IGNORE FILES
   *************************************/

  void test_ignore_files() {
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    std::string backup = repo->ignorePatterns();

    std::string file_path = std::string(local_rep).append("/myfile.pyc");
    { // create a file inside
      Poco::File f(file_path);
      f.createFile();
      Poco::FileStream _out(file_path);
      _out << "qq";
      _out.close();
    }

    // myfile.pyc should be ignored
    repo->setIgnorePatterns("*.pyc");
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    TS_ASSERT_THROWS(repo->info("myfile.pyc"), const ScriptRepoException &);

    // myfile.pyc should not be ignored
    repo->setIgnorePatterns("");
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    TS_ASSERT_THROWS_NOTHING(repo->info("myfile.pyc"));
    // clean the ignore patterns
    repo->setIgnorePatterns(backup);
  }

  void test_construct_without_parameters() {
    TS_ASSERT_THROWS_NOTHING(repo = std::make_unique<ScriptRepositoryImplLocal>());
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
  }

  /**This test ensure that when you remove a file from the central repository,

     the entry will be available only internally as LOCAL_ONLY file.
   */
  void test_delete_remove_valid_file_from_central_repository() {
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    std::string file_name = "TofConv/TofConverter.py";
    // download the file
    TS_ASSERT_THROWS_NOTHING(repo->download(file_name));

    // it must be unchanged
    TS_ASSERT(repo->fileStatus(file_name) == Mantid::API::BOTH_UNCHANGED);

    // now, lets delete this file from the central repository
    TS_ASSERT_THROWS_NOTHING(repo->remove(file_name, "please remove it", "noauthor", "noemail"));

    // you should not find the file, so fileStatus should throw exception entry
    // not inside repository
    TS_ASSERT(repo->fileStatus(file_name) == Mantid::API::LOCAL_ONLY);

    // even if you re-read the repository listing the files
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());

    // you should not find this file agin
    TS_ASSERT(repo->fileStatus(file_name) == Mantid::API::LOCAL_ONLY);

    // assert file does exist inside the local folder
    Poco::File f(std::string(local_rep).append(file_name));
    TS_ASSERT(f.exists());
  }

  /** This test simulate the reaction when the delete from the central
     repository
      fails.
   */
  void test_delete_remove_valid_file_from_central_repository_simulate_server_rejection() {
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    std::string file_name = "TofConv/TofConverter.py";
    // download
    TS_ASSERT_THROWS_NOTHING(repo->download(file_name));

    // it must be unchanged
    TS_ASSERT(repo->fileStatus(file_name) == Mantid::API::BOTH_UNCHANGED);
    repo->fail = true;
    // now, lets delete this file from the repository
    // it must throw exception describing the reason for failuring.
    TS_ASSERT_THROWS(repo->remove(file_name, "please remove it", "noauthor", "noemail"), const ScriptRepoException &);

    // you should find the file internally and externally
    TS_ASSERT(repo->fileStatus(file_name) == Mantid::API::BOTH_UNCHANGED);
    // nothing should changing, re-reading the whole repository list
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    // you should find the file
    TS_ASSERT(repo->fileStatus(file_name) == Mantid::API::BOTH_UNCHANGED);
  }

  /** Test invalid entry for removing files, when they are not local (not
     downloaded)

      Ensure that removing from the central repository is not allowed, if the
     file has
      not been downloaded first.
   */
  void test_delete_file_not_local() {
    TS_ASSERT_THROWS_NOTHING(repo->install(local_rep));
    TS_ASSERT_THROWS_NOTHING(repo->listFiles());
    std::string file_name = "TofConv/TofConverter.py";

    // attempt to remove file that is not local (no download was done)
    // it must throw exception, to inform that it is not allowed to remove it.
    TS_ASSERT_THROWS(repo->remove(file_name, "please remove it", "noauthor", "noemail"), const ScriptRepoException &);
    // the state is still remote-only
    TS_ASSERT(repo->fileStatus(file_name) == Mantid::API::REMOTE_ONLY);
  }
};
