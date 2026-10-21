# Project Faux

I haven't decided on a name for this mini project yet, but I find myself in
need of a test modbus & bacnet server.  Since I'm trying to get better at C and
C++, I decided to go ahead and write it myself.  Here goes nothing!

My basic requirements are this:

1. The user provides a yaml file containing the mapping for field names,
   default values, and modbus/bacnet registers.
2. This program serves those fields on the respective platforms as configured.
3. The user can change the values in real-time via a scriptable interface.
