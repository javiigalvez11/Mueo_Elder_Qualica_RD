#include <Preferences.h>
#include "config_params.hpp"
#include "definiciones.hpp"
#include "logBuf.hpp"

static Preferences pPrefs;
static const char *NS_P = "params";
static const char *K_PCRC = "pcrc"; // Firma de parámetros técnicos

bool paramsBegin() {
    return pPrefs.begin(NS_P, false);
}

// ========================= Lógica de Firma =========================
// Basada en tus valores hardcodeados para detectar cambios
static uint32_t paramsDefaultsSignature() {
    // Usamos una suma simple o el mismo FNV1a de tu código
    uint32_t h = 2166136261u;
    h ^= (uint32_t)36; // Número de parámetros
    h *= 16777619u;
    return h;
}

// ========================= Carga y Guardado =========================
TornoParams paramsLoad() {
    TornoParams p;
    // Usamos getUInt/getInt con los valores default que me diste
    p.machineId      = pPrefs.getUInt("p0", 1);
    p.openingMode    = pPrefs.getUInt("p1", 1);
    p.waitTime       = pPrefs.getUInt("p2", 8);
    p.voiceLeft      = pPrefs.getUInt("p3", 3);
    p.voiceRight     = pPrefs.getUInt("p4", 5);
    p.voiceVol       = pPrefs.getUInt("p5", 12);
    p.masterSpeed    = pPrefs.getUInt("p6", 10);
    p.slaveSpeed     = pPrefs.getUInt("p7", 10);
    p.debugMode      = pPrefs.getUInt("p8", 0);
    p.decelRange     = pPrefs.getUInt("p9", 10);
    p.selfTestSpeed  = pPrefs.getUInt("p10", 3);
    p.passageMode    = pPrefs.getUInt("p11", 0);
    p.closeControl   = pPrefs.getUInt("p12", 2);
    p.singleMotor    = pPrefs.getUInt("p13", 0);
    p.language       = pPrefs.getUInt("p14", 4);
    p.irRebound      = pPrefs.getUInt("p15", 1);
    p.pinchSens      = pPrefs.getUInt("p16", 5);
    p.reverseEntry   = pPrefs.getUInt("p17", 1);
    p.turnstileType  = pPrefs.getUInt("p18", 0);
    p.emergencyDir   = pPrefs.getUInt("p19", 2);
    p.motorResist    = pPrefs.getUInt("p20", 5);
    p.intrusionVoice = pPrefs.getUInt("p21", 1);
    p.irDelay        = pPrefs.getUInt("p22", 5);
    p.motorDir       = pPrefs.getUInt("p23", 1);
    p.clutchLock     = pPrefs.getUInt("p24", 0);
    p.hallType       = pPrefs.getUInt("p25", 0);
    p.signalFilter   = pPrefs.getUInt("p26", 3);
    p.cardInside     = pPrefs.getUInt("p27", 0);
    p.tailgateAlarm  = pPrefs.getUInt("p28", 2);
    p.limitDev       = pPrefs.getUInt("p29", 3);
    p.pinchFree      = pPrefs.getUInt("p30", 1);
    p.memoryFree     = pPrefs.getUInt("p31", 0);
    p.slipMaster     = pPrefs.getUInt("p32", 0);
    p.slipSlave      = pPrefs.getUInt("p33", 0);
    p.irLogicMode    = pPrefs.getUInt("p34", 0);
    p.lightMaster    = pPrefs.getUInt("p35", 1);
    p.lightSlave     = pPrefs.getUInt("p36", 2);
    return p;
}

