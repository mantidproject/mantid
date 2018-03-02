#!/usr/bin/env python
from __future__ import (absolute_import, division, print_function, unicode_literals)
import os
import requests

doc='''.. _v{version}:

==========================
Mantid {version} Release Notes
==========================

.. contents:: Table of Contents
   :local:

This is a patch release that corrects some significant issues since :ref:`version {version_previous} <v{version_previous}>`.

The main changes are:

**ADD SUMMARY HERE**

Citation
--------

Please cite any usage of Mantid as follows:

- *Mantid {version}: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*.
  `doi: 10.5286/Software/Mantid{version} <http://dx.doi.org/10.5286/Software/Mantid{version}>`_

- Arnold, O. et al. *Mantid-Data Analysis and Visualization Package for Neutron Scattering and mu-SR Experiments.* Nuclear Instruments
  and Methods in Physics Research Section A: Accelerators, Spectrometers, Detectors and Associated Equipment 764 (2014): 156-166
  `doi: 10.1016/j.nima.2014.07.029 <https://doi.org/10.1016/j.nima.2014.07.029>`_
  (`download bibtex <https://raw.githubusercontent.com/mantidproject/mantid/master/docs/source/mantid.bib>`_)

Changes in this version
-----------------------

{changes}

Summary of impact
-----------------
{impact}

.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v{version}
'''


def getOauth(oauth = None):
    oauthfile = os.path.expanduser("~/.ssh/github_oauth")
    if oauth is None and os.path.exists(oauthfile):
        print("Found oauth token '%s'" % oauthfile)
        with open(oauthfile, 'r') as handle:
            oauth='\n'.join(handle.readlines())
            handle.close()
            oauth=oauth.strip()
    return oauth


def getInfo(number, oauth):
    urls = ['{keyword}/{number:d}'.format(keyword=item, number=number)
            for item in ['pulls', 'issues']]
    endpoint = 'https://api.github.com/repos/mantidproject/mantid/'
    urls = [endpoint + url for url in urls]

    req_params={}
    if oauth is not None:
        req_params['access_token']=oauth

    for url in urls:
        req = requests.get(url, params=req_params)
        json = req.json()
        if req.status_code == 403:
            print('For', number, 'status:', req.status_code, 'skipping')
            print(json['message'])
            continue

        json = req.json()
        if json.get('message', None) == 'Not Found':
            print('Failed to find information on', number, 'try as an issue')
            continue

        return dict(
            number = int(json['number']),
            url = json['html_url'],
            title = json['title'].strip()
        )

    return None


def formatChanges(pullrequests):
    changes = ['* `{number:d} <{url}>`_ {title} '.format(**pr) for pr in pullrequests]
    changes = '\n'.join(changes)
    return changes


def formatImpact(pullrequests):
    length_number = 5 # length of the word 'Issue'
    length_title = 6 # length of the word 'Impact'

    for pullrequest in pullrequests:
        temp = len(str(pullrequest['number']))
        if temp > length_number:
            length_number = temp

        temp = len(pullrequest['title'])
        if temp > length_title:
            length_title = temp
    impact_format = '| {number:%dd} | {title:%d} |          | **unknown**  |' % (length_number, length_title)

    # sorry for this looking wierd
    impact_join = [length_number + 2, length_title+2, 10, 14]
    title_format = '|{:%d}|{:%d}|{:%d}|{:%d}|' % tuple(impact_join)
    title_join = '+' + '+'.join(['='*length for length in impact_join]) + '+'

    impact_title = [title_format.format(' Issue', ' Impact', ' Solution', ' Side Effect'),
                    title_format.format('', '', '', ' Probability'),
                    title_join]

    impact_join = ['-'*length for length in impact_join]
    impact_join = '\n+' + '+'.join(impact_join) + '+'

    impact = [impact_format.format(**pr) + impact_join
              for pr in pullrequests]

    for i, line in enumerate(impact_title):
        impact.insert(i, line)
    impact.insert(0, impact_join)

    return '\n'.join(impact)


def getPreviousRelease(release, previous):
    if previous is not None:
        return previous
    splitted = release.split('.')
    splitted[-1] = (str(int(splitted[-1])-1))
    return '.'.join(splitted)


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description="Generate patch release page")
    parser.add_argument('--release', required=True)
    parser.add_argument('--previous', help='specify a particular previous release')
    parser.add_argument('-p', '--pulls', nargs='+', type=int,
                        help='specify a list of pull requests and/or issues')
    parser.add_argument('--oauth',
                        help='github oauth token - automatically checks ~/.ssh/github_oauth')
    args=parser.parse_args()

    oauth = getOauth(args.oauth)

    pullrequests = [getInfo(pullrequest, oauth) for pullrequest in args.pulls]
    pullrequests = [pullrequest for pullrequest in pullrequests
                    if pullrequest is not None]

    changes = formatChanges(pullrequests)
    impact = formatImpact(pullrequests)

    config = dict(
        version=args.release,
        version_previous=getPreviousRelease(args.release, args.previous),
        changes=changes,
        impact=impact)

    filename = 'index.rst'
    print('Writing output to', filename)
    with open(filename, 'w') as handle:
        handle.write(doc.format(**config))
