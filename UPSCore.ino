#include <Arduino.h>

#include "config.h"
#include "avr/wdt.h"
#include "Settings.h"

#include "SimpleTimer.h"

#include "Sensor.h"
#include "Display.h"
#include "Interactive.h"
#include "Charger.h"

#include "Voltronic.h"

Settings settings;

SimpleTimerManager timer_manager(&Serial);

//init sensors
Sensor vac_in(SENSOR_INPUT_VAC_IN, 0, 0.875, SENSOR_NUMSAMPLES );   // AC input voltage - 300V max
Sensor vac_out(SENSOR_OUTPUT_VAC_IN, 0.0F, 0.375, SENSOR_NUMSAMPLES ); // AC output voltage - 300V max
Sensor ac_out(SENSOR_OUTPUT_C_IN, 0.0F, 0.007, SENSOR_NUMSAMPLES );  // AC output current 
Sensor v_bat(SENSOR_BAT_V_IN, 0.0F, 0.05298, SENSOR_NUMSAMPLES );   // Battery voltage 
Sensor c_bat(SENSOR_BAT_C_IN, -37.61F, 0.07362F, SENSOR_NUMSAMPLES );    // Battery current +/- 29.9A

SensorManager sensor_manager(&Serial, &settings);

// init the charger on DEFAULT_CHARGER_PWM_OUT pin
Charger charger(&Serial, &settings, &c_bat, &v_bat);
void start_charging();
SimpleTimer* delayed_charge = nullptr;

// init the beeper timer
void beep_on();
void beep_off();
SimpleTimer* beeper_timer = nullptr;

// init line interactive ups module
Interactive lineups( &vac_in, &vac_out, &ac_out, &v_bat);

void start_self_test();
void stop_self_test();
SimpleTimer* self_test = nullptr;

// init Display module
Display display;
bool blink_state = false; 
void refresh_display();
SimpleTimer* display_refresh_timer = nullptr;

Voltronic serial_protocol( &Serial, 'V' );

void output_power_on();
void output_power_off();
SimpleTimer* output_power_timer =  nullptr;

void setup() {
  wdt_disable();
  pinMode(RESET_PIN, INPUT_PULLUP);

  serial_protocol.begin(SERIAL_MONITOR_BAUD_RATE);
  
  serial_protocol.printPartModel();

  // register sensors
  sensor_manager.registerSensor(&vac_in);
  sensor_manager.registerSensor(&vac_out);
  sensor_manager.registerSensor(&ac_out);
  sensor_manager.registerSensor(&v_bat);
  sensor_manager.registerSensor(&c_bat);
  
  // load params from EEPROM
  sensor_manager.loadParams();
  charger.loadParams();

  // create timers
  delayed_charge = timer_manager.create( 0,TIMER_ONE_SEC,false,nullptr,start_charging);
  beeper_timer = timer_manager.create(0,0,false,beep_on, beep_off);
  display_refresh_timer = timer_manager.create(DISPLAY_BLINK_FREQ, 0, false, refresh_display);
  self_test = timer_manager.create(0, MIN_SELFTEST_DURATION * 60 * TIMER_ONE_SEC, false, start_self_test, stop_self_test);
  output_power_timer = timer_manager.create();

  cli(); // stop interrupts

  // Timer 0 490Hz. Used for blinker and sensor readings.
  TCNT0 = 0;
  TCCR0A = _BV(WGM00);                       /* Phase Correct, pins not activated */
  TCCR0B = _BV(WGM02)|_BV(CS01)|_BV(CS00);   /* Phase Correct, Prescaler = 64     */
  OCR0A = 255;
  TIMSK0 = _BV(OCIE0A);

  // Timer 1 used for charging (15.6KHz), screen refresh and sensor readings
  TCCR1A = _BV(WGM10) | _BV(WGM11);  // 10bit
  TCCR1B = _BV(WGM12) | _BV(CS10);   // x1 fast pwm

  sei(); // resume interrupts

  display_refresh_timer->start();

  pinMode(BEEPER_OUT, OUTPUT);

  beep_on();
  delay(1000);
  beep_off();

  serial_protocol.printPrompt();
  serial_protocol.writeEOL();

  wdt_enable(WDTO_2S);
}



ISR(TIMER0_COMPA_vect) {

  // Sample sensors
  sensor_manager.sample();

  // increment timers and call callback functions where applicable
  timer_manager.tick();
  
}


