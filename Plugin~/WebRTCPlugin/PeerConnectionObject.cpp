#include "pch.h"
#include "Context.h"
#include "PeerConnectionObject.h"
#include "WebRTCMacros.h"
#include "SetSessionDescriptionObserver.h"

namespace WebRTC
{
    PeerConnectionObject::PeerConnectionObject(Context& context) : context(context)
    {
        m_statsCollectorCallback = new PeerConnectionStatsCollectorCallback(this);
        
    }

    PeerConnectionObject::~PeerConnectionObject()
    {
        SAFE_DELETE(m_statsCollectorCallback);
        if (connection == nullptr)
        {
            return;
        }
        auto senders = connection->GetSenders();
        for (const auto& sender : senders)
        {
            connection->RemoveTrack(sender);
        }

        const auto state = connection->peer_connection_state();
        if (state != webrtc::PeerConnectionInterface::PeerConnectionState::kClosed)
        {
            connection->Close();
        }
        connection.release();
    }

    PeerConnectionObject* Context::CreatePeerConnection(const webrtc::PeerConnectionInterface::RTCConfiguration& config)
    {
        //rtc::scoped_refptr<PeerConnectionObject> obj = new rtc::RefCountedObject<PeerConnectionObject>(*this);
        auto obj = std::make_unique<PeerConnectionObject>(*this);

        obj->connection = m_peerConnectionFactory->CreatePeerConnection(config, nullptr, nullptr, obj.get());
        if (obj->connection == nullptr)
        {
            return nullptr;
        }
        auto ptr = obj.get();
        m_mapClients[ptr] = std::move(obj);
        return m_mapClients[ptr].get();
    }

    /*
    void PeerConnectionObject::OnSuccess(webrtc::SessionDescriptionInterface* desc)
    {
        std::string out;
        desc->ToString(&out);
        const auto type = ConvertSdpType(desc->GetType());
        ::OutputDebugStringA(out.c_str());
        if (onCreateSDSuccess != nullptr)
        {
            onCreateSDSuccess(this, type, out.c_str());
        }
    }

    void PeerConnectionObject::OnFailure(webrtc::RTCError error)
    {
        //::TODO
        //RTCError _error = { RTCErrorDetailType::IdpTimeout };
        if (onCreateSDFailure != nullptr)
        {
            onCreateSDFailure(this);
        }
    }
    */

