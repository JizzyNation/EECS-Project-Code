import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.nio.charset.StandardCharsets;
import java.util.function.Consumer;

public class UdpSensorServer implements Runnable {
    private final int port;
    private final Consumer<SensorData> dataConsumer;
    private final Consumer<String> logConsumer;

    public UdpSensorServer(int port, Consumer<SensorData> dataConsumer, Consumer<String> logConsumer) {
        this.port = port;
        this.dataConsumer = dataConsumer;
        this.logConsumer = logConsumer;
    }

    @Override
    public void run() {
        try (DatagramSocket socket = new DatagramSocket(port)) {
            logConsumer.accept("UDP server started on port " + port);

            byte[] buffer = new byte[512];

            while (true) {
                DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
                socket.receive(packet);

                String message = new String(packet.getData(), 0, packet.getLength(), StandardCharsets.UTF_8).trim();
                logConsumer.accept("UDP packet from " + packet.getAddress() + ": " + message);

                try {
                    SensorData data = MessageParser.parseSensorMessage(message);
                    dataConsumer.accept(data);
                } catch (Exception ex) {
                    logConsumer.accept("Failed to parse UDP message: " + message + " | " + ex.getMessage());
                }
            }
        } catch (Exception e) {
            logConsumer.accept("UDP server error: " + e.getMessage());
        }
    }
}