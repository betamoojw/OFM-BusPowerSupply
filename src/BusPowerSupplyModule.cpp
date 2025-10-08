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
    pwr1RelayCoilsOff();
    openknx.gpio.pinMode(OPENKNX_BPS_PWR1_ALERT_PIN, INPUT);

    openknx.gpio.pinMode(OPENKNX_BPS_PWR2_CHECK_PIN, INPUT);
    openknx.gpio.pinMode(OPENKNX_BPS_PWR2_SWITCH_ON_PIN, OUTPUT, true, !OPENKNX_BPS_PWR_SWITCH_ACTIVE_ON);
    openknx.gpio.pinMode(OPENKNX_BPS_PWR2_SWITCH_OFF_PIN, OUTPUT, true, !OPENKNX_BPS_PWR_SWITCH_ACTIVE_ON);
    pwr2RelayCoilsOff();
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

    analogReadResolution(12);

    if (_inaKnx.begin())
    {
        logDebugP("KNX INA226 setup done with address %u", _inaKnx.getAddress());
        logIndentUp();

        uint32_t result = _inaKnx.setMaxCurrentShunt(3, OPENKNX_BPS_CURRENT_KNX_INA_SHUNT);
        if (result != 0)
            logDebugP("KNX INA226 setMaxCurrentShunt failed with error code %u", result);
        _inaKnx.setAverage(INA226_16_SAMPLES);
        _inaKnx.setModeShuntContinuous();

#ifdef OPENKNX_DEBUG
        delay(1000);
        logDebugP("getMode %u", _inaKnx.getMode());
        logDebugP("isCalibrated %u", _inaKnx.isCalibrated());
        logDebugP("getCurrentLSB %.4f", _inaKnx.getCurrentLSB());
        logDebugP("getShunt %.4f", _inaKnx.getShunt());
        logDebugP("getMaxCurrent %.4f", _inaKnx.getMaxCurrent());
#endif
        logIndentDown();
    }
    else
        logDebugP("KNX INA226 not found at address %u", _inaKnx.getAddress());

    if (_inaAux.begin())
    {
        logDebugP("AUX INA226 setup done with address %u", _inaAux.getAddress());
        logIndentUp();

        uint32_t result = _inaAux.setMaxCurrentShunt(3, OPENKNX_BPS_CURRENT_AUX_INA_SHUNT);
        if (result != 0)
            logDebugP("AUX INA226 setMaxCurrentShunt failed with error code %u", result);
        _inaAux.setAverage(INA226_16_SAMPLES);
        _inaAux.setModeShuntContinuous();

#ifdef OPENKNX_DEBUG
        delay(1000);
        logDebugP("getMode %u", _inaAux.getMode());
        logDebugP("isCalibrated %u", _inaAux.isCalibrated());
        logDebugP("getCurrentLSB %.4f", _inaAux.getCurrentLSB());
        logDebugP("getShunt %.4f", _inaAux.getShunt());
        logDebugP("getMaxCurrent %.4f", _inaAux.getMaxCurrent());
#endif
        logIndentDown();
    }
    else
        logDebugP("AUX INA226 not found at address %u", _inaAux.getAddress());
}

float BusPowerSupplyModule::estimateBusLoad()
{
    TpUartDataLinkLayer* dll = knx.bau().getDataLinkLayer();
    TPUart::Statistics& statistics = dll->getTPUart().getStatistics();
    uint32_t currentTime = millis();
    uint32_t timeDiff = currentTime - _rxLastBusLoadTime;
    uint32_t currentBytes = statistics.getRxBusBytes();
    uint32_t bytesPerSecond = (currentBytes - _rxLastBusBytes) / timeDiff * 1000;
    _rxLastBusLoadTime = currentTime;
    _rxLastBusBytes = currentBytes;

    return (float)bytesPerSecond / (float)BUS_LOAD_MAX_BYTES_PER_SECOND;
}

