/*
 * main.cxx
 *
 * H.323 call generator
 *
 * Copyright (c) 2001 Benny L. Prijono <seventhson@theseventhson.freeserve.co.uk>
 * Copyright (c) 2008-2018 Jan Willamowius <jan@willamowius.de>
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * https://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is CallGen323.
 *
 * The Initial Developer of the Original Code is Benny L. Prijono
 *
 * Contributor(s): Equivalence Pty. Ltd.
 *
 */

#include <ptlib.h>
#include "main.h"
#include "version.h"

#include <ptclib/random.h>
#include <ptlib/video.h>
#ifndef _WIN32
#include <signal.h>
#endif

PCREATE_PROCESS(CallGen);

///////////////////////////////////////////////////////////////////////////////

CallGen::CallGen()
  : PProcess("H323Plus", "CallGen", MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER),
    console(PConsoleChannel::StandardInput)
{
  totalAttempts = 0;
  totalEstablished = 0;
  h323 = NULL;
}

void CallGen::Main()
{
#ifndef _WIN32
  signal(SIGCHLD, SIG_IGN);	// avoid zombies from H.264 plugin helper
#endif

  PArgList & args = GetArguments();
  args.Parse("a-access-token-oid:"
             "b-bandwidth:"
             "c-cdr:"
             "C-cycle."
             "D-disable:"
             "f-fast-disable."
             "g-gatekeeper:"
#ifdef H323_H235
             "-mediaenc:"
             "-maxtoken:"
#endif
#ifdef H323_H46017
             "k-h46017:"
#endif
#ifdef H323_H46018
             "-h46018enable."
#endif
#ifdef H323_H46019M
             "-h46019multiplexenable."
#endif
#ifdef H323_H46023
             "-h46023enable."
#endif
             "I-in-dir:"
             "i-interface:"
             "l-listen."
             "m-max:"
             "n-no-gatekeeper."
             "O-out-msg:"
             "o-output:"
             "P-prefer:"
             "p-password:"
             "r-repeat:"
             "-require-gatekeeper."
             "T-h245tunneldisable."
             "t-trace."
#ifdef H323_VIDEO
			 "v-video."
			 "-videopattern:"
			 "R-framerate:"
			 "-maxframe."
#endif
             "-tmaxest:"
             "-tmincall:"
             "-tmaxcall:"
             "-tminwait:"
             "-tmaxwait:"
             "-tcp-base:"
             "-tcp-max:"
             "-udp-base:"
             "-udp-max:"
             "-rtp-base:"
             "-rtp-max:"
             "u-user:"
             "-fuzzing."
             "-fuzz-header:"
             "-fuzz-media:"
             "-fuzz-rtcp:"
             , FALSE);

  if (args.GetCount() == 0 && !args.HasOption('l')) {
    cout << "Usage:\n"
            "  callgen [options] -l\n"
            "  callgen [options] destination [ destination ... ]\n"
            "where options:\n"
            "  -l                   Passive/listening mode\n"
            "  -m --max num         Maximum number of simultaneous calls\n"
            "  -r --repeat num      Repeat calls n times\n"
            "  -C --cycle           Each simultaneous call cycles through destination list\n"
            "  -t --trace           Trace enable (use multiple times for more detail)\n"
            "  -o --output file     Specify filename for trace output [stdout]\n"
            "  -i --interface addr  Specify IP address and port listen on [*:1720]\n"
            "  -g --gatekeeper host Specify gatekeeper host [auto-discover]\n"
#ifdef H323_H235
            "     --mediaenc        Enable Media encryption (value max cipher 128, 192 or 256)\n"
            "     --maxtoken        Set max token size for H.235.6 (1024, 2048, 4096, ...)\n"
#endif
#ifdef H323_H46017
            "  -k --h46017          Use H.460.17 Gatekeeper\n"
#endif
#ifdef H323_H46018
            "  --h46018enable       Enable H.460.18/.19\n"
#endif
#ifdef H323_H46019M
            "  --h46019multiplexenable  Enable H.460.19 RTP multiplexing\n"
#endif
#ifdef H323_H46023
            "  --h46023enable       Enable H.460.23/.24\n"
#endif
            "  -n --no-gatekeeper   Disable gatekeeper discovery [false]\n"
            "  --require-gatekeeper Exit if gatekeeper discovery fails [false]\n"
            "  -u --user username   Specify local username [login name]\n"
            "  -p --password pwd    Specify gatekeeper H.235 password [none]\n"
            "  -P --prefer codec    Set codec preference (use multiple times) [none]\n"
            "  -D --disable codec   Disable codec (use multiple times) [none]\n"
            "  -b -- bandwidth kbps Specify bandwidth per call\n"
#ifdef H323_VIDEO
            "  -v --video           Enable Video Support\n"
            "     --videopattern    Set video pattern to send, eg. 'Fake/BouncingBoxes' or 'Fake/MovingBlocks'\n"
            "  -R --framerate n     Set frame rate for outgoing video (fps)\n"
            "  --maxframe           Maximum Frame Size\n"
#endif
            "  -f --fast-disable    Disable fast start\n"
            "  -T --h245tunneldisable  Disable H245 tunneling\n"
            "  -O --out-msg file    Specify PCM16 WAV file for outgoing message [ogm.wav]\n"
            "  -I --in-dir dir      Specify directory for incoming WAV files [disabled]\n"
            "  -c --cdr file        Specify Call Detail Record file [none]\n"
            "  --tcp-base port      Specific the base TCP port to use\n"
            "  --tcp-max port       Specific the maximum TCP port to use\n"
            "  --udp-base port      Specific the base UDP port to use\n"
            "  --udp-max port       Specific the maximum UDP port to use\n"
            "  --rtp-base port      Specific the base RTP/RTCP pair of UDP port to use\n"
            "  --rtp-max port       Specific the maximum RTP/RTCP pair of UDP port to use\n"
            "  --tmaxest  secs      Maximum time to wait for \"Established\" [0]\n"
            "  --tmincall secs      Minimum call duration in seconds [10]\n"
            "  --tmaxcall secs      Maximum call duration in seconds [30]\n"
            "  --tminwait secs      Minimum interval between calls in seconds [10]\n"
            "  --tmaxwait secs      Maximum interval between calls in seconds [30]\n"
            "  --fuzzing            Enable RTP fuzzing\n"
            "  --fuzz-header        Percentage of RTP header to randomly overwrite [50]\n"
            "  --fuzz-media         Percentage of RTP media to randomly overwrite [0]\n"
            "  --fuzz-rtcp          Percentage of RTCP to randomly overwrite [5]\n"
            "\n"
            "Notes:\n"
            "  If --tmaxest is set a non-zero value then --tmincall is the time to leave\n"
            "  the call running once established. If zero (the default) then --tmincall\n"
            "  is the length of the call from initiation. The call may or may not be\n"
            "  \"answered\" within that time.\n"
            "\n";
    return;
  }

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
		             PTrace::DateAndTime | PTrace::TraceLevel | PTrace::FileAndLine);
