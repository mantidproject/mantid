"""A script for generating DataCite DOI's for Mantid releases, to be called by
a Jenkins job during the release process.  When given a major and minor release
number along with username and password credentials, it will build a DOI of the
form "10.5286/Software/Mantid[major].[minor]", and post it to the DataCite DOI
API.

NOTES:

- This script is tailored for use at RAL, where the internet must be accessed
  via the "wwwcache.rl.ac.uk" proxy.  Making it work off site will require some
  tweaking.

- A requirement for the script to run is for cURL to be installed and its
  executable to be on the PATH.

USEFUL LINKS:

- The DataCite DOI API documentation can be found at:
  https://mds.datacite.org/static/apidoc

- Example Python code for submitting DOI's and metadata:
  https://github.com/datacite/mds/blob/master/client/python/put_doi.py
  https://github.com/datacite/mds/blob/master/client/python/put_metadata.py

- HTTP status codes:
  http://docs.python.org/2/library/httplib.html#httplib.HTTPS_PORT

PROBLEMS LEFT TO SOLVE:

- How best to scrape author information from git.  This seems best:
  git log --pretty=short v2.5...v2.6 --format="%aN" --reverse
  Using sort means that we could ensure only unique names, but then we have to
  assume presence of sort on the command line.
  git log --pretty=short v2.5...v2.6 --format="%aN" --reverse | sort -u

- Once we have the author list, then how do we deal with *almost* duplicates?
  I.e. a user might have logged into git as "firstname-lastname" on one
  occasion, and then "Firstname Lastname" on another occasion.  A short-term
  mapping of names to translate, plus a longer-term enforcement of correct
  names seems appropriate.

- The second half of the DOI submission process (sending the doi/url pair)
  returns an error from the server: "domain of URL is not allowed".  It seems
  likely that the www.mantidproject.org domain needs to be registered with
  DataCite before it is allowed.  Waiting for more input from Tom about this.
  """

import argparse
import os, sys
import re

import xml.etree.ElementTree as ET

import subprocess
from datetime import date

def build_xml_form(major, minor, doi):
    '''Builds the xml form containing the metadata for the DOI.  Where helpful,
    comments showing the definition / allowed values of the data fields have
    been taken from section 2.3 of:
    http://schema.datacite.org/meta/kernel-3/doc/DataCite-MetadataKernel_v3.0.pdf
    '''
    # The root resource node must contain the various schema information.
    root = ET.Element('resource')
    root.set('xmlns',              'http://datacite.org/schema/kernel-3')
    root.set('xmlns:xsi',          'http://www.w3.org/2001/XMLSchema-instance')
    root.set('xsi:schemaLocation', 'http://datacite.org/schema/kernel-3 http://schema.datacite.org/meta/kernel-3/metadata.xsd')

    # "The identifier is a unique string that identifies a resource." In our
    # case, the actual DOI. "Format should be '10.1234/foo'."
    identifier = ET.SubElement(root, 'identifier')
    identifier.text = doi
    identifier.set('identifierType', 'DOI')

    # Creators are defined as "the main researchers involved in producing the 
    # data, or the authors of the publication, in priority order".  Allowed
    # values are "a corporate/institutional or personal name".
    creator_name_list = 'Creator A', 'Creator B'
    creators = ET.SubElement(root, 'creators')
    for creator_name in creator_name_list:
        creator = ET.SubElement(creators, 'creator')
        ET.SubElement(creator, 'creatorName').text = creator_name

    # Titles are defined as a "name or title by which a resource is known".
    title_text_list = 'Mantid, A high performance framework for reduction ' + \
                      'and analysis of neutron scattering data.',
    titles = ET.SubElement(root, 'titles')
    for title_text in title_text_list:
        ET.SubElement(titles, 'title').text = title_text

    # "The name of the entity that holds, archives, publishes prints,
    # distributes, releases, issues, or produces the resource. This property
    # will be used to formulate the citation, so consider the prominence of
    # the role."
    ET.SubElement(root, 'publisher').text = 'Mantid Project'

    # "The year when the data was or will be made publicly available."
    ET.SubElement(root, 'publicationYear').text = str(date.today().year)

    # "Subject, keyword, classification code, or key phrase describing the
    # resource."
    subject_text_list = 'Neutron Scattering', 'Muon Scattering', 'Data Analysis'
    subjects = ET.SubElement(root, 'subjects')
    for subject_text in subject_text_list:
        ET.SubElement(subjects, 'subject').text = subject_text

    # "The primary language of the resource."
    ET.SubElement(root, 'language').text = 'en'

    # "The version number of the resource." Suggested practice is to "register
    # a new identifier for a major version change."  We'll be ignoring this
    # should we stick to having a new DOI for every major/minor release, too.
    ET.SubElement(root, 'version').text = '%d.%d' % (major, minor)

    # "All additional information that does not fit in any of the other
    # categories. May be used for technical information."
    descriptions = ET.SubElement(root, 'descriptions')
    description = ET.SubElement(descriptions, 'description')
    description.text = 'A description of Mantid.'
    description.set('descriptionType', 'Abstract')

    return ET.tostring(root, encoding='utf-8')

