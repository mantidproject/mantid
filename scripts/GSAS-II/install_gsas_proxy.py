#!/usr/bin/env python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import argparse
import os
import pip
import shutil
import site
import subprocess
import urllib2


FAILED_DOWNLOAD_MESSAGE = (
    "Could not download the GSAS installation package. "
    "This can occur for many reasons, one of which is "
    "that your computer is not connected to the internet.\n"
    "A common reason is that your version of SSL is out of date "
    "(often seen on OSX). From a Python shell, run "
    "'import ssl; print(ssl.OPENSSL_VERSION)'\n"
    "If you see a version number less than 1.0, you need to either "
    "upgrade SSL (contact the Mantid team or seek help online) "
    "or do a manual installation from the GSAS-II website.\n"
    "If neither of these solutions yield anything useful, please "
    "get in contact with the Mantid team."
)
GSAS_SVN_URL = "https://subversion.xray.aps.anl.gov/pyGSAS/install/GSASIIproxy.zip"
GSAS_BOOTSTRAP_URL = "https://subversion.xray.aps.anl.gov/pyGSAS/install/bootstrap.py"
GSAS_PROXY_FILE_NAME = "GSASIIProxy.zip"
GSAS_HOME_DIR_NAME = "g2conda"


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


def install_gsasii(install_directory, revision_number, force_overwrite):
    gsas_home_dir = os.path.join(install_directory, GSAS_HOME_DIR_NAME, "GSASII")

    if force_overwrite and os.path.exists(gsas_home_dir):
        print("Removing {}".format(gsas_home_dir))
        shutil.rmtree(gsas_home_dir)

    if not os.path.exists(gsas_home_dir):
        os.makedirs(gsas_home_dir)

    print("Downloading correct version of bootstrap.py")
    bootstrap_file_name = os.path.join(gsas_home_dir, "bootstrap.py")
    download_bootstrap(revision_number, bootstrap_file_name)

    if not package_is_installed("wx"):
        print("Installing wxPython")
        install_package("wxPython")

    print("Installing GSAS-II")
    bootstrap_process = subprocess.Popen(["python", bootstrap_file_name], stdin=subprocess.PIPE)
    bootstrap_process.communicate(input="\n\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Script to install GSAS-II")

    parser.add_argument(
        "-d",
        "--install-dir",
        default=os.path.abspath(os.sep),
        type=str,
        dest="install_dir",
        help="Directory to install GSAS-II in (leave blank to use current drive (Windows) or / (Linux)",
    )

    parser.add_argument(
        "-v",
        "--version",
        default=0,
        type=int,
        dest="version",
        help="SVN revision number to install (leave blank to use the latest revision",
    )

    parser.add_argument(
        "-b",
        "--build-server",
        action="store_true",
        default=False,
        dest="build_server_mode",
        help="Build server mode. Install GSAS-II in Python user site package directory and don't wait for prompt before exiting",
    )

    parser.add_argument(
        "-f",
        "--force-overwrite",
        action="store_true",
        default=False,
        dest="force_overwrite",
        help="Force overwrite mode. If a GSAS-II installation is found at the requested directory, remove it and perform a fresh install",
    )

    args = parser.parse_args()

    if args.build_server_mode:
        install_dir = site.USER_SITE
    else:
        install_dir = args.install_dir

    install_gsasii(install_directory=install_dir, revision_number=args.version, force_overwrite=args.force_overwrite)