#endif

  h323 = new MyH323EndPoint();

  outgoingMessageFile = args.GetOptionString('O', "ogm.wav");
  if (outgoingMessageFile.IsEmpty())
    cout << "Not using outgoing message file." << endl;
  else if (PFile::Exists(outgoingMessageFile))
    cout << "Using outgoing message file: " << outgoingMessageFile << endl;
  else {
    cout << "Outgoing message file  \"" << outgoingMessageFile << "\" does not exist!" << endl;
    PTRACE(1, "CallGen\tOutgoing message file \"" << outgoingMessageFile << "\" does not exist");
    outgoingMessageFile = PString::Empty();
  }

  incomingAudioDirectory = args.GetOptionString('I');
  if (incomingAudioDirectory.IsEmpty())
    cout << "Not saving incoming audio data." << endl;
  else if (PDirectory::Exists(incomingAudioDirectory) ||
           PDirectory::Create(incomingAudioDirectory)) {
    incomingAudioDirectory = PDirectory(incomingAudioDirectory);
    cout << "Using incoming audio directory: " << incomingAudioDirectory << endl;
  }
  else {
    cout << "Could not create incoming audio directory \"" << incomingAudioDirectory << "\"!" << endl;
    PTRACE(1, "CallGen\tCould not create incoming audio directory \"" << incomingAudioDirectory << '"');
    incomingAudioDirectory = PString::Empty();
  }

  // start the H.323 listener
  H323ListenerTCP * listener = NULL;
  PIPSocket::Address interfaceAddress(INADDR_ANY);
  WORD listenPort = H323EndPoint::DefaultTcpPort;
  if (args.HasOption('i')) {
    PString interface = args.GetOptionString('i');
    PINDEX colon = interface.Find(":");
    if (colon != P_MAX_INDEX) {
      interfaceAddress = interface.Left(colon);
      listenPort = interface.Mid(colon + 1).AsUnsigned();
    } else {
      interfaceAddress = interface;
    }
  }

  listener = new H323ListenerTCP(*h323, interfaceAddress, listenPort);

  if (!h323->StartListener(listener)) {
    cout << "Could not open H.323 listener port on " << interfaceAddress << ":" << listener->GetListenerPort() << endl;
    delete listener;
    return;
  }

  cout << "H.323 listening on: " << setfill(',') << h323->GetListeners() << setfill(' ') << endl;

  if (args.HasOption('c')) {
    if (cdrFile.Open(args.GetOptionString('c'), PFile::WriteOnly, PFile::Create)) {
      cdrFile.SetPosition(0, PFile::End);
      PTRACE(1, "CallGen\tSetting CDR to \"" << cdrFile.GetFilePath() << '"');
      cout << "Sending Call Detail Records to \"" << cdrFile.GetFilePath() << '"' << endl;
    }
    else {
      cout << "Could not open \"" << cdrFile.GetFilePath() << "\"!" << endl;
    }
  }

  if (args.HasOption("tcp-base"))
    h323->SetTCPPorts(args.GetOptionString("tcp-base").AsUnsigned(),
                     args.GetOptionString("tcp-max").AsUnsigned());
  if (args.HasOption("udp-base"))
    h323->SetUDPPorts(args.GetOptionString("udp-base").AsUnsigned(),
                     args.GetOptionString("udp-max").AsUnsigned());
  if (args.HasOption("rtp-base"))
    h323->SetRtpIpPorts(args.GetOptionString("rtp-base").AsUnsigned(),
                       args.GetOptionString("rtp-max").AsUnsigned());

  h323->RemoveCapabilities(args.GetOptionString('D').Lines());
  h323->ReorderCapabilities(args.GetOptionString('P').Lines());
  cout << "Local capabilities:\n" << h323->GetCapabilities() << endl;

  // set local username, is necessary
  if (args.HasOption('u')) {
    PStringArray aliases = args.GetOptionString('u').Lines();
    h323->SetLocalUserName(aliases[0]);
    for (PINDEX i = 1; i < aliases.GetSize(); ++i)
      h323->AddAliasName(aliases[i]);
  }
  cout << "Local username: \"" << h323->GetLocalUserName() << '"' << endl;

  if (args.HasOption('p')) {
    h323->SetGatekeeperPassword(args.GetOptionString('p'));
    cout << "Using H.235 security." << endl;
  }

  if (args.HasOption('a')) {
    h323->SetGkAccessTokenOID(args.GetOptionString('a'));
    cout << "Set Access Token OID to \"" << h323->GetGkAccessTokenOID() << '"' << endl;
  }

  // process gatekeeper registration options
