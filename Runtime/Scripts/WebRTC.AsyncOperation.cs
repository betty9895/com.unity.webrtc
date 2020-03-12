using System;
using UnityEngine;

namespace Unity.WebRTC
{

    /// <inheritdoc />
    /// <summary>
    /// </summary>
    public class AsyncOperationBase : CustomYieldInstruction
    {
        public RTCError Error { get; internal set; }

        public bool IsError { get; internal set; }
        public bool IsDone { get; internal set; }

        public override bool keepWaiting
        {
            get
            {
                if (IsDone)
                {
                    return false;
                }
                else
                {
                    return true;  
                }   
            }
        }

        internal void Done()
        {
            IsDone = true;
        }
    }

    public class RTCSessionDescriptionAsyncOperation : AsyncOperationBase
    {
        internal RTCSessionDescriptionAsyncOperation(RTCPeerConnection connection, ref RTCOfferOptions options)
        {
            Action<string, RTCSdpType> actionOnSuccess = OnSuccess;
            var hashOnSuccess = actionOnSuccess.GetHashCode();
            WebRTC.CallbackTable[hashOnSuccess] = actionOnSuccess;

            Action actionOnFailure = OnFailure;
            var hashOnFailure = actionOnFailure.GetHashCode();
            WebRTC.CallbackTable[hashOnSuccess] = actionOnFailure;

            WebRTC.Context.PeerConnectionCreateOffer(connection.self, ref options, hashOnSuccess, hashOnFailure);
        }

        internal RTCSessionDescriptionAsyncOperation(RTCPeerConnection connection, ref RTCAnswerOptions options)
        {
            Action<string, RTCSdpType> actionOnSuccess = OnSuccess;
            var hashOnSuccess = actionOnSuccess.GetHashCode();
            WebRTC.CallbackTable[hashOnSuccess] = actionOnSuccess;

            Action actionOnFailure = OnFailure;
            var hashOnFailure = actionOnFailure.GetHashCode();
            WebRTC.CallbackTable[hashOnSuccess] = actionOnFailure;

            WebRTC.Context.PeerConnectionCreateAnswer(connection.self, ref options, hashOnSuccess, hashOnFailure);
        }

        void OnSuccess(string sdp, RTCSdpType type)
        {
            this.Desc = new RTCSessionDescription { sdp = sdp, type = type };
            this.Done();
        }
        void OnFailure()
        {
            this.IsError = true;
            this.Done();
        }

        void RegisterOnFailure()
        {
            /*
            Action<string, RTCSdpType> callback = (string sdp, RTCSdpType type) =>
            {
                this.Desc = new RTCSessionDescription { sdp = sdp, type = type };
                this.Done();
            };
            var hash = callback.GetHashCode();
            WebRTC.CallbackTable[hash] = callback;

            NativeMethods.PeerConnectionRegisterOnCreateSessionDescSuccess(hash, RTCPeerConnection.OnSuccessCreateSessionDesc);
            */
        }


        public RTCSessionDescription Desc { get; internal set; }
    }

    public class RTCSetSessionDescriptionAsyncOperation : AsyncOperationBase
    {
        internal RTCSetSessionDescriptionAsyncOperation(RTCPeerConnection connection)
        {
            connection.OnSetSessionDescriptionSuccess = () =>
            {
                IsError = false;
                this.Done();
            };
            connection.OnSetSessionDescriptionFailure = () =>
            {
                IsError = true;
                this.Done();
            };
        }
    }


    public class RTCIceCandidateRequestAsyncOperation : CustomYieldInstruction
    {
        public bool isError { get; private set;  }
        public RTCError error { get; private set; }
        public bool isDone { get; private set;  }

        public override bool keepWaiting
        {
            get
            {
                return isDone;
            }
        }

        public void Done()
        {
            isDone = true;
        }
    }
    public class RTCAsyncOperation : CustomYieldInstruction
    {
        public bool isError { get; private set; }
        public RTCError error { get; private set; }
        public bool isDone { get; private set; }

        public override bool keepWaiting
        {
            get
            {
                return isDone;
            }
        }

        public void Done()
        {
            isDone = true;
        }
    }
}
