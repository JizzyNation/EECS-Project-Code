public class SensorData {
    private final String deviceId;
    private final long timestampMs;
    private final int adcRaw;
    private final double voltage;
    private final boolean ledState;

    public SensorData(String deviceId, long timestampMs, int adcRaw, double voltage, boolean ledState) {
        this.deviceId = deviceId;
        this.timestampMs = timestampMs;
        this.adcRaw = adcRaw;
        this.voltage = voltage;
        this.ledState = ledState;
    }

    public String getDeviceId() {
        return deviceId;
    }

    public long getTimestampMs() {
        return timestampMs;
    }

    public int getAdcRaw() {
        return adcRaw;
    }

    public double getVoltage() {
        return voltage;
    }

    public boolean isLedState() {
        return ledState;
    }

    @Override
    public String toString() {
        return "SensorData{" +
                "deviceId='" + deviceId + '\'' +
                ", timestampMs=" + timestampMs +
                ", adcRaw=" + adcRaw +
                ", voltage=" + voltage +
                ", ledState=" + ledState +
                '}';
    }
}