void BusPowerSupplyModule::loop()
{
    float pwr1Voltage = (float)analogRead(OPENKNX_BPS_PWR1_CHECK_PIN) / (float)4095 * (float)3.3 * (float)OPENKNX_BPS_PWR_CHECK_FACTOR;
    bool pwr1Ok = pwr1Voltage > POWER_OK_THRESHOLD_VOLTAGE;
    if (_pwr1Ok != pwr1Ok)
    {
        _pwr1Ok = pwr1Ok;
        openknx.gpio.digitalWrite(OPENKNX_BPS_STATUS_PW1, _pwr1Ok ? OPENKNX_BPS_STATUS_ACTIVE_ON : !OPENKNX_BPS_STATUS_ACTIVE_ON);

        if (ParamBPS_PowerSupply1ChangeSend)
            KoBPS_PowerSupply1Status.value(pwr1Ok, DPT_Switch);
    }

    float pwr2Voltage = (float)analogRead(OPENKNX_BPS_PWR2_CHECK_PIN) / (float)4095 * (float)3.3 * (float)OPENKNX_BPS_PWR_CHECK_FACTOR;
    bool pwr2Ok = pwr2Voltage > POWER_OK_THRESHOLD_VOLTAGE;
    if (_pwr2Ok != pwr2Ok)
    {
        _pwr2Ok = pwr2Ok;
        openknx.gpio.digitalWrite(OPENKNX_BPS_STATUS_PW2, _pwr2Ok ? OPENKNX_BPS_STATUS_ACTIVE_ON : !OPENKNX_BPS_STATUS_ACTIVE_ON);

        if (ParamBPS_PowerSupply2ChangeSend)
            KoBPS_PowerSupply2Status.value(pwr2Ok, DPT_Switch);
    }

    // if ((_pwr1Ok || _pwr2Ok) && openknx.common.isSaveTriggered())
    // {
    //     openknx.common.restoreSavePin();
    //     logInfoP("SAVE restored.");
    // }
    
    if (_pwrActive == 1 && !_pwr1Ok)
    {
        if (_pwr2Ok)
        {
            pwr1Off();
            pwr2On();
            _pwrActive = 2;
            _pwrErrorLogged = false;
            logInfoP("PWR1 failed, switched to PWR2");
        }
        else
        {
            if (!_pwrErrorLogged)
            {
                _pwrErrorLogged = true;
                logErrorP("PWR1 failed, PWR2 is NOT available, too!");

                openknx.common.triggerSavePin();
                logInfoP("SAVE triggered.");
            }
        }
    }
    else if (_pwrActive == 2 && !_pwr2Ok)
    {
        if (_pwr1Ok)
        {
            pwr2Off();
            pwr1On();
            _pwrActive = 1;
            _pwrErrorLogged = false;
            logInfoP("PWR2 failed, switched to PWR1");
        }
        else
        {
            if (!_pwrErrorLogged)
            {
                _pwrErrorLogged = true;
                logErrorP("PWR2 failed, PWR1 is NOT available, too!");

                openknx.common.triggerSavePin();
                logInfoP("SAVE triggered.");
            }
        }
    }
    else if (_pwrActive == 0)
    {
        if (_pwr1Ok)
        {
            pwr2Off();
            pwr1On();
            _pwrActive = 1;
            _pwrErrorLogged = false;
            logInfoP("Power supply started, PWR1 available, switching to PWR1");
        }
        else if (_pwr2Ok)
        {
            pwr1Off();
            pwr2On();
            _pwrActive = 2;
            _pwrErrorLogged = false;
            logInfoP("Power supply started, PWR2 available, switching to PWR2");
        }
        else
        {
            if (!_pwrErrorLogged)
            {
                _pwrErrorLogged = true;
                logErrorP("Power supply started, no power supply available!");
            }
        }
    }

#ifdef OPENKNX_DEBUG
    if (delayCheck(_debugTimer, 1000))
    {
        logDebugP("PWR1 Voltage: %.2f V, PWR2 Voltage: %.2f V", pwr1Voltage, pwr2Voltage);

        float busCurrent = _inaKnx.getCurrent_mA();
        float busVoltage = _inaKnx.getBusVoltage();
        logDebugP("KNX Power: %.2f mA at %.2f V", busCurrent, busVoltage);

        float auxCurrent = _inaAux.getCurrent_mA();
        float auxVoltage = _inaAux.getBusVoltage();
        logDebugP("AUX Power: %.2f mA at %.2f V", auxCurrent, auxVoltage);

        _debugTimer = delayTimerInit();
    }
#endif

    if (_relayBistableImpulsTimerPwr1 > 0 && delayCheck(_relayBistableImpulsTimerPwr1, OPENKNX_BPS_BISTABLE_IMPULSE_LENGTH))
    {
        pwr1RelayCoilsOff();
        _relayBistableImpulsTimerPwr1 = 0;
    }

    if (_relayBistableImpulsTimerPwr2 > 0 && delayCheck(_relayBistableImpulsTimerPwr2, OPENKNX_BPS_BISTABLE_IMPULSE_LENGTH))
    {
        pwr2RelayCoilsOff();
        _relayBistableImpulsTimerPwr2 = 0;
    }

    float busVoltage = _inaKnx.getBusVoltage_mV();
    bool busOk = busVoltage > POWER_OK_THRESHOLD_VOLTAGE * 1000;
    if (_busOk != busOk)
    {
        _busOk = busOk;
        openknx.gpio.digitalWrite(OPENKNX_BPS_STATUS_BUS, _busOk ? !OPENKNX_BPS_STATUS_ACTIVE_ON : OPENKNX_BPS_STATUS_ACTIVE_ON);
    }

    float totalCurrent = _inaKnx.getCurrent_mA() + _inaAux.getCurrent_mA();
    bool currentOk = totalCurrent < CURRENT_THRESHOLD_MA;
    if (_currentOk != currentOk)
    {
        _currentOk = currentOk;
        openknx.gpio.digitalWrite(OPENKNX_BPS_STATUS_MAX, _currentOk ? !OPENKNX_BPS_STATUS_ACTIVE_ON : OPENKNX_BPS_STATUS_ACTIVE_ON);
    }

    if (ParamBPS_PowerSupply1SendCyclicTimeMS > 0 && delayCheck(_powerSupply1SendTimer, ParamBPS_PowerSupply1SendCyclicTimeMS))
    {
        KoBPS_PowerSupply1Status.value(_pwr1Ok, DPT_Switch);
        _powerSupply1SendTimer = delayTimerInit();
    }

    if (ParamBPS_PowerSupply2SendCyclicTimeMS > 0 && delayCheck(_powerSupply2SendTimer, ParamBPS_PowerSupply2SendCyclicTimeMS))
    {
        KoBPS_PowerSupply2Status.value(_pwr2Ok, DPT_Switch);
        _powerSupply2SendTimer = delayTimerInit();
    }

    if (ParamBPS_BusVoltageChangeSend)
    {
        float busVoltageDifference = abs(_lastBusVoltageSent - busVoltage);
        if (_lastBusVoltageSent * ParamBPS_BusVoltageSendMinChangePercent / 100.0f > busVoltageDifference ||
            busVoltageDifference > ParamBPS_BusVoltageSendMinChangeAbsolute ||
            ParamBPS_BusVoltageSendCyclicTimeMS > 0 && delayCheck(_busVoltageSendTimer, ParamBPS_BusVoltageSendCyclicTimeMS))
        {
            KoBPS_BusVoltage.value(busVoltage, DPT_Value_Volt);
            _lastBusVoltageSent = busVoltage;
            _busVoltageSendTimer = delayTimerInit();
        }
    }

    if (ParamBPS_BusCurrentChangeSend)
    {
        float busCurrent = _inaKnx.getCurrent_mA();
        float busCurrentDifference = abs(_lastBusCurrentSent - busCurrent);
        if (_lastBusCurrentSent * ParamBPS_BusCurrentSendMinChangePercent / 100.0f > busCurrentDifference ||
            busCurrentDifference > ParamBPS_BusCurrentSendMinChangeAbsolute ||
            ParamBPS_BusCurrentSendCyclicTimeMS > 0 && delayCheck(_busCurrentSendTimer, ParamBPS_BusCurrentSendCyclicTimeMS))
        {
            KoBPS_BusCurrent.value(busCurrent, DPT_Value_Volt);
            _lastBusCurrentSent = busCurrent;
            _busCurrentSendTimer = delayTimerInit();
        }
    }

    if (ParamBPS_BusLoadChangeSend)
    {
        if (delayCheck(_busLoadSendTimer, BUS_LOAD_UPDATE_RATE))
        {
            float busLoad = estimateBusLoad();
            float busLoadDifference = abs(_lastBusLoadSent - busLoad);
            if (_lastBusLoadSent * ParamBPS_BusLoadSendMinChangePercent / 100.0f > busLoadDifference ||
                busLoadDifference > ParamBPS_BusLoadSendMinChangeAbsolute ||
                ParamBPS_BusLoadSendCyclicTimeMS > 0 && delayCheck(_busLoadSendTimer, ParamBPS_BusLoadSendCyclicTimeMS))
            {
                KoBPS_BusLoad.value(busLoad, DPT_Scaling);
                _lastBusLoadSent = busLoad;
                _busLoadSendTimer = delayTimerInit();
            }
        }
    }

    if (ParamBPS_AuxVoltageChangeSend)
    {
        float auxVoltage = _inaKnx.getBusVoltage_mV();
        float auxVoltageDifference = abs(_lastAuxVoltageSent - auxVoltage);
        if (_lastAuxVoltageSent * ParamBPS_AuxVoltageSendMinChangePercent / 100.0f > auxVoltageDifference ||
            auxVoltageDifference > ParamBPS_AuxVoltageSendMinChangeAbsolute ||
            ParamBPS_AuxVoltageSendCyclicTimeMS > 0 && delayCheck(_auxVoltageSendTimer, ParamBPS_AuxVoltageSendCyclicTimeMS))
        {
            KoBPS_AuxVoltage.value(auxVoltage, DPT_Value_Volt);
            _lastAuxVoltageSent = auxVoltage;
            _auxVoltageSendTimer = delayTimerInit();
        }
    }

    if (ParamBPS_AuxCurrentChangeSend)
    {
        float auxCurrent = _inaKnx.getCurrent_mA();
        float auxCurrentDifference = abs(_lastAuxCurrentSent - auxCurrent);
        if (_lastAuxCurrentSent * ParamBPS_AuxCurrentSendMinChangePercent / 100.0f > auxCurrentDifference ||
            auxCurrentDifference > ParamBPS_AuxCurrentSendMinChangeAbsolute ||
            ParamBPS_AuxCurrentSendCyclicTimeMS > 0 && delayCheck(_auxCurrentSendTimer, ParamBPS_AuxCurrentSendCyclicTimeMS))
        {
            KoBPS_AuxCurrent.value(auxCurrent, DPT_Value_Volt);
            _lastAuxCurrentSent = auxCurrent;
            _auxCurrentSendTimer = delayTimerInit();
        }
    }
    
    if (ParamBPS_TemperatureChangeSend)
    {
        float temperature = _temperature.readTemperatureC();
        float temperatureDifference = abs(_lastTemperatureSent - temperature);
        if (_lastTemperatureSent * ParamBPS_TemperatureSendMinChangePercent / 100.0f > temperatureDifference ||
            temperatureDifference > ParamBPS_TemperatureSendMinChangeAbsolute ||
            ParamBPS_TemperatureSendCyclicTimeMS > 0 && delayCheck(_temperaturSendTimer, ParamBPS_TemperatureSendCyclicTimeMS))
        {
            KoBPS_Temperature.value(temperature, DPT_Value_Temp);
            _lastTemperatureSent = temperature;
            _temperaturSendTimer = delayTimerInit();
        }
    }
}

