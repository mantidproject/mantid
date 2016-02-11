#!usr/bin/python

# -*- coding: utf-8 -*-


__author__ = 'Ben'


import json
import sys
import math
import os
import xml.etree.cElementTree as ET


maxCountPerFile = 2000
reversed_json_file_name = 'compile_commands'

# see http://docs.oclint.org/en/dev/customizing/rules.html
CYCLOMATIC_COMPLEXITY = 10  # default 10
LONG_CLASS = 1000  # default 1000
LONG_LINE = 200  # default 100
LONG_METHOD = 100  # default 50
MINIMUM_CASES_IN_SWITCH = 3  # default 3
NPATH_COMPLEXITY = 200  # default 200
NCSS_METHOD = 30  # default 30
NESTED_BLOCK_DEPTH = 5  # default 5


def split_json(all_json_objects):
    total_count = len(all_json_objects)
    sub_file_count = int(math.ceil(float(total_count) / float(maxCountPerFile)))

    sub_files = []

    for i in range(sub_file_count):
        start = i*maxCountPerFile
        end = min((i+1)*maxCountPerFile, total_count - 1)
        sub_json_objects = all_json_objects[start:end]
        file_name = 'compile_commands%02d.json' %(i+1)
        sub_files.append(file_name)

        with open(file_name, 'w') as outputHandler:
            outputHandler.write(json.dumps(sub_json_objects, indent=4))

    return sub_files


def lint_jsonfiles(jsonfiles):

    i = 0
    result_files = []
    for file_name in jsonfiles:
        print 'linting ... %s' %file_name
        input_file = rename(file_name, 'compile_commands.json')
        out_file = 'oclint%02d.xml' %i
        lint(out_file)
        result_files.append(out_file)
        i += 1
        os.remove(input_file)

    return result_files


def lint(out_file):
        lint_command = '''oclint-json-compilation-database -- \
        --verbose \
        -rc CYCLOMATIC_COMPLEXITY=%d \
        -rc LONG_CLASS=%d \
        -rc LONG_LINE=%d \
        -rc LONG_METHOD=%d \
        -rc MINIMUM_CASES_IN_SWITCH=%d \
        -rc NPATH_COMPLEXITY=%d \
        -rc NCSS_METHOD=%d \
        -rc NESTED_BLOCK_DEPTH=%d \
        --report-type pmd \
        -o %s''' % (CYCLOMATIC_COMPLEXITY, LONG_CLASS, LONG_LINE, LONG_METHOD, MINIMUM_CASES_IN_SWITCH, NPATH_COMPLEXITY, NCSS_METHOD, NESTED_BLOCK_DEPTH, out_file)
        os.system(lint_command)


def combine_outputs(output_files):
    # first file
    base_tree = ET.ElementTree(file=output_files[0])
    base_root = base_tree.getroot()

    left_files = output_files[1:len(output_files)-1]
    for left_file in left_files:
        tree = ET.ElementTree(file=left_file)
        root = tree.getroot()

        for child in root:
            base_root.append(child)

    base_tree.write('oclint.xml', encoding='utf-8', xml_declaration=True)


def rename(file_path, new_name):
    paths = os.path.split(file_path)
    new_path = os.path.join(paths[0], new_name)
    os.rename(file_path, new_path)
    return new_path

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print "Missing argument!"
    else:

        with open(sys.argv[1], 'r') as r_handler:
            json_objects = json.loads(r_handler.read())
        if len(json_objects) <= maxCountPerFile:
            lint('oclint.xml')
        else:
            json_file = rename(sys.argv[1], 'input.json')
            json_files = split_json(json_objects)
            xml_files = lint_jsonfiles(json_files)
            combine_outputs(xml_files)
            for xml_file in xml_files:
                os.remove(xml_file)
            rename(json_file, 'compile_commands.json')