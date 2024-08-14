# jsd-helper
**jsd-helper** is a utility to allow an unprivileged user to exec [jupyterhub systemdSpawner](https://github.com/jupyterhub/jupyterhub/) related commands ([systemd-run](https://www.freedesktop.org/software/systemd/man/latest/systemd-run.html) and  [systemctl](https://www.freedesktop.org/software/systemd/man/latest/systemctl.html)) as root if the file owner gets set to root and in addition the suid bit gets set.

The intention behind it is to let the jupyterhub service (and thus the hub
including the required proxy) run by an unprivileged user to limit the damage
which could be done, if the service gets compromised or attacked in some way. 

If a user logs in via jupyterhub, it spawns a jupyter-singleuser instance.
Per default this instance gets run as the same OS user, which runs the
hub and proxy and thus does not provide a proper separation of the user
environments and data. To circumvent this, one may use instead of the default
spawner the systemdSpawner, which launches the jupyter-singleuser instance
for the user logged in via jupyterhub as the same OS user via a transient
systemd service using systemd-run. The problem is, to be able to start the
service via systemd-run for a different user, and put it into desired slices
one privileges a normal user does not have. So the current recommendation is
to simply run the jupyterhub service as root, which is a bad idea for security
reasons.

An here **jsd-helper** slips in: installed as suid program and executable for
trusted user(s), only. I.e. per default the program should be executable only
by user root (permission 04750), the group, which owns the file (or simply use
04700 if this is not desired) and the users, which are able to execute the file
which in turn gets controlled by ACLs.

## Compile
Set your environment variable CC to the desired C compiler and run GNU make.  E.g.:
```
export CC=gcc
make
```

## Install
The program needs to be copied to the same directory, which contains the jupyterhub-singleuser, which gets used by the spawner to launch a new instance which than can be used by the user to run jupyter-notebook, jupyter-lab, the terminal emulator, ... . After this the owner of the file must be set to root, setting the group to a desired value is optional. Finally the file permission must be set to either 04750 or 04700, depending on whether you wanna allow the file owning group to execute it or not. E.g. if the jupyterhub service is run by the OS user `juppy`:
```
# you probably need additional (root) privileges for the following commands:
export HELPER=/usr/local/bin/jsd-helper

cp jsd-helper ${HELPER}
chown root:staff ${HELPER}
chmod 04750 ${HELPER}
setfacl -m u:juppy:r-x ${HELPER}
# check ACLs
getfacl ${HELPER}
# in case you made an error or wanna revoke the permission for a user, just
# run the same command, but with the option -x like
setfacl -x u:juppy:r-x ${HELPER}

# Apply the patch to the systemd.py coming with the systemdSpawner:
PATCH=${PWD}/systemdSpawner.patch
cd ${HELPER%/*}/..
patch -p1 -i ${PATCH}
```
If you installed jupyterhub in a non-default location (/usr/local or /usr)
you should have a look at the [syspath.patch](./syspath.patch) as well.

If you get a 'Operation not supported' error when setting the ACLs either the used filesystem does not support ACLs, or they are not enabled. In case of ZFS one may use `df -h /usr/local/bin/.` to get the name of the ZFS, which contains the jsd-helper and than run as root or privileged user:
```
zfs set xattr=sa ${zfs_name}
zfs set acltype=posixacl ${zfs_name}
```

## Configuration
To make it as safe as possible, jsd-helper runs *systemd-run* and *systemctl*, only. Furthermore it manages only services whose name starts with `jsu-` - so the c.SystemdSpawner.unit\_name\_template needs to be properly set. Another thing is, that the jupyterhub service need to pass certain environment variables to the jsu-\* service to start. However this can be done via a file, only (see systemd.service(5) *EnvironmentFile*) and because the vars are sensible (like API\_TOKEN which allows one to take over a session) all vars need to be written to a file, which can be read/write only by the OS user which runs the jupyterhub service and the OS user, whose instance gets launched. SystemdSpawner uses for this the **RuntimeDirectory** of the jupyterhub service and uses the unit name with extension '.env' as filename. That's why make sure, that every potential OS user has access to this directory, because the [systemdSpawner.patch](./systemdSpawner.patch) just changes the defaults to be suitable and lets the spwawner apply the required ACLs to the EnvironmentFile file, only!

So a complete configuration may look like this:
- [/etc/systemd/system/jhub.service](./jhub.service)
- [/usr/local/etc/run-jhub.sh](./run-jhub.sh)
- [/usr/local/etc/jhub4\_cfg.py](./jhub4_cfg.py)

## Example
Using the configuration above, the command used to launch the jupyter-singleuser instance for an OS user having the uid 1234 and gid 5678 would be something like this:
```
/usr/local/jupyter/bin/jsd-helper systemd-run '--unit' 'jsu-testus' \
    '--working-directory' '/home/testus' '--uid=1234' '--gid=5678' \
    '--slice=jhub-node08' '--property=NoNewPrivileges=yes' \
    '--property=RuntimeDirectory=jhub/jsu-testus' \
    '--property=RuntimeDirectoryMode=755' \
    '--property=RuntimeDirectoryPreserve=restart' \
    '--property=OOMPolicy=continue' \
    '--property=EnvironmentFile=/run/jhub/jsu-testus/jsu-testus.env' \
    '/usr/local/jupyter/bin/jupyterhub-singleuser' \
    '--NotebookApp.allow_origin=*' '--log-level=DEBUG' '--debug'
```
