# Palnagotchi for M5 Cardputer

![Palnagotchi](https://github.com/viniciusbo/m5-palnagotchi/blob/master/palnagotchi.jpg?raw=true)

This is meant to be a friendly unit for those lonely Pwnagotchis out there. It's written to run on the M5 Cardputer, but I'll try to add support to other M5 devices in the future.

I dumped the raw frame from my own Pwnagotchi in Wireshark, hardcoded it here, removed Radiotap headers and some other stuff to make it work.

I still don't know what are the implications of a dummy friendly unit for your Pwnagotchi, though. Guess it's fine. Either way, I'll take some time to know better how Pwngrid works internally.

## Planned features

- Customize name, face and other Pwngrid advertising data
- Detect and show friendly units in Cardputer (code adapted from ESP32 Marauder)
