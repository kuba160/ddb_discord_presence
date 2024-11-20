<!--
SPDX-FileCopyrightText: 2022 Jakub Wasylków <kuba_160@protonmail.com>
SPDX-License-Identifier: CC0-1.0
-->

# Discord RP plugin for DeaDBeeF 
Discord Rich Presence Plugin shows your current playing track on your Discord status.

![image](https://github.com/user-attachments/assets/dc7128d3-3f34-4276-92d1-9b2a35f7c3ac)


## Download
You can find my local builds on https://github.com/kuba160/ddb_discord_presence/releases

Another alternative is to download it from http://deadbeef.sourceforge.net/plugins.html (current stable release)

## Configuration
Plugin connects with Discord through Discord Rich Presence API, no further authentication is needed.
You can configure displayed information through plugin settings:

![image](https://github.com/user-attachments/assets/05133568-8f06-41c3-a253-469839d50af4)

For more information about title formatting please visit [https://github.com/DeaDBeeF-Player/deadbeef/wiki/Title-formatting-2.0](https://github.com/DeaDBeeF-Player/deadbeef/wiki/Title-formatting-2.0)


## Compile
Submodules are not updated automatically: run `git submodule update --init` first.

To compile discord_presence plugin simply do `make` and `sudo make install`. For debug build, compile with `make DEBUG=1`.

## Troubleshooting

#### Plugin not working with discord installed through flatpak

Flatpak by default sandboxes applications which makes rich presence unavailable to other applications. To fix this issue look at this [wiki page](https://github.com/flathub/com.discordapp.Discord/wiki/Rich-Precense-(discord-rpc)).

### More information
This plugin uses `libdiscord-rpc` library. By running `make` it will automatically download discord-rpc library through submodule and it will be patched so the library is reallocable (`-fPIC`).
It will build`libdiscord-rpc` library and then move static library file (`libdiscord-rpc.a`) into the main directory. Plugin will be linked with this file.

To compile without building `libdiscord-rpc` run `make discord_presence`.

## License

This work is licensed under multiple licences. Plugin itself and artwork code (with exception of escape.c) is licensed under Zlib. artwork/escape.c is licensed under curl license. discord-rpc is itself licensed under MIT.
