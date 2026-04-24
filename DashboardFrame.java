import javax.swing.*;
import java.awt.*;
import java.util.function.Consumer;

public class DashboardFrame extends JFrame {
    private final JLabel deviceLabel = new JLabel("Device: -");
    private final JLabel timeLabel = new JLabel("Timestamp: -");
    private final JLabel adcLabel = new JLabel("ADC Raw: -");
    private final JLabel voltageLabel = new JLabel("Voltage: -");
    private final JLabel ledLabel = new JLabel("LED State: -");
    private final JTextArea logArea = new JTextArea();

    private final JButton ledOnButton = new JButton("LED ON");
    private final JButton ledOffButton = new JButton("LED OFF");
    private final JButton setSampleButton = new JButton("Set Sample = 1000 ms");

    public DashboardFrame() {
        setTitle("Wireless Sensor Dashboard");
        setSize(700, 500);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setLocationRelativeTo(null);

        JPanel mainPanel = new JPanel(new BorderLayout(10, 10));
        JPanel dataPanel = new JPanel(new GridLayout(5, 1, 5, 5));
        JPanel controlPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        JScrollPane scrollPane = new JScrollPane(logArea);

        dataPanel.setBorder(BorderFactory.createTitledBorder("Live Sensor Data"));
        controlPanel.setBorder(BorderFactory.createTitledBorder("Controls"));

        dataPanel.add(deviceLabel);
        dataPanel.add(timeLabel);
        dataPanel.add(adcLabel);
        dataPanel.add(voltageLabel);
        dataPanel.add(ledLabel);

        controlPanel.add(ledOnButton);
        controlPanel.add(ledOffButton);
        controlPanel.add(setSampleButton);

        logArea.setEditable(false);

        mainPanel.add(dataPanel, BorderLayout.NORTH);
        mainPanel.add(controlPanel, BorderLayout.CENTER);
        mainPanel.add(scrollPane, BorderLayout.SOUTH);

        scrollPane.setPreferredSize(new Dimension(650, 220));

        add(mainPanel);
    }

    public void updateSensorData(SensorData data) {
        SwingUtilities.invokeLater(() -> {
            deviceLabel.setText("Device: " + data.getDeviceId());
            timeLabel.setText("Timestamp: " + data.getTimestampMs() + " ms");
            adcLabel.setText("ADC Raw: " + data.getAdcRaw());
            voltageLabel.setText(String.format("Voltage: %.3f V", data.getVoltage()));
            ledLabel.setText("LED State: " + (data.isLedState() ? "ON" : "OFF"));
            appendLog("Received: " + data);
        });
    }

    public void appendLog(String message) {
        SwingUtilities.invokeLater(() -> logArea.append(message + "\n"));
    }

    public void setCommandHandler(Consumer<String> commandHandler) {
        ledOnButton.addActionListener(e -> commandHandler.accept("LED,ON"));
        ledOffButton.addActionListener(e -> commandHandler.accept("LED,OFF"));
        setSampleButton.addActionListener(e -> commandHandler.accept("SAMPLE,1000"));
    }
}