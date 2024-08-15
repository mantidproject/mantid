# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from os.path import abspath, dirname, join

TEMPLATE_DIRECTORY = join(dirname(abspath(__file__)), "templates")


def generate_file_from(name: str, file_type: str):
    with open(join(TEMPLATE_DIRECTORY, f"{file_type.lower()}.py.in"), mode="r") as file:
        content = file.read()

    content = content.replace(file_type, f"{name}{file_type}")

    with open(join(TEMPLATE_DIRECTORY, f"{name}_{file_type.lower()}.py"), mode="w") as file:
        file.write(content)


def generate_python_files(name: str) -> None:
    print("Generating Python files with an MVP pattern...")
    generate_file_from(name, "View")
    generate_file_from(name, "Presenter")
    generate_file_from(name, "Model")


def generate_cpp_files(name: str) -> None:
    print("Generating C++ files with an MVP pattern...")
    generate_file_from(name, "View")
    generate_file_from(name, "Presenter")
    generate_file_from(name, "Model")


def generate_files(name: str, language: str) -> None:
    match language.lower():
        case "python":
            generate_python_files(name)
        case "c++":
            generate_cpp_files(name)
        case _:
            raise ValueError(f"An unsupported language '{language}' has been provided. Choose one: [Python, C++].")


if __name__ == "__main__":
    from argparse import ArgumentParser

    parser = ArgumentParser(description="Generates files which can be used as an initial Model-View-Presenter template.")
    parser.add_argument("-n", "--name", required=True, help="The base name to use for the files and classes.")
    parser.add_argument("-l", "--language", required=True, help="The language to generate template MVP files for [Python or C++].")
    args = parser.parse_args()

    generate_files(args.name, args.language)
