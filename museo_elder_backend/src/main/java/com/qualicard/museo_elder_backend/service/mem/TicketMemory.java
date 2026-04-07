// src/main/java/.../service/mem/TicketMemory.java
package com.qualicard.museo_elder_backend.service.mem;

import org.springframework.stereotype.Component;

import java.time.Instant;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

@Component
public class TicketMemory {

    public enum State { NEW, VALIDATED, USED }

    public static final class Rec {
        public final String barcode;
        public volatile State state = State.NEW;
        public volatile String dir; // "IN"/"OUT"
        public volatile int nt = 1;
        public final Instant createdAt = Instant.now();
        public volatile Instant updatedAt = Instant.now();

        public Rec(String barcode) { this.barcode = barcode; }
    }

    private final Map<String, Rec> store = new ConcurrentHashMap<>();

    private static String norm(String s) {
        return s == null ? "" : s.trim().toUpperCase(Locale.ROOT);
    }

    public Rec get(String bc) { return store.get(norm(bc)); }

    public boolean isUsed(String bc) {
        Rec r = store.get(norm(bc));
        return r != null && r.state == State.USED;
    }

    public Rec ensure(String bc) {
        return store.computeIfAbsent(norm(bc), Rec::new);
    }

    public void markValidated(String bc, String dir, int nt) {
        Rec r = ensure(bc);
        r.dir = dir;
        r.nt  = nt;
        r.state = State.VALIDATED;
        r.updatedAt = Instant.now();
    }

    public void markUsed(String bc) {
        Rec r = ensure(bc);
        r.state = State.USED;
        r.updatedAt = Instant.now();
    }
}
