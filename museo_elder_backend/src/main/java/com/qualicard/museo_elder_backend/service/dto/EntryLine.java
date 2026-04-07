// src/main/java/com/qualicard/museo_elder_backend/service/dto/EntryLine.java
package com.qualicard.museo_elder_backend.service.dto;

import java.time.LocalTime;

public class EntryLine {
    public final String key;       // clave normalizada (la que irá en el fichero)
    public final int pasos;
    public final LocalTime time;   // hh:mm:ss

    public EntryLine(String key, int pasos, LocalTime time) {
        this.key = key; this.pasos = pasos; this.time = time;
    }
}
