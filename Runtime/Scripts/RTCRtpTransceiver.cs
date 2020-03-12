using System;

namespace Unity.WebRTC
{
    public enum RTCRtpTransceiverDirection
    {
        SendRecv,
        SendOnly,
        RecvOnly,
        Inactive
    }

    /// <summary>
    /// Not implemented
    /// </summary>
    public struct RTCRtpCodecCapability
    {
    }

    public class RTCRtpTransceiver
    {
        internal IntPtr self;

        internal RTCRtpTransceiver(IntPtr ptr)
        {
            self = ptr;
        }

        /// <summary>
        /// 
        /// </summary>
        public RTCRtpTransceiverDirection Direction
        {
            get
            {
                return NativeMethods.TransceiverGetDirection(self);
            }
            set
            {
                NativeMethods.TransceiverSetDirection(self, value);
            }
        }

        public RTCRtpTransceiverDirection CurrentDirection
        {
            get
            {
                var direction = RTCRtpTransceiverDirection.RecvOnly;
                if (NativeMethods.TransceiverGetCurrentDirection(self, ref direction))
                {
                    return direction;
                }
                throw new InvalidOperationException("Transceiver is not running");
            }
        }

        /// <summary>
        /// 
        /// </summary>
        public RTCRtpReceiver Receiver
        {
            get { return new RTCRtpReceiver(NativeMethods.TransceiverGetReceiver(self)); }
        }

        /// <summary>
        /// 
        /// </summary>
        public RTCRtpSender Sender
        {
            get { return new RTCRtpSender(NativeMethods.TransceiverGetSender(self)); }
        }

        public void SetCodecPreferences(RTCRtpCodecCapability[] capabilities)
        {
            throw new NotImplementedException("SetCodecPreferences is not implemented");
        }

        public void Stop()
        {
            NativeMethods.TransceiverStop(self);
        }
    }

    public class RTCRtpReceiver
    {
        internal IntPtr self;
        internal RTCRtpReceiver(IntPtr ptr)
        {
            self = ptr;
        }

        public MediaStreamTrack Track
        {
            get
            {
                var ptrTrack = NativeMethods.RtpReceiverGetMediaStreamTrack(self);
                return WebRTC.FindOrCreate(ptrTrack, ptr => new MediaStreamTrack(ptr));
            }
        }
    }

    public class RTCRtpSender
    {
        internal IntPtr self;
        internal RTCRtpSender(IntPtr ptr)
        {
            self = ptr;
        }

        public bool ReplaceTrack(MediaStreamTrack track)
        {
            return NativeMethods.RtpSenderReplaceTrack(self, track.self);
        }
    }
}
