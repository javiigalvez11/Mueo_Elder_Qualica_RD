package com.qualicard.museo_elder_backend.service;

import com.qualicard.museo_elder_backend.config.domain.Device;
import com.qualicard.museo_elder_backend.config.domain.Direction;
import com.qualicard.museo_elder_backend.service.dto.StatusInicioReq;
import com.qualicard.museo_elder_backend.service.dto.ValidatePassReq;
import com.qualicard.museo_elder_backend.service.dto.ValidateQRReq;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Service;

import java.time.Instant;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;

@Service
public class CoreService {

    private static final Logger log = LoggerFactory.getLogger(CoreService.class);

    /** Dispositivos en memoria */
    private final Map<String, Device> devices = new ConcurrentHashMap<>();

    /** Sesiones de paso por (deviceId|barcode) */
    private final Map<String, PassSession> sessions = new ConcurrentHashMap<>();

    /** Estructura mínima para trackear una entrada en curso */
    private static final class PassSession {
        final String deviceId;
        final String barcode;
        volatile int ntExpected;      // total de pasos esperado para esta entrada
        volatile int lastNp;          // último np observado
        volatile Direction dir;       // === NUEVO: dirección fijada en validateQR
        final Instant startedAt;

        PassSession(String deviceId, String barcode, int ntExpected, Direction dir) {
            this.deviceId = deviceId;
            this.barcode  = barcode == null ? "" : barcode;
            this.ntExpected = Math.max(1, ntExpected);
            this.lastNp = 0;
            this.dir = (dir == null ? Direction.IN : dir);
            this.startedAt = Instant.now();
        }

        String key() { return key(deviceId, barcode); }

        static String key(String deviceId, String barcode) {
            return (deviceId == null ? "" : deviceId) + "|" + (barcode == null ? "" : barcode);
        }
    }

    /* =================== API pública usada por tu controlador =================== */

    /** Crea o devuelve un Device ya existente. Actualiza comercio si viene no-nulo. */
    public Device getOrCreate(String id, String comercio) {
        Objects.requireNonNull(id, "id");
        Device dev = devices.computeIfAbsent(id, k -> new Device(k, comercio));
        if (comercio != null && !comercio.isBlank()) {
            dev.comercio = comercio;
        }
        return dev;
    }

    /** Actualiza/crea el dispositivo con datos del heartbeat/status. */
    public Device updateStatus(StatusInicioReq req) {
        Device dev = getOrCreate(req.id, req.comercio);
        dev.registered = true;
        if (req.status != null && !req.status.isBlank()) dev.status = req.status;
        if (req.version != null && !req.version.isBlank()) dev.version = req.version;
        return dev;
    }

    /**
     * Registrar paso (progreso). Devuelve el NT "consolidado" conocido por el backend
     * para la sesión (deviceId|barcode). Si la petición trae nt, se actualiza.
     */
    public int registrarPaso(ValidatePassReq req) {
        Objects.requireNonNull(req.id, "id (device)");
        final String deviceId = req.id;
        final String barcode  = Optional.ofNullable(req.barcode).orElse("");
        final int ntIncoming  = parseIntSafe(req.nt, 0);
        final int npIncoming  = parseIntSafe(req.np, 0);

        PassSession session = sessions.compute(PassSession.key(deviceId, barcode), (k, old) -> {
            if (old == null) {
                int nt0 = ntIncoming > 0 ? ntIncoming : 1;
                // Si no hay sesión previa, no sabemos dirección aún: por defecto IN
                return new PassSession(deviceId, barcode, nt0, Direction.IN);
            } else {
                if (ntIncoming > 0) {
                    old.ntExpected = ntIncoming;
                }
                if (npIncoming > old.lastNp) {
                    old.lastNp = npIncoming;
                }
                return old;
            }
        });

        if (log.isDebugEnabled()) {
            log.debug("[SESSION] dev={} barcode={} ntExpected={} lastNp={} dir={} since={}",
                    session.deviceId, session.barcode, session.ntExpected, session.lastNp,
                    session.dir, session.startedAt);
        }

        return session.ntExpected;
    }

    /** Guarda NT y dirección de la sesión en validateQR. */
    public void recordValidation(ValidateQRReq req, int ntFromValidation, Direction dir) {
        if (req == null || req.id == null) return;
        final String key = PassSession.key(req.id, req.barcode);
        sessions.compute(key, (k, old) -> {
            if (old == null) {
                return new PassSession(req.id, req.barcode, ntFromValidation, dir);
            } else {
                old.ntExpected = ntFromValidation;
                old.dir = (dir == null ? Direction.IN : dir);
                return old;
            }
        });
        log.info("[SESSION] VALIDATE stored nt={} dir={} for dev={} barcode={}",
                ntFromValidation, (dir == null ? Direction.IN : dir), req.id, req.barcode);
    }

    /** Devuelve la dirección registrada para (deviceId|barcode), si existe. */
    public Optional<Direction> getSessionDirection(String deviceId, String barcode) {
        PassSession s = sessions.get(PassSession.key(deviceId, barcode));
        return Optional.ofNullable(s == null ? null : s.dir);
    }

    /** Limpia/termina una sesión cuando acabes (por ejemplo, tras PASSED). */
    public void endSession(String deviceId, String barcode) {
        sessions.remove(PassSession.key(deviceId, barcode));
    }

    /* =================== Utils =================== */

    private static int parseIntSafe(String s, int fallback) {
        try { return (s == null || s.isBlank()) ? fallback : Integer.parseInt(s); }
        catch (Exception e) { return fallback; }
    }
}
