#!/usr/bin/python3
from __future__ import print_function

import posixpath as path
import pprint

import requests

JENKINS_ROOT_URL = 'http://builds.mantidproject.org'
QUEUE_ENDPOINT_JSON = path.join(JENKINS_ROOT_URL, 'queue/api/json')

req = requests.get(QUEUE_ENDPOINT_JSON)
payload = req.json()

# extract items by job name
queue = payload['items']
task_queues = {}
for item in queue:
    params = item['params'].split('\n')
    try:
        pr = params[5]
        job_name = item['task']['name']
        if job_name not in task_queues:
            task_queues[job_name] = list()
        task_queues[job_name].append(pr)
    except IndexError:
        pass

# Dump to screen
pprint.pprint(task_queues)
