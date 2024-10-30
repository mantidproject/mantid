# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from os.path import abspath, dirname, join
from os import rename
from typing import Callable, Union
from itertools import product

TEMPLATE_DIRECTORY = join(dirname(abspath(__file__)), "templates")


def _python_filename(file_type: str, name: Union[str, None] = None) -> str:
    """Create a filename string with a Python filename convention."""
    return f"{name.lower()}_{file_type.lower()}" if name is not None else file_type.lower()


def _cpp_filename(file_type: str, name: Union[str, None] = None) -> str:
    """Create a filename string with a C++ filename convention."""
    return f"{name}{file_type}" if name is not None else file_type


def _generate_setup_file(name: str, filename: Callable, setup_filename: str, extension: str, output_directory: str) -> None:
    """Generates a file which is used to setup or launch the generated MVP widget."""
    template_filepath = join(TEMPLATE_DIRECTORY, f"{setup_filename}.{extension}.in")
    with open(template_filepath, mode="r") as file:
        content = file.read()

    content = content.replace("Model", f"{name}Model")
    content = content.replace("View", f"{name}View")
    content = content.replace("Presenter", f"{name}Presenter")
    content = content.replace("GUIWidget", f"{name}GUIWidget")
    # Only required for python launch file
    content = content.replace("from model", "from " + filename("Model", name))
    content = content.replace("from view", "from " + filename("View", name))
    content = content.replace("from presenter", "from " + filename("Presenter", name))

    output_filepath = join(output_directory, f"{setup_filename}.{extension}")
    with open(output_filepath, mode="w") as file:
        file.write(content)


def _generate_test_file(name: str, filename: Callable, file_type: str, extension: str, output_directory: str) -> None:
    test_filename = f"test_{file_type}" if extension == "py" else f"{file_type}Test"
    _generate_setup_file(name, filename, test_filename, extension, output_directory)
    output_name = f"test_{filename(file_type, name)}.py" if extension == "py" else f"{filename(test_filename, name)}.h"
    rename(join(output_directory, f"{test_filename}.{extension}"), join(output_directory, output_name))


def _generate_mvp_file(name: str, filename: Callable, file_type: str, extension: str, output_directory: str) -> None:
    """Generates a file using the corresponding template in the template directory."""
    template_filepath = (
        join(TEMPLATE_DIRECTORY, f"{filename(file_type)}.{extension}.in")
        if (extension != "ui")
        else join(TEMPLATE_DIRECTORY, f"{file_type}.{extension}.in")
    )
    with open(template_filepath, mode="r") as file:
        content = file.read()

    for mvp_type in ["Model", "View", "Presenter"]:
        content = content.replace(mvp_type, f"{name}{mvp_type}")

    # For adding the ui file
    content = (
        content.replace("GUIWidget", f"{name}GUIWidget") if extension == "h" else content.replace("GUIWidget", filename("GUIWidget", name))
    )

    output_filepath = join(output_directory, f"{filename(file_type, name)}.{extension}")
    with open(output_filepath, mode="w") as file:
        file.write(content)


def _generate_python_files(name: str, include_setup: bool, output_directory: str, add_tests: bool) -> None:
    """Generate MVP files for a Python use case."""
    print("Generating Python files with an MVP pattern...")
    for file_type in ["View", "Presenter", "Model"]:
        _generate_mvp_file(name, _python_filename, file_type, "py", output_directory)

    _generate_mvp_file(name, _python_filename, "GUIWidget", "ui", output_directory)
    if include_setup:
        _generate_setup_file(name, _python_filename, "launch", "py", output_directory)

    if add_tests:
        _generate_test_file(name, _python_filename, "presenter", "py", output_directory)
        _generate_test_file(name, _python_filename, "model", "py", output_directory)

    print(f"Output directory: {output_directory}")
    print("Done!")


def _generate_cpp_files(name: str, include_setup: bool, output_directory: str, add_tests: bool) -> None:
    """Generate MVP files for a C++ use case."""
    print("Generating C++ files with an MVP pattern...")
    for file_type in product(["View", "Presenter", "Model"], ["cpp", "h"]):
        _generate_mvp_file(name, _cpp_filename, file_type[0], file_type[1], output_directory)

    _generate_mvp_file(name, _cpp_filename, "GUIWidget", "ui", output_directory)
    if include_setup:
        _generate_setup_file(name, _cpp_filename, "main", "cpp", output_directory)
        _generate_setup_file(name, _cpp_filename, "CMakeLists", "txt", output_directory)

    if add_tests:
        _generate_test_file(name, _cpp_filename, "Presenter", "h", output_directory)
        _generate_test_file(name, _cpp_filename, "Model", "h", output_directory)

    print(f"Output directory: {output_directory}")
    print("Done!")


def _generate_files(name: str, language: str, include_setup: bool, output_directory: str, add_tests: bool) -> None:
    """Generate MVP files for a specific programming language."""
    match language.lower():
        case "python":
            _generate_python_files(name, include_setup, output_directory, add_tests)
        case "c++" | "cpp":
            _generate_cpp_files(name, include_setup, output_directory, add_tests)
        case _:
            raise ValueError(f"An unsupported language '{language}' has been provided. Choose one: [Python, C++].")


if __name__ == "__main__":
    from argparse import ArgumentParser, BooleanOptionalAction

    parser = ArgumentParser(description="Generates files which can be used as an initial Model-View-Presenter template.")
    parser.add_argument("-n", "--name", required=True, help="The base name to use for the files and classes.")
    parser.add_argument("-l", "--language", required=True, help="The language to generate template MVP files for [Python or C++].")
    parser.add_argument(
        "-s",
        "--include-setup",
        action=BooleanOptionalAction,
        help="Whether to include setup files such as a launch script (and CMakeLists.txt for C++).",
    )
    parser.add_argument("-o", "--output-dir", required=True, help="The absolute path to output the generated files to.")
    parser.add_argument(
        "-t", "--include-tests", action=BooleanOptionalAction, help="Whether to include basic test files for the presenters and models"
    )
    args = parser.parse_args()

    _generate_files(args.name[:1].upper() + args.name[1:], args.language, args.include_setup, args.output_dir, args.add_tests)
