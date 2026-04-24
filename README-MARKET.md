# Flipper ham APRS TX

An experimental APRS / AX.25 transmitter for Flipper Zero.

There are plenty of APRS contraptions that use the Flipper as an audio source for a handheld. This is not that. This app abuses the Sub-GHz radio directly and transmits something APRS-like using only the Flipper itself. It works rather better than it ought to, and rather less properly than any respectable radio design would.

The result is usable, but still experimental. The signal is unconventional, the modulation is a compromise, and decode quality depends heavily on the receiver. Software decoders tend to be more forgiving. Hardware varies from pleasantly cooperative to deeply unimpressed.

## What it can send

- APRS messages
- Status packets
- Bulletins
- Fixed-position packets

It also keeps a small destination callbook, remembers settings, and gives you a few RF-side controls for persuading difficult receivers to decode the thing.

## What to expect

This is a deliberately rough FSK hack pretending to be FM. If it decodes cleanly first time, do enjoy the moment. If it does not, debug mode lets you adjust deviation and switch between 2FSK and GFSK until your receiver becomes less offended.

With roughly 100 mW to work with, height matters. Indoors is not the ideal place to expect heroics.

## More

Tech post: https://yo3gnd.ro/blog/2604a--flipper-zero-aprs-tx

Demo video: https://www.youtube.com/watch?v=OhWlq-4IK9E
