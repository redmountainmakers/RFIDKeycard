# RFIDKeycard
/*
   -----------------------------------------------------------
   RFID access control project
   Joint project with James Nylen and Daniel Near based on MFRC522 library.
   Linking an Arduino and RPI to our user database to provide access control.


  Workflow:

  Query RFID reader every minute or so and verify it's accessible.  Store this in a boolean flag

  If the RFID reader is not present, attempt to reset/reinitialize it.  If that fails, sleep some and attempt to repeat the process later.

  If an RFID tag is present, read the UID and pass it to the serial interface.

  Pass occasional heartbeat status to the serial port.  Include the processor ticks value.  This could alert the RPI if there's processor resets or errors that need addressing.

  Set status LEDs to default states (solid BLUE LED when both RPI and RFID reader are both connected, solid red if the link is down.
  RPI will send commands to the RPI to set status lights
  Flash red if an invalid badge is presented or flash green if a validated badge is presented.  Rapid blue flash when a badge is presented but not yet validated.

  Output one pin to activate a relay or MOSFET that will release the door latch.
  
  
  Received keycards will be passed to an application running on an RPI, which will query our WordPress user database.  If the database passes back a valid and active keyholding member, the RPI will instruct the Arduino to unlock the door.
  
  Frequent ping of database and RFID reader will verify system integrity and display this as a blue LED "Heartbeat" visible to the user outside the door.
  */
