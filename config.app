#All item for the config file are in a specific order and should not be changed
#1. Reader ID
#2. IP Address that the data will be sent to.
#3. First part of the string sent to the IP
#4. Power level we want the reader to operate at. 
#Otherwise the application will ignore any and all lines beginning with the hash (#) sign
#Reader ID
40046b96-8b08-44f9-a301-b9ba2e23c879
#IP to send the data packet to. 
192.168.2.73
#First part of the URL to attach to the IP
/LogTransaction.aspx?readerId=
#The power level for the device that can be set here for ease of use
#Valid values between 500 and 3000 with 2100 being 25% power.
1250 
