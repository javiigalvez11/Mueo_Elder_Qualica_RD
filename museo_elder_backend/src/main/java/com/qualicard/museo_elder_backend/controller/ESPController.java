package com.qualicard.museo_elder_backend.controller;

import com.qualicard.museo_elder_backend.config.domain.Direction;
import com.qualicard.museo_elder_backend.config.domain.StatusCode;
import com.qualicard.museo_elder_backend.service.CoreService;
import com.qualicard.museo_elder_backend.service.dto.StatusInicioReq;
import com.qualicard.museo_elder_backend.service.dto.ValidatePassReq;
import com.qualicard.museo_elder_backend.service.dto.ValidateQRReq;
import com.qualicard.museo_elder_backend.service.mem.TicketMemory;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.regex.Pattern;

@RestController
@RequestMapping(
        path = {"/QRDService/api"}, // compatibilidad opcional
        produces = MediaType.APPLICATION_JSON_VALUE
)
public class ESPController {

    private static final Logger log = LoggerFactory.getLogger(ESPController.class);

    private final CoreService core;
    private final TicketMemory ticketMemory;

    public ESPController(CoreService core, TicketMemory ticketMemory) {
        this.core = core;
        this.ticketMemory = ticketMemory;
    }

    // =================== Helpers comunes ===================
    private Map<String, String> ok(Map<String, String> extra) {
        Map<String, String> base = new HashMap<>();
        base.put("r", "ok"); // minúscula según nueva especificación
        if (extra != null) base.putAll(extra);
        return base;
    }

    private Map<String, String> ko(String code, String desc) {
        Map<String, String> base = new HashMap<>();
        base.put("r", "ko");
        base.put("ec", code);
        base.put("ed", desc);
        return base;
    }

    /** 201 = IN, 202 = OUT (default IN) */
    private Direction directionFromStatus(String status) {
        if ("202".equals(status)) return Direction.OUT;
        if ("201".equals(status)) return Direction.IN;
        return Direction.IN;
    }

    private static final Pattern TEC_COLON = Pattern.compile("^\\d{6}:\\d{6}$");
    private static final Pattern UUID36    = Pattern.compile("^[A-Za-z0-9-]{36}$");
    private static final Pattern MAGE19    = Pattern.compile("^\\d{19}$");

    private static boolean isBlank(String s){ return s == null || s.isBlank(); }

    private enum Kind { TEC, ODOO, MAGE, UNKNOWN }

    // =========================================================
    // 1) /status  -> SOLO exige id; ignora el resto y responde fijo
    //    Respuesta: {"r":"ok","id":"<ID>","status":"200","ec":"CMD_READY"}
    // =========================================================
    @PostMapping(path = "/status", consumes = MediaType.APPLICATION_JSON_VALUE)
    public ResponseEntity<Map<String,String>> status(@RequestBody(required = false) Map<String, Object> body){

        if (body == null) {
            log.warn("[STATUS][REQ] body NULL (request sin body o ilegible)");
            return ResponseEntity.badRequest().body(ko("400","body missing"));
        }

        String id = Optional.ofNullable(body.get("id")).map(Object::toString).orElse("");
        if (id.isBlank()) {
            log.info("[STATUS][REQ] ERROR id vacío");
            return ResponseEntity.badRequest().body(ko("400","id vacío"));
        }
        log.info("[STATUS][RES] id={} status_tx=200 ec_tx=CMD_READY", id);

        Map<String,String> resp = ok(null);
        resp.put("id", id);
        resp.put("status", "200");
        resp.put("ec", "CMD_READY");
        return ResponseEntity.ok(resp);
    }

