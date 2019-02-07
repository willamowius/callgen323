H.323 Call Generator
====================

This call generator allows you to do load testing of H.323 endpoints,
gateways and gatekeepers.

It supports audio, video and H.239. It also supports H.235 AES media encoding
and RTP fuzzing to test the codecs.

License: MPL

Repository: https://github.com/willamowius/callgen323
Support:    https://www.willamowius.com/h323plus-support.html


HOW TO COMPILE
==============

On Linux, *BSD, Solaris or MacOS X:

Install gcc, OpenSSL dev package and all libraries that might be needed to compile H323Plus video codecs.

Get and compile PTLib:

cd ~
git clone https://github.com/willamowius/ptlib.git
cd ptlib
export PTLIBDIR=~/ptlib
./configure --enable-ipv6 --disable-odbc --disable-sdl --disable-lua --disable-expat
make debugnoshared

Get and compile H323Plus:

cd ~
git clone https://github.com/willamowius/h323plus.git
cd h323plus
export OPENH323DIR=~/h323plus
./configure --enable-h235 -enable-h46017 --enable-h46019m
make debugnoshared

Get and compile callgen323:

cd ~
git clone https://github.com/willamowius/callgen323.git
cd callgen323
make debugnoshared

Once the compile is finished, the binary can be found as

~/gnugk/obj_linux_x86_64_d_s/callgen323

(assuming you use a 64bit Linux system).


HOW TO RUN
==========

Every call has 2 sides: The dialing side and the side answering the call.
Callgen323 can act as either side (or both if you start 2 instances).

If you want to test a H.323 endpoint, you can let it wait for calls and
have callgen323 call it or you can start callgen323 in listening mode (-l) and
let the endpoint dial out to it.

if you want to test a gatekeeper or gateway, you would start one instance
of callgen323 in listening mode and one in dialing mode.

By default calls are made audio-only. Use command lines switches to enable video and H.239.

Examples
--------

Start in listening mode (no gatekeeper) and allow it to receive a maximum of 5 concurrent calls:
  callgen323 -n -m 5 -l

Start in dialing mode, 5 concurrent calls, dialing IP 1.2.3.4
  callgen -n -m 5 1.2.4

Start in dialing mode, register to a gatekeeper using H.460.18 and H.460.19 RTP multiplexing,
enable H.264 video and sending of H.239:
  PWLIBPLUGINDIR=/usr/local/lib/pwlib
  callgen323 -g 192.168.1.189 --h46018enable --h46019multiplexenable -b 768 -v -P H.264 --h239enable -m 10 -r 1 1.2.3.4

Make sure you have compiled and installed the H323Plus H.264 video codec in /usr/local/lib/pwlib before you do this.


You can run both instances in a single host if you want, as long as
you have two IP interfaces on your host. All you need to do is to
specify different IP or port to listen for each callgen (with
the -i option).

Audio files for OGM messages must be 16bit Microsoft PCM files
in WAV format at 8000 Hz (like the supplied ogm.wav).


Limitations
-----------

Establishing lots of calls uses lots of resources. Make sure the process get enough resources.
On Linux set

ulimit -n 10240
ulimit -s unlimited

You can also start multiple instances of callgen323 to produce more calls.


COMMAND LINE OPTIONS (SELECTED)
===============================
  -h                   Show usage with all command line options
  -l                   Passive/listening mode
  -m --max num         Maximum number of simultaneous calls
  -r --repeat num      Repeat calls n times
  -C --cycle           Each simultaneous call cycles through destination list
  -t --trace           Trace enable (use multiple times for more detail)
  -o --output file     Specify filename for trace output [stdout]
  -i --interface addr  Specify IP address and port listen on [*:1720]
  -g --gatekeeper host Specify gatekeeper host [auto-discover]
     --mediaenc        Enable Media encryption (value max cipher 128, 192 or 256)
     --maxtoken        Set max token size for H.235.6 (1024, 2048, 4096, ...)
  -k --h46017          Use H.460.17 Gatekeeper
  --h46018enable       Enable H.460.18/.19
  --h46019multiplexenable  Enable H.460.19 RTP multiplexing
  --h46023enable       Enable H.460.23/.24
  --h239enable         Enable sending and receiving H.239 presentations
  --h239videopattern   Set video pattern to send for H.239, eg. 'Fake', 'Fake/BouncingBoxes' or 'Fake/MovingBlocks'
  -n --no-gatekeeper   Disable gatekeeper discovery [false]
  --require-gatekeeper Exit if gatekeeper discovery fails [false]
  -u --user username   Specify local username [login name]
  -p --password pwd    Specify gatekeeper H.235 password [none]
  -P --prefer codec    Set codec preference (use multiple times) [none]
  -D --disable codec   Disable codec (use multiple times) [none]
  -b -- bandwidth kbps Specify bandwidth per call
  -v --video           Enable Video Support
     --videopattern    Set video pattern to send, eg. 'Fake', 'Fake/BouncingBoxes' or 'Fake/MovingBlocks'
  -R --framerate n     Set frame rate for outgoing video (fps)
  --maxframe name      Maximum Frame Size (qcif, cif, 4cif, 16cif, 480i, 720p, 1080i)
  -f --fast-disable    Disable fast start
  -T --h245tunneldisable  Disable H245 tunneling
  -O --out-msg file    Specify PCM16 WAV file for outgoing message [ogm.wav]
  -I --in-dir dir      Specify directory for incoming WAV files [disabled]
  -c --cdr file        Specify Call Detail Record file [none]
  --tcp-base port      Specific the base TCP port to use
  --tcp-max port       Specific the maximum TCP port to use
  --udp-base port      Specific the base UDP port to use
  --udp-max port       Specific the maximum UDP port to use
  --rtp-base port      Specific the base RTP/RTCP pair of UDP port to use
  --rtp-max port       Specific the maximum RTP/RTCP pair of UDP port to use
  --tmaxest  secs      Maximum time to wait for "Established" [0]
  --tmincall secs      Minimum call duration in seconds [10]
  --tmaxcall secs      Maximum call duration in seconds [30]
  --tminwait secs      Minimum interval between calls in seconds [10]
  --tmaxwait secs      Maximum interval between calls in seconds [30]
  --fuzzing            Enable RTP fuzzing
  --fuzz-header        Percentage of RTP header to randomly overwrite [50]
  --fuzz-media         Percentage of RTP media to randomly overwrite [0]
  --fuzz-rtcp          Percentage of RTCP to randomly overwrite [5]


