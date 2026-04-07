package com.qualicard.museo_elder_backend.controller;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.SerializationFeature;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.HashMap;
import java.util.Map;
import java.util.TreeMap;

@RestController
@RequestMapping("/QRDService/prueba")
public class MockControllerESP32 {

    private static final Logger log = LoggerFactory.getLogger(MockControllerESP32.class);
    private static final ObjectMapper mapper = new ObjectMapper().enable(SerializationFeature.INDENT_OUTPUT);

    private String formatJson(Map<String, Object> map) {
        try {
            return mapper.writeValueAsString(map);
        } catch (JsonProcessingException e) {
            return String.valueOf(map);
        }
    }

    /**
     * Súper Helper: No solo es insensible a mayúsculas/minúsculas,
     * sino que acepta varios posibles nombres para una misma clave.
     */
    private String getFlex(Map<String, Object> body, String... possibleKeys) {
        TreeMap<String, Object> caseInsensitiveMap = new TreeMap<>(String.CASE_INSENSITIVE_ORDER);
        caseInsensitiveMap.putAll(body);

        for (String key : possibleKeys) {
            if (caseInsensitiveMap.containsKey(key)) {
                Object val = caseInsensitiveMap.get(key);
                return val != null ? String.valueOf(val) : "";
            }
        }
        return "";
    }

    @PostMapping(path = {"/inicio", "/status"}, consumes = MediaType.APPLICATION_JSON_VALUE)
    public ResponseEntity<Map<String, Object>> status(@RequestBody Map<String, Object> body) {
        log.info("[REQ - /status] Recibido de ESP32:\n{}", formatJson(body));

        // Buscamos la clave por sus distintos nombres habituales
        String id = getFlex(body, "id", "ID", "Id");
        String ec = getFlex(body, "ec", "EC", "estadoMaquina");

        // Usamos siempre minúsculas de vuelta porque ArduinoJson lo requiere así
        Map<String, Object> resp = new HashMap<>();
        resp.put("r", "OK");
        resp.put("id", id);
        resp.put("status", 200);
        resp.put("ec", ec.isEmpty() ? "CMD_READY" : ec);

        log.info("[RES - /status] Enviando a ESP32:\n{}", formatJson(resp));
        return ResponseEntity.ok(resp);
    }

    @PostMapping(path = "/validateQR", consumes = MediaType.APPLICATION_JSON_VALUE)
    public ResponseEntity<Map<String, Object>> validateQR(@RequestBody Map<String, Object> body) {
        log.info("[REQ - /validateQR] Recibido de ESP32:\n{}", formatJson(body));

        String id = getFlex(body, "id", "ID");
        // El ESP32 envía "status", no "S", así que buscamos ambos por si acaso
        String statusRx = getFlex(body, "status", "s", "S");

        Map<String, Object> resp = new HashMap<>();
        resp.put("r", "OK");
        resp.put("id", id);

        if ("201".equals(statusRx)) { // Petición de Entrada
            resp.put("nt", 1);
            resp.put("np", 0);
            resp.put("status", 203);
            // CLAVE: El ESP32 espera exactamente CMD_VALIDATE_IN para abrir
            if(id.startsWith("ME")){
                resp.put("ec", "CMD_PASS_IN");
            }else{
                resp.put("ec", "PASS_IN");
            }
        } else if ("202".equals(statusRx)) { // Petición de Salida
            resp.put("nt", 1);
            resp.put("np", 0);
            resp.put("status", 204);
            if(id.startsWith("ME")){
                resp.put("ec", "CMD_PASS_OUT");
            }else{
                resp.put("ec", "PASS_OUT");
            }

        } else {
            resp.put("status", 402);
            if(id.startsWith("ME")){
                resp.put("ec", "CMD_BAD_REQUEST");
            }else{
                resp.put("ec", "BAD_REQUEST");
            }
        }

        log.info("[RES - /validateQR] Enviando a ESP32:\n{}", formatJson(resp));
        return ResponseEntity.ok(resp);
    }

    @PostMapping(path = "/validatePass", consumes = MediaType.APPLICATION_JSON_VALUE)
    public ResponseEntity<Map<String, Object>> validatePass(@RequestBody Map<String, Object> body) {
        log.info("[REQ - /validatePass] Recibido de ESP32:\n{}", formatJson(body));

        String id = getFlex(body, "id", "ID");
        String ecRx = getFlex(body, "ec", "EC");
        String np = getFlex(body, "np", "NP");
        String nt = getFlex(body, "nt", "NT");

        Map<String, Object> resp = new HashMap<>();
        resp.put("r", "OK");
        resp.put("id", id);

        if (ecRx.contains("OK") || ecRx.contains("TIMEOUT") || np.equals(nt)) {
            resp.put("status", 200);
            if(id.startsWith("ME")){
                resp.put("ec", "CMD_READY");
            }else{
                resp.put("ec", "READY");
            }

        } else {
            resp.put("status", getFlex(body, "status", "s", "S"));
            resp.put("ec", ecRx);
            resp.put("np", np);
            resp.put("nt", nt);
        }

        log.info("[RES - /validatePass] Enviando a ESP32:\n{}", formatJson(resp));
        return ResponseEntity.ok(resp);
    }

    @PostMapping(path = "/reportFailure", consumes = MediaType.APPLICATION_JSON_VALUE)
    public ResponseEntity<Map<String, Object>> reportFailure(@RequestBody Map<String, Object> body) {
        log.warn("[REQ - /reportFailure] Recibido de ESP32:\n{}", formatJson(body));

        String id = getFlex(body, "id", "ID");
        Map<String, Object> resp = new HashMap<>();
        resp.put("r", "OK");
        resp.put("id", id);
        resp.put("status", 200);
        if(id.startsWith("ME")){
            resp.put("ec", "CMD_READY");
        }else{
            resp.put("ec", "READY");
        }


        log.info("[RES - /reportFailure] Enviando a ESP32:\n{}", formatJson(resp));
        return ResponseEntity.ok(resp);
    }
}