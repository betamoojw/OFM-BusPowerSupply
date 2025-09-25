#include "BusPowerSupplyModule.h"
#include "OpenKNX.h"
#include "ModuleVersionCheck.h"

BusPowerSupplyModule openknxBusPowerSupplyModule;

BusPowerSupplyModule::BusPowerSupplyModule()
{
}

BusPowerSupplyModule::~BusPowerSupplyModule()
{
}

const std::string BusPowerSupplyModule::name()
{
    return "BusPower";
}

const std::string BusPowerSupplyModule::version()
{
    return MODULE_BusPowerSupply_Version;
}

void BusPowerSupplyModule::processInputKo(GroupObject &ko)
{
    logDebugP("processInputKo");
    logIndentUp();

    uint16_t asap = ko.asap();
    switch (asap)
    {
    }

    logIndentDown();
}

void BusPowerSupplyModule::setup(bool configured)
{
    openknx.gpio.pinMode(OPENKNX_BPS_PWR1_CHECK_PIN, INPUT);
    openknx.gpio.pinMode(OPENKNX_BPS_PWR1_SWITCH_ON_PIN, OUTPUT, true, !OPENKNX_BPS_PWR_SWITCH_ACTIVE_ON);
    openknx.gpio.pinMode(OPENKNX_BPS_PWR1_SWITCH_OFF_PIN, OUTPUT, true, !OPENKNX_BPS_PWR_SWITCH_ACTIVE_ON);
    openknx.gpio.pinMode(OPENKNX_BPS_PWR1_ALERT_PIN, INPUT);

    openknx.gpio.pinMode(OPENKNX_BPS_PWR2_CHECK_PIN, INPUT);
    openknx.gpio.pinMode(OPENKNX_BPS_PWR2_SWITCH_ON_PIN, OUTPUT, true, !OPENKNX_BPS_PWR_SWITCH_ACTIVE_ON);
    openknx.gpio.pinMode(OPENKNX_BPS_PWR2_SWITCH_OFF_PIN, OUTPUT, true, !OPENKNX_BPS_PWR_SWITCH_ACTIVE_ON);
    openknx.gpio.pinMode(OPENKNX_BPS_PWR2_ALERT_PIN, INPUT);

    openknx.gpio.pinMode(OPENKNX_BPS_STATUS_BUS, OUTPUT, true, !OPENKNX_BPS_STATUS_ACTIVE_ON);
    openknx.gpio.pinMode(OPENKNX_BPS_STATUS_TMP, OUTPUT, true, !OPENKNX_BPS_STATUS_ACTIVE_ON);
    openknx.gpio.pinMode(OPENKNX_BPS_STATUS_TRC, OUTPUT, true, !OPENKNX_BPS_STATUS_ACTIVE_ON);
    openknx.gpio.pinMode(OPENKNX_BPS_STATUS_DEV, OUTPUT, true, !OPENKNX_BPS_STATUS_ACTIVE_ON);
    openknx.gpio.pinMode(OPENKNX_BPS_STATUS_PW1, OUTPUT, true, !OPENKNX_BPS_STATUS_ACTIVE_ON);
    openknx.gpio.pinMode(OPENKNX_BPS_STATUS_PW2, OUTPUT, true, !OPENKNX_BPS_STATUS_ACTIVE_ON);
    openknx.gpio.pinMode(OPENKNX_BPS_STATUS_MAX, OUTPUT, true, !OPENKNX_BPS_STATUS_ACTIVE_ON);
    openknx.gpio.pinMode(OPENKNX_BPS_STATUS_RST, OUTPUT, true, !OPENKNX_BPS_STATUS_ACTIVE_ON);
    openknx.gpio.pinMode(OPENKNX_BPS_SWITCH_RST, INPUT);

    if (inaKnx.begin())
    {
        logDebugP("KNX INA226 setup done with address %u", inaKnx.getAddress());
        logIndentUp();

        // inaKnx.setBusVoltageRange(32);
        // inaKnx.setGain(1);
        uint32_t result = inaKnx.setMaxCurrentShunt(0.5, 0.1);
        if (result != 0)
            logDebugP("KNX INA226 setMaxCurrentShunt failed with error code %u", result);
        inaKnx.setModeShuntContinuous();
        delay(1000);

        // logDebugP("getBusVoltageRange %u", inaKnx.getBusVoltageRange());
        // logDebugP("getGain %u", inaKnx.getGain());
        // logDebugP("getBusADC %u", inaKnx.getBusADC());
        // logDebugP("getShuntADC %u", inaKnx.getShuntADC());
        logDebugP("getMode %u", inaKnx.getMode());

        logDebugP("isCalibrated %u", inaKnx.isCalibrated());
        logDebugP("getCurrentLSB %.4f", inaKnx.getCurrentLSB());
        logDebugP("getShunt %.4f", inaKnx.getShunt());
        logDebugP("getMaxCurrent %.4f", inaKnx.getMaxCurrent());
        logIndentDown();
    }
    else
        logDebugP("KNX INA226 not found at address %u", inaKnx.getAddress());

    if (inaAux.begin())
    {
        logDebugP("AUX INA226 setup done with address %u", inaAux.getAddress());
        logIndentUp();

        // inaAux.setBusVoltageRange(32);
        // inaAux.setGain(1);
        uint32_t result = inaAux.setMaxCurrentShunt(0.5, 0.1);
        if (result != 0)
            logDebugP("AUX INA226 setMaxCurrentShunt failed with error code %u", result);
        inaAux.setAverage(INA226_128_SAMPLES);
        inaAux.setModeShuntContinuous();
        delay(1000);

        // logDebugP("getBusVoltageRange %u", inaAux.getBusVoltageRange());
        // logDebugP("getGain %u", inaAux.getGain());
        // logDebugP("getBusADC %u", inaAux.getBusADC());
        // logDebugP("getShuntADC %u", inaAux.getShuntADC());
        logDebugP("getMode %u", inaAux.getMode());

        logDebugP("isCalibrated %u", inaAux.isCalibrated());
        logDebugP("getCurrentLSB %.4f", inaAux.getCurrentLSB());
        logDebugP("getShunt %.4f", inaAux.getShunt());
        logDebugP("getMaxCurrent %.4f", inaAux.getMaxCurrent());
        logIndentDown();
    }
    else
        logDebugP("AUX INA226 not found at address %u", inaAux.getAddress());
}

