// src/main/java/com/qualicard/museo_elder_backend/controller/TicketsController.java
package com.qualicard.museo_elder_backend.controller;

import com.qualicard.museo_elder_backend.service.dto.EntryLine;
import com.qualicard.museo_elder_backend.service.mem.EntryStore;
import com.qualicard.museo_elder_backend.service.qr.QRParser;
import com.qualicard.museo_elder_backend.util.Resp;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.core.io.ClassPathResource;
import org.springframework.http.*;
import org.springframework.web.bind.annotation.*;


import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.time.LocalTime;
import java.util.*;
import java.util.stream.Collectors;

@RestController
@RequestMapping(path = "/PTService/ESP32", produces = MediaType.APPLICATION_JSON_VALUE)
public class TicketsController {

    private static final Logger log = LoggerFactory.getLogger(TicketsController.class);
    private final EntryStore entryStore;

    // Si colocas un fichero estático: src/main/resources/entradas/entradas.txt (formato v2)
    private static final String CLASSPATH_TXT = "entradas/entradas.txt";

    public TicketsController(EntryStore entryStore) {
        this.entryStore = entryStore;
    }

    // =========================================================
    // A) GET /entries?id=ME002   (compatibilidad; devuelve texto plano)
    //    Formato v1 (<code>:<pasos>:HH:MM:SS) — tu firmware usa v2, así que mejor el POST de abajo.
    // =========================================================
    @GetMapping(path = "/entries", produces = MediaType.TEXT_PLAIN_VALUE)
    public ResponseEntity<String> entries(@RequestParam("id") String deviceId) {
        if (deviceId == null || deviceId.isBlank()) {
            return ResponseEntity.badRequest()
                    .contentType(MediaType.APPLICATION_JSON)
                    .body(Resp.ko("400","id vacío").toString());
        }

        List<EntryLine> lines = entryStore.getEntries(deviceId);
        StringBuilder sb = new StringBuilder("OK\n");
        for (EntryLine e : lines) {
            sb.append(e.key).append(':')
                    .append(e.pasos).append(':')
                    .append(String.format("%02d", e.time.getHour())).append(':')
                    .append(String.format("%02d", e.time.getMinute())).append(':')
                    .append(String.format("%02d", e.time.getSecond()))
                    .append('\n');
        }
        String body = sb.toString();
        log.info("[ENTRIES][GET][RES] id={} bytes={}", deviceId, body.getBytes(StandardCharsets.UTF_8).length);
        return ResponseEntity.ok().contentType(MediaType.TEXT_PLAIN).body(body);
    }

    // =========================================================
    // A2) POST /entries   (NUEVO) — recibe JSON {id,status,ec} y
    //      responde TEXTO PLANO en formato v2: "OK\n<code>:HH:MM;\n..."
    //      1) Si existe entradas/entradas.txt en classpath => lo sirve tal cual
    //      2) Si no, genera desde EntryStore a formato v2
    // =========================================================
    public static class StateReq { public String id; public String status; public String ec; }

    @PostMapping(path = "/entries", consumes = MediaType.APPLICATION_JSON_VALUE, produces = MediaType.TEXT_PLAIN_VALUE)
    public ResponseEntity<String> entriesPost(@RequestBody StateReq req) {
        if (req == null || req.id == null || req.id.isBlank()) {
            return ResponseEntity.badRequest()
                    .contentType(MediaType.APPLICATION_JSON)
                    .body(Resp.ko("400","id vacío").toString());
        }

        // 1) Intentar servir fichero estático (si lo has puesto)
        try {
            ClassPathResource res = new ClassPathResource(CLASSPATH_TXT);
            if (res.exists()) {
                String body = Files.readString(res.getFile().toPath(), StandardCharsets.UTF_8);
                log.info("[ENTRIES][POST][RAW] id={} bytes={} (classpath)", req.id, body.getBytes(StandardCharsets.UTF_8).length);
                return ResponseEntity.ok().contentType(MediaType.TEXT_PLAIN).body(body);
            }
        } catch (Exception ignore) {}

        // 2) Generar desde store a formato v2 (<code>:HH:MM;)
        List<EntryLine> lines = entryStore.getEntries(req.id);
        StringBuilder sb = new StringBuilder("OK\n");
        for (EntryLine e : lines) {
            sb.append(e.key).append(':')
                    .append(String.format("%02d", e.time.getHour())).append(':')
                    .append(String.format("%02d", e.time.getMinute()))
                    .append(';')
                    .append('\n');
        }
        String body = sb.toString();
        log.info("[ENTRIES][POST][BUILT] id={} bytes={}", req.id, body.getBytes(StandardCharsets.UTF_8).length);
        return ResponseEntity.ok().contentType(MediaType.TEXT_PLAIN).body(body);
    }

