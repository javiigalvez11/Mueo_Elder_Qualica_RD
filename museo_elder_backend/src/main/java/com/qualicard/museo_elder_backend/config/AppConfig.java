// src/main/java/com/qualicard/museo_elder_backend/config/AppConfig.java
package com.qualicard.museo_elder_backend.config;

import com.qualicard.museo_elder_backend.service.mem.EntryStore;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

@Configuration
public class AppConfig {

    @Bean
    public EntryStore entryStore() {
        EntryStore s = new EntryStore();
        // s.seedDemo("ME002"); // opcional para probar
        return s;
    }
}
