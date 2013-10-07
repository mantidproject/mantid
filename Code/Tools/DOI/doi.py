"""A script for generating DataCite DOI's for Mantid releases, to be called by
a Jenkins job during the release process.  When given a major, minor and patch
release number along with username and password credentials, it will build a
DOI of the form "10.5286/Software/Mantid[major].[minor].[patch]", and post it
to the DataCite DOI API.

A special one-time "main" landing page DOI will be created using:

python doi.py --major=3 --minor=0 --patch=0 --username=[] --password=[] --main

Then at every release, the script will run again without the "--main" flag to
generate a DOI pointing to the release notes for that particular version.

Using the "--test" flag will run the script and post DOI's to the DataCite test
server at https://test.datacite.org/mds/doi/10.5286/Software/.

Using the "--debug" flag should print out some (hopefully) useful extra info
about what is going on under the hood.

Using the "--delete" flag will "make inactive" the DOI metadata with the given
details.  Note that this does NOT delete the DOI itself, just the metadata that
accompanies the DOI.

NOTES:

- This script is tailored for use at RAL, where the internet must be accessed
  via the "wwwcache.rl.ac.uk" proxy.  Making it work off site will require some
  tweaking.

- A requirement for the script to run is for cURL to be installed and its
  executable to be on the PATH.

- The "www.mantidproject.org" domain had to be registered with DataCite (on
  both the test server and the main server) before a valid DOI could be
  created.  This was done through the British Library, via Tom Griffin.

USEFUL LINKS:

- The DataCite DOI API documentation can be found at:
  https://mds.datacite.org/static/apidoc

- Example Python code for submitting DOI's and metadata:
  https://github.com/datacite/mds/blob/master/client/python/put_doi.py
  https://github.com/datacite/mds/blob/master/client/python/put_metadata.py

- HTTP status codes:
  http://docs.python.org/2/library/httplib.html#httplib.HTTPS_PORT

PROBLEMS LEFT TO SOLVE:

- The exact wording of the "description" metadata field.
"""

import argparse

import xml.etree.ElementTree as ET

import subprocess
import re
from datetime import date

import authors

def build_xml_form(doi, relationships, creator_name_list, version_str):
    '''Builds the xml form containing the metadata for the DOI.  Where helpful,
    comments showing the definition / allowed values of the data fields have
    been taken from section 2.3 of:
    http://schema.datacite.org/meta/kernel-3/doc/DataCite-MetadataKernel_v3.0.pdf

    The decision has been made to not use the optional "contributors" field,
    since creators works just as well and is mandatory anyway.
    '''
    # The root resource node must contain the various schema information.
    root = ET.Element('resource')
    root.set('xmlns',              'http://datacite.org/schema/kernel-3')
    root.set('xmlns:xsi',          'http://www.w3.org/2001/XMLSchema-instance')
    root.set('xsi:schemaLocation', 'http://datacite.org/schema/kernel-3 ht' + \
                                   'tp://schema.datacite.org/meta/kernel-3' + \
                                   '/metadata.xsd')

    # "The identifier is a unique string that identifies a resource." In our
    # case, the actual DOI. "Format should be '10.1234/foo'."
    identifier = ET.SubElement(root, 'identifier')
    identifier.text = doi
    identifier.set('identifierType', 'DOI')

    # Creators are defined as "the main researchers involved in producing the 
    # data, or the authors of the publication, in priority order".  Allowed
    # values are "a corporate/institutional or personal name".
    #
    # Use all authors up to and including the version tag if creating the
    # "main" DOI, else only use authors who contributed to that version.
    creators = ET.SubElement(root, 'creators')
    for creator_name in creator_name_list:
        creator = ET.SubElement(creators, 'creator')
        ET.SubElement(creator, 'creatorName').text = creator_name

    # Titles are defined as a "name or title by which a resource is known".
    title_text_list = 'Mantid: A high performance framework for the ' + \
                      'reduction and analysis of muon spin resonance and ' + \
                      'neutron scattering data.',
    titles = ET.SubElement(root, 'titles')
    for title_text in title_text_list:
        ET.SubElement(titles, 'title').text = title_text

    # "The name of the entity that holds, archives, publishes, prints,
    # distributes, releases, issues, or produces the resource. This property
    # will be used to formulate the citation, so consider the prominence of
    # the role."
    ET.SubElement(root, 'publisher').text = 'Mantid Project'

    # "The year when the data was or will be made publicly available."
    ET.SubElement(root, 'publicationYear').text = str(date.today().year)

    # "Subject, keyword, classification code, or key phrase describing the
    # resource."
    subject_text_list = [
        'Neutron Scattering',
        'Muon Spin Resonance',
        'Data Analysis'
    ]
    subjects = ET.SubElement(root, 'subjects')
    for subject_text in subject_text_list:
        ET.SubElement(subjects, 'subject').text = subject_text

    # "The primary language of the resource."
    ET.SubElement(root, 'language').text = 'en'

    # "A description of the resource. The format is open, but the
    # preferred format is a single term of some detail so that a pair can be
    # formed with the sub-property."  Just using the general type "software"
    # seems good enough for our purposes.
    resource_type = ET.SubElement(root, 'resourceType')
    resource_type.text = ''
    resource_type.set('resourceTypeGeneral', 'Software')

    # "The version number of the resource." Suggested practice is to "register
    # a new identifier for a major version change."  We'll be ignoring this
    # as we're having a new DOI for every major/minor/patch release.
    ET.SubElement(root, 'version').text = version_str

    # "Identifiers of related resources. These must be globally unique
    # identifiers."
    if relationships:
        related_identifiers = ET.SubElement(root, 'relatedIdentifiers')
    for doi, relation_type in relationships.items():
        related_identifier = ET.SubElement(
            related_identifiers, 'relatedIdentifier'
        )
        related_identifier.text = doi
        related_identifier.set('relatedIdentifierType', 'DOI')
        related_identifier.set('relationType', relation_type)

    # "Provide a rights management statement for the resource or reference a
    # service providing such information. Include embargo information if
    # applicable. Use the complete title of a license and include version
    # information if applicable."
    rights_list = ET.SubElement(root, 'rightsList')
    rights = ET.SubElement(rights_list, 'rights')
    rights.text = 'GNU General Public Release (Version 3)'
    rights.set('rightsURI', 'http://www.gnu.org/licenses/gpl.html')

    # "All additional information that does not fit in any of the other
    # categories. May be used for technical information."
    descriptions = ET.SubElement(root, 'descriptions')
    description = ET.SubElement(descriptions, 'description')
    description.text = 'A high performance framework for the reduction and' + \
        ' analysis of muon spin resonance and neutron scattering data.'
    description.set('descriptionType', 'Abstract')

    return ET.tostring(root, encoding='utf-8')

