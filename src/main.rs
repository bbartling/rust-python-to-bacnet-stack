use std::error::Error;
use tokio::net::UdpSocket;
mod npdu;
use npdu::NPDU;

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    let socket_addr = "0.0.0.0:47808"; // BACnet standard port
    println!("Starting UDP server on {}", socket_addr);
    let socket = UdpSocket::bind(socket_addr).await?;
    socket.set_broadcast(true)?;

    loop {
        let mut buf = [0; 1024];
        let (len, addr) = socket.recv_from(&mut buf).await?;

        println!("Received {} bytes from {}", len, addr);

        // Decode the received data into an NPDU
        match NPDU::decode(&buf[..len]) {
            Ok(npdu_message) => {
                println!("Processed NPDU: {:?}", npdu_message);

                // Determine BACnet message type
                if let Some(message_type) = NPDU::determine_bacnet_message(&buf[..len]) {
                    println!("Received a BACnet message: {:?}", message_type);
                    // Further processing based on the message type
                } else {
                    println!("Not a BACnet message or unknown message type");
                }
            },
            Err(e) => println!("Failed to decode NPDU: {}", e),
        }
    }
}
