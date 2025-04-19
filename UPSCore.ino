#include <Arduino.h>

#include "config.h"
#include "utilities.h"

#include "avr/wdt.h"
#include "Settings.h"

#include "SimpleTimer.h"

#include "Sensor.h"
#include "Display.h"
#include "Interactive.h"
#include "Charger.h"

#include "Voltronic.h"

Settings settings;

SimpleTimerManager timer_manager;

//init sensors

// AC input voltage - 300V max
RMSSensor vac_in(SENSOR_INPUT_VAC_IN, -50.0F, 2.58F, 80, 1, 0, 3); 
// AC output voltage - 300V max
RMSSensor vac_out(SENSOR_OUTPUT_VAC_IN, 0.0F, 2.32F, 80, 1, 0, 3);   
// AC output current 
SimpleSensor ac_out(SENSOR_OUTPUT_C_IN, 0.0F, 0.007, 20, 5, 2 );  
// Battery voltage
SimpleSensor v_bat(SENSOR_BAT_V_IN, 0.0F, 0.05298, 20, 5 ,3 );    
// Battery current +/- 29.9A
SimpleSensor c_bat(SENSOR_BAT_C_IN, -37.61F, 0.07362F, 20, 5, 4 );    

SensorManager sensor_manager(&settings, &Serial);

// init the charger on DEFAULT_CHARGER_PWM_OUT pin
Charger charger(&settings, &c_bat, &v_bat);
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

#ifndef DISPLAY_TYPE_NONE
// init Display module
Display display(&lineups, &charger, &vac_in, &vac_out, &ac_out, &v_bat, &c_bat);
void refresh_display();
SimpleTimer* display_refresh_timer = nullptr;
#endif

Voltronic serial_protocol( &Serial );


void wakeup_ups(); // put the lineups in normal mode
void shutdown_ups(); // put the lineups in shutdown mode

uint32_t resume_timeout  = 0;

SimpleTimer* shutdown_timer =  nullptr;

void setup() {
  wdt_disable();
  pinMode(RESET_PIN, INPUT_PULLUP);

  Serial.begin(SERIAL_MONITOR_BAUD_RATE);
  
  Serial.write(VOLTRONIC_PROMPT);
  ex_print_str_to_stream( &Serial, PART_MODEL, true);
  Serial.println();

  // register sensors
  sensor_manager.register_sensor(&vac_in);
  sensor_manager.register_sensor(&ac_out);
  sensor_manager.register_sensor(&vac_out);
  sensor_manager.register_sensor(&v_bat);
  sensor_manager.register_sensor(&c_bat);
  
  // load params from EEPROM
  sensor_manager.loadParams();
  charger.loadParams();

  // create timers
  delayed_charge = timer_manager.create( 0,TIMER_ONE_SEC,false,nullptr,start_charging);
  beeper_timer = timer_manager.create(0,0,false,beep_on, beep_off);
#ifndef DISPLAY_TYPE_NONE 
  display_refresh_timer = timer_manager.create(DISPLAY_BLINK_FREQ, 0, false, refresh_display);
#endif
  self_test = timer_manager.create(0, MIN_SELFTEST_DURATION * 60 * TIMER_ONE_SEC, false, start_self_test, stop_self_test);
  shutdown_timer = timer_manager.create();

  cli(); // stop interrupts

  // Timer 0 1000Hz. Used for display refresh, blinker and sensor readings.
  TCNT0 = 0;
  TCCR0A = _BV(WGM00)|_BV(WGM01);            /* Fast PWM, pins not activated */
  TCCR0B = _BV(WGM02)|_BV(CS01)|_BV(CS00);   /* Fast PWM, Prescaler = x64     */
  OCR0A = 249;
  TIMSK0 = _BV(OCIE0A);

  // Timer 1 used for charging (15.6KHz)
  TCCR1A = _BV(WGM10) | _BV(WGM11);  // 10bit
  TCCR1B = _BV(WGM12) | _BV(CS10);   // x1 fast pwm

 // accelerate analogRead

  ADCSRA |= _BV(ADPS2); 
  ADCSRA &= ~_BV(ADPS1);
  ADCSRA &= ~_BV(ADPS0);
  sei(); // resume interrupts
  
  pinMode(BUZZ_PIN, OUTPUT);

#ifndef DISPLAY_TYPE_NONE 
  display.initialize();
  beep_on();
  delay(1000);
  beep_off();
  display_refresh_timer->start();
#else
  beep_on();
  delay(1000);
  beep_off();
#endif

  Serial.println(VOLTRONIC_PROMPT);

  wdt_enable(WDTO_2S);
}



ISR(TIMER0_COMPA_vect) {

  // Sample sensors
  sensor_manager.sample();

  // increment timers and call callback functions where applicable
  timer_manager.tick();
  
}


