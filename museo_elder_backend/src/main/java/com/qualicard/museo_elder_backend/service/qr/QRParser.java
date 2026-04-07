package com.qualicard.museo_elder_backend.service.qr;

import java.util.Objects;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Parser y normalizador de códigos QR/barcodes.
 *
 * Formatos soportados:
 *  - ODOO  → UUID 36 con guiones (hex) → kind = ACCESS_TOKEN, code = uuid (lowercase)
 *  - TEC   → "ticket_id=######&event_id=######" → kind = TICKET_EVENT, code = forma canónica
 *  - MAGE  → numérico largo → kind = PLAIN_NUMERIC, code = número
 *
 * Dos modalidades:
 *  - parse(...)       → PERMISIVO: detecta señales dentro de URLs o textos (p.ej. ?access_token=...).
 *  - parseStrict(...) → ESTRICTO: acepta solo los 3 formatos exactos (uuid 36, tid/eid 6+6, 19 dígitos).
 */
public final class QRParser {

    private QRParser() {}

    // ===== Tipos =====
    public enum QRKind { ACCESS_TOKEN, TICKET_EVENT, PLAIN_NUMERIC, QR_UNKNOWN }

    public static final class Parsed {
        public final QRKind kind;     // tipo detectado
        public final String code;     // código normalizado (clave offline)
        public final String original; // valor recibido (sin modificar)

        public Parsed(QRKind kind, String code, String original) {
            this.kind = Objects.requireNonNull(kind, "kind");
            this.code = code == null ? "" : code;
            this.original = original;
        }

        /** true si es uno de los formatos conocidos y hay code no vacío */
        public boolean isKnown() { return kind != QRKind.QR_UNKNOWN && !code.isEmpty(); }

        @Override public String toString() {
            return "Parsed{kind=" + kind + ", code='" + code + "', original='" + original + "'}";
        }
    }

    // ====== PATRONES (permisivos) ======
    // ODOO dentro de cadenas/URLs: access_token=<uuid-36-hex-con-guiones>
    private static final Pattern ACCESS_TOKEN_IN_TEXT =
            Pattern.compile("access_token=([0-9a-fA-F\\-]{36})");

    // TEC dentro de cadenas/URLs: ticket_id=digits & event_id=digits (orden indiferente en parse())
    private static final Pattern TICKET_ID_IN_TEXT =
            Pattern.compile("ticket_id=(\\d+)");
    private static final Pattern EVENT_ID_IN_TEXT  =
            Pattern.compile("event_id=(\\d+)");

    // Numérico largo (≥ 6 dígitos) — permisivo
    private static final Pattern ONLY_DIGITS_LONG =
            Pattern.compile("^\\d{6,}$");

    // ====== PATRONES (estrictos) ======
    // UUID 36 exacto 8-4-4-4-12
    private static final Pattern UUID36_STRICT =
            Pattern.compile("^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$");

    // TEC exacto: ticket_id=######&event_id=######
    private static final Pattern TEC_STRICT =
            Pattern.compile("^ticket_id=(\\d{6})&event_id=(\\d{6})$");

    // MAGE exacto: 19 dígitos
    private static final Pattern NUM19_STRICT =
            Pattern.compile("^\\d{19}$");

    // ====== API: PARSE PERMISIVO ======
    /**
     * Parser PERMISIVO:
     *  - Busca access_token=UUID en cualquier parte del texto.
     *  - Busca ticket_id y event_id (en cualquier orden) y devuelve la forma canónica "ticket_id=..&event_id=..".
     *  - Si la cadena es numérica larga (>=6), la devuelve tal cual como PLAIN_NUMERIC.
     *  - Si nada coincide, QR_UNKNOWN.
     */
    public static Parsed parse(String raw) {
        if (raw == null) {
            return new Parsed(QRKind.QR_UNKNOWN, "", null);
        }
        String s = raw.trim();
        if (s.isEmpty()) {
            return new Parsed(QRKind.QR_UNKNOWN, "", raw);
        }

        // 1) ODOO: access_token=uuid
        Matcher mTok = ACCESS_TOKEN_IN_TEXT.matcher(s);
        if (mTok.find()) {
            String token = mTok.group(1);
            if (UUID36_STRICT.matcher(token).matches()) {
                return new Parsed(QRKind.ACCESS_TOKEN, token.toLowerCase(), raw);
            }
        }

        // 2) TEC: ticket_id y event_id (en cualquier orden), luego normalizamos a "ticket_id=..&event_id=.."
        String ticketId = null, eventId = null;
        Matcher mTid = TICKET_ID_IN_TEXT.matcher(s);
        if (mTid.find()) ticketId = mTid.group(1);
        Matcher mEid = EVENT_ID_IN_TEXT.matcher(s);
        if (mEid.find()) eventId = mEid.group(1);
        if (ticketId != null && eventId != null) {
            String canonical = "ticket_id=" + ticketId + "&event_id=" + eventId;
            return new Parsed(QRKind.TICKET_EVENT, canonical, raw);
        }

        // 3) Numérico largo (permite códigos legacy)
        if (ONLY_DIGITS_LONG.matcher(s).matches()) {
            return new Parsed(QRKind.PLAIN_NUMERIC, s, raw);
        }

        // 4) Desconocido
        return new Parsed(QRKind.QR_UNKNOWN, "", raw);
    }

    // ====== API: PARSE ESTRICTO ======
    /**
     * Parser ESTRICTO:
     *  - Acepta SOLO:
     *      ODOO: UUID 36 exacto (hex con guiones)
     *      TEC:  "ticket_id=######&event_id=######" (6 dígitos cada uno)
     *      MAGE: exactamente 19 dígitos
     *  - Devuelve QR_UNKNOWN en cualquier otro caso.
     */
    public static Parsed parseStrict(String raw) {
        if (raw == null) {
            return new Parsed(QRKind.QR_UNKNOWN, "", null);
        }
        String s = raw.trim();
        if (s.isEmpty()) {
            return new Parsed(QRKind.QR_UNKNOWN, "", raw);
        }

        // ODOO
        if (UUID36_STRICT.matcher(s).matches()) {
            return new Parsed(QRKind.ACCESS_TOKEN, s.toLowerCase(), raw);
        }

        // TEC (forma canónica exacta)
        Matcher mTec = TEC_STRICT.matcher(s);
        if (mTec.matches()) {
            // ya viene normalizado; garantizamos la forma canónica
            String canonical = "ticket_id=" + mTec.group(1) + "&event_id=" + mTec.group(2);
            return new Parsed(QRKind.TICKET_EVENT, canonical, raw);
        }

        // MAGE (19 dígitos exactos)
        if (NUM19_STRICT.matcher(s).matches()) {
            return new Parsed(QRKind.PLAIN_NUMERIC, s, raw);
        }

        return new Parsed(QRKind.QR_UNKNOWN, "", raw);
    }

    // ===== Helpers opcionales =====
    public static boolean isKnown(Parsed p)  { return p != null && p.isKnown(); }
    public static boolean isOdoo(Parsed p)   { return p != null && p.kind == QRKind.ACCESS_TOKEN && !p.code.isEmpty(); }
    public static boolean isTec(Parsed p)    { return p != null && p.kind == QRKind.TICKET_EVENT && !p.code.isEmpty(); }
    public static boolean isMage(Parsed p)   { return p != null && p.kind == QRKind.PLAIN_NUMERIC && !p.code.isEmpty(); }
}
