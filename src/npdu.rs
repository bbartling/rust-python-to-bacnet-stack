#[derive(Debug)]
pub struct NPDU {
    // Example fields - adjust according to your actual NPDU structure
    npdu_version: u8,
    npdu_control: u8,
    // ... other fields ...
}

impl NPDU {
    pub fn new() -> Self {
        // Initialize the NPDU struct with default values
        NPDU {
            npdu_version: 1,
            npdu_control: 0,
            // ... initialize other fields ...
        }
    }

    pub fn encode(&self) -> Vec<u8> {
        // Logic to encode NPDU into bytes
        // This is a placeholder example
        vec![self.npdu_version, self.npdu_control]
    }

    pub fn decode(data: &[u8]) -> Result<Self, &'static str> {
        // Logic to decode bytes into an NPDU
        // This needs to be fleshed out based on your protocol specifics
        if data.len() < 2 {
            return Err("Data too short for NPDU decoding");
        }

        Ok(NPDU {
            npdu_version: data[0],
            npdu_control: data[1],
            // ... parse other fields ...
        })
    }
}

#[derive(Debug)]
pub enum NPDUMessage {
        WhoIsRouterToNetwork,
        IAmRouterToNetwork,
        ICouldBeRouterToNetwork,
        RejectMessageToNetwork,
        RouterBusyToNetwork,
        RouterAvailableToNetwork,
        InitializeRoutingTable,
        InitializeRoutingTableAck,
        EstablishConnectionToNetwork,
        DisconnectConnectionToNetwork,
        ChallengeRequest,
        SecurityPayload,
        SecurityResponse,
        RequestKeyUpdate,
        UpdateKeySet,
        UpdateDistributionKey,
        RequestMasterKey,
        SetMasterKey,
        WhatIsNetworkNumber,
        NetworkNumberIs,
        Proprietary(u8),
    }

impl NPDU {
    pub fn determine_bacnet_message(data: &[u8]) -> Option<NPDUMessage> {
        if data.len() < 2 {
            return None; // Not enough data to determine the message type
        }
        println!("data[0] {:X} ", data[0]);

        match data[0] {
            0x00 => Some(NPDUMessage::WhoIsRouterToNetwork),
            0x01 => Some(NPDUMessage::IAmRouterToNetwork),
            0x02 => Some(NPDUMessage::ICouldBeRouterToNetwork),
            0x03 => Some(NPDUMessage::RejectMessageToNetwork),
            0x04 => Some(NPDUMessage::RouterBusyToNetwork),
            0x05 => Some(NPDUMessage::RouterAvailableToNetwork),
            0x06 => Some(NPDUMessage::InitializeRoutingTable),
            0x07 => Some(NPDUMessage::InitializeRoutingTableAck),
            0x08 => Some(NPDUMessage::EstablishConnectionToNetwork),
            0x09 => Some(NPDUMessage::DisconnectConnectionToNetwork),
            0x0A => Some(NPDUMessage::ChallengeRequest),
            0x0B => Some(NPDUMessage::SecurityPayload),
            0x0C => Some(NPDUMessage::SecurityResponse),
            0x0D => Some(NPDUMessage::RequestKeyUpdate),
            0x0E => Some(NPDUMessage::UpdateKeySet),
            0x0F => Some(NPDUMessage::UpdateDistributionKey),
            0x10 => Some(NPDUMessage::RequestMasterKey),
            0x11 => Some(NPDUMessage::SetMasterKey),
            0x12 => Some(NPDUMessage::WhatIsNetworkNumber),
            0x13 => Some(NPDUMessage::NetworkNumberIs),
            0x80..=0xFF => Some(NPDUMessage::Proprietary(data[0])),
            _ => None, // Unknown message type
        }
    }
}