void BusPowerSupplyModule::pwr1On()
{
    logDebugP("Turn PWR1 on");

    openknx.gpio.digitalWrite(OPENKNX_BPS_PWR1_SWITCH_OFF_PIN, !OPENKNX_BPS_PWR_SWITCH_ACTIVE_ON);
    openknx.gpio.digitalWrite(OPENKNX_BPS_PWR1_SWITCH_ON_PIN, OPENKNX_BPS_PWR_SWITCH_ACTIVE_ON);
    _relayBistableImpulsTimerPwr1 = delayTimerInit();
}

void BusPowerSupplyModule::pwr1Off()
{
    logDebugP("Turn PWR1 off");

    openknx.gpio.digitalWrite(OPENKNX_BPS_PWR1_SWITCH_ON_PIN, !OPENKNX_BPS_PWR_SWITCH_ACTIVE_ON);
    openknx.gpio.digitalWrite(OPENKNX_BPS_PWR1_SWITCH_OFF_PIN, OPENKNX_BPS_PWR_SWITCH_ACTIVE_ON);
    _relayBistableImpulsTimerPwr1 = delayTimerInit();
}

void BusPowerSupplyModule::pwr2On()
{
    logDebugP("Turn PWR2 on");

    openknx.gpio.digitalWrite(OPENKNX_BPS_PWR2_SWITCH_OFF_PIN, !OPENKNX_BPS_PWR_SWITCH_ACTIVE_ON);
    openknx.gpio.digitalWrite(OPENKNX_BPS_PWR2_SWITCH_ON_PIN, OPENKNX_BPS_PWR_SWITCH_ACTIVE_ON);
    _relayBistableImpulsTimerPwr2 = delayTimerInit();
}

