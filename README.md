# Discord RP plugin for DeaDBeeF 
Discord Rich Presence Plugin shows your current playing track on your Discord status.

![image](https://user-images.githubusercontent.com/6359901/37570313-94e681fa-2aee-11e8-8b65-cd786c999a0f.png)

## Configuration
Plugin connects with Discord through Discord Rich Prensence API, no further authentication is needed.
You can configure displayed information through plugin settings:

![image](https://user-images.githubusercontent.com/6359901/37570322-c8a79236-2aee-11e8-875f-ba317ded6b25.png)

For more information about title formatting please visit [https://github.com/DeaDBeeF-Player/deadbeef/wiki/Title-formatting-2.0](https://github.com/DeaDBeeF-Player/deadbeef/wiki/Title-formatting-2.0)

## Compile
To compile that plugin you will need `libdiscord-rpc` static library (`libdiscord-rpc.a`).
This repo includes that precompiled library for x86_64.

If you'd like to compile libdirscord-rpc self, you will need to compile it as reallocable with `-fPIC` option.
You can find libdiscord-rpc library [here](https://github.com/discordapp/discord-rpc). After compiling libdiscord-rpc you can copy `libdiscord-rpc.a` and `discord_rpc.h` into that repo.

To compile discord_presence plugin simply do `make` and `sudo make install`. For debug build, compile with `make DEBUG=1`.
