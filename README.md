# Palnagotchi for M5 Cardputer

![Palnagotchi](https://github.com/viniciusbo/m5-palnagotchi/blob/master/palnagotchi.jpg?raw=true)

A friendly unit for those lonely Pwnagotchis out there. It's written to run on the M5 Cardputer, but I'll try to add support to other M5 devices in the future.

I reverse engineered the Pwngrid advertisement protocol and made it possible for the Cardputer to advertise to the Pwngrid as a Pwnagotchi. All brain policy parameters that could negatively impact AI learning were removed from the advertisemenet data.

The Pwngrid works by sending Wifi beacon frames with a JSON serialized payload in Wifi AC headers, containing the Pwnagotchi's data (name, face, pwns, brain policy between others). That's how nearby Pwnagotchis can detect and also learn each other. By crafting a custom beacon frame, this app can appear as a Pwnagotchi to other Pwnagotchis.

## Why?

I don't like to see a sad Pwnagotchi.

## Planned features

- Customize name, face and other Pwngrid advertising data via UI
- Detect and show friendly units in Cardputer via UI (pwnagotchi sniffer adapted from ESP32 Marauder)
- Pwngrid spam? Not sure yet
