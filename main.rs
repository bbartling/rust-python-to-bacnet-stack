use std::collections::HashMap;
use std::error::Error;
use std::net::SocketAddr;
use tokio::net::UdpSocket;

type Any = String;

#[derive(Debug)]
struct PCI {
    pdu_source: Option<SocketAddr>,
    pdu_destination: Option<SocketAddr>,
    pdu_expecting_reply: bool,
    pdu_network_priority: i32,
    pdu_user_data: Option<Vec<u8>>,
}

impl PCI {
    fn new(
        source: Option<SocketAddr>,
        destination: Option<SocketAddr>,
        expecting_reply: bool,
        network_priority: i32,
        user_data: Option<Vec<u8>>,
    ) -> Self {
        PCI {
            pdu_source: source,
            pdu_destination: destination,
            pdu_expecting_reply: expecting_reply,
            pdu_network_priority: network_priority,
            pdu_user_data: user_data,
        }
    }

    fn pci_contents(&self) -> HashMap<String, Any> {
        let mut contents = HashMap::new();
        contents.insert("source".to_string(), self.pdu_source.map_or("None".to_string(), |v| v.to_string()));
        contents.insert("destination".to_string(), self.pdu_destination.map_or("None".to_string(), |v| v.to_string()));
        contents.insert("expectingReply".to_string(), self.pdu_expecting_reply.to_string());
        contents.insert("networkPriority".to_string(), self.pdu_network_priority.to_string());
        if let Some(ref data) = self.pdu_user_data {
            contents.insert("user_data_length".to_string(), data.len().to_string());
        } else {
            contents.insert("user_data_length".to_string(), "0".to_string());
        }

        contents
    }
}

#[derive(Debug)]
struct PDUData {
    pdu_data: Vec<u8>,
}

impl PDUData {
    fn new(data: Option<&[u8]>) -> Self {
        PDUData { pdu_data: data.unwrap_or(&[]).to_vec() }
    }

    fn to_string(&self) -> Result<String, std::string::FromUtf8Error> {
        String::from_utf8(self.pdu_data.clone())
    }
}

#[derive(Debug)]
struct PDU {
    pci: PCI,
    pdu_data: PDUData,
}

impl PDU {
    fn new(pci: PCI, pdu_data: PDUData) -> Self {
        PDU { pci, pdu_data }
    }

    fn dict_contents(&self) -> HashMap<String, Any> {
        let mut contents = self.pci.pci_contents();

        let pdu_data_string = match self.pdu_data.to_string() {
            Ok(str) => str,
            Err(_) => "Invalid UTF-8 sequence".to_string(),
        };

        contents.insert("pdu_data".to_string(), pdu_data_string);
        contents.insert("pdu_data_length".to_string(), self.pdu_data.pdu_data.len().to_string());

        contents
    }
}

fn is_bacnet_message(data: &[u8]) -> bool {
    data.starts_with(&[0x81, 0x0B]) || data.starts_with(&[0x81, 0x0A])
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    println!("Starting UDP server on 0.0.0.0:47808...");
    let socket = UdpSocket::bind("0.0.0.0:47808").await?;
    println!("UDP server listening...");

    let mut buf = [0; 1024];

    loop {
        let (len, addr) = socket.recv_from(&mut buf).await?;
        println!("Received {} bytes from {}", len, addr);

        let destination_addr = "0.0.0.0:47808".parse::<SocketAddr>().unwrap();

        let pci = PCI::new(Some(addr), Some(destination_addr), true, 3, Some(buf[..len].to_vec()));
        let pdu_data = PDUData::new(Some(&buf[..len]));
        let pdu = PDU::new(pci, pdu_data);

        println!("Structured PDU: {:?}", pdu);

        if is_bacnet_message(&buf[..len]) {
            println!("Received a BACnet message");
            // Further BACnet-specific processing can be done here
        } else {
            match pdu.pdu_data.to_string() {
                Ok(message) => println!("Decoded message: {}", message),
                Err(e) => println!("Failed to decode message: {:?}", e),
            }
        }
        println!("PDU contents: {:?}", pdu.dict_contents());
    }
}