    // =========================================================
    // 2) /validateQR -> TEC (preferido), Odoo, Mage
    // =========================================================
    @PostMapping(path = "/validateQR", consumes = MediaType.APPLICATION_JSON_VALUE)
    public ResponseEntity<Map<String,String>> validateQR(@RequestBody ValidateQRReq req){
        if (req == null || isBlank(req.id)) {
            log.info("[VALIDATE_QR][REQ] ERROR id vacío");
            return ResponseEntity.badRequest().body(ko("400","id vacío"));
        }

        final String id = req.id;
        String ticketId = req.ticket_id;
        String eventId  = req.event_id;
        String barcode  = req.barcode;

        // TEC vía barcode "123456:654321"
        if (isBlank(ticketId) && isBlank(eventId) && !isBlank(barcode) && TEC_COLON.matcher(barcode).matches()) {
            String[] parts = barcode.split(":");
            ticketId = parts[0];
            eventId  = parts[1];
            barcode  = null; // ya tratado como TEC
        }

        // Determinar tipo final
        Kind kind;
        if (!isBlank(ticketId) && !isBlank(eventId) && ticketId.matches("\\d{6}") && eventId.matches("\\d{6}")) {
            kind = Kind.TEC;
        } else if (!isBlank(barcode) && UUID36.matcher(barcode).matches()) {
            kind = Kind.ODOO;
        } else if (!isBlank(barcode) && MAGE19.matcher(barcode).matches()) {
            kind = Kind.MAGE;
        } else {
            kind = Kind.UNKNOWN;
        }

        if (kind == Kind.UNKNOWN) {
            Map<String,String> body = ko("404","BARCODE_NO_MATCH");
            log.info("[VALIDATE_QR][RES] id={} http=404 body={}", id, body);
            return ResponseEntity.status(HttpStatus.NOT_FOUND).body(body);
        }

        // Normalización para control de doble-uso
        final String normCode = (kind == Kind.TEC) ? (ticketId + ":" + eventId) : barcode;

        // 409 si ya USADO
        if (ticketMemory.isUsed(normCode)) {
            Map<String,String> body = ko("409","TICKET_ALREADY_USED");
            log.info("[VALIDATE_QR][RES] id={} http=409 body={}", id, body);
            return ResponseEntity.status(HttpStatus.CONFLICT).body(body);
        }

        // Dirección (por status Rx si llega; si no, IN por defecto)
        String rxStatus = Optional.ofNullable(req.status).orElse("");
        Direction dir   = directionFromStatus(rxStatus);
        final String statusNumeric = (dir == Direction.IN) ? "201" : "202";
        final String ecTx          = (dir == Direction.IN) ? "CMD_VALIDATE_IN" : "CMD_VALIDATE_OUT";

        // Marcar validación (queda VALIDATED; se marcará USED en /validatePass)
        ticketMemory.markValidated(normCode, dir.name(), 1);
        core.recordValidation(req, 1, dir); // si tu servicio lo usa

        Map<String,String> resp = ok(null);
        resp.put("id", id);
        resp.put("status", statusNumeric);
        resp.put("ec", ecTx);
        resp.put("nt", "1");
        resp.put("np","0");
        if (kind == Kind.TEC) {
            resp.put("ticket_id", ticketId);
            resp.put("event_id",  eventId);
        } else {
            resp.put("barcode",   normCode);
        }

        log.info("[VALIDATE_QR][RES] id={} http=200 kind={} status_tx={} ec_tx={}", id, kind, statusNumeric, ecTx);
        return ResponseEntity.ok(resp);
    }

