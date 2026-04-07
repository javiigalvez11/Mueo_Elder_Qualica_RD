package com.qualicard.museo_elder_backend;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.web.context.WebServerInitializedEvent;
import org.springframework.context.event.EventListener;

import java.net.InetAddress;

@SpringBootApplication
public class MuseoElderBackendApplication {

    private int serverPort;

    public static void main(String[] args) {
        SpringApplication.run(MuseoElderBackendApplication.class, args);
    }

    @EventListener
    public void onApplicationEvent(WebServerInitializedEvent event) {
        this.serverPort = event.getWebServer().getPort();
        try {
            String ip = InetAddress.getLocalHost().getHostAddress();
            System.out.println("SYSTEM --> Backend disponible en: http://" + ip + ":" + serverPort);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
