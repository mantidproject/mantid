import argparse
import os
import pip
import site
import subprocess
import urllib2
import zipfile


GSAS_SVN_URL = "https://subversion.xray.aps.anl.gov/pyGSAS/install/GSASIIproxy.zip"
GSAS_BOOTSTRAP_URL = "https://subversion.xray.aps.anl.gov/pyGSAS/install/bootstrap.py"
GSAS_PROXY_FILE_NAME = "GSASIIProxy.zip"
GSAS_HOME_DIR_NAME = "g2conda"


def download_zip_file(target_location):
    response = urllib2.urlopen(GSAS_SVN_URL)
    zip_file = response.read()
    response.close()

    with open(target_location, "wb") as out:
        out.write(zip_file)


def unzip_file(zip_file_name, target_directory):
    zip_file = zipfile.ZipFile(zip_file_name, "r")
    zip_file.extractall(target_directory)
    zip_file.close()


def download_bootstrap(revision_number, target_location):
    url = GSAS_BOOTSTRAP_URL
    if revision_number:
        url += "?r={}".format(revision_number)

    response = urllib2.urlopen(url)
    bootstrap_file = response.read()
    response.close()

    with open(target_location, "w") as out_file:
        out_file.write(bootstrap_file)


def package_is_installed(package_name):
    try:
        exec("import " + package_name)
    except ImportError:
        return False
    return True


def install_package(package_name):
    pip.main(["install", package_name])


def install_gsasii(install_directory, revision_number):
    gsas_home_dir = os.path.join(install_directory, GSAS_HOME_DIR_NAME)

    if not os.path.exists(gsas_home_dir):
        # This is the first time installing GSAS-II, so we have to make a home directory and download the SVN kit
        print("Downloading GSAS mini-SVN kit")

        if not os.path.exists(install_directory):
            os.makedirs(install_directory)

        proxy_zip_file = os.path.join(install_directory, GSAS_PROXY_FILE_NAME)
        download_zip_file(target_location=proxy_zip_file)

        print("Extracting GSAS proxy installation")
        gsas_home_dir = os.path.join(install_directory, GSAS_HOME_DIR_NAME)
        unzip_file(zip_file_name=proxy_zip_file, target_directory=gsas_home_dir)

        os.remove(proxy_zip_file)

    print("Downloading correct version of bootstrap.py")
    bootstrap_file_name = os.path.join(gsas_home_dir, "GSASII", "bootstrap.py")
    download_bootstrap(revision_number, bootstrap_file_name)

    if not package_is_installed("wx"):
        print("Installing wxPython")
        install_package("wxPython")

    print("Installing GSAS-II")
    subprocess.call(["python", bootstrap_file_name])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Script to install GSAS-II")

    parser.add_argument("-d", "--install-dir",
                        default=os.path.abspath(os.sep),
                        type=str,
                        dest="install_dir",
                        help="Directory to install GSAS-II in "
                             "(leave blank to use current drive (Windows) or / (Linux)")

    parser.add_argument("-v", "--version",
                        default=0,
                        type=int,
                        dest="version",
                        help="SVN revision number to install (leave blank to use the latest revision")

    parser.add_argument("-b", "--build-server",
                        action="store_true",
                        default=False,
                        dest="build_server_mode",
                        help="Build server mode. Install GSAS-II in Python user site package directory "
                             "and don't wait for prompt before exiting")

    args = parser.parse_args()

    if args.build_server_mode:
        install_dir = site.USER_SITE
    else:
        install_dir = args.install_dir

    install_gsasii(install_directory=install_dir, revision_number=args.version)