#ifdef H323_H46017
  if (args.HasOption('k')) {
    PString gk17 = args.GetOptionString('k');
    if (h323->H46017CreateConnection(gk17, false)) {
      PTRACE(2, "Using H.460.17 Gatekeeper Tunneling.");
    } else {
      cout << "Error: H.460.17 Gatekeeper Tunneling Failed: Gatekeeper=" << gk17 << endl;
      return;
    }
  } else
#endif
  {
    if (args.HasOption('g')) {
#ifdef H323_H46018
      cout << "H.460.18/.19: " << (args.HasOption("h46018enable") ? "enabled" : "disabled") << endl;
      h323->H46018Enable(args.HasOption("h46018enable"));
#endif
#ifdef H323_H46019M
      cout << "H.460.19 RTP multiplexing: " << (args.HasOption("h46019multiplexenable") ? "enabled" : "disabled") << endl;
      h323->H46019MEnable(args.HasOption("h46019multiplexenable"));
      h323->H46019MSending(args.HasOption("h46019multiplexenable"));
#endif
#ifdef H323_H46023
      cout << "H.460.23/.24: " << (args.HasOption("h46023enable") ? "enabled" : "disabled") << endl;
      h323->H46023Enable(args.HasOption("h46023enable"));
#endif
      PString gkAddr = args.GetOptionString('g');
      cout << "Registering with gatekeeper \"" << gkAddr << "\" ..." << flush;
      if (h323->SetGatekeeper(gkAddr, new H323TransportUDP(*h323, interfaceAddress))) {
        cout << "\nGatekeeper set to \"" << *h323->GetGatekeeper() << '"' << endl;
      } else {
        cout << "\nError registering with gatekeeper at \"" << gkAddr << '"' << endl;
        return;
      }
    }
    else if (!args.HasOption('n')) {
      cout << "Searching for gatekeeper ..." << flush;
      if (h323->UseGatekeeper())
        cout << "\nGatekeeper found: " << *h323->GetGatekeeper() << endl;
      else {
        cout << "\nNo gatekeeper found." << endl;
        if (args.HasOption("require-gatekeeper"))
          return;
      }
    }
  }

  if (args.HasOption('f'))
    h323->DisableFastStart(TRUE);
  if (args.HasOption('T'))
    h323->DisableH245Tunneling(TRUE);

#ifdef H323_H235
  if (args.HasOption("mediaenc"))  {
    H323EndPoint::H235MediaCipher ncipher = H323EndPoint::encypt128;
#ifdef H323_H235_AES256
    unsigned maxtoken = 2048;
    unsigned cipher = args.GetOptionString("mediaenc").AsInteger();
    if (cipher >= H323EndPoint::encypt192) ncipher = H323EndPoint::encypt192;
    if (cipher >= H323EndPoint::encypt256) ncipher = H323EndPoint::encypt256;
    if (args.HasOption("maxtoken")) {
      maxtoken = args.GetOptionString("maxtoken").AsInteger();
    }
#else
    unsigned maxtoken = 1024;
#endif
    h323->SetH235MediaEncryption(H323EndPoint::encyptRequest, ncipher, maxtoken);
    cout << "Enabled Media Encryption AES" << ncipher << endl;
  }
#endif

  unsigned bandwidth = 768; // default to 768 kbps
  if (args.HasOption('b')) {
    bandwidth = args.GetOptionString('b').AsUnsigned();
  }
  cout << "Per call bandwidth: " << bandwidth << " kbps" << endl;
  h323->SetPerCallBandwidth(bandwidth);

#ifdef H323_VIDEO
  if (!args.HasOption('v')) {
    cout << "Video is disabled" << endl;
    h323->RemoveCapability(H323Capability::e_Video);
  }
  PString videoPattern = "Fake/MovingBlocks";
  if (args.HasOption("videopattern")) {
    // options for demo pattern include: Fake/MovingLine, Fake/BouncingBoxes, Text
    videoPattern = args.GetOptionString("videopattern");
  }
  h323->SetVideoPattern(videoPattern);

  if (args.HasOption('R')) {
    h323->SetFrameRate(args.GetOptionString('R').AsUnsigned());
  }

  if (args.HasOption("maxframe")) {
    PCaselessString maxframe = args.GetOptionString("maxframe");
	if (maxframe == "qcif")
		h323->SetVideoFrameSize(H323Capability::qcifMPI);
	else if (maxframe == "cif")
	    h323->SetVideoFrameSize(H323Capability::cifMPI);
	else if (maxframe == "4cif")
        h323->SetVideoFrameSize(H323Capability::cif4MPI);
	else if (maxframe == "16cif")
        h323->SetVideoFrameSize(H323Capability::cif16MPI);
  }