    // =========================================================
    // B) POST /entries/pending (igual que tenías)
    // =========================================================
    @PostMapping(path = "/entries/pending", consumes = MediaType.TEXT_PLAIN_VALUE)
    public ResponseEntity<Map<String,String>> receivePending(@RequestBody String block) {
        if (block == null || block.isBlank()) {
            return ResponseEntity.badRequest().body(Resp.ko("400","contenido vacío"));
        }

        List<String> lines = Arrays.stream(block.split("\\r?\\n"))
                .map(String::trim).filter(s -> !s.isEmpty()).collect(Collectors.toList());
        if (lines.isEmpty()) {
            return ResponseEntity.badRequest().body(Resp.ko("400","contenido inválido"));
        }

        String deviceIdLine = lines.get(0);
        String deviceId = deviceIdLine.replace(";", "");
        if (deviceId.isBlank()) {
            return ResponseEntity.badRequest().body(Resp.ko("400","id vacío en cabecera"));
        }

        int okCount = 0, failCount = 0;
        for (int i=1; i<lines.size(); i++) {
            String L = lines.get(i);
            if (L.endsWith(";")) L = L.substring(0, L.length()-1);

            int p = L.lastIndexOf(':');
            if (p <= 0 || p >= L.length()-1) { failCount++; continue; }

            String rawKey = L.substring(0, p);
            String pasosStr = L.substring(p+1);

            int pasos;
            try { pasos = Integer.parseInt(pasosStr); } catch (Exception e) { failCount++; continue; }

            QRParser.Parsed parsed = QRParser.parse(rawKey);
            if (parsed.kind == QRParser.QRKind.QR_UNKNOWN || parsed.code.isEmpty()) { failCount++; continue; }

            // TODO: marcar usado en repositorio/BBDD
            okCount++;
            log.info("[PENDING] id={} key={} pasos={} -> USADO", deviceId, parsed.code, pasos);
        }

        Map<String,String> resp = new HashMap<>();
        resp.put("R", "OK");
        resp.put("id", deviceId);
        resp.put("ok", String.valueOf(okCount));
        resp.put("fail", String.valueOf(failCount));
        return ResponseEntity.ok(resp);
    }

    // =========================================================
    // C) POST /entries/upload (como tenías)
    // =========================================================
    public static class UploadLine { public String barcode; public Integer pasos; public Integer hh; public Integer mm; public Integer ss; }
    public static class UploadReq { public String id; public List<UploadLine> lines; }

    @PostMapping(path = "/entries/upload", consumes = MediaType.APPLICATION_JSON_VALUE)
    public ResponseEntity<Map<String,String>> upload(@RequestBody UploadReq req) {
        if (req == null || req.id == null || req.id.isBlank()) {
            return ResponseEntity.badRequest().body(Resp.ko("400","id vacío"));
        }
        if (req.lines == null || req.lines.isEmpty()) {
            return ResponseEntity.badRequest().body(Resp.ko("400","sin líneas"));
        }

        List<EntryLine> parsed = new ArrayList<>();
        for (UploadLine L : req.lines) {
            if (L == null || L.barcode == null || L.pasos == null || L.hh == null || L.mm == null || L.ss == null) continue;
            QRParser.Parsed p = QRParser.parse(L.barcode);
            if (p.kind == QRParser.QRKind.QR_UNKNOWN || p.code.isEmpty()) continue;
            parsed.add(new EntryLine(p.code, L.pasos, LocalTime.of(L.hh, L.mm, L.ss)));
        }

        entryStore.putEntries(req.id, parsed);
        Map<String,String> resp = new HashMap<>();
        resp.put("R","OK");
        resp.put("id", req.id);
        resp.put("n", String.valueOf(parsed.size()));
        return ResponseEntity.ok(resp);
    }
}
