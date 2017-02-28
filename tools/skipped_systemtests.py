#pylint: disable=invalid-name
#!/usr/bin/env python
from __future__ import absolute_import, division, print_function
import datetime
import requests # python-requests

URL = "http://builds.mantidproject.org/job/master_systemtests"
PLATFORMS=['rhel7','osx', 'win7', 'ubuntu', 'ubuntu-16.04']


class TestCase(object):
    def __init__(self, kwargs):
        self.status = kwargs['status']
        if self.status == "REGRESSION":
            self.status = "FAILED"
        self.name = kwargs['name']

    def __str__(self):
        return self.name


def getLabel(url):
    label = url.split('label=')[-1]
    return label.split('/')[0:2]


def addTestResult(results, case, label):
    case = str(case)
    if case not in results:
        results[case] = [label]
    else:
        results[case].append(label)


def printResultCell(mark, length):
    if mark:
        left = int(length/2)
        right = length-left-1
        print("%sx%s" % (' '*left, ' '*right),end=' ')
    else:
        print(' '*(length),end=' ')
    print('|',end=' ')


def generateTable(interesting, labels, heading):
    if len(interesting) <= 0:
        return

    print()
    print(heading)
    print("-"*len(heading))
    print()

    # get the maximum test name length
    tests = sorted(interesting.keys())
    maxlength = 0
    for test in tests:
        if len(test) > maxlength:
            maxlength = len(test)
    maxlength += 2

    # print out the heading line with separator
    header = '| Test '+' '*(maxlength-5)+'|'
    for label in labels:
        header += ' '+label+' |'
    print(header)
    header = header.split('|')
    header = ['-'*len(item) for item in header]
    print('|'.join(header))

    # sort the tests so the least tested is first
    ordered_tests = {}
    for test in tests:
        numSkip = len(interesting[test])
        if numSkip not in ordered_tests:
            ordered_tests[numSkip] = []
        ordered_tests[numSkip].append(test)
    tests = []
    for i in range(len(labels), 0, -1):
        if i in ordered_tests:
            tests.extend(ordered_tests[i])

    # print the table
    for test in tests:
        print('|', test, ' '*(maxlength-len(test)-2), '|',end=' ')
        for label in labels:
            printResultCell(label in interesting[test], len(label))
        print()

skipped = {}
failed = {}
totalCount = 0

for platform in PLATFORMS:
    url = URL+"-"+platform+"/lastCompletedBuild/testReport/api/json"
    params = {}

    request = requests.get(url, params=params)

    if request.status_code != 200:
        raise RuntimeError("'%s' returned %d" % (url, request.getcode()))
    json = request.json()

    label=platform
    totalCount += int(json['failCount'])+int(json['passCount'])+int(json['skipCount'])

    for case in json["suites"][0]["cases"]:
        case = TestCase(case)
        if case.status != "PASSED":
            if case.status == "SKIPPED":
                addTestResult(skipped, case, label)
            elif case.status == "FAILED":
                addTestResult(failed, case, label)

# get the unique labels
labels = []
for key in skipped.keys():
    for item in skipped[key]:
        if item not in labels:
            labels.append(item)
for key in failed.keys():
    for item in failed[key]:
        if item not in labels:
            labels.append(item)
labels.sort()

# print out the yaml header so it gets parsed by jekyll
print('---')
print('layout: default')
print('date:', datetime.datetime.now().strftime("%Y-%m-%d"))
print('author: Peter Peterson')
print('title: Currently Skipped System Tests')
print('---')

print("Summary")
print("=======")
print
print("* Job    : [%s](%s)" % ('Master Pipeline', 'http://builds.mantidproject.org/view/Master%20Pipeline/'),end='')
print(datetime.datetime.now().strftime("%Y-%m-%d"))
print("* Labels :", ', '.join(labels))
print("* Failed :", json['failCount'],end='')
if len(failed.keys()) < 2:
    print()
else:
    print("(%d unique)" % len(failed.keys()))
print("* Skipped:", json['skipCount'],end='')
if len(skipped.keys()) < 2:
    print()
else:
    print("(%d unique)" % len(skipped.keys()))
print("* Total  :", totalCount,)
print("(= %d * %d)" % (totalCount/len(labels), len(labels)))

print()

print("Details")
print("=======")

generateTable(failed, labels, "Failed")
generateTable(skipped, labels, "Skipped")
