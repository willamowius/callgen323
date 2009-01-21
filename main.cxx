/*
 * main.cxx
 *
 * OpenH323 call generator
 *
 * Copyright (c) 2001 Benny L. Prijono <seventhson@theseventhson.freeserve.co.uk>
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
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
 * $Log$
 * Revision 1.3  2008/09/03 12:21:24  willamowius
 * switch BOOL to PBoolean to be able to compile with Ptlib 2.2.x
 *
 * Revision 1.2  2008/02/07 10:13:32  shorne
 * added video support
 *
 * Revision 1.1  2007/11/21 14:53:51  shorne
 * First commit to h323plus
 *
 *
 *
 * 25 Jan 2002 Substantial improvement [Equivalence Pty. Ltd.]
 * 25 Jan 2000 Update to incorporate openh323 v.01 alpha2 and fix gatekeeper
 *             related codes [bennylp]
 */

#include "precompile.h"
#include "main.h"
#include "version.h"

#include <ptclib/random.h>
#include <ptlib/video.h>


PCREATE_PROCESS(CallGen);


///////////////////////////////////////////////////////////////////////////////

CallGen::CallGen()
  : PProcess("H323Plus", "CallGen", MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER),
    console(PConsoleChannel::StandardInput)
{
  totalAttempts = 0;
  totalEstablished = 0;
}


