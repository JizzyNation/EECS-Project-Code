public class MessageParser {

    private MessageParser() {
    }

    public static SensorData parseSensorMessage(String message) {
        if (message == null || message.isBlank()) {
            throw new IllegalArgumentException("Message is empty");
        }

        String[] parts = message.trim().split(",");
        if (parts.length != 6) {
            throw new IllegalArgumentException("Invalid message format: " + message);
        }

        if (!"SENSOR".equalsIgnoreCase(parts[0])) {
            throw new IllegalArgumentException("Unsupported message type: " + parts[0]);
        }

        String deviceId = parts[1];
        long timestampMs = Long.parseLong(parts[2]);
        int adcRaw = Integer.parseInt(parts[3]);
        double voltage = Double.parseDouble(parts[4]);
        boolean ledState = Integer.parseInt(parts[5]) == 1;

        return new SensorData(deviceId, timestampMs, adcRaw, voltage, ledState);
    }
}