void loop() {

  if(vac_in.ready() && ac_out.ready() && v_bat.ready() ) {

    RegulateStatus result = lineups.regulate(timer_manager.getTicks());

    // print_readings(result);

    switch(result) {

      case REGULATE_STATUS_FAIL:

        if( !lineups.isBatteryMode() ) {
          // stop the charger to prevent interference with the inverter
          delayed_charge->stop();
          charger.stop();
          
          beeper_timer->start( 8 * TIMER_ONE_SEC, TIMER_ONE_SEC );
          lineups.toggleInverter(true);
        }
        if( !lineups.readStatus(OUTPUT_CONNECTED) ) lineups.toggleOutput(true);

        // enable beep every second if the battery is low
        if(beeper_timer->isEnabled() && 
           bitRead(lineups.getStatus(), BATTERY_LOW) &&  
           beeper_timer->getPeriod() != 2 * TIMER_ONE_SEC ) {

          beeper_timer->start( 2 * TIMER_ONE_SEC, TIMER_ONE_SEC );

        }
        break;

      case REGULATE_STATUS_SUCCESS:

        // stop inverter (will set batteryMode to false)
        if(lineups.isBatteryMode()) {
          lineups.toggleInverter(false);
          beeper_timer->stop();
        }
        
        // connect to the mains
        lineups.toggleInput( true );

        // connect the load
        if( !lineups.readStatus(OUTPUT_CONNECTED) )  {
          lineups.toggleOutput(true);
        }

        if( !charger.is_charging() && !delayed_charge->isEnabled() && ( charger.get_mode() <= CHARGING_COMPLETE ) ) {
          delayed_charge->start( 0, 3 * TIMER_ONE_SEC );          
        }

        charger.regulate(timer_manager.getTicks());

        break;

      case REGULATE_STATUS_ERROR:
        if(lineups.readStatus(OUTPUT_CONNECTED)) {
          lineups.toggleOutput(false);
          beeper_timer->stop();
          delayed_charge->stop();
          self_test->stop();
          charger.stop();

          beep_on();
        }

        break;
      
      case REGULATE_STATUS_SHUTDOWN:
        if(lineups.readStatus(OUTPUT_CONNECTED)) {
          lineups.toggleOutput(false);
          delayed_charge->stop();
          beeper_timer->stop();
          self_test->stop();
          charger.stop();

          if( serial_protocol.getParam( PARAM_RESTORE_MIN ) > 0.0 && !output_power_timer->isEnabled() ) {
            output_power_timer->setOnFinish( output_power_on );
            output_power_timer->start( 0, serial_protocol.getParam(PARAM_RESTORE_MIN) * 60 * TIMER_ONE_SEC );
          }
        }

        break;

      default:
        break;

    }

    if( serial_protocol.process() == '\r' ) {

      serial_protocol.setParam(PARAM_INPUT_VAC, vac_in.reading());
      serial_protocol.setParam(PARAM_INPUT_FAULT_VAC,lineups.getLastFaultInputVoltage());
      serial_protocol.setParam(PARAM_OUTPUT_VAC, vac_out.reading());
      serial_protocol.setParam(PARAM_OUTPUT_LOAD_LEVEL, ac_out.reading() / INTERACTIVE_MAX_AC_OUT );
      serial_protocol.setParam(PARAM_BATTERY_LEVEL, lineups.getBatteryLevel() );

      //TODO: add estimation of remaining time here

      
      serial_protocol.setParam(PARAM_REMAINING_MIN,0);

      serial_protocol.setParam(PARAM_BATTERY_VDC, v_bat.reading());
      serial_protocol.setParam(PARAM_INTERNAL_TEMP, 25.0); //TODO: replace with sensor reading
      serial_protocol.setStatus(lowByte(lineups.getStatus()));

      ExecuteCommand exec_command = serial_protocol.executeCommand();

      switch(exec_command) {
        case COMMAND_BEEPER_MUTE:
          lineups.toggleBeeper();
          break;
        case COMMAND_SELF_TEST:
          if(!lineups.isBatteryMode() && lineups.getBatteryLevel() >= SELF_TEST_MIN_BAT_LVL && !self_test->isEnabled()) {
            self_test->start(0, serial_protocol.getParam(PARAM_SELFTEST_MIN) * 60 * TIMER_ONE_SEC );
          }
          break;
        case COMMAND_SELF_TEST_CANCEL:
          self_test->stop();
          break;
        case COMMAND_SHUTDOWN:
          if( serial_protocol.getParam(PARAM_SHUTDOWN_MIN) == 0.0F ) {
            output_power_off();
          }
          else if(! output_power_timer->isEnabled() && !output_power_timer->isEnabled() ) {
            output_power_timer->setOnFinish( output_power_off );
            output_power_timer->start( 0, (int) ( serial_protocol.getParam(PARAM_SHUTDOWN_MIN) * 60 * TIMER_ONE_SEC ) );
          }
          break;
        case COMMAND_SHUTDOWN_CANCEL:
          if(output_power_timer->isEnabled()) {
            if(lineups.readStatus(SHUTDOWN_ACTIVE)) {  
              output_power_timer->setOnFinish( output_power_on );
              output_power_timer->start( 0, 10 * TIMER_ONE_SEC );
            }
            else {
              output_power_timer->setOnFinish(nullptr);
              output_power_timer->stop();
            }

          }
          else if(lineups.readStatus(SHUTDOWN_ACTIVE)) {
            output_power_on();
          }

          break;

        case COMMAND_SET_BRIGHTNESS:
          display.setupDisplay( true, (int)serial_protocol.getParam(PARAM_DISPLAY_BRIGHTNESS_LEVEL) );
          break;
        
        case COMMAND_TOGGLE_DISPLAY:
          display.toggle();
          break;
        
        case COMMAND_READ_SENSOR:
          if( serial_protocol.getSensorPtr() < sensor_manager.getNumSensors() ) {
            Sensor* sensor = sensor_manager.get(serial_protocol.getSensorPtr());
            serial_protocol.printSensorParams(sensor->getParam(SENSOR_PARAM_OFFSET), 
                                              sensor->getParam(SENSOR_PARAM_SCALE),
                                              sensor->reading());
          }
          else if(serial_protocol.getSensorPtr() == sensor_manager.getNumSensors()) {
            serial_protocol.printChargerParams(charger.is_charging(),
                                              charger.get_mode(),
                                              charger.get_current(),                                             
                                              charger.get_voltage(),
                                              c_bat.reading(),
                                              v_bat.reading(),
                                              charger.get_last_deviation(),
                                              charger.get_output());
          }
          break;

        case COMMAND_TUNE_SENSOR:
          if( serial_protocol.getSensorPtr() < sensor_manager.getNumSensors() && 
              serial_protocol.getSensorParam() < 2 ) {

            Sensor* sensor = sensor_manager.get(serial_protocol.getSensorPtr());
            sensor->setParam(serial_protocol.getSensorParamValue(), serial_protocol.getSensorParam());
            serial_protocol.printSensorParams(sensor->getParam(SENSOR_PARAM_OFFSET), 
                                              sensor->getParam(SENSOR_PARAM_SCALE),
                                              sensor->reading());
          }
          else if(serial_protocol.getSensorPtr() == sensor_manager.getNumSensors()) {
            charger.setParam(serial_protocol.getSensorParamValue(), serial_protocol.getSensorParam());
            serial_protocol.printParam("(%f %f %f %f %f %f %f %i %i %f %i\r\n",
                                        charger.getParam(CHARGING_KP),
                                        charger.getParam(CHARGING_KI),
                                        charger.getParam(CHARGING_KD),
                                        charger.get_voltage(),
                                        charger.get_current(),
                                        v_bat.reading(),
                                        c_bat.reading(),
                                        charger.is_charging(),
                                        charger.get_mode(),
                                        charger.get_last_deviation(),
                                        charger.get_output());
          }
          break;
        
        case COMMAND_SAVE_SENSORS:
          sensor_manager.saveParams();
          charger.saveParams();
          break;

        default:
          break;
      }
    } 

  }

  wdt_reset();

}

