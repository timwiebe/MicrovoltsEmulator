# 3.4 Setting up the client
This is the final step — you're almost there!

We now need to make sure the client is configured to connect to your local server.

### Download the client
You can technically use any client version newer than Surge, but I  recommend the ToyBattles client. Here's the download link: https://mega.nz/file/fR13xIJY#fYM0yGEZcaqz3MbhD5tqBaUEByIX5jHUxYV9GniXeZU

Using the ToyBattles client is highly recommended since it will already contain some client modifications that allow specific features like the Gamble System, Custom error messages and correct map IDs.

**Note**: If you still decide to use the original Surge client, you will need to update the GameMaps enum since this branch is targeted towards a modified version of the client that uses a different order of the maps (the one given above).
Refer to the branch `mv.1_1` to get the correct GameMaps enum for the original Surge client.
Also, the original Surge client will not have the correct error messages in certain cases and some UI names will be wrong when used with this server emulator.

### Install the `cgd.dip` archive with localhost IPs
Next, download the latest cgd.dip archive from the link below. This file is already set up to point to 127.0.0.1 (localhost) and includes all necessary client data: https://www.mediafire.com/file/5uw65fwps66fsll/cgd.dip/file

This will allow your client to connect to your localhost (127.0.0.1) server.

✅ Note: This archive is pre-configured to work with the provided config.ini file. You don’t need to change the ports — everything should match up automatically.

When done, make sure to replace the `cgd.dip` you have in your game folder / `data` with this one. (Take a backup of the original one just in case)

### Start the client
To launch the game, you have two options:

- Run the _Launcher.bat script, or
- Open the client folder, go to /Bin, and run either MicroVolts.exe or ToyBattles.exe, depending on the client version you downloaded.

# Final Checklist: Starting Everything Up
By now, you should have completed the following steps:
- ✅ Compiled all the executables (Common, MainServer, AuthServer, and CastServer)
- ✅ Launched each server executable without seeing any red error messages
- ✅ Set up and started your MariaDB database
- ✅ Edited your config.ini file so that all IPs point to 127.0.0.1 (localhost), if you're using the cgd.dip file from above
- ✅ Downloaded and started the client

If everything’s in place and running correctly, you should be able to log in using the following test credentials:

Username: test
Password: test


## Next
[4.1 Config setup for localhost](https://github.com/SoWeBegin/MicrovoltsEmulator/blob/mv1.1_2.0/doc/example_localhost.md)
