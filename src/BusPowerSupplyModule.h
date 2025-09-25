#pragma once
#include "OpenKNX.h"
#include "hardware.h"
#include "knxprod.h"
#include "INA226.h"

// #define OPENKNX_BPS_FLASH_VERSION 0
// #define OPENKNX_BPS_FLASH_MAGIC_WORD 0

#define CH_SWITCH_DEBOUNCE 250

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

    void showHelp() override;
    bool processCommand(const std::string cmd, bool diagnoseKo) override;
    void runTestMode();

  private:
    INA226 inaKnx = INA226(OPENKNX_BPS_CURRENT_KNX_INA_ADDR, &OPENKNX_GPIO_WIRE);
    INA226 inaAux = INA226(OPENKNX_BPS_CURRENT_AUX_INA_ADDR, &OPENKNX_GPIO_WIRE);

    uint32_t debugTimer = 0;
};

extern BusPowerSupplyModule openknxBusPowerSupplyModule;