void loop() {

  if( vac_in.ready() && vac_out.ready() && ac_out.ready() && v_bat.ready() && c_bat.ready() ) {
    
    // calculate sensors
    vac_in.compute_reading();
    vac_out.compute_reading();
    ac_out.compute_reading();
    v_bat.compute_reading();
    c_bat.compute_reading();

    RegulateStatus result = lineups.regulate(timer_manager.getTicks());

    switch(result) {

      case REGULATE_STATUS_FAIL:

        if( !lineups.isBatteryMode() ) {
          // stop the charger to prevent interference with the inverter
          delayed_charge->stop();
          charger.stop();
          lineups.toggleInverter(true);
          beeper_timer->start( 8 * TIMER_ONE_SEC, TIMER_ONE_SEC );
        }
        
        if( !lineups.readStatus(OUTPUT_CONNECTED) ) lineups.toggleOutput(true);
        
        if( lineups.isBatteryMode() && bitRead(lineups.getStatus(), BATTERY_LOW)) { 
           
            // start beep every second if the battery is low
            if(beeper_timer->isEnabled() && beeper_timer->getPeriod() != 2 * TIMER_ONE_SEC ) 
              beeper_timer->start( 2 * TIMER_ONE_SEC, TIMER_ONE_SEC );

            // if the battery level is critical, shutdown in 10 sec
            if( !shutdown_timer->isEnabled() && lineups.isBatteryMode() && lineups.getBatteryLevel() < INTERACTIVE_BATTERY_CRITICAL ) {

              // we sleep forever till the AC power is back to norm
              serial_protocol.setParam( PARAM_RESTORE_MIN, 0);

              shutdown_timer->setOnFinish( shutdown_ups );
              shutdown_timer->start( 0, (int) ( 10 * TIMER_ONE_SEC ) );
            }  
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

        // sensor_manager.suspend();
        charger.regulate(timer_manager.getTicks());
        // sensor_manager.resume();

        break;

      case REGULATE_STATUS_ERROR:
        if(lineups.readStatus(OUTPUT_CONNECTED)) {
          lineups.toggleOutput(false);
          beeper_timer->stop();
          delayed_charge->stop();
          self_test->stop();
          charger.stop();

          beep_on();

          // shutdown in 5 sec
          if( !shutdown_timer->isEnabled() && lineups.isBatteryMode() ) {

            // we sleep forever till the AC power is back to norm
            serial_protocol.setParam( PARAM_RESTORE_MIN, 0);

            shutdown_timer->setOnFinish( shutdown_ups );
            shutdown_timer->start( 0, (int) ( 5 * TIMER_ONE_SEC ) );
          }  
        }

        break;

      case REGULATE_STATUS_WAKEUP:
        serial_protocol.setParam( PARAM_RESTORE_MIN , 0.0F);
        resume_timeout = 0;

        lineups.writeStatus(SHUTDOWN_ACTIVE, false);
        // if the battery is critically low, block the shutdown for 1 min to let battery to gain charge
        if(lineups.getBatteryLevel() < INTERACTIVE_BATTERY_CRITICAL) {
          shutdown_timer->setOnFinish(nullptr);
          shutdown_timer->start(0, 60 * TIMER_ONE_SEC);
        }

#ifndef DISPLAY_TYPE_NONE
        display.toggle(DISPLAY_ON);
#endif        
        break;

      case REGULATE_STATUS_SHUTDOWN:
        if(!lineups.readStatus(SHUTDOWN_ACTIVE)) {
          lineups.writeStatus(SHUTDOWN_ACTIVE, true);
#ifndef DISPLAY_TYPE_NONE
          display.toggle(DISPLAY_OFF);
#endif          
          delayed_charge->stop();
          beeper_timer->stop();
          self_test->stop();
          charger.stop();

          // switch off all the relays 
          lineups.toggleOutput(false);
          lineups.toggleInverter(false);
          lineups.toggleInput(false);
          lineups.adjustOutput(REGULATE_NONE);
          lineups.toggleError(false);
          vac_in.reset();
          vac_in.clear_ready();

          if( serial_protocol.getParam( PARAM_RESTORE_MIN ) > 0.0 ) {
            resume_timeout = (uint32_t)( serial_protocol.getParam(PARAM_RESTORE_MIN) * 60 );
          }
        }

        // Put the system in deep sleep for a fixed time period. Once the delay is over, the system will resume the loop
        // cycle as normal, re-take all the sensor readings and fall back here if the shutdown is still active

        lineups.sleep();
        
        if( serial_protocol.getParam( PARAM_RESTORE_MIN ) > 0.0 ) {
          resume_timeout--;

          if( resume_timeout == 0 ) {
            wakeup_ups();
            return;
          }
          
        }
        else {
          if( !bitRead(lineups.getStatus(), UTILITY_FAIL) ) {
            wakeup_ups();
            return;
          } 
          else {
            vac_in.reset();
            vac_in.clear_ready();
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
      serial_protocol.setParam(PARAM_OUTPUT_FREQ, vac_out.get_frequency() );

      // Estimate remaining battery time in minutes
      if (c_bat.reading() <= 0) { // Discharge mode 
        float remaining_capacity = lineups.getBatteryLevel() * INTERACTIVE_TOTAL_BATTERY_CAP; // Approximate remaining capacity in Ah
        float discharge_rate = -c_bat.reading(); // Convert to positive value
        // Time in minutes
        serial_protocol.setParam( PARAM_REMAINING_MIN, min(discharge_rate > 0? (remaining_capacity / discharge_rate) * 60.0F : 0 , 999.0) ); 
      } else {
        // Charging mode
        serial_protocol.setParam(PARAM_REMAINING_MIN, -1); 
      }
      
      // serial_protocol.setParam(PARAM_REMAINING_MIN,0);

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
          // shutdown command received, we need to set the Interactive in shutdown mode either immediately or after the delay,
          // depending on the shutdown time parameter
          if( serial_protocol.getParam(PARAM_SHUTDOWN_MIN) == 0.0F ) {
            shutdown_ups();
          }
          else if(! shutdown_timer->isEnabled() ) {
            shutdown_timer->setOnFinish( shutdown_ups );
            shutdown_timer->start( 0, (int) ( serial_protocol.getParam(PARAM_SHUTDOWN_MIN) * 60 * TIMER_ONE_SEC ) );
          }
          break;
        case COMMAND_SHUTDOWN_CANCEL:
          // shutdown cancel command received, we need to stop the shutdown timer
          // and wake up the Interactive if it was in shutdown mode
          if(shutdown_timer->isEnabled()) {
            shutdown_timer->setOnFinish(nullptr);
            shutdown_timer->stop();
          }
          
          if(lineups.readStatus(SHUTDOWN_ACTIVE)) {
            wakeup_ups();
          }

          break;

#ifndef DISPLAY_TYPE_NONE          
        case COMMAND_SET_BRIGHTNESS:
          display.set_brightness( (int)serial_protocol.getParam(PARAM_DISPLAY_BRIGHTNESS_LEVEL) );
          break;
        
        case COMMAND_TOGGLE_DISPLAY:
          display.toggle();
          break;
        
        case COMMAND_TOGGLE_DISPLAY_MODE:
          display.toggle_display_mode();
          break;
#endif        
        case COMMAND_READ_SENSOR:
          if( serial_protocol.getSensorPtr() < sensor_manager.get_num_sensors() ) {
            sensor_manager.print(serial_protocol.getSensorPtr());
          }
          else if(serial_protocol.getSensorPtr() == sensor_manager.get_num_sensors()) {
            ex_printf_to_stream(&Serial, "#%i %i %f %f %f %f %f %i\r\n",
                                              charger.is_charging(),
                                              charger.get_mode(),
                                              charger.get_current(),                                             
                                              charger.get_voltage(),
                                              c_bat.reading(),
                                              v_bat.reading(),
                                              charger.get_last_deviation(),
                                              charger.get_output());
          }
          break;
        case COMMAND_DUMP_SENSOR:
          if( serial_protocol.getSensorPtr() < sensor_manager.get_num_sensors() ) {
            sensor_manager.print(serial_protocol.getSensorPtr(), SENSOR_PRINT_DUMP );
          }
          break;
        case COMMAND_TUNE_SENSOR:
          if( serial_protocol.getSensorPtr() < sensor_manager.get_num_sensors() && 
              serial_protocol.getSensorParam() < SENSOR_NUMPARAMS ) {

            Sensor* sensor = sensor_manager.get(serial_protocol.getSensorPtr());
            sensor->setParam(serial_protocol.getSensorParamValue(), serial_protocol.getSensorParam());
            sensor->compute_reading();
            sensor_manager.print(serial_protocol.getSensorPtr());

          }
          else if(serial_protocol.getSensorPtr() == sensor_manager.get_num_sensors()) {
            charger.setParam(serial_protocol.getSensorParamValue(), serial_protocol.getSensorParam());
            ex_printf_to_stream(&Serial, "(%f %f %f %f %f %f %f %i %i %f %i\r\n",
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

#ifndef DISPLAY_TYPE_NONE
    // refresh the display based on sensor readings, lineups and charger state
    display.refresh();
#endif

  }

  wdt_reset();

}

#ifndef DISPLAY_TYPE_NONE
// initiate display refresh
void refresh_display() {
  display.init_refresh();
}
#endif

void beep_on() {
  digitalWrite(BUZZ_PIN, ( bitRead(lineups.getStatus(), BEEPER_IS_ACTIVE) ? HIGH: LOW ));
}

void beep_off() {
  digitalWrite(BUZZ_PIN, LOW);
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

void shutdown_ups() {
  lineups.setShutdownMode(true);
}

void wakeup_ups() {
  lineups.setShutdownMode(false);
}