    // =========================================================
    // 3) /validatePass
    // =========================================================
    @PostMapping(path = "/validatePass", consumes = MediaType.APPLICATION_JSON_VALUE)
    public ResponseEntity<Map<String,String>> validatePass(@RequestBody ValidatePassReq req){
        if (req == null || isBlank(req.id)) {
            log.info("[VALIDATE_PASS][REQ] ERROR id vacío");
            return ResponseEntity.badRequest().body(ko("400","id vacío"));
        }
        if (isBlank(req.barcode) && (isBlank(req.ticket_id) || isBlank(req.event_id))) {
            log.info("[VALIDATE_PASS][REQ] ERROR identificador vacío (ni barcode ni ticket+event)");
            return ResponseEntity.badRequest().body(ko("400","identificador vacío"));
        }

        final String id = req.id;

        // Reconstruir normCode igual que en validateQR
        String normCode;
        if (!isBlank(req.ticket_id) && !isBlank(req.event_id)) {
            normCode = req.ticket_id + ":" + req.event_id;
        } else if (!isBlank(req.barcode) && TEC_COLON.matcher(req.barcode).matches()) {
            normCode = req.barcode; // TEC por barcode colon
        } else {
            normCode = req.barcode; // Odoo/Mage
        }

        String rxStatus  = Optional.ofNullable(req.status).orElse("");
        String rxEc      = Optional.ofNullable(req.ec).orElse(""); // CMD_PASS_OK / CMD_PASS_TIMEOUT / CMD_PASS_PROGRESS
        int np = safeInt(req.np, 0);
        int nt = safeInt(req.nt, 1);
        Direction dir = directionFromStatus(rxStatus);

        log.info("[VALIDATE_PASS][REQ] id={} status_rx={} ec_rx={} dir={} normCode='{}' np={} nt={}",
                id, rxStatus, rxEc, dir, normCode, np, nt);

        String ecTx;
        Map<String,String> resp = ok(null);

        switch (rxEc) {
            case "CMD_PASS_OK":
                // Se completó el paso -> marcar USADO y volver a READY
                ticketMemory.markUsed(normCode);
                core.endSession(id, normCode);


                resp.put("id", id);
                resp.put("status", StatusCode.READY); // "200"
                resp.put("ec", "CMD_READY");
                return ResponseEntity.ok(resp);

            case "CMD_PASS_TIMEOUT":
                ecTx = "CMD_TIMEOUT";

                resp.put("id", id);
                resp.put("status", StatusCode.READY);
                resp.put("ec", ecTx);
                return ResponseEntity.ok(resp);

            case "CMD_PASS_PROGRESS":
                ecTx = (dir == Direction.OUT) ? "CMD_PASSING_OUT" : "CMD_PASSING_IN";

                resp.put("id", id);
                resp.put("status", (dir == Direction.IN) ? "201" : "202");
                resp.put("ec", ecTx);
                resp.put("np", String.valueOf(np));
                resp.put("nt", String.valueOf(nt));
                return ResponseEntity.ok(resp);

            default:
                resp.put("id", id);
                resp.put("status", StatusCode.READY);
                resp.put("ec", "READY");
                return ResponseEntity.ok(resp);
        }
    }

    // =========================================================
    // 4) /reportFailure
    // =========================================================
    @PostMapping(path = "/reportFailure", consumes = MediaType.APPLICATION_JSON_VALUE)
    public ResponseEntity<Map<String,String>> reportFailure(@RequestBody StatusInicioReq req){
        if (req == null || isBlank(req.id)) {
            log.info("[FAILURE][REQ] ERROR id vacío");
            return ResponseEntity.badRequest().body(ko("400","id vacío"));
        }
        Map<String,String> resp = ok(null);
        resp.put("id", req.id);
        resp.put("status", StatusCode.ABORTED); // "300"
        resp.put("ec", "FAILURE");

        log.info("[FAILURE][RES] id={} http=200 status_tx=300 ec_tx=FAILURE", req.id);
        return ResponseEntity.ok(resp);
    }

    // Ping básico
    @GetMapping("/ping")
    public Map<String,String> ping() {
        Map<String,String> resp = ok(null);
        resp.put("id", "ESP");
        resp.put("status", StatusCode.READY);
        resp.put("ec", "PONG");
        log.info("[PING] http=200 status_tx=200 ec_tx=PONG");
        return resp;
    }

    private static int safeInt(String s, int fallback) {
        try { return (s == null || s.isBlank()) ? fallback : Integer.parseInt(s); }
        catch (Exception e) { return fallback; }
    }
}