void BusPowerSupplyModule::loop()
{
    if (delayCheck(debugTimer, 1000)) {
        float pwr1Voltage = (float)analogRead(OPENKNX_BPS_PWR1_CHECK_PIN) / (float)1023 * (float)3.3 * (float)OPENKNX_BPS_PWR_CHECK_FACTOR;
        float pwr2Voltage = (float)analogRead(OPENKNX_BPS_PWR2_CHECK_PIN) / (float)1023 * (float)3.3 * (float)OPENKNX_BPS_PWR_CHECK_FACTOR;
        logDebugP("PWR1 Voltage: %.2f V, PWR2 Voltage: %.2f V", pwr1Voltage, pwr2Voltage);

        float knxCurrent = inaKnx.getCurrent_mA();
        float knxVoltage = inaKnx.getBusVoltage();
        logDebugP("KNX Power: %.2f mA at %.2f V", knxCurrent, knxVoltage);

        float auxCurrent = inaAux.getCurrent_mA();
        float auxVoltage = inaAux.getBusVoltage();
        float auxPower = inaAux.getPower_mW();
        int test = inaAux.getRegister(0x02);
        logDebugP("AUX Power: %.2f mA at %.2f V, auxPower: %.2f, REG 0x02: %u", auxCurrent, auxVoltage, auxPower, test);

        debugTimer = delayTimerInit();
    }
}

void BusPowerSupplyModule::readFlash(const uint8_t *data, const uint16_t size)
{
    if (size == 0)
        return;

    // logDebugP("Reading state from flash");
    // logIndentUp();

    // uint8_t version = openknx.flash.readByte();
    // if (version != OPENKNX_BPS_FLASH_VERSION)
    // {
    //     logDebugP("Invalid flash version %u", version);
    //     return;
    // }

    // uint32_t magicWord = openknx.flash.readInt();
    // if (magicWord != OPENKNX_BPS_FLASH_MAGIC_WORD)
    // {
    //     logDebugP("Flash content invalid");
    //     return;
    // }

    
    // logIndentDown();
}

void BusPowerSupplyModule::writeFlash()
{
    // openknx.flash.writeByte(OPENKNX_BPS_FLASH_VERSION);
    // openknx.flash.writeInt(OPENKNX_BPS_FLASH_MAGIC_WORD);
}

uint16_t BusPowerSupplyModule::flashSize()
{
    return 0;
}

void BusPowerSupplyModule::savePower()
{
    
}

bool BusPowerSupplyModule::restorePower()
{
    bool success = true;
    
    return success;
}

void BusPowerSupplyModule::showHelp()
{
    //logInfo("sa run test mode", "Test all channels one after the other.");
}

bool BusPowerSupplyModule::processCommand(const std::string cmd, bool diagnoseKo)
{
    if (cmd.substr(0, 2) != "bs")
        return false;


    // Commands starting with ba are our diagnose commands
    logInfoP("ba (BusPowerSupply) command with bad args");
    if (diagnoseKo)
    {
        openknx.console.writeDiagenoseKo("ba: bad args");
    }
    return true;
}

void BusPowerSupplyModule::runTestMode()
{
    // logInfoP("Starting test mode");
    // logIndentUp();


    // logInfoP("Testing finished.");
    // logIndentDown();
}