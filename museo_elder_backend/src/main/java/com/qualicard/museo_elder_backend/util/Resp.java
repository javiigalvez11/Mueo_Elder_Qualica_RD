// src/main/java/com/qualicard/museo_elder_backend/util/Resp.java
package com.qualicard.museo_elder_backend.util;

import java.util.HashMap;
import java.util.Map;

public final class Resp {
    private Resp(){}

    public static Map<String,String> ok(Map<String,String> extra){
        Map<String,String> base = new HashMap<>();
        base.put("R","OK");
        if (extra != null) base.putAll(extra);
        return base;
    }

    public static Map<String,String> ko(String code, String desc){
        Map<String,String> base = new HashMap<>();
        base.put("R","KO");
        if (code!=null) base.put("status", code); // coherente con tu contrato numérico
        if (desc!=null) base.put("ed", desc);
        return base;
    }
}