void CallGen::Main()
{
  PArgList & args = GetArguments();
  args.Parse("a-access-token-oid:"
             "c-cdr:"
             "C-cycle."
             "D-disable:"
             "f-fast-disable."
             "g-gatekeeper:"
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
			 "v-video."
			 "m-maxframe."
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
             , FALSE);
  
  if (args.GetCount() == 0 && !args.HasOption('l')) {
    cout << "Usage:\n"
            "  callgen [options] -l\n"
            "  callgen [options] destination [ destination ... ]\n"
            "where options:\n"
            "  -l                   Passive/listening mode.\n"
            "  -m --max num         Maximum number of simultaneous calls\n"
            "  -r --repeat num      Repeat calls n times\n"
            "  -C --cycle           Each simultaneous call cycles through destination list\n"
            "  -t --trace           Trace enable (use multiple times for more detail)\n"
            "  -o --output file     Specify filename for trace output [stdout]\n"
            "  -i --interface addr  Specify IP address and port listen on [*:1720]\n"
            "  -g --gatekeeper host Specify gatekeeper host [auto-discover]\n"
            "  -n --no-gatekeeper   Disable gatekeeper discovery [false]\n"
            "  --require-gatekeeper Exit if gatekeeper discovery fails [false]\n"
            "  -u --user username   Specify local username [login name]\n"
            "  -p --password pwd    Specify gatekeeper H.235 password [none]\n"
            "  -P --prefer codec    Set codec preference (use multiple times) [none]\n"
            "  -D --disable codec   Disable codec (use multiple times) [none]\n"
            "  -v --video enable    Enable Video Support\n"
            "  -m --maxframe        Maximum Frame Size\n"
            "  -f --fast-disable    Disable fast start\n"
            "  -T --h245tunneldisable  Disable H245 tunnelling.\n"
            "  -O --out-msg file    Specify PCM16 WAV file for outgoing message [ogm.wav]\n"
            "  -I --in-dir dir      Specify directory for incoming WAV files [disabled]\n"
            "  -c --cdr file        Specify Call Detail Record file [none]\n"
            "  --tcp-base port      Specific the base TCP port to use.\n"
            "  --tcp-max port       Specific the maximum TCP port to use.\n"
            "  --udp-base port      Specific the base UDP port to use.\n"
            "  --udp-max port       Specific the maximum UDP port to use.\n"
            "  --rtp-base port      Specific the base RTP/RTCP pair of UDP port to use.\n"
            "  --rtp-max port       Specific the maximum RTP/RTCP pair of UDP port to use.\n"
            "  --tmaxest  secs      Maximum time to wait for \"Established\" [0]\n"
            "  --tmincall secs      Minimum call duration in seconds [10]\n"
            "  --tmaxcall secs      Maximum call duration in seconds [30]\n"
            "  --tminwait secs      Minimum interval between calls in seconds [10]\n"
            "  --tmaxwait secs      Maximum interval between calls in seconds [30]\n"
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
		     PTrace::Blocks | PTrace::DateAndTime | PTrace::Thread | PTrace::FileAndLine);
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

  PStringArray interfaces = args.GetOptionString('i').Lines();
  if (!h323->StartListeners(interfaces)) {
    cout << "Couldn't start any listeners on interfaces/ports:\n"
         << setfill('\n') << interfaces << setfill(' ') << endl;
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
  if (args.HasOption('g')) {
    PString gkAddr = args.GetOptionString('g');
    cout << "Registering with gatekeeper \"" << gkAddr << "\" ..." << flush;
    if (h323->UseGatekeeper(gkAddr))
      cout << "\nGatekeeper set to \"" << *h323->GetGatekeeper() << '"' << endl;
    else {
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

  if (args.HasOption('f'))
    h323->DisableFastStart(TRUE);
  if (args.HasOption('T'))
    h323->DisableH245Tunneling(TRUE);

  if (!args.HasOption('v'))
    h323->RemoveCapability(H323Capability::e_Video);

  if (args.HasOption('m')) {
    PCaselessString maxframe = args.GetOptionString('m');
	if (maxframe == "qcif") 
		h323->SetVideoFrameSize(H323Capability::qcifMPI);
	else if (maxframe == "cif") 
	    h323->SetVideoFrameSize(H323Capability::cifMPI);
	else if (maxframe == "4cif")
        h323->SetVideoFrameSize(H323Capability::cif4MPI);
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
        PINDEX arg = idx%args.GetCount();
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

CallThread::CallThread(unsigned _index,
                       const PStringArray & _destinations,
                       const CallParams & _params)
  : PThread(1000, NoAutoDeleteThread, NormalPriority, psprintf("CallGen %u", _index)),
    destinations(_destinations),
    index(_index),
    params(_params)
{
  Resume();
}


static unsigned RandomRange(PRandom & rand,
                            const PTimeInterval & tmin,
                            const PTimeInterval & tmax)
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
    PString destination = destinations[(index-1 + count-1)%destinations.GetSize()];

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
               "Signalling gateway,"
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
  if (receivedMedia.GetTimeInSeconds() == 0 && session.GetPacketsReceived() > 0) {
    receivedMedia = PTime();
    OUTPUT("", token, "Received media");

    const RTP_UDP * udpSess = dynamic_cast<const RTP_UDP *>(&session);
    if (udpSess != NULL) 
      mediaGateway = H323TransportAddress(udpSess->GetRemoteAddress(), udpSess->GetRemoteDataPort());
  }
}

///////////////////////////////////////////////////////////////////////////////

MyH323EndPoint::MyH323EndPoint()
{
  // Set capability
  AddAllCapabilities(0, 0, "*");
  AddAllUserInputCapabilities(0, P_MAX_INDEX);
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


void MyH323EndPoint::OnConnectionEstablished(H323Connection & connection,
                                             const PString & token)
{
  OUTPUT("", token, "Established \"" << TidyRemotePartyName(connection) << "\""
                    " " << connection.GetControlChannel().GetRemoteAddress() <<
                    " active=" << connectionsActive.GetSize() <<
                    " total=" << ++CallGen::Current().totalEstablished);
}


void MyH323EndPoint::OnConnectionCleared(H323Connection & connection,
                                         const PString & token)
{
  OUTPUT("", token, "Cleared \"" << TidyRemotePartyName(connection) << "\""
                    " " << connection.GetControlChannel().GetRemoteAddress() <<
                    " reason=" << connection.GetCallEndReason());
  ((MyH323Connection&)connection).details.Drop(connection);
}


PBoolean MyH323EndPoint::OnStartLogicalChannel(H323Connection & connection,
                                           H323Channel & channel)
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

MyH323Connection::MyH323Connection(MyH323EndPoint & ep,
                                   unsigned callRef)
  : H323Connection(ep, callRef),endpoint(ep)
{
}


void MyH323Connection::OnRTPStatistics(const RTP_Session & session) const
{
  ((MyH323Connection *)this)->details.OnRTPStatistics(session, GetCallToken());
}


PBoolean MyH323Connection::OpenAudioChannel(PBoolean isEncoding,
                                        unsigned bufferSize,
                                        H323AudioCodec & codec)
{

  unsigned frameDelay = bufferSize/16; // Assume 16 bit PCM

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
PBoolean MyH323Connection::OpenVideoChannel(PBoolean isEncoding, 
										H323VideoCodec & codec)
{
  PString deviceName = isEncoding ? "fake" : "NULL";

  PVideoDevice * device = isEncoding ? (PVideoDevice *)PVideoInputDevice::CreateDeviceByName(deviceName)
                                     : (PVideoDevice *)PVideoOutputDevice::CreateDeviceByName(deviceName);

  if (!device->SetFrameSize(codec.GetWidth(), codec.GetHeight()) ||
      !device->SetColourFormatConverter("YUV420P") ||
      !device->Open(deviceName, TRUE)) {
    PTRACE(1, "Failed to open or configure the video device \"" << deviceName << '"');
    return FALSE;
  }

  PVideoChannel * channel = new PVideoChannel;

  if (isEncoding)
    channel->AttachVideoReader((PVideoInputDevice *)device);
  else
    channel->AttachVideoPlayer((PVideoOutputDevice *)device);

  return codec.AttachChannel(channel,TRUE);
};
#endif

///////////////////////////////////////////////////////////////////////////////

PlayMessage::PlayMessage(const PString & filename,
                         unsigned frameDelay,
                         unsigned frameSize)
  : PDelayChannel(PDelayChannel::DelayReadsOnly, frameDelay, frameSize)
{
  if (filename.IsEmpty())
      PTRACE(2, "CallGen\tPlaying silence, no outgoing message file");
  else {
    if (wavFile.Open(filename, PFile::ReadOnly)) {
      Open(wavFile);
      PTRACE(2, "CallGen\tPlaying outgoing message file \"" << wavFile.GetFilePath() << '"');
    }
    else {
      PTRACE(2, "CallGen\tCould not open outgoing message file \"" << wavFile.GetFilePath() << '"');
    }
  }

  reallyClose = FALSE;
}


PBoolean PlayMessage::Read(void * buf, PINDEX len)
{
  if (PDelayChannel::Read(buf, len))
    return TRUE;

  if (reallyClose)
    return FALSE;

  // By opening the file as soon as we get a read error, we continually play
  // out the outgoing message as the usual error is end of file.
  if (wavFile.Open(PFile::ReadOnly)) {
    if (PDelayChannel::Read(buf, len))
      return TRUE;
  }

  // Just play out silence
  memset(buf, len, 0);
  lastReadCount = len;
  return TRUE;
}


PBoolean PlayMessage::Close()
{
  reallyClose = TRUE;
  return PDelayChannel::Close();
}


///////////////////////////////////////////////////////////////////////////////

RecordMessage::RecordMessage(const PString & wavFileName,
                             unsigned frameDelay,
                             unsigned frameSize)
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


// End of file ////////////////////////////////////////////////////////////////
