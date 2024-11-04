#!/usr/bin/env python

# -*- coding: utf-8 -*-

import json
import math
import os
import xml.etree.cElementTree as ET


maxCountPerFile = 1500
reversed_json_file_name = "compile_commands"


def load_config_json(filename):
    if os.path.exists(filename):
        with open(filename, "r") as handle:
            config = json.load(handle)
    else:
        config = []

    commandline = [str(item) for item in config]

    return " ".join(commandline)


def split_json(all_json_objects):
    total_count = len(all_json_objects)
    sub_file_count = int(math.ceil(float(total_count) / float(maxCountPerFile)))

    sub_files = []

    for i in range(sub_file_count):
        start = i * maxCountPerFile
        end = min((i + 1) * maxCountPerFile, total_count - 1)
        sub_json_objects = all_json_objects[start:end]
        file_name = "compile_commands%02d.json" % (i + 1)
        sub_files.append(file_name)

        with open(file_name, "w") as outputHandler:
            outputHandler.write(json.dumps(sub_json_objects, indent=4))

    return sub_files


def lint_jsonfiles(oclint, jsonfiles, config):
    i = 0
    result_files = []
    for file_name in jsonfiles:
        print("linting ... %s" % file_name)
        input_file = rename(file_name, "compile_commands.json")
        out_file = "oclint%02d.xml" % i
        lint(oclint, out_file, config)
        result_files.append(out_file)
        i += 1
        os.remove(input_file)

    return result_files


def lint(oclint, out_file, config):
    lint_command = """%s -- --verbose \
    %s \
    --report-type pmd \
    -o %s""" % (
        oclint,
        config,
        out_file,
    )
    print(lint_command)
    os.system(lint_command)


def combine_outputs(output_files):
    print("combining output files")

    base_tree = None
    base_root = None

    for filename in output_files:
        if not os.path.exists(filename):
            continue

        tree = ET.ElementTree(file=filename)
        root = tree.getroot()

        if base_tree is None:
            base_tree = tree
            base_root = root
        else:
            for child in root:
                base_root.append(child)

    base_tree.write("oclint.xml", encoding="utf-8", xml_declaration=True)


def rename(file_path, new_name):
    paths = os.path.split(file_path)
    new_path = os.path.join(paths[0], new_name)
    os.rename(file_path, new_path)
    return new_path


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="wrapper around oclint-json-compilation-database")
    parser.add_argument("compile_commands", help="location of 'compile_commands.json')")
    parser.add_argument(
        "--oclint", default="oclint-json-compilation-database", help="location of 'oclint-json-compilation-database' if not in path"
    )
    parser.add_argument("--config", default="oclint_config.json", help="configuration file (default='oclint_config.json')")
    args = parser.parse_args()

    if not os.path.exists(args.compile_commands):
        parser.error("File '%s' does not exist" % args.compile_commands)
    if args.oclint != "oclint-json-compilation-database" and not os.path.exists(args.oclint):
        parser.error("File '%s' does not exist" % args.oclint)

    config = load_config_json(args.config)
    print(config)

    with open(args.compile_commands, "r") as r_handler:
        json_objects = json.loads(r_handler.read())

    if len(json_objects) <= maxCountPerFile:
        lint(args.oclint, "oclint.xml", config)
    else:
        json_file = rename(args.compile_commands, "input.json")
        try:
            json_files = split_json(json_objects)
            xml_files = lint_jsonfiles(args.oclint, json_files, config)
            combine_outputs(xml_files)
            for xml_file in xml_files:
                if os.path.exists(xml_file):
                    os.remove(xml_file)
        finally:
            rename(json_file, args.compile_commands)