bool paramsSave(const TornoParams &p) {
    pPrefs.putUInt("p0", p.machineId);
    pPrefs.putUInt("p1", p.openingMode);
    pPrefs.putUInt("p2", p.waitTime);
    pPrefs.putUInt("p3", p.voiceLeft);
    pPrefs.putUInt("p4", p.voiceRight);
    pPrefs.putUInt("p5", p.voiceVol);
    pPrefs.putUInt("p6", p.masterSpeed);
    pPrefs.putUInt("p7", p.slaveSpeed);
    pPrefs.putUInt("p8", p.debugMode);
    pPrefs.putUInt("p9", p.decelRange);
    pPrefs.putUInt("p10", p.selfTestSpeed);
    pPrefs.putUInt("p11", p.passageMode);
    pPrefs.putUInt("p12", p.closeControl);
    pPrefs.putUInt("p13", p.singleMotor);
    pPrefs.putUInt("p14", p.language);
    pPrefs.putUInt("p15", p.irRebound);
    pPrefs.putUInt("p16", p.pinchSens);
    pPrefs.putUInt("p17", p.reverseEntry);
    pPrefs.putUInt("p18", p.turnstileType);
    pPrefs.putUInt("p19", p.emergencyDir);
    pPrefs.putUInt("p20", p.motorResist);
    pPrefs.putUInt("p21", p.intrusionVoice);
    pPrefs.putUInt("p22", p.irDelay);
    pPrefs.putUInt("p23", p.motorDir);
    pPrefs.putUInt("p24", p.clutchLock);
    pPrefs.putUInt("p25", p.hallType);
    pPrefs.putUInt("p26", p.signalFilter);
    pPrefs.putUInt("p27", p.cardInside);
    pPrefs.putUInt("p28", p.tailgateAlarm);
    pPrefs.putUInt("p29", p.limitDev);
    pPrefs.putUInt("p30", p.pinchFree);
    pPrefs.putUInt("p31", p.memoryFree);
    pPrefs.putUInt("p32", p.slipMaster);
    pPrefs.putUInt("p33", p.slipSlave);
    pPrefs.putUInt("p34", p.irLogicMode);
    pPrefs.putUInt("p35", p.lightMaster);
    pPrefs.putUInt("p36", p.lightSlave);
    return true;
}

// ========================= Aplicación =========================
void paramsApplyToGlobals(const TornoParams &p) {
    p_machineId = p.machineId;
    p_openingMode = p.openingMode;
    p_waitTime = p.waitTime;
    p_voiceLeft = p.voiceLeft;
    p_voiceRight = p.voiceRight;
    p_voiceVol = p.voiceVol;
    p_masterSpeed = p.masterSpeed;
    p_slaveSpeed = p.slaveSpeed;
    p_debugMode = p.debugMode;
    p_decelRange = p.decelRange;
    p_selfTestSpeed = p.selfTestSpeed;
    p_passageMode = p.passageMode;
    p_closeControl = p.closeControl;
    p_singleMotor = p.singleMotor;
    p_language = p.language;
    p_irRebound = p.irRebound;
    p_pinchSens = p.pinchSens;
    p_reverseEntry = p.reverseEntry;
    p_turnstileType = p.turnstileType;
    p_emergencyDir = p.emergencyDir;
    p_motorResist = p.motorResist;
    p_intrusionVoice = p.intrusionVoice;
    p_irDelay = p.irDelay;
    p_motorDir = p.motorDir;
    p_clutchLock = p.clutchLock;
    p_hallType = p.hallType;
    p_signalFilter = p.signalFilter;
    p_cardInside = p.cardInside;
    p_tailgateAlarm = p.tailgateAlarm;
    p_limitDev = p.limitDev;
    p_pinchFree = p.pinchFree;
    p_memoryFree = p.memoryFree;
    p_slipMaster = p.slipMaster;
    p_slipSlave = p.slipSlave;
    p_irLogicMode = p.irLogicMode;
    p_lightMaster = p.lightMaster;
    p_lightSlave = p.lightSlave;
}

void paramsEnsureDefaults() {
    uint32_t now = paramsDefaultsSignature();
    uint32_t saved = pPrefs.getUInt(K_PCRC, 0);

    if (saved == 0 || saved != now) {
        logbuf_pushf("Params: Detectada NVS técnica vacía. Aplicando defaults.");
        TornoParams d; // Se inicializa con los valores de paramsLoad()
        d.machineId = 1; d.openingMode = 1; d.waitTime = 8; d.voiceLeft = 3; d.voiceRight = 5;
        d.voiceVol = 12; d.masterSpeed = 10; d.slaveSpeed = 10; d.debugMode = 0; d.decelRange = 10;
        d.selfTestSpeed = 3; d.passageMode = 0; d.closeControl = 2; d.singleMotor = 0;
        d.language = 4; d.irRebound = 1; d.pinchSens = 5; d.reverseEntry = 1; d.turnstileType = 0;
        d.emergencyDir = 2; d.motorResist = 5; d.intrusionVoice = 1; d.irDelay = 5; d.motorDir = 1;
        d.clutchLock = 0; d.hallType = 0; d.signalFilter = 3; d.cardInside = 0; d.tailgateAlarm = 2;
        d.limitDev = 3; d.pinchFree = 1; d.memoryFree = 0; d.slipMaster = 0; d.slipSlave = 0;
        d.irLogicMode = 0; d.lightMaster = 1; d.lightSlave = 2;
        
        if (paramsSave(d)) {
            pPrefs.putUInt(K_PCRC, now);
        }
    }
}