void BusPowerSupplyModule::pwr2Off()
{
    logDebugP("Turn PWR2 off");

    openknx.gpio.digitalWrite(OPENKNX_BPS_PWR2_SWITCH_ON_PIN, !OPENKNX_BPS_PWR_SWITCH_ACTIVE_ON);
    openknx.gpio.digitalWrite(OPENKNX_BPS_PWR2_SWITCH_OFF_PIN, OPENKNX_BPS_PWR_SWITCH_ACTIVE_ON);
    _relayBistableImpulsTimerPwr2 = delayTimerInit();
}

void BusPowerSupplyModule::pwr1RelayCoilsOff()
{
    logDebugP("Turn both PWR1 relay coils off");

    openknx.gpio.digitalWrite(OPENKNX_BPS_PWR1_SWITCH_ON_PIN, !OPENKNX_BPS_PWR_SWITCH_ACTIVE_ON);
    openknx.gpio.digitalWrite(OPENKNX_BPS_PWR1_SWITCH_OFF_PIN, !OPENKNX_BPS_PWR_SWITCH_ACTIVE_ON);
}

void BusPowerSupplyModule::pwr2RelayCoilsOff()
{
    logDebugP("Turn both PWR2 relay coils off");

    openknx.gpio.digitalWrite(OPENKNX_BPS_PWR2_SWITCH_ON_PIN, !OPENKNX_BPS_PWR_SWITCH_ACTIVE_ON);
    openknx.gpio.digitalWrite(OPENKNX_BPS_PWR2_SWITCH_OFF_PIN, !OPENKNX_BPS_PWR_SWITCH_ACTIVE_ON);
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

    if (cmd.length() == 10 && cmd.substr(3, 7) == "pwr1 on")
    {
        pwr1On();
        return true;
    }
    else if (cmd.length() == 11 && cmd.substr(3, 8) == "pwr1 off")
    {
        pwr1Off();
        return true;
    }
    else if (cmd.length() == 10 && cmd.substr(3, 7) == "pwr2 on")
    {
        pwr2On();
        return true;
    }
    else if (cmd.length() == 11 && cmd.substr(3, 8) == "pwr2 off")
    {
        pwr2Off();
        return true;
    }

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