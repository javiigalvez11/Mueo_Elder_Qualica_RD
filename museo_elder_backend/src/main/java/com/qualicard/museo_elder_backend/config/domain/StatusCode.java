package com.qualicard.museo_elder_backend.config.domain;

public final class StatusCode {
    private StatusCode() {}

    // ====== Status numéricos ======
    public static final String READY     = "200";
    public static final String ENTER     = "201";
    public static final String EXIT      = "202";
    public static final String OPEN_CONT = "203";
    public static final String ABORTED   = "300";
    public static final String UPDATE    = "310";

    // ====== Etiquetas "ec" ======
    public static final String EC_READY           = "CMD_READY";
    public static final String EC_VALIDATE_IN     = "CMD_VALIDATE_IN";
    public static final String EC_VALIDATE_OUT    = "CMD_VALIDATE_OUT";
    public static final String EC_PASS_OK         = "CMD_PASS_OK";
    public static final String EC_PASS_TIMEOUT    = "CMD_PASS_TIMEOUT";
    public static final String EC_ENTRY           = "CMD_ENTRY";
    public static final String EC_PASS_PROGRESS   = "CMD_PASS_PROGRESS";
    public static final String EC_UPDATE          = "CMD_UPDATE";

    // >>> NUEVO: mantener/apuntar apertura continua <<<
    public static final String EC_OPEN_CONTINIUS  = "CMD_OPEN_CONTINIUS";

    public static boolean isEnter(String status) { return ENTER.equals(status); }
    public static boolean isExit (String status) { return EXIT.equals(status); }

    public static String defaultStatusForEc(String ec, String fallback) {
        if (ec == null) return READY;
        switch (ec) {
            case EC_READY:          return READY;
            case EC_ENTRY:          return READY;
            case EC_UPDATE:         return UPDATE;
            case EC_PASS_OK:        return READY;
            case EC_PASS_TIMEOUT:   return READY;
            case EC_VALIDATE_IN:    return ENTER;
            case EC_VALIDATE_OUT:   return EXIT;
            case EC_OPEN_CONTINIUS: return OPEN_CONT; // <<< NUEVO
            case EC_PASS_PROGRESS:  return (fallback != null ? fallback : READY);
            default:                return READY;
        }
    }
}
