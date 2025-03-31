# Line-interactive UPS on Arduino
This project is implementing core functions of a basic single-phase line-interactive UPS using Arduino. The main idea is to have a fully functional controller, which can manage display indication, relays, invertor and sensors and also support external communication based on the [Voltronic protocol](https://networkupstools.org/protocols/voltronic.html). 

## Introduction 

The central part of a typical line-interactive UPS is a Big Iron Transformer, which plays the role of a back-boost voltage regulator with help of relays. 

<center class="half">
    <div style="background-color:#ffffff;">
    <img src="docs/line-interactive-ups-diagram.png" title="Line Interactive UPS with the Back-Boost Transformer"/>
</center>

If the input voltage is within the acceptable limits (230V +/- 10%) then the UPS passes it to the load with no changes, just applying EMI/RFI and load filters. But once the voltage is deviating for more than 10%, the relays are switching to ensure voltage back or boost, depending on the sign of deviation. 

Finally, if the input voltage is beyond the limits of back/boost regulation then the input relay is disconnected and the invertor kicks in to ensure the uninterrupted power feed to the load.

Since the invertor is taking power form the battery, the latter needs to be charged when the load is powered from the mains. There are different options how the charger can be implemented but the most recent ones involve some sort of fast-switching voltage convertors maintaining the required charging current by pulse-width modulation.

The line-interactive UPS controller features implemented in this project include:

1. Manage UPS relays to ensure the back/boost regulation of the output voltage when on AC power.
2. Automatically switch on the inverter once the AC power falls beyond the regulation limits and safely return to the AC once the mains power is restored back to normal.
3. Support user-friendly display indication. There are numerous display options for the UPS so for this project. For this projects, 3 options are supported: a 1602 or 2003 LCD display based on HD44780 controller (the most common for Arduino community), or an LED indicator based on [TM1640 chip](https://www.alldatasheet.com/datasheet-pdf/pdf/1133630/TITAN/TM1640.html). The latter has been re-used from a broken commercial UPS. One can modify or amend the Display class (see **Display.h**) in case if different hardware is to be supported.   
4. Display switch on/off and brightness regulation
5. Support of pulse-width modulated output for driving the battery charging process.
6. Support of the sensors: input/output AC voltage, output current (load), battery voltage and current. 
7. Support of the UPS sound indication (buzzer).
8. Support of the self-test scenario 
9. Support of the standby/restore scenario
10. Serial communication based on [Voltronic protocol](https://networkupstools.org/protocols/voltronic.html) implementation. See supported commands below:

## Configuration
Main parameters are set in the **config.h** and can be tweaked based on the Arduino board used. The solution is tested on the Arduino Nano board and 2x12V PbAc batteries. 

## Voltronic: supported commands
The solution supports most of the Voltronic commands. Some extensions are added to enable display and sensor management. Also, the default baud rate is set to 9600bps in order to minimize the blocking delays needed for the serial processing and allow more time for the main loop. However, the baud rate can be adjusted to the standard 2400bps in the **config.h**.

<table>
<thead><td><b>Command</b></td><td><b>Description</b></td></th></thead>
<tbody>
<tr><td>M</td><td>Returns the Voltronic protocol. Currently only 'V' is supported</td></tr>
<tr><td>Q</td><td>Toggle the UPS buzzer.</td></tr>
<tr><td>QS</td><td>Query UPS for status (short) (old)</td></tr>
<tr><td>QMD</td><td>Query UPS for rated information #1</td></tr>
<tr><td>QRI</td><td>Query UPS for rated information #2</td></tr>
<tr><td>QMF</td><td>Query UPS for manufacturer</td></tr>
<tr><td>QBV</td><td>Query UPS for battery information</td></tr>
<tr><td>D</td><td>Toggle display on or off</td></tr>
<tr><td>Dn</td><td>Set the brightness level for the display where <b>n</b> is representing the brightness level and can be from 0 to 4</td></tr>
<tr><td>T</td><td>Invoke a quick battery self-test</td></tr>
<tr><td>Tn</td><td>Invoke a battery self-test lasting n (.2→.9, 01→99) minutes</td></tr>
<tr><td>CT</td><td>Cancel the self-test</td></tr>
<tr><td>SnRm</td><td>Disconnect the load in n (.2→.9, 01→99) minutes and then connect again after m (0001..9999) minutes</td></tr>
<tr><td>CS</td><td>Re-connect the load or cancel the previous S command</td></tr>
<tr><td>R</td><td>Reset the controller board</td></tr>
<tr><td>VN</td><td>Print the scale factor and the offset of the sensor specified by the index N</td></tr>
<tr><td>VNPMVK...K</td><td>allows to tune or read sensor params, where:<br>
N - index of the sensor (1 digit). See the Sensors section below for the list of available indexes (0-4).<br>
M - can be 0 (scale) or 1 (offset).<br>
K...K - float value to be set (17 symbols, counting with the decimal dot).<br>
The same command can also modify PID parameters of the charger regulator (index=5). Please see the Charger section for details.</td></tr>
<tr><td>W</td><td>Save the sensor params in the EEPROM</td></tr>
</tbody>
</table>

Commands are posted through Serial  bus. Each command should be sent for execution followed by the `CR` symbol. 

## Sensors
The crucial part of line-interactive UPS is a set of sensors measuring input and output voltage and current as well as battery parameters. It is very important to ensure that these sensors are configured and tuned correctly so that the UPS could function properly.

The comprehensive list of sensors is represented in the table below. Each sensor is characterized by two parameters: <I>scale factor</I> and <I>offset</I>, both of the float type. The purpose of these parameters is to transform the integer reading from the Arduino analog input to the voltage, or amperage, or temperature in physical units. 

<table>
<thead>
    <td><b>Index</b></td>
    <td><b>Purpose</b></td>
    <td><b>Limits</b></td>
    <td><b>Default scale</b></td>
    <td><b>Default offset</b></td>
</thead>
<tbody>
    <tr>
        <td>0</td>
        <td>Input voltage</td>
        <td>0...300VAC</td>
        <td>2.05</td>
        <td>0</td>
    </tr>
    <tr>
        <td>1</td>
        <td>Output voltage</td>
        <td>0...300VAC</td>
        <td>2.05</td>
        <td>0</td>
    </tr> 
    <tr>
        <td>2</td>
        <td>Output current</td>
        <td>0...9A</td>
        <td>0.007</td>
        <td>0</td>
    </tr>
    <tr>
        <td>3</td>
        <td>Battery voltage</td>
        <td>0...49.9V</td>
        <td>0.05298</td>
        <td>0</td>
    </tr>     
     <tr>
        <td>4</td>
        <td>Battery current</td>
        <td>-29.9...29.9A</td>
        <td>0.07362</td>
        <td>-37.61</td>
    </tr>           
</tbody>
</table>

Two types of sensors are supported - Running Average (for non-periodic signal) and true RMS for AC voltage.  

### AC voltage sensors
AC voltage sensors can be implemented based on ZMPT101b signal transformer. The schema of the sensor is below. 

<center class="half">
    <div style="background-color:#ffffff;">
    <img src="docs/zmpt101b.png" title="Line Interactive UPS with the Back-Boost Transformer"/>
</center>

The R1 is limiting the input current as required by the transformer module. The signal voltage is formed on R2. R5/C1 is forming the low-pass filter with approx 1kHz of the cutoff frequency. It is needed to filter out all the high-frequency noise.     

### DC Current sensor
Measurement of the DC current through the battery can be done with help if the standard ACS712 module based on Hall effect. There are 5A, 20A and 30A modules available on the market. The schema of these modules is below:

<center class="half">
    <div style="background-color:#ffffff;">
    <img src="docs/acs712.jpg" title="Line Interactive UPS with the Back-Boost Transformer"/>
</center>

## Charger
Battery charging is kicking in 2 seconds when the input VAC is within the acceptable limits. The algorithm of charging is "constant current->constant voltage->standby":

1. Constant Current. The charger is maintaining the battery current at 10% of the battery capacity till the voltage is reaching the maximum as defined for the cell per cycle use.
2. Constant voltage. The charger is maintaining the maximum voltage per cell till the charging current become less than 2% of the battery capacity.
3. Standby. The voltage is dropped to the standby level and kept at it thus maintaining the battery at the charged state.

Regulation of the current and voltage is based on the PID regulator. Values of PID coefficients can be configured using the "V"/"W" commands with the index 5 (similar to a sensor):

<table>
<thead>
<td><b>#</b></td>
<td><b>Parameter</b></td>
<td><b>Description</b></td>
<td><b>Default value</b></td>
</thead>
<tbody>
<tr>
<td>0</td><td>Kp</td><td>Proportional</td><td>400</td>
</tr>
<tr>
<td>0</td><td>Ki</td><td>Integral</td><td>0.02</td>
</tr>
<tr>
<td>0</td><td>Kd</td><td>Derivative</td><td>50.0</td>
</tr>
</tbody>
</table>

Output of the charger is a PWM signal on the pin 10. 

Charger parameters can be changed similar to sensor parameters, by the command <b>VNPMVK...K</b> where N=5, M is the index of the parameter and K...K is the new value. Dumping of parameters canbe invoked by <b>V5</b> command, also similar to sensors. Example of the response is below:

```
(5 250.00 0.02 50.00 0.90 28.80 0.79 26.52 1 1 0.12 358
```
Values are space-delimited and come as follows:
- index of the charger, always equal to 5
- Kp PID parameter
- Ki PID parameter
- Kd PID parameter
- target battery current during the 1st charging phase
- target battery voltage during the 2nd charging phase
- measured battery current 
- measured battery voltage 
- '1' if charging or '0' otherwise
- phase of charging ('0' - constant current, '1' - constant voltage, '2' - standby). If the value is greater than 2, this indicates that some erroneous situation happened during the charging. Possible error codes are defined in the ChargingStatus enum in the Charger.h header.
- relative deviation from the target (current or voltage)
- output value of the charger regulator. Can be from 0 to 512. The maximum value corresponds to the 50% duty cycle.

## Display
Indication of the line-interactive modes and parameters can be done in many different ways. The Display class is supporting several options, which are defined in the **config.h** header by modifying corresponding macro as listed below.

```
// uncomment only one line for enabling display of the supported type
// #define DISPLAY_TYPE_NONE                        // no display. Saves resources at the expense of real-time visual info on the UPS. 
// #define DISPLAY_TYPE_LED_TM1640                  // LED assembly display based on TM1640
// #define DISPLAY_TYPE_LCD_HD44780                 // 16x2 or 20x4 LCD matrix display based on HD44780

// allows to configure display I2C address and resolution for HD44780
#ifdef DISPLAY_TYPE_LCD_HD44780
#define DISPLAY_I2C_ADDRESS     0x27
#define DISPLAY_SCREEN_WIDTH    20
#define DISPLAY_SCREEN_HEIGHT   4
#endif
```

  

## License
[GPLv3](/LICENSE)