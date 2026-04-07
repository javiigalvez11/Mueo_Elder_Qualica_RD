// src/main/java/com/qualicard/museo_elder_backend/service/mem/EntryStore.java
package com.qualicard.museo_elder_backend.service.mem;

import com.qualicard.museo_elder_backend.service.dto.EntryLine;

import java.time.LocalTime;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

public class EntryStore {

    // deviceId -> lista de entradas programadas
    private final Map<String, List<EntryLine>> store = new ConcurrentHashMap<>();

    public List<EntryLine> getEntries(String deviceId) {
        return store.getOrDefault(deviceId, Collections.emptyList());
    }

    public void putEntries(String deviceId, List<EntryLine> lines) {
        store.put(deviceId, new ArrayList<>(lines));
    }

    public void clear(String deviceId) {
        store.remove(deviceId);
    }

    // helper para tests / seed
    public void seedDemo(String deviceId) {
        List<EntryLine> list = new ArrayList<>();
        list.add(new EntryLine("9d9c2a28-aaaa-bbbb-cccc-1234567890ab", 1, LocalTime.of(10, 15, 0)));
        list.add(new EntryLine("ticket_id=110798&event_id=110793", 2, LocalTime.of(11, 45, 30)));
        list.add(new EntryLine("3132269128684132270", 1, LocalTime.of(12, 0, 0)));
        putEntries(deviceId, list);
    }
}