#endif

  if (args.HasOption("fuzzing")) {
      h323->SetFuzzing(true);
  }
  if (args.HasOption("fuzz-header")) {
      h323->SetPercentBadRTPHeader(args.GetOptionString("fuzz-header").AsUnsigned());
  }
  if (args.HasOption("fuzz-media")) {
      h323->SetPercentBadRTPMedia(args.GetOptionString("fuzz-media").AsUnsigned());
  }
  if (args.HasOption("fuzz-rtcp")) {
      h323->SetPercentBadRTCP(args.GetOptionString("fuzz-rtcp").AsUnsigned());
  }

  if (args.HasOption('l')) {
    cout << "Endpoint is listening for incoming calls, press ENTER to exit.\n";
    console.ReadChar();
    h323->ClearAllCalls();
  }
  else {
    CallParams params(*this);
    params.tmax_est .SetInterval(0, args.GetOptionString("tmaxest",  "0" ).AsUnsigned());
    params.tmin_call.SetInterval(0, args.GetOptionString("tmincall", "10").AsUnsigned());
    params.tmax_call.SetInterval(0, args.GetOptionString("tmaxcall", "60").AsUnsigned());
    params.tmin_wait.SetInterval(0, args.GetOptionString("tminwait", "10").AsUnsigned());
    params.tmax_wait.SetInterval(0, args.GetOptionString("tmaxwait", "30").AsUnsigned());

    if (params.tmin_call == 0 ||
        params.tmin_wait == 0 ||
        params.tmin_call > params.tmax_call ||
        params.tmin_wait > params.tmax_wait) {
      cerr << "Invalid times entered!\n";
      return;
    }

    unsigned number = args.GetOptionString('m').AsUnsigned();
    if (number == 0)
      number = 1;
    cout << "Endpoint starting " << number << " simultaneous call";
    if (number > 1)
      cout << 's';
    cout << ' ';

    params.repeat = args.GetOptionString('r', "10").AsUnsigned();
    if (params.repeat != 0)
      cout << params.repeat;
    else
      cout << "infinite";
    cout << " time";
    if (params.repeat != 1)
      cout << 's';
    if (params.repeat != 0)
      cout << ", grand total of " << number*params.repeat << " calls";
    cout << '.' << endl;

    // create some threads to do calls, but start them randomly
    for (unsigned idx = 0; idx < number; idx++) {
      if (args.HasOption('C'))
        threadList.Append(new CallThread(idx+1, args.GetParameters(), params));
      else {
        PINDEX arg = idx % args.GetCount();
        threadList.Append(new CallThread(idx+1, args.GetParameters(arg, arg), params));
      }
    }

    PThread::Create(PCREATE_NOTIFIER(Cancel), 0);

    for (;;) {
      threadEnded.Wait();
      PThread::Sleep(100);

      PBoolean finished = TRUE;
      for (PINDEX i = 0; i < threadList.GetSize(); i++) {
        if (!threadList[i].IsTerminated()) {
          finished = FALSE;
          break;
        }
      }

      if (finished) {
        cout << "\nAll call sets completed." << endl;
        console.Close();
        break;
      }
    }
  }

  if (totalAttempts > 0)
    cout << "Total calls: " << totalAttempts
         << " attempted, " << totalEstablished << " established\n";

  // delete endpoint object so we unregister cleanly
  delete h323;
}

void CallGen::Cancel(PThread &, INT)
{
  PTRACE(3, "CallGen\tCancel thread started.");

  coutMutex.Wait();
  cout << "Press ENTER at any time to quit.\n" << endl;
  coutMutex.Signal();

  // wait for a keypress
  while (console.ReadChar() != '\n') {
    if (!console.IsOpen()) {
      PTRACE(3, "CallGen\tCancel thread ended.");
      return;
    }
  }

  PTRACE(2, "CallGen\tCancelling calls.");

  coutMutex.Wait();
  cout << "\nAborting all calls ..." << endl;
  coutMutex.Signal();

  // stop threads
  for (PINDEX i = 0; i < threadList.GetSize(); i++)
    threadList[i].Stop();

  // stop all calls
  CallGen::Current().ClearAll();

  PTRACE(1, "CallGen\tCancelled calls.");
}

///////////////////////////////////////////////////////////////////////////////

CallThread::CallThread(unsigned _index, const PStringArray & _destinations, const CallParams & _params)
  : PThread(1000, NoAutoDeleteThread, NormalPriority, psprintf("CallGen %u", _index)),
    destinations(_destinations),
    index(_index),
    params(_params)
{
  Resume();
}

static unsigned RandomRange(PRandom & rand, const PTimeInterval & tmin, const PTimeInterval & tmax)
{
  unsigned umax = tmax.GetInterval();
  unsigned umin = tmin.GetInterval();
  return rand.Generate() % (umax - umin + 1) + umin;
}

#define START_OUTPUT(index, token) \
{ \
  CallGen::Current().coutMutex.Wait(); \
  cout << setw(3) << index << ": " << setw(20) << token.Left(20) << ": "

#define END_OUTPUT() \
  cout << endl; \
  CallGen::Current().coutMutex.Signal(); \
}

#define OUTPUT(index, token, info) START_OUTPUT(index, token) << info; END_OUTPUT()

void CallThread::Main()
{
  PTRACE(2, "CallGen\tStarted thread " << index);

  CallGen & callgen = CallGen::Current();
  PRandom rand(PRandom::Number());

  PTimeInterval delay = RandomRange(rand, (index-1)*500, (index+1)*500);
  OUTPUT(index, PString::Empty(), "Initial delay of " << delay << " seconds");

  if (exit.Wait(delay)) {
    PTRACE(2, "CallGen\tAborted thread " << index);
    callgen.threadEnded.Signal();
    return;
  }

  // Loop "repeat" times for (repeat > 0), or loop forever for (repeat == 0)
  unsigned count = 1;
  do {
    PString destination = destinations[(index-1 + count-1) % destinations.GetSize()];

    // trigger a call
    PString token;
    PTRACE(1, "CallGen\tMaking call to " << destination);
    unsigned totalAttempts = ++callgen.totalAttempts;
    if (!callgen.Start(destination, token))
      PError << setw(3) << index << ": Call creation to " << destination << " failed" << endl;
    else {
      PBoolean stopping = FALSE;

      delay = RandomRange(rand, params.tmin_call, params.tmax_call);

      START_OUTPUT(index, token) << "Making call " << count;
      if (params.repeat)
        cout << " of " << params.repeat;
      cout << " (total=" << totalAttempts
           << ") for " << delay << " seconds to "
           << destination;
      END_OUTPUT();

      if (params.tmax_est > 0) {
        OUTPUT(index, token, "Waiting " << params.tmax_est << " seconds for establishment");

        PTimer timeout = params.tmax_est;
        while (!callgen.IsEstablished(token)) {
          stopping = exit.Wait(100);
          if (stopping || !timeout.IsRunning() || !callgen.Exists(token)) {
            delay = 0;
            break;
          }
        }
      }

      if (delay > 0) {
        // wait for a random time
        PTRACE(1, "CallGen\tWaiting for " << delay);
        stopping = exit.Wait(delay);
      }

      // end the call
      OUTPUT(index, token, "Clearing call");

      callgen.Clear(token);

      if (stopping)
        break;
    }

    count++;
    if (params.repeat > 0 && count > params.repeat)
      break;

    // wait for a random delay
    delay = RandomRange(rand, params.tmin_wait, params.tmax_wait);
    OUTPUT(index, PString::Empty(), "Delaying for " << delay << " seconds");

    PTRACE(1, "CallGen\tDelaying for " << delay);
    // wait for a random time
  } while (!exit.Wait(delay));

  OUTPUT(index, PString::Empty(), "Completed call set.");
  PTRACE(2, "CallGen\tFinished thread " << index);

  callgen.threadEnded.Signal();
}

