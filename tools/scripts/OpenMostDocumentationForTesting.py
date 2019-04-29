# This was written for Python 2.7
# You are likely doing unscripted testing for MantidPlot/Workbench when using this script so basically follow this
# advice as well as using this script and it's results. (Confirm these all work as well)
# - Algorithm dialog snapshots should appear on algorithm pages in offline help
# - Math formulae should appear on algorithm pages in offline help
# - workflow diagrams should appear on algorithm pages in offline help
#
# Author: Samuel Jones - ISIS

# Need to install BeautifulSoup:
# pip install beautifulsoup
from BeautifulSoup import BeautifulSoup
import urllib2
import re
import webbrowser
import time
import argparse


def crawl_url_for_html_addons(url):
    parent_url = url
    parent_url = re.sub('index.html$', '', parent_url)
    html_page = urllib2.urlopen(url)
    soup = BeautifulSoup(html_page)
    urls = []
    for link in soup.findAll('a', attrs={'href': re.compile(".html")}):
        html_ref = link.get('href')
        urls.append(parent_url + html_ref)
    return urls


def open_urls(list_of_urls, delay=1):
    """

    :param list_of_urls:
    :param delay: in seconds
    :return:
    """
    for url in list_of_urls:
        time.sleep(delay)
        webbrowser.open(url)


parser = argparse.ArgumentParser()
parser.add_argument(
        '-d', '--open-tab-delay', type=int, help="Delay between each new page tab in seconds.", default=1)
args = parser.parse_args()

all_urls = []

print("Crawling for Algorithm URLs...")
algorithm_urls = crawl_url_for_html_addons("http://docs.mantidproject.org/nightly/algorithms/index.html")
all_urls.extend(algorithm_urls)

print("Crawling for Concept URLs...")
concept_urls = crawl_url_for_html_addons("http://docs.mantidproject.org/nightly/concepts/index.html")
all_urls.extend(concept_urls)

print("Crawling for Interface URLs...")
interface_urls = crawl_url_for_html_addons("http://docs.mantidproject.org/nightly/interfaces/index.html")
all_urls.extend(interface_urls)

print("Crawling for Technique URLs...")
technique_urls = crawl_url_for_html_addons("http://docs.mantidproject.org/nightly/techniques/index.html")
all_urls.extend(technique_urls)

print("Crawling python api...")
mantid_kernel_urls = crawl_url_for_html_addons("http://docs.mantidproject.org/nightly/api/python/mantid/kernel/"
                                               "index.html")
mantid_geometry_urls = crawl_url_for_html_addons("http://docs.mantidproject.org/nightly/api/python/mantid/geometry/"
                                                 "index.html")
mantid_api_urls = crawl_url_for_html_addons("http://docs.mantidproject.org/nightly/api/python/mantid/api/"
                                            "index.html")
# Only one
mantid_plots_urls = ["http://docs.mantidproject.org/nightly/api/python/mantid/plots/index.html"]

# Only one
mantid_simpleapi_urls = ["http://docs.mantidproject.org/nightly/api/python/mantid/simpleapi.html"]

# Only one
mantid_fitfunctions = ["http://docs.mantidproject.org/nightly/api/python/mantid/fitfunctions.html"]

mantidplot_urls = crawl_url_for_html_addons("http://docs.mantidproject.org/nightly/api/python/mantidplot/index.html")

all_urls.extend(mantid_api_urls)
all_urls.extend(mantid_fitfunctions)
all_urls.extend(mantid_geometry_urls)
all_urls.extend(mantid_kernel_urls)
all_urls.extend(mantid_plots_urls)
all_urls.extend(mantid_simpleapi_urls)
all_urls.extend(mantidplot_urls)

print("All webpages crawled")

print("Opening Urls...")

delay = args.open_tab_delay

if delay is None:
        delay = 1

open_urls(all_urls, delay)
print("All URLs opened")
