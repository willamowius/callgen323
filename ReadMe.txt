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
make optnoshared

Get and compile H323Plus:

cd ~
git clone https://github.com/willamowius/h323plus.git
cd h323plus
export OPENH323DIR=~/h323plus
./configure --enable-h235 -enable-h46017 --enable-h46019m
make optnoshared

Get and compile callgen323:

cd ~
git clone https://github.com/willamowius/callgen323.git
cd callgen323
make optnoshared

Once the compile is finished, the binary can be found as

~/callgen323/obj_linux_x86_64_d_s/callgen323

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
  callgen323 -n -m 5 1.2.3.4

Start in dialing mode, 100 concurrent calls, starting a new call every 2000 ms
(default is 800 ms) to avoid flooding the destination during ramp-up:
  callgen323 -n -m 100 -d 2000 1.2.3.4

Start in dialing mode, register to a gatekeeper using H.460.18 and H.460.19 RTP multiplexing,
enable H.264 video and sending of H.239:
  PWLIBPLUGINDIR=/usr/local/lib/pwlib
  callgen323 -g 192.168.1.189 --h46018enable --h46019multiplexenable -b 768 -v -P H.264 --h239enable -m 10 -r 1 1.2.3.4

Make sure you have compiled and installed the H323Plus H.264 video codec in /usr/local/lib/pwlib before you do this.

Start in listening mode, register to a gatekeeper as gateway with prefixes 49 and 0049,
so the gatekeeper routes calls for these prefixes to callgen323:
  callgen323 -g 192.168.1.189 --gateway 49,0049 -m 10 -l

Start in dialing mode and write RTP statistics for each call to a CSV file:
  callgen323 -n -m 5 --rtp-stats rtpstats.csv 1.2.3.4

Without -u, callgen323 registers with a random username (login name plus a random suffix),
so multiple instances can register to the same gatekeeper without alias conflicts.

Press Ctrl-C to stop callgen323. It will clear all calls and unregister from the
gatekeeper. Press Ctrl-C a 2nd time to exit immediately if the unregistration hangs.


Call Timing
-----------

With -m N, callgen323 runs N call threads in parallel. Each thread does:

  1. wait its start delay: thread n waits (n-1) * -d milliseconds [800 ms]
  2. make a call lasting a random time between --tmincall and --tmaxcall
  3. wait a random time between --tminwait and --tmaxwait seconds
  4. repeat from step 2 until -r calls are done

So -d and --tminwait/--tmaxwait control different things:

  -d                     Ramp-up only: spreads out the first call of each thread,
                         so the N initial calls don't hit the destination all at
                         once. It is in milliseconds and is not used again later.
  --tminwait/--tmaxwait  Pause within one thread between the end of one call
                         and its next call. In whole seconds, minimum 1.
                         They don't apply to a thread's first call.

After the first round, the threads drift apart because of the random call
durations and waits, so -d does not keep calls evenly spaced over a long run.

Example: -m 3 with the default -d 800 starts the first calls at 0 ms, 800 ms
and 1600 ms. After each call ends, that thread waits 10-30 s (default) before
calling again.


RTP Statistics
--------------

With --rtp-stats, callgen323 appends one line per RTP session (audio, video, H.239)
to the given CSV file when the session ends. If the file is empty, a header line is
written first. Columns:

  Time, Call Id, RTP Session Id, Packets sent, Octets sent, Packets received,
  Octets received, Packets lost, Packets out of order,
  Avg/Min/Max send time (ms), Avg/Min/Max receive time (ms),
  Avg jitter (ms), Max jitter (ms), First data received

The file contains one line per RTP session in a call, so for an audio call you'll
see one line, for a video calls, you'll see two lines that you can aggregate by Calld ID,
if you want. Only RTP session that actually transmit any packets are shown.


You can run multiple instances in a single host if you want, as long as
they use different ports or a different interface. All you need to do is to
specify different IP or port to listen for each callgen323 (with
the -i option, or with --listenport if you only want to change the port).

  callgen323 -l -i 192.168.1.10:1721
  callgen323 -l --listenport 1721
  callgen323 -l -i 192.168.1.10 --listenport 1721

If both -i and --listenport are given and -i includes a port, the two
ports must match, otherwise callgen323 prints an error and exits.

Audio files for OGM messages must be 16bit Microsoft PCM files
in WAV format at 8000 Hz (like the supplied ogm.wav).


Limitations
-----------

Establishing lots of calls uses lots of resources. Make sure the process get enough resources.
On Linux set

