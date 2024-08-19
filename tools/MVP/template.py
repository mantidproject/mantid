# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from os.path import abspath, dirname, join
from typing import Callable, Union

TEMPLATE_DIRECTORY = join(dirname(abspath(__file__)), "templates")


def _python_filename(file_type: str, name: Union[str, None] = None) -> str:
    """Create a filename string with a Python filename convention."""
    return f"{name.lower()}_{file_type.lower()}" if name is not None else file_type.lower()


def _cpp_filename(file_type: str, name: Union[str, None] = None) -> str:
    """Create a filename string with a C++ filename convention."""
    return f"{name}{file_type}" if name is not None else file_type


def _generate_launch_file(name: str, filename: Callable, extension: str, output_directory: str) -> None:
    """Generates a file which can be used to launch the generated MVP widget."""
    template_filepath = join(TEMPLATE_DIRECTORY, f"launch.{extension}.in")
    with open(template_filepath, mode="r") as file:
        content = file.read()

    content = content.replace("Model", f"{name}Model")
    content = content.replace("View", f"{name}View")
    content = content.replace("Presenter", f"{name}Presenter")
    content = content.replace("model", filename("Model", name))
    content = content.replace("view", filename("View", name))
    content = content.replace("presenter", filename("Presenter", name))

    output_filepath = join(output_directory, f"launch.{extension}")
    with open(output_filepath, mode="w") as file:
        file.write(content)


def _generate_mvp_file(name: str, filename: Callable, file_type: str, extension: str, output_directory: str) -> None:
    """Generates a file using the corresponding template in the template directory."""
    template_filepath = join(TEMPLATE_DIRECTORY, f"{filename(file_type)}.{extension}.in")
    with open(template_filepath, mode="r") as file:
        content = file.read()

    content = content.replace(file_type, f"{name}{file_type}")

    output_filepath = join(output_directory, f"{filename(file_type, name)}.{extension}")
    with open(output_filepath, mode="w") as file:
        file.write(content)


def _generate_python_files(name: str, output_directory: str) -> None:
    """Generate MVP files for a Python use case."""
    print("Generating Python files with an MVP pattern...")

    _generate_mvp_file(name, _python_filename, "View", "py", output_directory)
    _generate_mvp_file(name, _python_filename, "Presenter", "py", output_directory)
    _generate_mvp_file(name, _python_filename, "Model", "py", output_directory)
    _generate_launch_file(name, _python_filename, "py", output_directory)

    print(f"Output directory: {output_directory}")
    print("Done!")


def _generate_cpp_files(name: str, output_directory: str) -> None:
    """Generate MVP files for a C++ use case."""
    print("Generating C++ files with an MVP pattern...")

    _generate_mvp_file(name, _cpp_filename, "View", "cpp", output_directory)
    _generate_mvp_file(name, _cpp_filename, "Presenter", "cpp", output_directory)
    _generate_mvp_file(name, _cpp_filename, "Model", "cpp", output_directory)
    _generate_mvp_file(name, _cpp_filename, "View", "h", output_directory)
    _generate_mvp_file(name, _cpp_filename, "Presenter", "h", output_directory)
    _generate_mvp_file(name, _cpp_filename, "Model", "h", output_directory)

    print(f"Output directory: {output_directory}")
    print("Done!")


def _generate_files(name: str, language: str, output_directory: str) -> None:
    """Generate MVP files for a specific programming language."""
    match language.lower():
        case "python":
            _generate_python_files(name, output_directory)
        case "c++":
            _generate_cpp_files(name, output_directory)
        case _:
            raise ValueError(f"An unsupported language '{language}' has been provided. Choose one: [Python, C++].")


if __name__ == "__main__":
    from argparse import ArgumentParser

    parser = ArgumentParser(description="Generates files which can be used as an initial Model-View-Presenter template.")
    parser.add_argument("-n", "--name", required=True, help="The base name to use for the files and classes.")
    parser.add_argument("-l", "--language", required=True, help="The language to generate template MVP files for [Python or C++].")
    parser.add_argument("-o", "--output-dir", required=True, help="The absolute path to output the generated files to.")
    args = parser.parse_args()

    _generate_files(args.name.capitalize(), args.language, args.output_dir)