    void PeerConnectionObject::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
        auto obj = std::make_unique<DataChannelObject>(channel, *this);
        const auto ptr = obj.get();
        context.AddDataChannel(obj);
        if (onDataChannel != nullptr) {
            onDataChannel(this, ptr);
        }
    }

    void PeerConnectionObject::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
    {
        std::string out;

        if (!candidate->ToString(&out))
        {
            DebugError("Can't make string form of sdp.");
        }
        if (onIceCandidate != nullptr)
        {
            onIceCandidate(this, out.c_str(), candidate->sdp_mid().c_str(), candidate->sdp_mline_index());
        }
    }

    void PeerConnectionObject::OnRenegotiationNeeded()
    {
        if (onRenegotiationNeeded != nullptr)
        {
            onRenegotiationNeeded(this);
        }
    }

    void PeerConnectionObject::OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
    {
        if (onTrack != nullptr)
        {
            onTrack(this, transceiver.get());
        }
    }
    // Called any time the IceConnectionState changes.
    void PeerConnectionObject::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state)
    {
        if (onIceConnectionChange != nullptr)
        {
            onIceConnectionChange(this, new_state);
        }
    }
    // Called any time the IceGatheringState changes.
    void PeerConnectionObject::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state)
    {
        DebugLog("OnIceGatheringChange");
    }

    void PeerConnectionObject::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state)
    {
        DebugLog("OnSignalingChange %d", new_state);
    }

    void PeerConnectionObject::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
    {
        DebugLog("OnAddStream");
    }

    void PeerConnectionObject::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
    {
        DebugLog("OnRemoveStream");
    }

    void PeerConnectionObject::Close()
    {
        if (connection != nullptr && connection->peer_connection_state() != webrtc::PeerConnectionInterface::PeerConnectionState::kClosed)
        {
            //Cleanup delegates/callbacks
//            onCreateSDSuccess = nullptr;
//            onCreateSDFailure = nullptr;
            onLocalSdpReady = nullptr;
            onIceCandidate = nullptr;
            onIceConnectionChange = nullptr;
            onDataChannel = nullptr;
            onRenegotiationNeeded = nullptr;
            onTrack = nullptr;

            connection->Close();
        }
    }

    void PeerConnectionObject::SetLocalDescription(const RTCSessionDescription& desc, webrtc::SetSessionDescriptionObserver* observer)
    {
        webrtc::SdpParseError error;
        auto _desc = webrtc::CreateSessionDescription(ConvertSdpType(desc.type), desc.sdp, &error);
        if (!_desc.get())
        {
            DebugLog("Can't parse received session description message.");
            DebugLog("SdpParseError:\n%s", error.description.c_str());
            return;
        }
        connection->SetLocalDescription(observer, _desc.release());
    }

    void PeerConnectionObject::SetRemoteDescription(const RTCSessionDescription& desc, webrtc::SetSessionDescriptionObserver* observer)
    {
        webrtc::SdpParseError error;
        auto _desc = webrtc::CreateSessionDescription(ConvertSdpType(desc.type), desc.sdp, &error);
        if (!_desc.get())
        {
            DebugLog("Can't parse received session description message.");
            DebugLog("SdpParseError:\n%s", error.description.c_str());
            return;
        }
        connection->SetRemoteDescription(observer, _desc.release());
    }

    webrtc::RTCErrorType PeerConnectionObject::SetConfiguration(const std::string& config)
    {
        webrtc::PeerConnectionInterface::RTCConfiguration _config;
        if (!Convert(config, _config))
            return webrtc::RTCErrorType::INVALID_PARAMETER;

        const auto error = connection->SetConfiguration(_config);
        if (!error.ok())
        {
            LogPrint(error.message());
        }
        return error.type();
    }

    void PeerConnectionObject::GetConfiguration(std::string& config) const
    {
        auto _config = connection->GetConfiguration();

        Json::Value root;
        root["iceServers"] = Json::Value(Json::arrayValue);
        for (auto iceServer : _config.servers)
        {
            Json::Value jsonIceServer = Json::Value(Json::objectValue);
            jsonIceServer["username"] = iceServer.username;
            jsonIceServer["credential"] = iceServer.password;
            jsonIceServer["credentialType"] = static_cast<int>(RTCIceCredentialType::Password);
            jsonIceServer["urls"] = Json::Value(Json::arrayValue);
            for (auto url : iceServer.urls)
            {
                jsonIceServer["urls"].append(url);
            }
            root["iceServers"].append(jsonIceServer);
        }
        Json::StreamWriterBuilder builder;
        config = Json::writeString(builder, root);
    }

    /*
    void PeerConnectionObject::CreateOffer(const RTCOfferOptions & options, webrtc::CreateSessionDescriptionObserver* observer)
    {
        webrtc::PeerConnectionInterface::RTCOfferAnswerOptions _options;
        _options.ice_restart = options.iceRestart;
        _options.offer_to_receive_audio = options.offerToReceiveAudio;
        _options.offer_to_receive_video = options.offerToReceiveVideo;
        connection->CreateOffer(observer, _options);
    }

    void PeerConnectionObject::CreateAnswer(const RTCAnswerOptions& options, webrtc::CreateSessionDescriptionObserver* observer)
    {
        webrtc::PeerConnectionInterface::RTCOfferAnswerOptions _options;
        _options.ice_restart = options.iceRestart;
        connection->CreateAnswer(observer, _options);
    }
    */

    void PeerConnectionObject::AddIceCandidate(const RTCIceCandidate& candidate)
    {
        if(connection.get() == nullptr) {
            LogPrint("peer connection is not initialized %d", this);
            return;
        }

        webrtc::SdpParseError error;
        const std::unique_ptr<webrtc::IceCandidateInterface> _candidate(
            webrtc::CreateIceCandidate(candidate.sdpMid, candidate.sdpMLineIndex, candidate.candidate, &error));
        connection->AddIceCandidate(_candidate.get());
    }

    bool PeerConnectionObject::GetSessionDescription(const webrtc::SessionDescriptionInterface* sdp, RTCSessionDescription& desc) const
    {
        if (sdp == nullptr)
        {
            return false;
        }

        std::string out;
        sdp->ToString(&out);

        desc.type = ConvertSdpType(sdp->GetType());
        desc.sdp = static_cast<char*>(CoTaskMemAlloc(out.size() + 1));
        out.copy(desc.sdp, out.size());
        desc.sdp[out.size()] = '\0';
        return true;
    }

    void PeerConnectionObject::CollectStats()
    {
        connection->GetStats(m_statsCollectorCallback);
    }

