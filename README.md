Fun project for educational reasons.

This project attempts to create an 
Endpoint detection response (EDR) progam fo Windows.
Currently focusing on x64, Windows 10+

In this project we implement as part of the EDR:
1. Driver which will manage all kernel logic from process notifications to dll injections.
2. UserMode service that will manage events
3. Anti malware flow which will manage the security check for each relevant event
   At the begining we will add simple security check
   i.e: Signature checking, Rule matching.
4. Event pipeline , currently for managing processes pool but in the future we will add more logic
  to the pipeline itself.11
