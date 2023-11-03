<!--
SPDX-FileCopyrightText: 2022 Jakub Wasylków <kuba_160@protonmail.com>
SPDX-License-Identifier: CC0-1.0
-->

# Discord RP plugin for DeaDBeeF 
Discord Rich Presence Plugin shows your current playing track on your Discord status.

![image](https://user-images.githubusercontent.com/6359901/64494663-11f67980-d290-11e9-8b88-8c0f6d011dca.png)

## Download
You can find my local builds on https://github.com/kuba160/ddb_discord_presence/releases

Another alternative is to download it from http://deadbeef.sourceforge.net/plugins.html (0.7 branch)

## Configuration
Plugin connects with Discord through Discord Rich Presence API, no further authentication is needed.
You can configure displayed information through plugin settings:

![image](https://user-images.githubusercontent.com/6359901/37570322-c8a79236-2aee-11e8-875f-ba317ded6b25.png)

For more information about title formatting please visit [https://github.com/DeaDBeeF-Player/deadbeef/wiki/Title-formatting-2.0](https://github.com/DeaDBeeF-Player/deadbeef/wiki/Title-formatting-2.0)


## Compile
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