def _http_request(body, method, url, options):
    '''Issue an HTTP request with the given options.

    We are forced to use a command line tool for this rather than use the
    in-built Python libraries since httplib, urllib and urllib2 all seem to
    have problems using HTTPS through the proxy at RAL.  HTTP works fine,
    but the DOI API is encrypted.

    We prefer cURL to wget since it exists on many Linux machines and even
    comes bundled with Git Bash for Windows!  Some good info on scripting
    with cURL can be found at:

    http://curl.haxx.se/docs/httpscripting.html'''

    args = [
        'curl',
        '--user',    options.username + ':' + options.password,
        '--proxy',   'wwwcache.rl.ac.uk:8080',
        '--header',  'Content-Type:text/plain;charset=UTF-8',
        # The bodies of an HTTP messages must be encoded:
        '--data',    body.encode('utf-8'),
        '--request', method,
    ]

    # Set how loud cURL should be while running.
    if options.debug: args.append('--verbose')
    else: args.append('--silent')

    args.append(url)

    result = subprocess.check_call(args)

    if options.debug: print result

def create_or_update(options):
    '''Creating a usable DOI is (for our purposes at least) a two step
    process: metadata has to be constructed and then sent to the server, and
    then the DOI itself has to be sent once the metadata is in place.

    If pre-existing DOI's or metadata are submitted then they will overwrite
    what was there previously.
    '''

    doi = '10.5286/Software/Mantid%d.%d' % (options.major, options.minor)

    if options.test:
        server_url_base = 'https://test.datacite.org/mds/'
    else:
        server_url_base = ''

    print "\nAttempting to create / update metadata:"
    _http_request(
        body    = build_xml_form(options.major, options.minor, doi),
        method  = "PUT",
        url     = server_url_base + "metadata/" + doi,
        options = options
    )

    destination = 'http://www.mantidproject.org/Release_Notes_' + \
        str(options.major) + '.' + str(options.minor)
    print "\n\nAttempting to create / update the following DOI:"
    print 'DOI = ' + doi
    print 'URL = ' + destination
    _http_request(
        body    = 'doi=' + doi + '\n' + 'url=' + destination,
        method  = "PUT",
        url     = server_url_base + "doi/" + doi,
        options = options
    )

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Script to generate the DOI needed for a Mantid release."
    )

    # REQUIRED
    parser.add_argument(
        '--major',
        type=int,
        required=True,
        help='The major version number of this Mantid release.'
    )
    parser.add_argument(
        '--minor',
        type=int,
        required=True,
        help='The minor version number of this Mantid release.'
    )
    parser.add_argument(
        '--password',
        type=str,
        required=True,
        help='Password. This should be hidden in the Jenkins\' job logs.'
    )
    parser.add_argument(
        '--username',
        type=str,
        required=True,
        help='Username to access DOI API.'
    )

    # OPTIONAL
    parser.add_argument(
        '--test',
        action='store_true',
        help='Send submissions to the test server to trial run the script.'
    )
    parser.add_argument(
        '--debug',
        action='store_true',
        help='Turn debug mode on.  Basically, makes cURL more talkative.'
    )

    create_or_update(parser.parse_args())