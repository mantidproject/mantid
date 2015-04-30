#!/bin/sh
#####################################################################
# Starts the Jenkins slave process if it is not already running.
# It also downloads the slave.jar to /tmp if it does not already
# exist.
#
# The settings at the top must be filled in for each slave.
#####################################################################
# Crontab setting should be something like
# 0,30 * * * * /home/builder/jenkins-linode/jenkins-slave.sh nodename secret
# or (on mac)
# 0,30 * * * * /Users/builder/jenkins-linode/jenkins-slave.sh nodename secret
#####################################################################
# User configuration
#####################################################################
# Main url for Jenkins
JENKINS_URL=http://builds.mantidproject.org
# URL for proxy (if required)
PROXY_HOST=${3}
# Port for proxy (if required)
PROXY_PORT=${4}
# Name of the node as it appears on jenkins
NODE_NAME=${1}
# Copy the secret from the command line given at the above URL
SECRET=${2}

#####################################################################
# Constants
#####################################################################
# URL of jnlp file for slave
SLAVE_AGENT_URL="${JENKINS_URL}/computer/${NODE_NAME}/slave-agent.jnlp"
# name of the slave jar - full path is determined later
JAR_FILE=slave.jar
# Some versions of cron don't set the USER environment variable
# required by vnc
[ -z "$USER" ] && export USER=$(whoami)
# Put /usr/local/bin on the PATH if brew is installed
[ -f /usr/local/bin/brew ] && export PATH=${PATH}:/usr/local/bin

#####################################################################
# Script
#####################################################################
# exit if it is already running
RUNNING=$(ps u -U $(whoami) | grep java | grep ${JAR_FILE})
if [ ! -z "${RUNNING}" ]; then
  echo "Slave process is already running"
  exit 0
else
  echo "Slave process is not running"
fi

# error out if there isn't a node name and secret
if [ "$#" -lt 2 ]; then
  echo "Usage: `basename ${0}` <NODE_NAME> <SECRET> [PROXY_HOST] [PROXY_PORT]"
  exit -1
fi

# setup the proxy
if [ ! -z "${PROXY_HOST}" ]; then
  PROXY_ARGS="-Dhttp.proxyHost=${PROXY_HOST} -Dhttp.proxyPort=${PROXY_PORT}"
  echo "using proxy ${PROXY_HOST} ${PROXY_PORT}"
  # For curl
  export http_proxy=http://$PROXY_HOST:$PROXY_PORT
fi

# find the jar file if it exists
if [ -f ${HOME}/jenkins-linode/${JAR_FILE} ]; then
  JAR_FILE=${HOME}/jenkins-linode/${JAR_FILE}
elif [ -f ${HOME}/Jenkins/${JAR_FILE} ]; then
  JAR_FILE=${HOME}/Jenkins/${JAR_FILE}
else
  JAR_FILE=/tmp/${JAR_FILE}
  if [ ! -f ${JAR_FILE} ]; then
    echo "Downloading slave jar file to ${JAR_FILE}"
    if [ $(command -v curl) ]; then
      echo "curl -o ${JAR_FILE} ${JENKINS_URL}/jnlpJars/slave.jar"
      curl -o ${JAR_FILE} ${JENKINS_URL}/jnlpJars/slave.jar
    else
      echo "Need curl to download ${JENKINS_URL}/jnlpJars/slave.jar"
      exit -1
    fi
  fi
fi

echo "starting ..."
JAVA=`which java`
JAVA_ARGS="${PROXY_ARGS} -jar ${JAR_FILE} -jnlpUrl ${SLAVE_AGENT_URL} -secret ${SECRET}"
echo "${JAVA} ${JAVA_ARGS}"
${JAVA} ${JAVA_ARGS}