def _http_request(body, method, url, options):
    '''Issue an HTTP request with the given options.

    We are forced to use a command line tool for this rather than use the
    in-built Python libraries since httplib, urllib and urllib2 all seem to
    have problems using HTTPS through the proxy at RAL.  HTTP works fine,
    but the DOI API is encrypted so that is not an option.

    We prefer cURL to wget since it exists on many Linux machines and even
    comes bundled with Git Bash for Windows!  Some good info on scripting
    with cURL can be found at:

    http://curl.haxx.se/docs/httpscripting.html'''

    args = [
        'curl',
        '--user',    options.username + ':' + options.password,
        '--proxy',   'wwwcache.rl.ac.uk:8080',
        '--header',  'Content-Type:text/plain;charset=UTF-8',
        # The bodies of HTTP messages must be encoded:
        '--data',    body.encode('utf-8'),
        '--request', method,
    ]

    # Set how loud cURL should be while running.
    if options.debug: args.append('--verbose')
    else: args.append('--silent')

    args.append(url)

    proc = subprocess.Popen(args,stdout=subprocess.PIPE)
    result = proc.stdout.readlines()

    print result
    return result

def delete_doi(base, doi, options):
    print "\n\nAttempting to delete the following DOI:"
    print 'DOI = ' + doi
    _http_request(
        body    = '',
        method  = 'DELETE',
        url     = base + "doi/" + doi,
        options = options
    )

def create_or_update_metadata(xml_form, base, doi, options):
    print "\nAttempting to create / update metadata:"
    _http_request(
        body    = xml_form,
        method  = "PUT",
        url     = base + "metadata/" + doi,
        options = options
    )

def create_or_update_doi(base, doi, destination, options):
    print "\n\nAttempting to create / update the following DOI:"
    print 'DOI = ' + doi
    print 'URL = ' + destination
    _http_request(
        body    = 'doi=' + doi + '\n' + 'url=' + destination,
        method  = "PUT",
        url     = base + "doi/" + doi,
        options = options
    )