// refresh the display based on sensor readings and the lineups state
void refresh_display() {
  // Serial.println("refresh display");
  blink_state = !blink_state;

  if( vac_in.ready() && ac_out.ready() && v_bat.ready() ) { 
    display.clear(false);
    display.setInputReading( vac_in.readingR() );
    display.setOutputReading( vac_out.readingR() );
    float battery_level = lineups.getBatteryLevel();
    ReadingDirection direction = lineups.isBatteryMode() ? 
                                    LEVEL_DECREASING : 
                                    ( charger.get_mode() <= CHARGING_BY_CV ? LEVEL_INCREASING : LEVEL_NO_CHANGE );

    display.setBatteryLevel( battery_level, direction );
    float load_level = ac_out.reading() / INTERACTIVE_MAX_AC_OUT;
    display.setLoadLevel( load_level );
    display.setFlag( ( load_level > 0.0 ? LOAD_INDICATOR : 0 ) | 
                    ( battery_level > 0.0 ? BATTERY_INDICATOR : 0 ) |
                    ( lineups.isBatteryMode() ? BATTERY_MODE_INDICATOR : AC_MODE_INDICATOR ) );
    display.setInputRelayStatus( lineups.readStatus(INPUT_CONNECTED) );
    display.setOutputRelayStatus( lineups.readStatus(OUTPUT_CONNECTED) );
    
    display.setBlink( ( lineups.readStatus( OVERLOAD ) ? LOAD_INDICATOR : 0) |
                      ( lineups.readStatus( BATTERY_LOW ) ? BATTERY_INDICATOR : 0) |
                      ( lineups.readStatus( UNUSUAL_STATE ) ? UNUSUAL_MODE_INDICATOR : 0) |
                      ( lineups.readStatus( UPS_FAULT ) ? UPS_FAULT_INDICATOR : 0) );

    display.show(blink_state);
  }
}

void beep_on() {
  digitalWrite(BEEPER_OUT, ( bitRead(lineups.getStatus(), BEEPER_IS_ACTIVE) ? HIGH: LOW ));
}

void beep_off() {
  digitalWrite(BEEPER_OUT, LOW);
}

void start_charging() {
  if( !lineups.readStatus(OUTPUT_CONNECTED) ) return;

  charger.set_min_battery_voltage(INTERACTIVE_MIN_V_BAT);        
  charger.set_cutoff_current(INTERACTIVE_BATTERY_AH * 0.02F);
  charger.start( INTERACTIVE_BATTERY_AH * 0.1F, INTERACTIVE_MAX_V_BAT, timer_manager.getTicks());
}

void start_self_test() {
  lineups.setSelfTestMode(true);
}

void stop_self_test() {
  lineups.setSelfTestMode(false);
}

void output_power_off() {
  lineups.setShutdownMode(true);
}

void output_power_on() {
  lineups.setShutdownMode(false);
}