void CallThread::Stop()
{
  if (!IsTerminated())
    OUTPUT(index, PString::Empty(), "Stopping.");

  exit.Signal();
}

///////////////////////////////////////////////////////////////////////////////

void CallDetail::Drop(H323Connection & connection)
{
  PTextFile & cdrFile = CallGen::Current().cdrFile;

  if (!cdrFile.IsOpen())
    return;

  static PMutex cdrMutex;
  cdrMutex.Wait();

  if (cdrFile.GetLength() == 0)
    cdrFile << "Call Start Time,"
               "Total duration,"
               "Media open transmit time,"
               "Media open received time,"
               "Media received time,"
               "ALERTING time,"
               "CONNECT time,"
               "Call End Reason,"
               "Remote party,"
               "Signaling gateway,"
               "Media gateway,"
               "Call Id,"
               "Call Token\n";

  PTime setupTime = connection.GetSetupUpTime();

  cdrFile << setupTime.AsString("yyyy/M/d hh:mm:ss") << ','
          << setprecision(1) << (connection.GetConnectionEndTime() - setupTime) << ',';

  if (openedTransmitMedia.IsValid())
    cdrFile << (openedTransmitMedia - setupTime);
  cdrFile << ',';

  if (openedReceiveMedia.IsValid())
    cdrFile << (openedReceiveMedia - setupTime);
  cdrFile << ',';

  if (receivedMedia.IsValid())
    cdrFile << (receivedMedia - setupTime);
  cdrFile << ',';

  if (connection.GetAlertingTime().IsValid())
    cdrFile << (connection.GetAlertingTime() - setupTime);
  cdrFile << ',';

  if (connection.GetConnectionStartTime().IsValid())
    cdrFile << (connection.GetConnectionStartTime() - setupTime);
  cdrFile << ',';

  cdrFile << connection.GetCallEndReason() << ','
          << connection.GetRemotePartyName() << ','
          << connection.GetRemotePartyAddress() << ','
          << mediaGateway << ','
          << connection.GetCallIdentifier() << ','
          << connection.GetCallToken()
          << endl;

  cdrMutex.Signal();
}

void CallDetail::OnRTPStatistics(const RTP_Session & session, const PString & token)
{
  if (session.GetSessionID() == 1 && !receivedAudio) {
    receivedAudio = true;
    OUTPUT("", token, "Received audio");
  }
  if (session.GetSessionID() == 2 && !receivedVideo) {
    receivedVideo = true;
    OUTPUT("", token, "Received video");
  }
  if (receivedMedia.GetTimeInSeconds() == 0 && session.GetPacketsReceived() > 0) {
    receivedMedia = PTime();

    const RTP_UDP * udpSess = dynamic_cast<const RTP_UDP *>(&session);
    if (udpSess != NULL)
      mediaGateway = H323TransportAddress(udpSess->GetRemoteAddress(), udpSess->GetRemoteDataPort());
  }
}

///////////////////////////////////////////////////////////////////////////////

MyH323EndPoint::MyH323EndPoint()
{
  // load plugins for H.460.17, .18 etc.
  LoadBaseFeatureSet();

  // Set capability
  AddAllCapabilities(0, 0, "*");
  AddAllUserInputCapabilities(0, P_MAX_INDEX);
  SetPerCallBandwidth(384);
  SetVideoPattern("Fake/MovingBlocks");
  SetFrameRate(30);
  useJitterBuffer = false; // save a little processing time
  SetFuzzing(false);
  SetPercentBadRTPHeader(50);
  SetPercentBadRTPMedia(0);
  SetPercentBadRTCP(5);
}

H323Connection * MyH323EndPoint::CreateConnection(unsigned callReference)
{
  return new MyH323Connection(*this, callReference);
}

static PString TidyRemotePartyName(const H323Connection & connection)
{
  PString name = connection.GetRemotePartyName();

  PINDEX bracket = name.FindLast('[');
  if (bracket == 0 || bracket == P_MAX_INDEX)
    return name;

  return name.Left(bracket).Trim();
}

void MyH323EndPoint::OnConnectionEstablished(H323Connection & connection, const PString & token)
{
  OUTPUT("", token, "Established \"" << TidyRemotePartyName(connection) << "\""
                    " " << connection.GetControlChannel().GetRemoteAddress() <<
                    " active=" << connectionsActive.GetSize() <<
                    " total=" << ++CallGen::Current().totalEstablished);
}

void MyH323EndPoint::OnConnectionCleared(H323Connection & connection, const PString & token)
{
  OUTPUT("", token, "Cleared \"" << TidyRemotePartyName(connection) << "\""
                    " " << connection.GetControlChannel().GetRemoteAddress() <<
                    " reason=" << connection.GetCallEndReason());
  ((MyH323Connection&)connection).details.Drop(connection);
}

