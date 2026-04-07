package com.qualicard.museo_elder_backend.config.domain;

import java.time.Instant;

public class Device {
    public final String id;
    public String comercio;
    public String status = "200";
    public String version = "";
    public boolean registered = false;

    public Device(String id, String comercio) {
        this.id = id;
        this.comercio = comercio;
    }

    @Override
    public String toString() {
        return "Device{" +
                "id='" + id + '\'' +
                ", comercio='" + comercio + '\'' +
                ", status='" + status + '\'' +
                ", version='" + version + '\'' +
                ", registered=" + registered +
                '}';
    }
}
