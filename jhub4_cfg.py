import os
import re

# /usr/local/etc/jhub4_cfg.py

c = get_config()

# Access jupiterhub via http://host.example.com:8000/jupyter/${HOSTNAME}
c.JupyterHub.base_url = '/'.join([ '/jupyter', os.environ['HOST'] ]) 
c.JupyterHub.spawner_class = 'systemdspawner.SystemdSpawner'
#c.JupyterHub.shutdown_on_logout = True
#c.JupyterHub.allow_origin = '*'

# defaults for backward compat < 5.0
c.Authenticator.allow_all = True
c.Authenticator.allow_existing_users = True

c.SystemdSpawner.default_shell = '/bin/bash'
c.SystemdSpawner.unit_name_template = 'jsu-{USERNAME}'
# For now we use for each user a separate dir to pass the env vars file as the
# default SystemdSpawner does. But should be ok, to use the same for all.
# NOTE: The name gets prefix with /run/ by systemd to construct the final path.
c.SystemdSpawner.unit_extra_properties = { 'RuntimeDirectory': 'jhub/jsu-{USERNAME}' }
# We want to have all jupyter-singleuser services in a separate slice to be
# able to manage the overall resource usage of them much easier. So e.g. we
# can stop all running jupyter-singleuser services at once using
# 'jsd-helper systemctl stop jhub.slice'.
# The default is None, i.e. it would end up within the same slice, in which the
# jupyterhub service runs.
c.SystemdSpawner.slice = 'jhub-' + (os.environ['HOST']).replace('-',':')

# Optional resource limits applied to each jupyter-singleuser service.
#c.SystemdSpawner.mem_limit = None
#c.SystemdSpawner.cpu_limit = None

# BTW: To apply specific resource limits depending on the instance user one may
# check https://gitlabph.physik.fu-berlin.de/behrmann/staticsystemdspawner
# (currently beta).
