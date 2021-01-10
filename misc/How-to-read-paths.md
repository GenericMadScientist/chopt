# How to read paths

## Text notation

A path is often abbreviated in the following form:

2-2(+1)-3

CHOpt will produce this type of abbreviation. You read it as follows:

2 - Get two star power phrases before activating

2(+1) - Get two star power phrases before activating, and overlap one phrase
during the activation

3 - Get three star power phrases before activating

Sometimes a path ends in ES1, for example the optimal path for Fire It Up from
Anti Hero 2 is 2(+1)-2(+1)-ES1. The ES1 means that you will acquire one star
power phrase after the last activation, but you don't use it. It is
theoretically possible for an optimal path to end in ES2, but I have never seen
a non-artificial example.

It is possible for a path to have you acquire 0 star power phrases before
activating, if you can get enough whammy to activate. An extreme example is the
optimal path for Time Traveler from Circuit Breaker, which is
3-1-0-0-0-2-1-0-0-0-2. It is also possible for a path to have you acquire 5 or
more star power phrases before activating. One example is the optimal path for
Higan Servant ~ One Conclusion from Anti Hero: Beach Episode, which is
5(+1)-2(+1).

CHOpt will also give the start of every activation in text. NN means next note,
otherwise it will count notes (1st G, 3rd RBO, and so on). If the activation is
mid sustain it will specify roughly where to activate. This is currently very
rudimentary though and it doesn't try if you do not get a phrase before an
activation.

## Images

CHOpt highlights regions of the song with the following colours:
* blue: where star power should be active
* green: where a star power phrase is
* red: where to squeeze at the start and end of an activation
* yellow: where the player should not gain star power from whammy
* grey: solo section

### Squeezing

The red shows how late/early you have to hit the first/late note of an
activation. If you have to hit a note before the start early or a note after
the end late, CHOpt will just draw star power up to those notes (suggestions to
notate this cleanly are welcome).

Sometimes a note during an activation must be hit early to ensure overlap, or a
note must be hit late to avoid wasting star power on getting full bar. For an
example of the latter, look at the Malevolence activation on Soulless 4 bass in
[RileyTheFox's video](https://www.youtube.com/watch?v=MWl9mmx7kpY&t=6m). If the
last note of that phrase is not hit late, CHelgaBot would max out on SP too
early. This would cost star power and then it would be impossible to squeeze to
the orange at the end. CHOpt will not draw this in any way but you may have to
keep it in mind.

### Compressed whammy

Yellow indicates where the player should stop acquiring star power from whammy.
This is not quite the same as where the player should not whammy, because there
is a delay from when the player stops whammying and when the game stops giving
star power. I don't know exactly how this delay works, so it's up to you to
adjust.

The most common situation where the player should stop whammying is if it's
optimal to get part of a star power sustain under star power, then get whammy
from the rest. Typically this will entail whammying to extend the activation,
stop whammying briefly, then start whammying once the activation ends. Be aware
that in this situation the window of no whammy is often small enough that CHOpt
will not draw it.

Sometimes longer stretches of no whammying is required to prevent an unwanted
overlap. A good example of this is afforded by the optimal path for More Than A
Feeling from the Guitar Hero 1 rip. It is necessary to forego the whammy before
the m76 activation because the whammy starting on m80 is needed to get maximum
ticks in the next activation.

[GH1 More Than A Feeling path](more-than-a-feeling.png)
