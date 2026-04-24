import javax.swing.*;

public class DashboardApp {
    private static final int PORT = 5000;

    // true = TCP, false = UDP
    private static final boolean USE_TCP = true;

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            DashboardFrame frame = new DashboardFrame();
            frame.setVisible(true);

            if (USE_TCP) {
                TcpSensorServer tcpServer = new TcpSensorServer(
                        PORT,
                        frame::updateSensorData,
                        frame::appendLog
                );

                frame.setCommandHandler(tcpServer::sendCommand);

                Thread serverThread = new Thread(tcpServer, "tcp-server-thread");
                serverThread.setDaemon(true);
                serverThread.start();

                frame.appendLog("Dashboard running in TCP mode");
            } else {
                UdpSensorServer udpServer = new UdpSensorServer(
                        PORT,
                        frame::updateSensorData,
                        frame::appendLog
                );

                frame.setCommandHandler(command ->
                        frame.appendLog("Command sending is disabled in UDP demo mode: " + command)
                );

                Thread serverThread = new Thread(udpServer, "udp-server-thread");
                serverThread.setDaemon(true);
                serverThread.start();

                frame.appendLog("Dashboard running in UDP mode");
            }
        });
    }
}