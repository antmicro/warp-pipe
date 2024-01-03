# Wireshark

You can install [the plugin][wireshark-plug] by simply copying it to `$XDG_CONFIG_HOME/wireshark/plugins` (`~/.config/wireshark/plugins` on Linux).
The directory might not exist yet.

Currently the plugin features a simple dissector for the protocol described in [](./pcie-over-tcp.md).

[wireshark-plug]: https://github.com/antmicro/wireshark-pcie-dissector/blob/main/pcie-pipe.lua
