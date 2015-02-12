#!/bin/sh
#####################################################################
# Starts the Jenkins slave process if it is not already running.
# It also downloads the slave.jar to /tmp if it does not already
# exist.
#
# The settings at the top must be filled in for each slave.
#####################################################################

#####################################################################
# User configuration
#####################################################################
# Main url for Jenkins
JENKINS_URL=
# URL for proxy (if required)
PROXY_HOST=
# Port for proxy (if required)
PROXY_PORT=
# URL of jnlp file for slave. Replace [NODENAME] with the slave name on Jenkins
SLAVE_AGENT_URL="${JENKINS_URL}/computer/[NODENAME]/slave-agent.jnlp"
# Copy the secret from the command line given at the above URL
SECRET=

#####################################################################
# Script
#####################################################################
JAR_FILE=/tmp/slave.jar
RUNNING=$(ps aux | grep java | grep $JAR_FILE)
if [ ! -z "${RUNNING}" ]; then
  echo "Slave process is already running"
  exit 0
else
  echo "Slave process is not running, starting..."
fi

if [ ! -z "${PROXY_HOST}" ]; then
  PROXY_ARGS="-Dhttp.proxyHost=${PROXY_HOST} -Dhttp.proxyPort=${PROXY_PORT}"
  # For curl
  export http_proxy=http://$PROXY_HOST:$PROXY_PORT
fi

if [ ! -f ${JAR_FILE} ]; then
  echo "Downloading slave jar file to ${JAR_FILE}"
  curl -o ${JAR_FILE} ${JENKINS_URL}/jnlpJars/slave.jar
fi

# Start
JAVA_ARGS="${PROXY_ARGS} -jar ${JAR_FILE} -jnlpUrl ${SLAVE_AGENT_URL} -secret ${SECRET}"
java ${JAVA_ARGS}