PBoolean MyH323EndPoint::OnStartLogicalChannel(H323Connection & connection, H323Channel & channel)
{
  (channel.GetDirection() == H323Channel::IsTransmitter
        ? ((MyH323Connection&)connection).details.openedTransmitMedia
        : ((MyH323Connection&)connection).details.openedReceiveMedia) = PTime();

  OUTPUT("", connection.GetCallToken(),
         "Opened " << (channel.GetDirection() == H323Channel::IsTransmitter ? "transmitter" : "receiver")
                   << " for " << channel.GetCapability());

  return H323EndPoint::OnStartLogicalChannel(connection, channel);
}

///////////////////////////////////////////////////////////////////////////////

MyH323Connection::MyH323Connection(MyH323EndPoint & ep, unsigned callRef)
  : H323Connection(ep, callRef), endpoint(ep), videoChannelIn(NULL), videoChannelOut(NULL)
{
    detectInBandDTMF = FALSE; // turn off in-band DTMF detection (uses a huge amount of CPU)
}

MyH323Connection::~MyH323Connection()
{
    delete videoChannelIn;
    delete videoChannelOut;
}

PBoolean MyH323Connection::OnSendSignalSetup(H323SignalPDU & setupPDU)
{
    // set outgoing bearer capability to unrestricted information transfer + transfer rate
	PBYTEArray caps;
	caps.SetSize(4);
	caps[0] = 0x88;
	caps[1] = 0x18;
	caps[2] = 0x80 | endpoint.GetRateMultiplier();
	caps[3] = 0xa5;
	setupPDU.GetQ931().SetIE(Q931::BearerCapabilityIE, caps);

    return H323Connection::OnSendSignalSetup(setupPDU);
}

H323Channel * MyH323Connection::CreateRealTimeLogicalChannel(const H323Capability & capability, H323Channel::Directions dir,
                                                unsigned sessionID, const H245_H2250LogicalChannelParameters * param, RTP_QOS * rtpqos)
{
    if (endpoint.IsFuzzing()) {
        WORD rtpPort = 0;
        map<unsigned, WORD>::const_iterator iter = m_sessionPorts.find(sessionID);
        if (iter != m_sessionPorts.end()) {
            rtpPort = iter->second;
        } else {
            rtpPort = endpoint.GetRtpIpPortPair();
            m_sessionPorts[sessionID] = rtpPort;
        }
        return new RTPFuzzingChannel(endpoint, *this, capability, dir, sessionID, rtpPort, rtpPort+1);
    } else {
        // call super class
        return H323Connection::CreateRealTimeLogicalChannel(capability, dir, sessionID, param, rtpqos);
    }
}

void MyH323Connection::OnRTPStatistics(const RTP_Session & session) const
{
  ((MyH323Connection *)this)->details.OnRTPStatistics(session, GetCallToken());
}

PBoolean MyH323Connection::OpenAudioChannel(PBoolean isEncoding, unsigned bufferSize, H323AudioCodec & codec)
{

  unsigned frameDelay = bufferSize/16; // assume 16 bit PCM

  PIndirectChannel * channel;
  if (isEncoding)
    channel = new PlayMessage(CallGen::Current().outgoingMessageFile, frameDelay, bufferSize);
  else {
    PString wavFileName;
    if (!CallGen::Current().incomingAudioDirectory) {
      PString token = GetCallToken();
      token.Replace("/", "_", TRUE);
      wavFileName = CallGen::Current().incomingAudioDirectory + token;
    }
    channel = new RecordMessage(wavFileName, frameDelay, bufferSize);
  }

  codec.AttachChannel(channel);

  return TRUE;
}

#ifdef H323_VIDEO
PBoolean MyH323Connection::OpenVideoChannel(PBoolean isEncoding, H323VideoCodec & codec)
{
  PString deviceName = isEncoding ? endpoint.GetVideoPattern() : "NULL";

  PVideoDevice * device = isEncoding ? (PVideoDevice *)PVideoInputDevice::CreateDeviceByName(deviceName)
                                     : (PVideoDevice *)PVideoOutputDevice::CreateDeviceByName(deviceName);

  // codec needs a list of possible formats, otherwise the frame size isn't negotiated properly
#if PTLIB_VER >= 2110
  if (isEncoding) {
      PVideoInputDevice::Capabilities videoCaps;
      if (((PVideoInputDevice *)device)->GetDeviceCapabilities(deviceName,deviceDriver,&videoCaps)) {
          codec.SetSupportedFormats(videoCaps.framesizes);
      } else {
        // set fixed list of resolutions for drivers that don't provide a list
        PVideoInputDevice::Capabilities caps;
        PVideoFrameInfo cap;
        cap.SetColourFormat("YUV420P");
        cap.SetFrameRate(endpoint.GetFrameRate());
        // sizes must be from largest to smallest
        cap.SetFrameSize(1920, 1080);
        caps.framesizes.push_back(cap);
        cap.SetFrameSize(1280, 720);
        caps.framesizes.push_back(cap);
        cap.SetFrameSize(704, 576);
        caps.framesizes.push_back(cap);
        cap.SetFrameSize(352, 288);
        caps.framesizes.push_back(cap);
        codec.SetSupportedFormats(caps.framesizes);
      }
  }
#else
  if (isEncoding) {
    PVideoInputDevice::Capabilities caps;
    PVideoFrameInfo cap;
    cap.SetColourFormat("YUV420P");
    cap.SetFrameRate(endpoint.GetFrameRate());
    // sizes must be from largest to smallest
    cap.SetFrameSize(1920, 1080);
    caps.framesizes.push_back(cap);
    cap.SetFrameSize(1280, 720);
    caps.framesizes.push_back(cap);
    cap.SetFrameSize(704, 576);
    caps.framesizes.push_back(cap);
    cap.SetFrameSize(640, 400);
    caps.framesizes.push_back(cap);
    cap.SetFrameSize(352, 288);
    caps.framesizes.push_back(cap);
    codec.SetSupportedFormats(caps.framesizes);
  }
#endif

  unsigned frameWidth = codec.GetWidth();
  unsigned frameHeight = codec.GetHeight();
  PTRACE(1, "Codec says:" << (isEncoding ? " OUT " : " IN ") << frameWidth << "x" << frameHeight);

  if (!device ||
	  !device->SetFrameSize(codec.GetWidth(), codec.GetHeight()) ||
      !device->SetColourFormatConverter("YUV420P") ||
      !device->SetFrameRate(endpoint.GetFrameRate()) ||
      !device->Open(deviceName, TRUE)) {
    PTRACE(1, "Failed to open or configure the video device \"" << deviceName << '"');
    return FALSE;
  }

  device->GetFrameSize(frameWidth, frameHeight);
  PTRACE(1, "Device says:" << (isEncoding ? " OUT " : " IN ") << frameWidth << "x" << frameHeight);

  if (isEncoding) {
    videoChannelOut = new PVideoChannel();
    videoChannelOut->AttachVideoReader((PVideoInputDevice *)device);
    return codec.AttachChannel(videoChannelOut, false);
  } else {
    videoChannelIn = new PVideoChannel();
    videoChannelIn->AttachVideoPlayer((PVideoOutputDevice *)device);
    return codec.AttachChannel(videoChannelIn, false);
  }
};
#endif

