# Palnagotchi for M5Stack

![Palnagotchi](https://github.com/viniciusbo/m5-palnagotchi/blob/master/palnagotchi.jpg?raw=true)

A friendly unit for those lonely Pwnagotchis out there. It's written to run on the M5Stack, see supported devices below.

I reverse engineered the Pwngrid advertisement protocol and made it possible for several M5Stack devices to advertise to the Pwngrid as a Pwnagotchi. All brain policy parameters that could negatively impact AI learning were removed from the advertisemenet data.

The Pwngrid works by sending Wifi beacon frames with a JSON serialized payload in Wifi AC headers, containing the Pwnagotchi's data (name, face, pwns, brain policy between others). That's how nearby Pwnagotchis can detect and also learn each other. By crafting a custom beacon frame, this app can appear as a Pwnagotchi to other Pwnagotchis.

## Supported devices

- Cardputer
- Cardputer ADV
- StickC Plus2
- StickC (untested)
- AtomS3
- Dial
- DinMeter
- Core

## Usage

- Run the app to start advertisement.
- Button layouts:
  - Cardputer / Cardputer ADV: ESC or m toggles the menu. Use arrow keys or tab to navigate and OK to select option. Esc or m to go back to main menu.
  - StickC Plus2: Long press M5 button to toggle menu. Use top and bottom keys to navigate and M5 button (short press) to select option.
  - AtomS3(R): Long press display to toggle menu. Use double/triple tap display to navigate and short press display to select option.
  - Dial: Long press M5 button to toggle menu. Use rotary encoder to navigate. Short press M5 button to select.
  - DinMeter: Long press rotary encoder to toggle menu. Use rotary encoder to navigate. Short press rotary encoder button to select.
- Top bar shows UPS level and uptime.
- Bottom bar shows total friends made in this run, all time total friends between parenthesis (needs EEPROM) and signal strengh indicator of closest friend.
- Nearby pwnagotchis show all nearby units and its signal strength.
- Palnagotchi gets a random mood every minute or so.

## Why?

I don't like to see a sad Pwnagotchi.

## Planned features

- Friend spam?