def check_if_doi_exists(base, doi, destination, options):
    print "\nChecking if \"" + base + "doi/" + doi + "\" DOI already exists."
    result = _http_request(
        body    = '',
        method  = 'GET',
        url     = base + "doi/" + doi,
        options = options
    )

    if result[0] == 'DOI not found':
        print "DOI not found."
        return False
    elif result[0] == destination:
        print "DOI found."
        return True
    else:
        raise Exception(
            "Unexpected result back from server: \"" + result[0] + "\"")

def run(options):
    '''Creating a usable DOI is (for our purposes at least) a two step
    process: metadata has to be constructed and then sent to the server, and
    then the DOI itself has to be sent once the metadata is in place.

    If pre-existing DOI's or metadata are submitted then they will overwrite
    what was there previously.

    We also have to amend the metadata for the previous DOI (if one exists),
    so that we can set up a IsPreviousVersionOf/IsNewVersionOf relationship
    between the two DOIs.
    '''
    # Get the git tag and "version string" of this version as well as the
    # version before it if this is an incremental release.
    version = options.major, options.minor, options.patch
    version_str = authors.get_version_string(options.major,
                                             options.minor,
                                             options.patch)
    tag = authors.find_tag(*version)
    if not options.main:
        prev_tag = authors.get_previous_tag(tag)
        prev_version = authors.get_version_from_git_tag(prev_tag)
        prev_version_str = authors.get_version_string(*prev_version)

    main_doi = '10.5286/Software/Mantid'

    if options.main:
        doi = main_doi
    else: # Incremental release DOI.
        prev_doi = '10.5286/Software/Mantid' + prev_version_str
        doi = '10.5286/Software/Mantid' + version_str

    if options.main:
        destination = 'http://www.mantidproject.org'
    else:
        destination = 'http://www.mantidproject.org/Release_Notes_' + \
                      version_str
        prev_destination = 'http://www.mantidproject.org/Release_Notes_' +\
                               prev_version_str

    # Use the test server if running in test mode.
    if options.test:
        server_url_base = 'https://test.datacite.org/mds/'
    else:
        server_url_base = 'https://mds.datacite.org/'

    if options.delete:
        delete_doi(server_url_base, doi, options)
        quit()

    if options.main:
        creator_name_list = authors.authors_up_to_git_tag(tag)
        xml_form = build_xml_form(doi, {}, creator_name_list, version_str)

        create_or_update_metadata(xml_form, server_url_base, doi, options)
        create_or_update_doi(server_url_base, doi, destination, options)

    else:
        has_previous_version = check_if_doi_exists(
            server_url_base,
            prev_doi,
            prev_destination,
            options
        )

        relationships = { main_doi : 'IsPartOf' }
        if has_previous_version:
            relationships[prev_doi] = 'IsNewVersionOf'
        
        creator_name_list = authors.authors_under_git_tag(tag)
        xml_form = build_xml_form(doi, relationships, creator_name_list, version_str)

        create_or_update_metadata(xml_form, server_url_base, doi, options)
        create_or_update_doi(server_url_base, doi, destination, options)

        if has_previous_version:
            prev_relationships = { 
                main_doi : 'IsPartOf',
                doi      : 'IsPreviousVersionOf'
            }
        
            prev_creator_name_list = authors.authors_under_git_tag(prev_tag)
            prev_xml_form = build_xml_form(prev_doi, prev_relationships, prev_creator_name_list, prev_version_str)

            create_or_update_metadata(prev_xml_form, server_url_base, prev_doi, options)

        if not options.test:
            print "\n\nIf successfully created, the DOI can be resolved at:"
            print 'http://dx.doi.org/' + doi

        print "\n\nIf successfully created, the metadata form can be inspected at:"
        if options.test:
            print 'https://test.datacite.org/mds/metadata/' + doi
        else:
            print 'https://mds.datacite.org/metadata/' + doi

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
        '--patch',
        type=int,
        required=True,
        help='The patch version number of this Mantid release.  Note: this' + \
             ' is NOT the SHA1 or the commit number from Git.  The patch ' + \
             'number for v3.0 (or v3.0.0) is 0, and the patch number for ' + \
             'v3.0.1 is 1.'
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
    parser.add_argument(
        '--main',
        action='store_true',
        help='Create the "main" DOI for Mantid.  Once it is created, this ' + \
             'will only have to run again if it needs to be updated.'
    )
    parser.add_argument(
        '--delete',
        action='store_true',
        help='Delete ("make inactive") the DOI metadata with the given ' + \
             'details.  Note that this does NOT delete the DOI.'
    )

    run(parser.parse_args())