///////////////////////////////////////////////////////////////////////////////

RTPFuzzingChannel::RTPFuzzingChannel(MyH323EndPoint & ep, H323Connection & connection, const H323Capability & capability, Directions direction, unsigned sessionID, WORD rtpPort, WORD rtcpPort)
    : H323_ExternalRTPChannel(connection, capability, direction, sessionID)
{
    m_percentBadRTPHeader = ep.GetPercentBadRTPHeader();
    m_percentBadRTPMedia = ep.GetPercentBadRTPMedia();
    m_percentBadRTCP = ep.GetPercentBadRTCP();
    PIPSocket::Address myip;
    const H323ListenerList & listeners = ep.GetListeners();
    if (listeners.GetSize() > 0) {
        listeners[0].GetTransportAddress().GetIpAddress(myip);
    }

    // set the local RTP address and port
    SetExternalAddress(H323TransportAddress(myip, rtpPort), H323TransportAddress(myip, rtcpPort));
    // for now we ignore everything sent to these ports
    m_rtpSocket.Listen(5, rtpPort);
    m_rtcpSocket.Listen(5, rtcpPort);

    // get the payload code
    OpalMediaFormat format(capability.GetFormatName(), false);
    m_payloadType = format.GetPayloadType();
    if (m_payloadType > RTP_DataFrame::MaxPayloadType)
        m_payloadType = RTP_DataFrame::DynamicBase;
    m_syncSource = PRandom::Number(65000);
    m_rtpPacket.SetPayloadSize(format.GetFrameTime() * format.GetFrameSize()); // G.711: 20 ms * 8 byte
    if (m_rtpPacket.GetPayloadSize() == 0)
        m_rtpPacket.SetPayloadSize(1400); // eg. for video there is no fixed size
    memset(m_rtpPacket.GetPayloadPtr(), 0, m_rtpPacket.GetPayloadSize()); // silence

    m_frameTime = format.GetFrameTime();
    if (m_frameTime == 0)
        m_frameTime = 100;
    m_frameTimeUnits = m_frameTime * format.GetTimeUnits();
    if (m_frameTimeUnits == 0)
        m_frameTimeUnits = m_frameTime * 8;
    m_timestamp = 0;
    PTRACE(2, "New fuzzing transmit channel: PT=" << (int)m_payloadType << " frame time=" << m_frameTime
           << " frame size=" << m_rtpPacket.GetPayloadSize());
}

RTPFuzzingChannel::~RTPFuzzingChannel()
{
    m_rtpSocket.Close();
    m_rtcpSocket.Close();
}

PBoolean RTPFuzzingChannel::Start()
{
    if (!H323_ExternalRTPChannel::Start())
        return false;

    if (GetDirection() == IsTransmitter) {
        m_rtpTransmitTimer.RunContinuous(m_frameTime);
        m_rtpTransmitTimer.SetNotifier(PCREATE_NOTIFIER(TransmitRTP));
        m_rtcpTransmitTimer.RunContinuous(m_frameTime); // way more often than regular RTCP, but we want to get a lot of test cases through
        m_rtcpTransmitTimer.SetNotifier(PCREATE_NOTIFIER(TransmitRTCP));
        PIPSocket::Address ip;
        WORD port = 0;
        remoteMediaAddress.GetIpAndPort(ip, port);
        m_rtpSocket.SetSendAddress(ip, port);
        remoteMediaControlAddress.GetIpAndPort(ip, port);
        m_rtcpSocket.SetSendAddress(ip, port);
    }
    return true;
}

