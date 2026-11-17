# PureC-OS-Settings

System settings of PureC OS, written in C on libgui (`PureC-OS-LibGui`) and the
video-backend display server. Package and program name: `settings`
(`/usr/bin/settings`).

Pages: About, Network (Wi-Fi scan and connect, adapters), Sound (volume, mute,
output, test tone), Display (resolution through videod), Appearance (11 themes,
wallpaper, desktop font; writes `/config/appear.ini`, every libgui window
follows the theme at once), Storage, Date & time (time zone), Power (battery,
temperature, restart / power off). System calls go through `src/sysapi.c`.

## Build

Inside the PureC-OS tree (libgui and libvb are linked statically):

    make -C programs/PureC-OS-Settings            # bin/settings/settings
    make -C programs/PureC-OS-Settings package    # bin/settings/settings-VERSION-1-x86_64.pkg.tar.gz (+ .sig)
    python3 mk/pkg/publish.py --registry Trusted bin/settings/settings-VERSION-1-x86_64.pkg.tar.gz

On PureC OS: `pkg -Sy && pkg -S settings`.
