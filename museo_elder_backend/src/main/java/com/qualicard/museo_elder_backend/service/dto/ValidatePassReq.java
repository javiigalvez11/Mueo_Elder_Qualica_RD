package com.qualicard.museo_elder_backend.service.dto;


// Mapea a tu serializaPaso()
public class ValidatePassReq {
    public String id;
    public String status;   // estadoPuerta
    public String barcode;  // ultimoPaso
    public String ec;       // "CMD_PASS"
    public String ticket_id;
    public String event_id;
    public String np;       // pasoActual
    public String nt;       // pasosTotales
}