void RTPFuzzingChannel::TransmitRTP(PTimer &, H323_INT)
{
    m_rtpPacket.SetPayloadType(m_payloadType);
    m_rtpPacket.SetSyncSource(m_syncSource);
    m_timestamp += m_frameTimeUnits;
    m_rtpPacket.SetTimestamp(m_timestamp);
    m_rtpPacket.SetSequenceNumber(m_rtpPacket.GetSequenceNumber() + 1);

    for (int i = 0; i < m_rtpPacket.GetHeaderSize(); i++) {
        // overwrite n% of the bytes with random values
        if (PRandom::Number(100) > (100 - m_percentBadRTPHeader)) {
            m_rtpPacket[i] = PRandom::Number(255);
        }
    }

    // random RTP media
    for (int i = 0; i < m_rtpPacket.GetPayloadSize(); i++) {
        if (PRandom::Number(100) > (100 - m_percentBadRTPMedia)) {
            *(m_rtpPacket.GetPayloadPtr() + i) = PRandom::Number(255);
        }
    }

    PTRACE(2, "Sending fuzzed RTP to " << remoteMediaControlAddress << " payload type=" << m_rtpPacket.GetPayloadType());
    m_rtpSocket.Write(m_rtpPacket, m_rtpPacket.GetHeaderSize() + m_rtpPacket.GetPayloadSize());
}

void RTPFuzzingChannel::TransmitRTCP(PTimer &, H323_INT)
{
    const unsigned SecondsFrom1900to1970 = (70*365+17)*24*60*60U;
    RTP_ControlFrame m_rtcpPacket;

    m_rtcpPacket.SetPayloadType(RTP_ControlFrame::e_SenderReport);
    m_rtcpPacket.SetPayloadSize(sizeof(RTP_ControlFrame::SenderReport));

    RTP_ControlFrame::SenderReport * sender = (RTP_ControlFrame::SenderReport *)m_rtcpPacket.GetPayloadPtr();
    sender->ssrc = m_syncSource;
    PTime now;
    sender->ntp_sec = now.GetTimeInSeconds() + SecondsFrom1900to1970; // Convert from 1970 to 1900
    sender->ntp_frac = now.GetMicrosecond() * 4294; // Scale microseconds to "fraction" from 0 to 2^32
    sender->rtp_ts = m_timestamp;
    sender->psent = m_rtpPacket.GetSequenceNumber();
    sender->osent = m_rtpPacket.GetSequenceNumber() * m_rtpPacket.GetPayloadSize();
/*
    // TODO: add Receiver report
    // TODO: etPayloadType(RTP_ControlFrame::e_ReceiverReport)
    m_rtcpPacket.SetPayloadSize(sizeof(RTP_ControlFrame::SenderReport) + sizeof(RTP_ControlFrame::ReceiverReport));
    m_rtcpPacket.SetCount(1);
    // TODO: insert ReceiverReport data, for now rely on the random data we insert
    //AddReceiverReport(*(RTP_ControlFrame::ReceiverReport *)&sender[1]);
*/
    m_rtcpPacket.WriteNextCompound();
    (void)m_rtcpPacket.AddSourceDescription(m_syncSource);

    // send random RTCP packet every time
    for (int i = 0; i < m_rtcpPacket.GetCompoundSize(); i++) {
        // overwrite n% of the bytes with random values
        if (PRandom::Number(100) > (100 - m_percentBadRTCP)) {
            m_rtcpPacket[i] = PRandom::Number(255);
        }
    }

    PTRACE(2, "Sending fuzzed RTCP to " << remoteMediaControlAddress);
    m_rtcpSocket.Write(m_rtcpPacket, m_rtcpPacket.GetCompoundSize());
}

///////////////////////////////////////////////////////////////////////////////


PlayMessage::PlayMessage(const PString & filename, unsigned frameDelay, unsigned frameSize)
  : PDelayChannel(PDelayChannel::DelayReadsOnly, frameDelay, frameSize)
{
  if (filename.IsEmpty())
      PTRACE(2, "CallGen\tPlaying silence, no outgoing message file");
  else {
    if (wavFile.Open(filename, PFile::ReadOnly)) {
      Open(wavFile);
      if (wavFile.GetFormat() != PWAVFile::fmt_PCM
          || wavFile.GetChannels() != 1
          || wavFile.GetSampleRate() != 8000
          || wavFile.GetSampleSize() != 16) {

        wavFile.Close();
        PTRACE(2, "CallGen\tWrong file format in outgoing message file \"" << wavFile.GetFilePath() << '"');
      } else {
        PTRACE(2, "CallGen\tPlaying outgoing message file \"" << wavFile.GetFilePath() << '"');
      }
    }
    else {
      PTRACE(2, "CallGen\tCould not open outgoing message file \"" << wavFile.GetFilePath() << '"');
    }
  }
}

PBoolean PlayMessage::Read(void * buf, PINDEX len)
{
  if (!wavFile.IsOpen()) {
    // just play out silence
    memset(buf, 0, len);
    lastReadCount = len;
    return TRUE;
  }

  if (PDelayChannel::Read(buf, len)) {
    // at end of file, re-open and start reading again
    if (lastReadCount < len) {
        wavFile.Open(PFile::ReadOnly);
        PDelayChannel::Read(buf, len);
    }
    return TRUE;
  }
  return FALSE;
}


PBoolean PlayMessage::Close()
{
  return PDelayChannel::Close();
}

///////////////////////////////////////////////////////////////////////////////

RecordMessage::RecordMessage(const PString & wavFileName, unsigned frameDelay, unsigned frameSize)
  : PDelayChannel(PDelayChannel::DelayWritesOnly, frameDelay, frameSize)
{
  reallyClose = FALSE;

  if (wavFileName.IsEmpty())
    return;

  PWAVFile * wavFile = new PWAVFile(wavFileName, PFile::WriteOnly);
  if (wavFile->IsOpen()) {
    Open(wavFile, TRUE);
    PTRACE(2, "CallGen\tRecording to file \"" << wavFileName << '"');
  }
  else
    delete wavFile;
}

PBoolean RecordMessage::Write(const void * buf, PINDEX len)
{
  if (PDelayChannel::Write(buf, len))
    return TRUE;

  lastWriteCount = len;
  return !reallyClose;
}

PBoolean RecordMessage::Close()
{
  reallyClose = TRUE;
  return PDelayChannel::Close();
}

