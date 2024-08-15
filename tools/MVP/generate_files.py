# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from os.path import abspath, dirname, join
from typing import Callable, Union

TEMPLATE_DIRECTORY = join(dirname(abspath(__file__)), "templates")


def python_filename(file_type: str, name: Union[str, None] = None) -> str:
    return f"{name}_{file_type.lower()}" if name is not None else file_type.lower()


def cpp_filename(file_type: str, name: Union[str, None] = None) -> str:
    return f"{name}{file_type}" if name is not None else file_type


def generate_file_from(name: str, filename: Callable, file_type: str, extension: str, output_directory: str) -> None:
    template_filepath = join(TEMPLATE_DIRECTORY, f"{filename(file_type)}.{extension}.in")
    with open(template_filepath, mode="r") as file:
        content = file.read()

    content = content.replace(file_type, f"{name}{file_type}")

    output_filepath = join(output_directory, f"{filename(file_type, name)}.{extension}")
    with open(output_filepath, mode="w") as file:
        file.write(content)


def generate_python_files(name: str, output_directory: str) -> None:
    print("Generating Python files with an MVP pattern...")
    generate_file_from(name, python_filename, "View", "py", output_directory)
    generate_file_from(name, python_filename, "Presenter", "py", output_directory)
    generate_file_from(name, python_filename, "Model", "py", output_directory)


def generate_cpp_files(name: str, output_directory: str) -> None:
    print("Generating C++ files with an MVP pattern...")
    generate_file_from(name, cpp_filename, "View", "cpp", output_directory)
    generate_file_from(name, cpp_filename, "Presenter", "cpp", output_directory)
    generate_file_from(name, cpp_filename, "Model", "cpp", output_directory)
    generate_file_from(name, cpp_filename, "View", "h", output_directory)
    generate_file_from(name, cpp_filename, "Presenter", "h", output_directory)
    generate_file_from(name, cpp_filename, "Model", "h", output_directory)


def generate_files(name: str, language: str, output_directory: str) -> None:
    match language.lower():
        case "python":
            generate_python_files(name, output_directory)
        case "c++":
            generate_cpp_files(name, output_directory)
        case _:
            raise ValueError(f"An unsupported language '{language}' has been provided. Choose one: [Python, C++].")


if __name__ == "__main__":
    from argparse import ArgumentParser

    parser = ArgumentParser(description="Generates files which can be used as an initial Model-View-Presenter template.")
    parser.add_argument("-n", "--name", required=True, help="The base name to use for the files and classes.")
    parser.add_argument("-l", "--language", required=True, help="The language to generate template MVP files for [Python or C++].")
    parser.add_argument("-o", "--output-dir", required=True, help="The absolute path to output the generated files to.")
    args = parser.parse_args()

    generate_files(args.name, args.language, args.output_dir)
