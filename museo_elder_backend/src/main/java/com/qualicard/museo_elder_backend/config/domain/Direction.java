package com.qualicard.museo_elder_backend.config.domain;

public enum Direction {
    IN, OUT;

    public static Direction fromString(String s) {
        if (s == null) return IN;
        switch (s.trim().toUpperCase()) {
            case "OUT": return OUT;
            default:    return IN;
        }
    }
}
