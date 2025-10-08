#pragma once
#include "OpenKNX.h"
#include "hardware.h"
#include "knxprod.h"
#include "INA226.h"
#ifdef OPENKNX_BPS_TEMPSENS_ADDR
  #include <Temperature_LM75_Derived.h>
#endif

// #define OPENKNX_BPS_FLASH_VERSION 0
// #define OPENKNX_BPS_FLASH_MAGIC_WORD 0

#define CH_SWITCH_DEBOUNCE 250

#define BUS_LOAD_UPDATE_RATE 1000
#define BUS_LOAD_MAX_BYTES_PER_SECOND 800

#define POWER_OK_THRESHOLD_VOLTAGE 28
#define CURRENT_THRESHOLD_MA 1280

class BusPowerSupplyModule : public OpenKNX::Module
{
  public:
    BusPowerSupplyModule();
    ~BusPowerSupplyModule();

    void processInputKo(GroupObject &ko);
    void setup(bool configured);
    void loop();

    void writeFlash() override;
    void readFlash(const uint8_t* data, const uint16_t size) override;
    uint16_t flashSize() override;

    void savePower() override;
    bool restorePower() override;

    const std::string name() override;
    const std::string version() override;

    float estimateBusLoad();

    void showHelp() override;
    bool processCommand(const std::string cmd, bool diagnoseKo) override;
    void runTestMode();

  private:
    void pwr1On();
    void pwr1Off();
    void pwr2On();
    void pwr2Off();
    void pwr1RelayCoilsOff();
    void pwr2RelayCoilsOff();

    INA226 _inaKnx = INA226(OPENKNX_BPS_CURRENT_KNX_INA_ADDR, &OPENKNX_GPIO_WIRE);
    INA226 _inaAux = INA226(OPENKNX_BPS_CURRENT_AUX_INA_ADDR, &OPENKNX_GPIO_WIRE);

#ifdef OPENKNX_BPS_TEMPSENS_ADDR
    Generic_LM75_9_to_12Bit_OneShot _temperature = Generic_LM75_9_to_12Bit_OneShot(&OPENKNX_GPIO_WIRE, OPENKNX_BPS_TEMPSENS_ADDR);
#endif

    uint8_t _pwrActive = 0;
    bool _pwrErrorLogged = false;
    bool _pwr1Ok = false;
    bool _pwr2Ok = false;
    bool _busOk = true;
    bool _currentOk = true;

    float _lastPowerSupply1Sent = 0;
    float _lastPowerSupply2Sent = 0;
    float _lastBusVoltageSent = 0;
    float _lastBusCurrentSent = 0;
    float _lastBusLoadSent = 0;
    float _lastAuxVoltageSent = 0;
    float _lastAuxCurrentSent = 0;
    float _lastTemperatureSent = 0;
    uint32_t _powerSupply1SendTimer = 0;
    uint32_t _powerSupply2SendTimer = 0;
    uint32_t _busVoltageSendTimer = 0;
    uint32_t _busCurrentSendTimer = 0;
    uint32_t _busLoadSendTimer = 0;
    uint32_t _auxVoltageSendTimer = 0;
    uint32_t _auxCurrentSendTimer = 0;
    uint32_t _temperaturSendTimer = 0;

    uint32_t _busLoadUpdateTimer = 0;
    uint32_t _rxLastBusLoadTime;
    uint32_t _rxLastBusBytes;

    uint32_t _relayBistableImpulsTimerPwr1 = 0;
    uint32_t _relayBistableImpulsTimerPwr2 = 0;

    uint32_t _debugTimer = 0;
};

extern BusPowerSupplyModule openknxBusPowerSupplyModule;