/*
 * main.h
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


#include <ptclib/delaychan.h>
#include <ptclib/pwavfile.h>

#include <h323.h>
#include <h323pdu.h>

#if !defined(P_USE_STANDARD_CXX_BOOL) && !defined(P_USE_INTEGER_BOOL)
    typedef int PBoolean;
#endif

///////////////////////////////////////////////////////////////////////////////

class PlayMessage : public PDelayChannel
{
    PCLASSINFO(PlayMessage, PDelayChannel);
  public:
    PlayMessage(const PString & filename, unsigned frameDelay, unsigned frameSize);
    virtual PBoolean Read(void *, PINDEX);
    virtual PBoolean Close();
  protected:
    PWAVFile wavFile;
};


///////////////////////////////////////////////////////////////////////////////

class RecordMessage : public PDelayChannel
{
    PCLASSINFO(RecordMessage, PDelayChannel);
  public:
    RecordMessage(const PString & filename, unsigned frameDelay, unsigned frameSize);
    virtual PBoolean Write(const void *, PINDEX);
    virtual PBoolean Close();
  protected:
    PBoolean reallyClose;
};

///////////////////////////////////////////////////////////////////////////////

class MyH323EndPoint;

class RTPFuzzingChannel : public H323_ExternalRTPChannel
{
    PCLASSINFO(RTPFuzzingChannel, H323_ExternalRTPChannel);
public:
    RTPFuzzingChannel(MyH323EndPoint & ep, H323Connection & connection, const H323Capability & capability, Directions direction, unsigned sessionID, WORD rtpPort, WORD rtcpPort);
    virtual ~RTPFuzzingChannel();

    virtual PBoolean Start();
    PDECLARE_NOTIFIER(PTimer, RTPFuzzingChannel, TransmitRTP);
    PDECLARE_NOTIFIER(PTimer, RTPFuzzingChannel, TransmitRTCP);

protected:
    PUDPSocket m_rtpSocket;
    PUDPSocket m_rtcpSocket;
    RTP_DataFrame m_rtpPacket;
    PTimer m_rtpTransmitTimer;
    PTimer m_rtcpTransmitTimer;
    unsigned m_frameTime;
    unsigned m_frameTimeUnits;
    RTP_DataFrame::PayloadTypes m_payloadType;
    DWORD m_syncSource;
    DWORD m_timestamp;
    unsigned m_percentBadRTPHeader;
    unsigned m_percentBadRTPMedia;
    unsigned m_percentBadRTCP;
};

///////////////////////////////////////////////////////////////////////////////

struct CallDetail
{
  CallDetail()
    : openedTransmitMedia(0),
      openedReceiveMedia(0),
      receivedMedia(0),
      receivedAudio(false),
      receivedVideo(false)
    { }

  PTime                openedTransmitMedia;
  PTime                openedReceiveMedia;
  PTime                receivedMedia;
  bool                 receivedAudio;
  bool                 receivedVideo;
  H323TransportAddress mediaGateway;

  void Drop(H323Connection & connection);

  void OnRTPStatistics(const RTP_Session & session, const PString & token);
};


///////////////////////////////////////////////////////////////////////////////

class MyH323EndPoint;

class MyH323Connection : public H323Connection
{
    PCLASSINFO(MyH323Connection, H323Connection);
  public:
    MyH323Connection(MyH323EndPoint & ep, unsigned callRef);
    virtual ~MyH323Connection();

    virtual PBoolean OnSendSignalSetup(H323SignalPDU & setupPDU);

    virtual PBoolean OpenAudioChannel(
      PBoolean isEncoding,          /// Direction of data flow
      unsigned bufferSize,          /// Size of each audio buffer
      H323AudioCodec & codec        /// codec that is doing the opening
    );

#ifdef H323_VIDEO
    virtual PBoolean OpenVideoChannel(PBoolean isEncoding, H323VideoCodec & codec);
#ifdef H323_H239
    void StartH239Transmission();
    PDECLARE_NOTIFIER(PTimer, MyH323Connection, StartH239TransmissionTrigger);
    virtual void OnEstablished();
    virtual PBoolean OnInitialFlowRestriction(H323Channel & channel);
	virtual PBoolean OpenExtendedVideoChannel(PBoolean isEncoding, H323VideoCodec & codec);
#endif
#endif

    virtual H323Channel * CreateRealTimeLogicalChannel(const H323Capability & capability, H323Channel::Directions dir,
                                                       unsigned sessionID, const H245_H2250LogicalChannelParameters * param, RTP_QOS * rtpqos = NULL);

    virtual void OnRTPStatistics(const RTP_Session & session) const;

    CallDetail details;

  protected:
    MyH323EndPoint & endpoint;
    PVideoChannel * videoChannelIn;
    PVideoChannel * videoChannelOut;
    map<unsigned, WORD> m_sessionPorts;
    bool m_haveStartedH239;
    PTimer m_h239StartTimer;
};

///////////////////////////////////////////////////////////////////////////////

class MyH323EndPoint : public H323EndPoint
{
    PCLASSINFO(MyH323EndPoint, H323EndPoint);
  public:
    MyH323EndPoint();

    // override from H323EndPoint
    virtual H323Connection * CreateConnection(unsigned callReference);

    virtual void OnConnectionEstablished(
      H323Connection & connection,    /// Connection that was established
      const PString & token           /// Token for identifying connection
    );
    virtual void OnConnectionCleared(
      H323Connection & connection,    /// Connection that was established
      const PString & token           /// Token for identifying connection
    );
    virtual PBoolean OnStartLogicalChannel(H323Connection & connection, H323Channel & PTRACE_channel);
    virtual PBoolean SetVideoFrameSize(H323Capability::CapabilityFrameSize frameSize, int frameUnits = 1);
    virtual H323Capability::CapabilityFrameSize GetMaxFrameSize() const { return m_maxFrameSize; }

    // TODO: include in codec negotiations, only sets bearer capabilities right now
    void SetPerCallBandwidth(unsigned bw) { m_rateMultiplier = ceil((float)bw / 64); }
    BYTE GetRateMultiplier() const { return m_rateMultiplier; }

    void SetVideoPattern(const PString & pattern, bool isH239 = false) { if (isH239) m_h239videoPattern = pattern; else m_videoPattern = pattern; }
    PString GetVideoPattern(bool isH239) const { return isH239 ? m_h239videoPattern : m_videoPattern; }

    void SetFrameRate(unsigned fps) { m_frameRate = fps; }
    unsigned GetFrameRate() const { return m_frameRate; }

    void SetFuzzing(bool val) { m_fuzzing = val; }
    bool IsFuzzing() const { return m_fuzzing; }
    void SetPercentBadRTPHeader(unsigned val) { m_percentBadRTPHeader = val; }
    unsigned GetPercentBadRTPHeader() const { return m_percentBadRTPHeader; }
    void SetPercentBadRTPMedia(unsigned val) { m_percentBadRTPMedia = val; }
    unsigned GetPercentBadRTPMedia() const { return m_percentBadRTPMedia; }
    void SetPercentBadRTCP(unsigned val) { m_percentBadRTCP = val; }
    unsigned GetPercentBadRTCP() const { return m_percentBadRTCP; }

    void SetStartH239(bool start) { m_startH239 = start; }
    bool IsStartH239() const { return m_startH239; }

  protected:
    BYTE m_rateMultiplier;
    PString m_videoPattern;
    PString m_h239videoPattern;
    unsigned m_frameRate;
    H323Capability::CapabilityFrameSize m_maxFrameSize;
    bool m_fuzzing;
    unsigned m_percentBadRTPHeader;
    unsigned m_percentBadRTPMedia;
    unsigned m_percentBadRTCP;
    bool m_startH239;
};

///////////////////////////////////////////////////////////////////////////////

class CallGen;

struct CallParams
{
  CallParams(CallGen & app)
    : callgen(app), repeat(0) { }

  CallGen & callgen;

  unsigned repeat;
  PTimeInterval tmax_est;
  PTimeInterval tmin_call;
  PTimeInterval tmax_call;
  PTimeInterval tmin_wait;
  PTimeInterval tmax_wait;
};


///////////////////////////////////////////////////////////////////////////////

class CallThread : public PThread
{
  PCLASSINFO(CallThread, PThread);
  public:
    CallThread(
      unsigned index,
      const PStringArray & destinations,
      const CallParams & params
    );
    void Main();
    void Stop();

  protected:
    PStringArray destinations;
    unsigned     index;
    CallParams   params;
    PSyncPoint   exit;
};

PLIST(CallThreadList, CallThread);


///////////////////////////////////////////////////////////////////////////////

class CallGen : public PProcess
{
  PCLASSINFO(CallGen, PProcess)

  public:
    CallGen();
    void Main();
    static CallGen & Current() { return (CallGen&)PProcess::Current(); }

    PString    outgoingMessageFile;
    PString    incomingAudioDirectory;
    PTextFile  cdrFile;

    PSyncPoint threadEnded;
    unsigned   totalAttempts;
    unsigned   totalEstablished;
    PMutex     coutMutex;

  MyH323EndPoint * h323;

  PBoolean Start(const PString & destination, PString & token) {
    return h323->MakeCall(destination, token) != NULL;
  }
  PBoolean Exists(const PString & token) {
    return h323->HasConnection(token);
  }
  PBoolean IsEstablished(const PString & token) {
    return h323->IsConnectionEstablished(token);
  }
  PBoolean Clear(PString & token) {
    return h323->ClearCallSynchronous(token);
  }
  void ClearAll() {
    h323->ClearAllCalls();
  }

  protected:
    PDECLARE_NOTIFIER(PThread, CallGen, Cancel);
    PConsoleChannel console;
    CallThreadList threadList;
};


///////////////////////////////////////////////////////////////////////////////
