# Capstone
ChE Capstone Lab Project: A visual display of temperature gradient in a concentric tube heat exchanger

The lab setup consists of a 1m long concentric tube heat exchanger with 10 DS18B20 temperature sensors (5 in each stream).  A Raspberry Pi A+ reads those temperatures to visually display the temperature gradient along two LPD8806 LED strips. The software denotes the hottest measured temperature as pure red, the coldest as pure blue, and pure green as the average of the two.  The LED colors are then set by the corresponding temperature sensor and interpolated between sensors. Thus, you can visually see how the heat is being transferred between streams.  The Raspberry Pi also acts as an access point and serves up a webpage with the exact temperature readings.  