#pragma warning(push)
#pragma warning(disable: 4715)
    RTCIceConnectionState PeerConnectionObject::GetIceCandidateState()
    {
        auto state = connection->ice_connection_state();
        switch (state)
        {
        case webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionNew:
            return RTCIceConnectionState::New;
        case webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionChecking:
            return RTCIceConnectionState::Checking;
        case webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionConnected:
            return RTCIceConnectionState::Connected;
        case webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionCompleted:
            return RTCIceConnectionState::Completed;
        case webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionFailed:
            return RTCIceConnectionState::Failed;
        case webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionDisconnected:
            return RTCIceConnectionState::Disconnected;
        case webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionClosed:
            return RTCIceConnectionState::Closed;
        case webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionMax:
            return RTCIceConnectionState::Max;
        }
        throw std::invalid_argument("Unknown ice connection type");
    }

    RTCPeerConnectionState PeerConnectionObject::GetConnectionState()
    {
        auto state = connection->peer_connection_state();
        switch (state)
        {
        case webrtc::PeerConnectionInterface::PeerConnectionState::kClosed:
            return RTCPeerConnectionState::Closed;
        case webrtc::PeerConnectionInterface::PeerConnectionState::kConnected:
            return RTCPeerConnectionState::Connected;
        case webrtc::PeerConnectionInterface::PeerConnectionState::kConnecting:
            return RTCPeerConnectionState::Connecting;
        case webrtc::PeerConnectionInterface::PeerConnectionState::kDisconnected:
            return RTCPeerConnectionState::Disconnected;
        case webrtc::PeerConnectionInterface::PeerConnectionState::kFailed:
            return RTCPeerConnectionState::Failed;
        case webrtc::PeerConnectionInterface::PeerConnectionState::kNew:
            return RTCPeerConnectionState::New;
        }
        throw std::invalid_argument("Unknown peer connection type");
    }

    rtc::scoped_refptr<CreateSessionDescriptionObserver> CreateSessionDescriptionObserver::Create(int hashOnSuccess, int hashOnFailure, DelegateCreateSDSuccess onSuccess, DelegateCreateSDFailure onFailure)
    {
        return new rtc::RefCountedObject<CreateSessionDescriptionObserver>(hashOnSuccess, hashOnSuccess, onSuccess, onFailure);
    }

    CreateSessionDescriptionObserver::CreateSessionDescriptionObserver(int hashOnSuccess, int hashOnFailure, DelegateCreateSDSuccess onSuccess, DelegateCreateSDFailure onFailure) :
        m_hashOnSuccess(hashOnSuccess),
        m_hashOnFailure(hashOnFailure),
        m_onSuccess(onSuccess),
        m_onFailure(onFailure)
    {
    }

    void CreateSessionDescriptionObserver::OnSuccess(webrtc::SessionDescriptionInterface* desc)
    {
        std::string out;
        desc->ToString(&out);
        const auto type = ConvertSdpType(desc->GetType());
        m_onSuccess(m_hashOnSuccess, type, out.c_str());
    }
    void CreateSessionDescriptionObserver::OnFailure(webrtc::RTCError error)
    {
        m_onFailure(m_hashOnFailure);
    }

#pragma warning(pop)
}

