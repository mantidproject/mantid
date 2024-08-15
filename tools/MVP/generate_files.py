# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from os.path import abspath, dirname, join

TEMPLATE_DIRECTORY = join(dirname(abspath(__file__)), "templates")


def generate_template_files(name: str, language: str) -> None:
    with open(join(TEMPLATE_DIRECTORY, "view.py.in"), mode="r") as file:
        content = file.read()

    content = content.replace("View", f"{name}View")

    with open(join(TEMPLATE_DIRECTORY, "file.py"), mode="w") as file:
        file.write(content)


if __name__ == "__main__":
    from argparse import ArgumentParser

    parser = ArgumentParser(description="Generates files which can be used as an initial Model-View-Presenter template.")
    parser.add_argument("-n", "--name", required=True, help="The base name to use for the files and classes.")
    parser.add_argument("-l", "--language", required=True, help="The language to generate template MVP files for [Python or C++].")
    args = parser.parse_args()

    generate_template_files(args.name, args.language)