ulimit -n 10240
ulimit -s unlimited

You can also start multiple instances of callgen323 to produce more calls.


COMMAND LINE OPTIONS
====================
  -h                   Show usage with all command line options
  -l                   Passive/listening mode
  -m --max num         Maximum number of simultaneous calls [1]
     --mcu             Pose as MCU (to always win master/slave negotiation)
  -r --repeat num      Repeat calls n times per simultaneous call, 0 = infinite [10]
  -C --cycle           Each simultaneous call cycles through destination list
  -d --delay ms        Delay between the first calls of the simultaneous call threads in ms [800]
  -t --trace           Trace enable (use multiple times for more detail)
  -o --output file     Specify filename for trace output [stdout]
  -i --interface addr  Specify IP address and port listen on [*:1720]
     --listenport port Specify only the port to listen on [1720]
  -g --gatekeeper host Specify gatekeeper host [auto-discover]
     --gateway prefix  Register as gateway with prefix (use multiple times or comma separated)
  -a --access-token-oid oid  Set OID of the gatekeeper access token to use [none]
     --mediaenc        Enable Media encryption (value max cipher 128, 192 or 256)
     --maxtoken        Set max token size for H.235.6 (1024, 2048, 4096, ...)
  -k --h46017          Use H.460.17 Gatekeeper
  --h46018enable       Enable H.460.18/.19
  --h46019multiplexenable  Enable H.460.19 RTP multiplexing
  --h46023enable       Enable H.460.23/.24
  --h239enable         Enable sending and receiving H.239 presentations
  --h239videopattern   Set video pattern to send for H.239, eg. 'Fake', 'Fake/BouncingBoxes' or 'Fake/MovingBlocks'
  --h239delay          Delay the start of the H.239 transmission in seconds [1 sec]
  --h239duration       Duration of the H.239 transmission in seconds [-1 = unlimited]
  -n --no-gatekeeper   Disable gatekeeper discovery [false]
  --require-gatekeeper Exit if gatekeeper discovery fails [false]
  -u --user username   Specify local username [login name plus random suffix]
  -p --password pwd    Specify gatekeeper H.235 password [none]
  -P --prefer codec    Set codec preference (use multiple times) [none]
  -D --disable codec   Disable codec (use multiple times) [none]
  -b -- bandwidth kbps Specify bandwidth per call
  -v --video           Enable Video Support
     --videopattern    Set video pattern to send, eg. 'Fake', 'Fake/BouncingBoxes' or 'Fake/MovingBlocks'
  -R --framerate n     Set frame rate for outgoing video (fps)
  --maxframe name      Maximum Frame Size (cif, 4cif, 16cif, 480i, 720p, 1080i)
  --tls                TLS Enabled (must be set for TLS).
  --tls-cafile         TLS Certificate Authority File.
  --tls-cert           TLS Certificate File.
  --tls-privkey        TLS Private Key File.
  --tls-passphrase     TLS Private Key PassPhrase.
  --tls-listenport     TLS listen port (default: 1300).
  -f --fast-disable    Disable fast start
  -T --h245tunneldisable  Disable H245 tunneling
  -O --out-msg file    Specify PCM16 WAV file for outgoing message [ogm.wav]
  -I --in-dir dir      Specify directory for incoming WAV files [disabled]
  -c --cdr file        Specify Call Detail Record file [none]
     --rtp-stats file  Specify CSV file for RTP statistics at end of each RTP session [none]
  --tcp-base port      Specific the base TCP port to use
  --tcp-max port       Specific the maximum TCP port to use
  --udp-base port      Specific the base UDP port to use
  --udp-max port       Specific the maximum UDP port to use
  --rtp-base port      Specific the base RTP/RTCP pair of UDP port to use
  --rtp-max port       Specific the maximum RTP/RTCP pair of UDP port to use
  --tmaxest  secs      Maximum time to wait for "Established" [0]
  --tmincall secs      Minimum call duration in seconds [10]
  --tmaxcall secs      Maximum call duration in seconds [60]
  --tminwait secs      Minimum wait after a call ends before the same thread calls again [10]
  --tmaxwait secs      Maximum wait after a call ends before the same thread calls again [30]
  --fuzzing            Enable RTP fuzzing
  --fuzz-header        Percentage of RTP header to randomly overwrite [50]
  --fuzz-media         Percentage of RTP media to randomly overwrite [0]
  --fuzz-rtcp          Percentage of RTCP to randomly overwrite [5]


