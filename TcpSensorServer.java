import java.io.*;
import java.net.*;
import java.util.function.Consumer;

public class TcpSensorServer implements Runnable {
    private final int port;
    private final Consumer<SensorData> dataConsumer;
    private final Consumer<String> logConsumer;

    private volatile PrintWriter clientWriter;

    public TcpSensorServer(int port, Consumer<SensorData> dataConsumer, Consumer<String> logConsumer) {
        this.port = port;
        this.dataConsumer = dataConsumer;
        this.logConsumer = logConsumer;
    }

    @Override
    public void run() {
        try (ServerSocket serverSocket = new ServerSocket(port)) {
            logConsumer.accept("TCP server started on port " + port);

            while (true) {
                Socket clientSocket = serverSocket.accept();
                logConsumer.accept("TCP client connected: " + clientSocket.getInetAddress());

                handleClient(clientSocket);
            }
        } catch (IOException e) {
            logConsumer.accept("TCP server error: " + e.getMessage());
        }
    }

    private void handleClient(Socket clientSocket) {
        try (
                Socket socket = clientSocket;
                BufferedReader reader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
                PrintWriter writer = new PrintWriter(new OutputStreamWriter(socket.getOutputStream()), true)
        ) {
            this.clientWriter = writer;

            String line;
            while ((line = reader.readLine()) != null) {
                try {
                    SensorData data = MessageParser.parseSensorMessage(line);
                    dataConsumer.accept(data);
                } catch (Exception ex) {
                    logConsumer.accept("Failed to parse TCP message: " + line + " | " + ex.getMessage());
                }
            }

            logConsumer.accept("TCP client disconnected");
        } catch (IOException e) {
            logConsumer.accept("TCP client error: " + e.getMessage());
        } finally {
            clientWriter = null;
        }
    }

    public void sendCommand(String command) {
        PrintWriter writer = clientWriter;
        if (writer != null) {
            writer.println(command);
            logConsumer.accept("Sent command: " + command);
        } else {
            logConsumer.accept("No TCP client connected. Command not sent: " + command);
        }
    }
}