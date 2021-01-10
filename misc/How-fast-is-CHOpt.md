# How fast is CHOpt

CHOpt is now fast enough that I can throw multiple setlists at it, so for fun I
decided to do so and see what results come up. Don't take this too seriously:
for one I only ran this on all these songs once. I did this with CHOpt 1.2.2.

## The songs

I ran CHOpt on every song from the following setlists and packs with Expert
Lead Guitar, except Classical Thump, Dawn Patrol, Imagine, and Space Cowboy for
which I did Expert Bass.

* Angevil Hero II
* Anti Hero
* Anti Hero: Beach Episode
* Anti Hero: Beach Episode - The Beach Sides
* Anti Hero 2
* Anti Hero 2 Charity Drive
* Band Hero
* Blanket Statement
* Brand New Hero
* Carpal Tunnel Hero: Remastered
* Carpal Tunnel Hero 2
* CHARTS
* CHARTS 2
* CHARTS 2 DLC (Pack 1)
* Circuit Breaker
* Cow Hero
* CSC Monthly Packs (June 2018 - December 2020)
* DF Discography CH
* Digitizer
* Dissonance Hero
* Dissonance Hero Free-LC (Pack 1)
* DJ Hero
* Djent Hero Collection (Pack #1 - Pack #5)
* DJMax Packs (Pack I - Pack II)
* Facelift (Pack 1 - Pack 2)
* Focal Point
* Focal Point 2
* GaMetal Power Pack
* Green Day Rock Band
* Guitar Hero
* Guitar Hero II
* Guitar Hero II DLC
* Guitar Hero Encore: Rocks the 80s
* Guitar Hero III: Legends of Rock
* Guitar Hero III: Legends of Rock DLC
* Guitar Hero: Aerosmith
* Guitar Hero World Tour
* Guitar Hero World Tour DLC
* Guitar Hero: Metallica
* Guitar Hero: Metallica DLC
* Guitar Hero Smash Hits
* Guitar Hero: Van Halen
* Guitar Hero 5
* Guitar Hero 5 DLC
* Guitar Hero: Warriors of Rock
* Guitar Hero: Warriors of Rock DLC
* Guitar Hero On Tour
* Guitar Hero On Tour: Decades
* Guitar Hero On Tour: Modern Hits
* Guitar Hero: Guitar Zero
* Guitar Hero: Guitar Zero DLC (DLC #1 - DLC #9)
* Guitar Hero X
* Koreaboo Hero
* Koreaboo Hero 2
* Lego Rock Band
* Marathon Hero
* Marathon Hero 2
* Marathon Hero 2 DLC
* Paradigm
* Phase Shift Guitar Project 4
* Redemption Arc
* Rock Band
* Rock Band 2
* Rock Band 3
* Rock Band ACDC Live Track Pack
* Rock Band Blitz
* The Beatles Rock Band
* The Fall of Troy Hero
* Trunks252JM's Classic Charts
* Verified Unverified
* Vortex Hero
* Zero Gravity

If there's a setlist or pack you'd like to see on here, let me know.

## Results

### Overall

The combined runtime was 64m46s. Below is a histogram of the runtime for each
song, with the top 1% combined as outliers.

![Runtime histogram](runtime-histogram.svg)

The 25 slowest songs are as follows:

| Song                                             | Setlist                      | Time (s) |
| ------------------------------------------------ | ---------------------------- | -------- |
| The Forever Saga                                 | Marathon Hero 2              | 61.185   |
| CHARTS 2: Endless Setlist                        | CHARTS 2                     | 45.437   |
| CHARTS: The (almost) Endless Setlist             | CHARTS                       | 29.816   |
| Way Too Damn Much Anti Hero 2                    | Anti Hero 2 Charity Drive    | 18.229   |
| Endless Setlist: The Fall of Troy Hero           | The Fall of Troy Hero        | 18.229   |
| Vector/Virus                                     | Marathon Hero 2              | 12.342   |
| Uranoid                                          | CHARTS                       | 11.535   |
| Chezy's Ultimate Solo Experience                 | CHARTS 2                     | 11.046   |
| The Dark Tower (Full Album)                      | Marathon Hero 2              | 8.207    |
| The Human Equation                               | Marathon Hero                | 8.146    |
| The Tale of Iso Subject 5                        | Marathon Hero 2              | 7.539    |
| Reincarnation (Full Album)                       | Marathon Hero 2              | 7.287    |
| Under the Force of Courage [FULL ALBUM]          | Anti Hero 2 Charity Drive    | 6.935    |
| Catch Thirtythree                                | Djent Hero Collection        | 6.913    |
| Ascendancy [FULL ALBUM]                          | Anti Hero 2 Charity Drive    | 6.843    |
| Operation: Mindcrime (Full Album)                | Marathon Hero 2              | 6.741    |
| Eskapist                                         | Marathon Hero                | 6.015    |
| Eskapist                                         | CHARTS                       | 5.965    |
| Automata                                         | Marathon Hero                | 5.848    |
| Tragic Separation (Full Album)                   | Marathon Hero 2              | 5.644    |
| Periphery IV: Hail Stan (Full Album)             | CHARTS 2                     | 5.381    |
| St. Anger (Full Album)                           | Marathon Hero 2              | 5.379    |
| Coma Witch (Full Album Chart)                    | CHARTS                       | 5.174    |
| Volition (Full Album Chart)                      | CHARTS                       | 5.167    |
| Frankenstein                                     | Marathon Hero 2              | 4.911    |

### By Setlist

| Setlist                                    | Number of songs | Total time (s) | Average time (s) |
| ------------------------------------------ | --------------- | -------------- | ---------------- |
| Angevil Hero II                            | 67              | 29.622         | 0.442            |
| Anti Hero                                  | 402             | 173.729        | 0.432            |
| Anti Hero: Beach Episode                   | 127             | 62.612         | 0.493            |
| Anti Hero: Beach Episode - The Beach Sides | 25              | 10.258         | 0.410            |
| Anti Hero 2                                | 365             | 164.161        | 0.450            |
| Anti Hero 2 Charity Drive                  | 9               | 51.491         | 5.721            |
| Band Hero                                  | 65              | 22.367         | 0.344            |
| Blanket Statement                          | 115             | 44.469         | 0.387            |
| Brand New Hero                             | 83              | 29.508         | 0.356            |
| Carpal Tunnel Hero: Remastered             | 104             | 64.803         | 0.623            |
| Carpal Tunnel Hero 2                       | 309             | 170.042        | 0.550            |
| CHARTS                                     | 646             | 331.574        | 0.513            |
| CHARTS 2                                   | 139             | 119.900        | 0.863            | 
| CHARTS 2 DLC                               | 43              | 21.029         | 0.489            |        
| Circuit Breaker                            | 116             | 47.473         | 0.409            |
| Cow Hero                                   | 71              | 28.559         | 0.402            |
| CSC Monthly Packs                          | 900             | 346.725        | 0.385            |
| DF Discography CH                          | 80              | 47.928         | 0.599            |
| Digitizer                                  | 82              | 32.102         | 0.391            |
| Dissonance Hero                            | 106             | 53.836         | 0.508            |
| Dissonance Hero Free-LC                    | 22              | 8.177          | 0.372            |
| DJ Hero                                    | 10              | 2.567          | 0.257            |
| Djent Hero Collection                      | 161             | 82.554         | 0.513            |
| DJMax Packs                                | 58              | 14.045         | 0.242            |
| Facelift                                   | 86              | 35.728         | 0.415            |
| Focal Point                                | 170             | 83.379         | 0.490            |
| Focal Point 2                              | 186             | 89.166         | 0.479            |
| GaMetal Power Pack                         | 54              | 27.033         | 0.501            |
| Green Day Rock Band                        | 43              | 15.180         | 0.353            |
| Guitar Hero                                | 49              | 15.787         | 0.322            |
| Guitar Hero II                             | 74              | 25.891         | 0.350            |
| Guitar Hero II DLC                         | 24              | 8.117          | 0.338            |
| Guitar Hero Encore: Rocks the 80s          | 30              | 10.350         | 0.345            |
| Guitar Hero III: Legends of Rock           | 70              | 25.148         | 0.359            |
| Guitar Hero III: Legends of Rock DLC       | 68              | 29.374         | 0.432            |
| Guitar Hero: Aerosmith                     | 41              | 14.099         | 0.344            |
| Guitar Hero World Tour                     | 84              | 31.830         | 0.379            |
| Guitar Hero World Tour DLC                 | 147             | 58.111         | 0.395            |
| Guitar Hero: Metallica                     | 49              | 25.917         | 0.529            |
| Guitar Hero: Metallica DLC                 | 10              | 6.995          | 0.699            |
| Guitar Hero Smash Hits                     | 48              | 18.500         | 0.385            |
| Guitar Hero: Van Halen                     | 47              | 18.935         | 0.403            |
| Guitar Hero 5                              | 84              | 34.967         | 0.416            |
| Guitar Hero 5 DLC                          | 158             | 59.731         | 0.378            |
| Guitar Hero: Warriors of Rock              | 93              | 34.703         | 0.373            |
| Guitar Hero: Warriors of Rock DLC          | 84              | 33.662         | 0.401            |
| Guitar Hero On Tour                        | 31              | 10.339         | 0.334            |
| Guitar Hero On Tour: Decades               | 36              | 11.343         | 0.315            |
| Guitar Hero On Tour: Modern Hits           | 44              | 13.907         | 0.316            |
| Guitar Hero: Guitar Zero                   | 64              | 22.925         | 0.358            |
| Guitar Hero: Guitar Zero DLC               | 38              | 20.104         | 0.529            |
| Guitar Hero X                              | 127             | 70.456         | 0.555            |
| Koreaboo Hero                              | 51              | 15.730         | 0.308            |
| Koreaboo Hero 2                            | 101             | 30.789         | 0.305            |
| Lego Rock Band                             | 45              | 13.360         | 0.297            |
| Marathon Hero                              | 49              | 88.601         | 1.808            |
| Marathon Hero 2                            | 152             | 279.476        | 1.839            |
| Marathon Hero 2 DLC                        | 216             | 111.646        | 0.517            |
| Paradigm                                   | 101             | 47.156         | 0.467            |
| Phase Shift Guitar Project 4               | 162             | 72.839         | 0.450            |
| Redemption Arc                             | 100             | 43.529         | 0.435            |
| Rock Band                                  | 58              | 20.918         | 0.361            |
| Rock Band 2                                | 84              | 29.288         | 0.349            |
| Rock Band 3                                | 83              | 29.931         | 0.361            |
| Rock Band ACDC Live Track Pack             | 18              | 8.214          | 0.456            |
| Rock Band Blitz                            | 25              | 8.553          | 0.342            |
| The Beatles Rock Band                      | 45              | 11.895         | 0.264            |
| The Fall of Troy Hero                      | 70              | 60.010         | 0.857            |
| Trunks252JM's Classic Charts               | 56              | 23.500         | 0.420            |
| Verified Unverified                        | 21              | 9.341          | 0.445            |
| Vortex Hero                                | 222             | 97.715         | 0.440            |
| Zero Gravity                               | 179             | 72.646         | 0.406            |

## Average Multiplier Outliers

I've started gathering more information when I measure performance, including
average multiplier. The 25 songs with the highest optimal average multiplier are
as follows:

| Song                                          | Setlist                          | Average Multiplier |
| --------------------------------------------- | -------------------------------- | ------------------ |
| Sugar Foot Rag                                | Carpal Tunnel Hero 2             | 7.276x             |
| Trojans                                       | Carpal Tunnel Hero 2             | 7.070x             |
| Downfall of Gaia                              | CSC Monthly Packs                | 6.874x             |
| Gee-Wiz                                       | Carpal Tunnel Hero 2             | 6.824x             |
| Star X Speed Story Solo Medley                | Anti Hero: Beach Episode         | 6.784x             |
| Thunder And Lightning                         | Carpal Tunnel Hero 2             | 6.752x             |
| Dithering                                     | Paradigm                         | 6.751x             |
| Black Hole Sun                                | Rock Band                        | 6.696x             |
| lifeisgood                                    | CHARTS 2                         | 6.658x             |
| All Is One                                    | Vortex Hero                      | 6.572x             |
| Solace                                        | Djent Hero Collection            | 6.554x             |
| This Ain't a Scene, It's an Arms Race         | Guitar Hero On Tour: Modern Hits | 6.467x             |
| Away / Poetic Justice                         | Vortex Hero                      | 6.463x             |
| Christmas Time is Here (Vince Guaraldi cover) | CSC Monthly Packs                | 6.455x             |
| ETERNAL NOW                                   | Paradigm                         | 6.433x             |
| Frankenstein                                  | Guitar Hero II DLC               | 6.432x             |
| Frankenstein                                  | Guitar Hero                      | 6.431x             |
| PN35                                          | CHARTS 2                         | 6.427x             |
| PN35                                          | Circuit Breaker                  | 6.427x             |
| Gamer National Anthem (feat. Coey)            | CSC Monthly Packs                | 6.418x             |
| Neonatalimpalionecrophiliation                | Carpal Tunnel Hero 2             | 6.415x             |
| Trippolette                                   | Guitar Hero                      | 6.366x             |
| Oblivion (Rockin' Night Style)                | DJMax Packs                      | 6.366x             |
| Tapping Boogie                                | Paradigm                         | 6.352x             |
| I'm So Sick                                   | Rock Band                        | 6.345x             |

There are 56 songs that cannot be 7 starred. 35 of them do not have any Star
Power; the 21 that do are as follows:

| Song                                    | Setlist                      | Average Multiplier |
| --------------------------------------- | ---------------------------- | ------------------ |
| All                                     | CSC Monthly Packs            | 1.016x             |
| No, All!                                | CSC Monthly Packs            | 1.095x             |
| March of the Machines                   | Marathon Hero 2 DLC          | 1.664x             |
| Curtain                                 | Marathon Hero 2 DLC          | 2.217x             |
| Daniel's Vision                         | Marathon Hero 2 DLC          | 3.039x             |
| They Don't Have To Believe...           | CHARTS                       | 3.594x             |
| Day One: Vigil                          | Marathon Hero 2 DLC          | 3.885x             |
| Uranoid                                 | CHARTS                       | 4.121x             |
| Tathagata                               | Marathon Hero 2 DLC          | 4.131x             |
| The Betrayal                            | Carpal Tunnel Hero 2         | 4.267x             |
| Bleed                                   | CHARTS                       | 4.311x             |
| Breakout (feat. Scandroid)              | Phase Shift Guitar Project 4 | 4.332x             |
| Luca (Demo)                             | Brand New Hero               | 4.335x             |
| Coffee Mug (Originally by Descendents)  | CHARTS                       | 4.335x             |
| Shards of Scorched Flesh                | Carpal Tunnel Hero 2         | 4.349x             |
| Cascading Failures, Diminishing Returns | Carpal Tunnel Hero 2         | 4.366x             |
| Built This Pool (blink-182 Cover)       | CHARTS                       | 4.368x             |
| The Pretender                           | Paradigm                     | 4.378x             |
| Infinitesimal to the Infinite           | Zero Gravity                 | 4.382x             |
| Get Possessed                           | Anti Hero                    | 4.386x             |
| Monkey Wrench                           | Guitar Hero II               | 4.400x             |

Note the average multiplier for Monkey Wrench is rounding from 4.39995x. Clone
Hero would display the average multiplier as 4.400x, but 5 more points would be
needed for the 7 star.
