package com.qualicard.museo_elder_backend.service.dto;

// Puedes ampliar con campos de QR si los envías
public class ValidateQRReq {
    public String id;
    public String status;
    public String ec;
    public String ticket_id;
    public String event_id;
    public String barcode; // opcional, si el lector GM65 te pasa algo aquí
}