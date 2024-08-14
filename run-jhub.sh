#!/bin/ksh93

# /usr/local/etc/run-jhub.sh

# A simple wrapper around jupyterhub to determine and set the right PYTHON and
# related env vars before it gets started.

LOGFILE=~${LOGNAME}/jupyterhub.log
CV=
OSR=${ lsb_release -rs; }
SSL=
(( ${OSR//.} > 2004 )) && CV=4 SSL='--no-ssl' #LOGFILE=/tmp/jupyterhub.log

if (( ${ id -u ; } == 0 )); then
    print -u2 This script should not be run as root!
    exit 99
fi
cd ~ && pwd || exit 1
export JUPYTER_PATH=/usr/local/jupyter
export PATH=${JUPYTER_PATH}/bin:${PATH}
CMD=( ${ python3 -V ; } )
CMD=( ${CMD[1]//./ } )
export PYTHONPATH=${JUPYTER_PATH}/lib/python${CMD}.${CMD[1]}/site-packages
export NPM_CONFIG_GLOBALCONFIG=${JUPYTER_PATH}/npmrc
export HOST=${ uname -n; }
env >/tmp/env.$$

[[ ${LOGLEVEL} =~ ^(DEBUG|INFO|WARN|ERROR|CRITICAL)$ ]] || LOGLEVEL='WARN'
exec "${JUPYTER_PATH}/bin/jupyterhub" ${SSL} \
	-f "${JUPYTER_PATH%/*}/etc/jhub${CV}_cfg.py" \
	--log-level=${LOGLEVEL} \
	>${LOGFILE} 2>&1 &
