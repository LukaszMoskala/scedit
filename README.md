# scedit
Program to edit smb.conf

Name comes from `Smb.Conf EDIT`
# DISCLAIMER!
This software is NOT meant for production usage! (YET)
# Usage
`scedit add testshare` creates samba share named test

`scedit set testshare.path=/my/share/path` sets (or creates if not exists) property `path` for `testshare`

`scedit del testshare.path` deletes `path` property from `testshare`

`scedit del testshare` deletes entire `testshare` share

`scedit get testshare.path` prints value of `testshare.path` to STDOUT.
Works in scripts, but probably isn't usefull

`scedit f testscript` executes commands from file `testscript`

# Modifying the [global] section
Just edit parameters as any share parameters:

`scedit set global.workgroup=myNewWorkgroup`

`scedit get global.workgroup`

# Script syntax
Syntax in scripts is identical as syntax on command line. Example script:
```
add testshare
set testshare.path=/my/share/path
set .create mask=0644
```
Program will guess share name if not specified (that is, it'll use whatever share was used before).
It's not recommended since it's easy to forget about the dot before parameter name, causing unexpected behaviour
# Working principle
Entire `smb.conf` is loaded into memory, then it's processed according to command line options or script.
Before finishing, program saves config as `smb.conf.bak` and writes contents of memory to `smb.conf`
